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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3AssertPre.h"

#अन्तर्भाव "V3Assert.h"
#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3Task.h"
#अन्तर्भाव "V3Timing.h"
#अन्तर्भाव "V3UniqueNames.h"

#अन्तर्भाव <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clocking blocks of clocking items

वग्ग AssertPreClockingVisitor final : सार्वजनिक VNVisitorConst {
    // Captures the clocking block of each clocking item before AssertPreVisitor, which needs it
    // at synchronous drives, often visited before the clocking block, e.g. in a parent module
    // NODE STATE
    // AstClockingItem::user3p()  // AstClocking*. Clocking block of the item, user3 claimed by
    //                            //               AssertPreVisitor

    // VISITORS
    शून्य visit(AstNetlist* nodep) override { iterateAndNextConstNull(nodep->modulesp()); }
    शून्य visit(Ast_ग्रन्थिः_Module* nodep) override { iterateAndNextConstNull(nodep->stmtsp()); }
    शून्य visit(AstGenBlock* nodep) override { iterateAndNextConstNull(nodep->itemsp()); }
    शून्य visit(AstClocking* nodep) override {
        अत्थाय (Ast_ग्रन्थिः* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            सचे (AstClockingItem* ध्रुव citemp = VN_CAST(itemp, ClockingItem)) {
                citemp->user3p(nodep);
            }
        }
    }
    // Clocking blocks are only in modules and their generate blocks
    शून्य visit(Ast_ग्रन्थिः*) override {}

सार्वजनिक:
    // CONSTRUCTORS
    explicit AssertPreClockingVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~AssertPreClockingVisitor() override = स्वभाव;
};

//######################################################################
// Assert class functions

वग्ग AssertPreVisitor final : सार्वजनिक VNVisitor {
    // Removes clocks and other pre-optimizations
    // Eventually inlines calls to sequences, properties, etc.
    // We're not parsing the tree, or anything more complicated.
गुप्त:
    // TYPES
    संरचना SynchDrive final {
        AstClockingItem* itemp;  // Clocking item of the driven clockvar
        Ast_ग्रन्थिः_व्यञ्जकम्* refp;  // Reference to the driven clockvar
    };

    // NODE STATE
    // AstClockingItem::user1p()         // AstVar*.      varp() of ClockingItem after unlink
    // AstClockingItem::user2p()         // AstVar*.      Flag set by drives of output clockvar
    // AstClockingItem::user3p()         // AstClocking*. Clocking block of the item, captured
    //                                   //               by AssertPreClockingVisitor
    // AstPExpr::user1()                 // bool.         Created from AstUntil
    ध्रुव VNUser1InUse m_inuser1;
    ध्रुव VNUser2InUse m_inuser2;
    ध्रुव VNUser3InUse m_inuser3;
    // STATE
    // Current context:
    AstNetlist* ध्रुव m_netlistp = nullptr;  // Current netlist
    Ast_ग्रन्थिः_Module* m_modp = nullptr;  // Current module
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
    Ast_ग्रन्थिः_व्यञ्जकम्* m_disablep = nullptr;  // Last disable
    AstIf* m_disableSeqIfp = nullptr;  // Used for handling disable iff in sequences
    AstPExpr* m_pexprp = nullptr;  // Last AstPExpr
    तार्किक m_underCover = मिच्छा;  // True if the enclosing assertion is a cover
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
    तार्किक m_inAssign = मिच्छा;  // True if in an AssignNode
    तार्किक m_inAssignDlyLhs = मिच्छा;  // True if in AssignDly's LHS
    तार्किक m_inSynchDrive = मिच्छा;  // True if in synchronous drive
    तार्किक m_inConcatAssign = मिच्छा;  // True if in assignment to a concatenation
    std::vector<SynchDrive> m_drives;  // Clockvars written by the synchronous drive
    तार्किक m_hasCycleDelay = मिच्छा;  // True if node has cycle delay beneath
    std::vector<AstVarXRef*> m_xrefsp;  // list of xrefs that need name fixup
    std::vector<AstSequence*> m_seqsToCleanup;  // Sequences to clean up after traversal

    // METHODS

    // Flag set by each drive of an output clockvar, added to the module with its clocking item
    AstVar* getCreateDrivenVarp(AstClockingItem* itemp) {
        सचे (!itemp->user2p()) {
            AstVar* ध्रुव varp = नूतन AstVar{itemp->fileline(), VVarType::MODULETEMP,
                                            m_drivenNames.get(itemp), itemp->findBitDType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            itemp->user2p(varp);
        }
        पटिनिवत्त VN_AS(itemp->user2p(), Var);
    }
    // Reference to a variable in the scope of a driven clockvar, made like the clockvar's
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* newDriveRefp(File_पङ्क्तिः* flp, ध्रुव SynchDrive& drive, AstVar* varp,
                                     ध्रुव VAccess& access) {
        सचे (ध्रुव AstVarXRef* ध्रुव xrefp = VN_CAST(drive.refp, VarXRef)) {
            पटिनिवत्त नूतन AstVarXRef{flp, varp, xrefp->dotted(), access};
        } अञ्ञथा सचे (ध्रुव AstMemberSel* ध्रुव selp = VN_CAST(drive.refp, MemberSel)) {
            // The interface expression is evaluated again, so it must not have side effects;
            // cloneTreePure warns if it has any
            AstMemberSel* ध्रुव newSelp
                = नूतन AstMemberSel{flp, selp->fromp()->clone_वृक्षः_Pure(मिच्छा), varp};
            newSelp->access(access);
            पटिनिवत्त newSelp;
        }
        पटिनिवत्त नूतन AstVarRef{flp, varp, access};
    }
    // Assignment setting the drive flag of a driven clockvar
    AstAssign* newDrivenSetp(File_पङ्क्तिः* flp, ध्रुव SynchDrive& drive) {
        AstVar* ध्रुव varp = getCreateDrivenVarp(drive.itemp);
        AstAssign* ध्रुव setp = नूतन AstAssign{flp, newDriveRefp(flp, drive, varp, VAccess::WRITE),
                                              नूतन AstConst{flp, AstConst::BitTrue{}}};
        setp->user1(सच्च);
        पटिनिवत्त setp;
    }
    // Clocking block of a clocking item
    स्थिर AstClocking* clockingOf(ध्रुव AstClockingItem* itemp) {
        UASSERT_OBJ(itemp->user3p(), itemp, "Clocking item not captured in a clocking block");
        पटिनिवत्त VN_AS(itemp->user3p(), Clocking);
    }
    // Clocking event of a driven clockvar, referenced from the drive like the clockvar, or
    // nullptr if unsupported, which is reported on the drive's cycle delay
    Ast_Sen_वृक्षः* newDriveSentreep(AstDelay* delayp, ध्रुव SynchDrive& drive) {
        Ast_Sen_वृक्षः* ध्रुव origp = clockingOf(drive.itemp)->sentreep();
        सचे (VN_IS(drive.refp, VarRef)) पटिनिवत्त origp->clone_वृक्षः(मिच्छा);
        सचे (origp->exists([](ध्रुव AstVarXRef*) { पटिनिवत्त सच्च; })) {
            // The reference is relative to the clocking block
            delayp->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in synchronous drive to"
                                          " clockvar in another scope, whose clocking event"
                                          " has a hierarchical reference");
            पटिनिवत्त nullptr;
        }
        ध्रुव AstMemberSel* ध्रुव selp = VN_CAST(drive.refp, MemberSel);
        // Processes outside classes evaluate their events also when not waiting on them,
        // possibly before the interface reference is set, so the event is guarded against a
        // null reference, which needs an integral value; scheduling does not guard it
        ध्रुव तार्किक guardNull = selp && !VN_IS(m_modp, Class);
        सचे (guardNull) {
            अत्थाय (AstSenItem* itemp = origp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                सचे (!itemp->sensp()->dtypep()->skipRefp()->isIntegralOrPacked()) {
                    delayp->v3warn(E_UNSUPPORTED,
                                   "Unsupported: cycle delay in synchronous drive through"
                                   " an interface reference outside a class, to clocking"
                                   " block with non-integral clocking event");
                    पटिनिवत्त nullptr;
                }
            }
        }
        // Interface declaring the clocking block, if driven through an interface reference
        AstIface* ध्रुव ifacep
            = selp ? VN_AS(selp->fromp()->dtypep()->skipRefp(), IfaceRefDType)->ifaceViaCellp()
                   : nullptr;
        Ast_Sen_वृक्षः* ध्रुव sentreep = origp->clone_वृक्षः(मिच्छा);
        std::vector<AstVarRef*> refps;
        sentreep->foreach([&](AstVarRef* refp) {
            सचे (!refp->classOrPackagep()) refps.प्रक्षिप_back(refp);
        });
        अत्थाय (AstVarRef* ध्रुव refp : refps) {
            AstVar* ध्रुव varp = refp->varp();
            सचे (ifacep) {
                // Mark the variable as read through an interface reference, as V3Width does for
                // a member select. The mark is the interface declaring the variable, whose
                // instances all share it, so drives through references to any instances agree.
                UASSERT_OBJ(!varp->sensIfacep() || varp->sensIfacep() == ifacep, refp,
                            "Variable read through references to different interfaces");
                varp->sensIfacep(ifacep);
            }
            refp->replaceWith(newDriveRefp(refp->fileline(), drive, varp, refp->access()));
            VL_DO_DANGLING(प्रक्षिप_Deletep(refp), refp);
        }
        सचे (guardNull) {
            अत्थाय (AstSenItem* itemp = sentreep->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                guardNullIface(itemp, selp->fromp());
            }
        }
        पटिनिवत्त sentreep;
    }
    // Make an integral clocking event through an interface reference false while the reference
    // is null
    स्थिर शून्य guardNullIface(AstSenItem* sensesp, Ast_ग्रन्थिः_व्यञ्जकम्* fromp) {
        File_पङ्क्तिः* ध्रुव flp = sensesp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव sensp = sensesp->sensp()->unlinkFrBack();
        sensesp->sensp(नूतन AstCond{flp, newNotNullp(flp, fromp), sensp,
                                   नूतन AstConst{flp, AstConst::DTyped{}, sensp->dtypep()}});
        सचे (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = sensesp->condp()) {
            condp->unlinkFrBack();
            sensesp->condp(नूतन AstLogAnd{flp, newNotNullp(flp, fromp), condp});
        }
    }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* newNotNullp(File_पङ्क्तिः* flp, Ast_ग्रन्थिः_व्यञ्जकम्* fromp) {
        पटिनिवत्त नूतन AstNeq{flp, fromp->clone_वृक्षः_Pure(मिच्छा), नूतन AstConst{flp, AstConst::Null{}}};
    }
    // Procedural '##0': if the default clocking event has not yet occurred in this time step,
    // wait for it; otherwise continue without suspension (IEEE 1800-2023 14.11)
    AstIf* newCycleDelayZerop(File_पङ्क्तिः* flp) ध्रुव {
        AstVar* ध्रुव evtVarp = m_defaultClkEvtVarp;
        UASSERT_OBJ(evtVarp, flp, "Default clocking event var not pre-created");
        AstCMethodHard* ध्रुव isTriggeredp = नूतन AstCMethodHard{
            flp, नूतन AstVarRef{flp, evtVarp, VAccess::READ}, VCMethod::EVENT_IS_TRIGGERED};
        isTriggeredp->dtypeSetBit();
        AstEventControl* ध्रुव waitp = नूतन AstEventControl{
            flp,
            नूतन Ast_Sen_वृक्षः{flp, नूतन AstSenItem{flp, VEdgeType::ET_EVENT,
                                               नूतन AstVarRef{flp, evtVarp, VAccess::READ}}},
            nullptr};
        पटिनिवत्त नूतन AstIf{flp, नूतन AstNot{flp, isTriggeredp}, waitp};
    }

    स्थिर शून्य checkSamplingFuncDType(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, ध्रुव Ast_ग्रन्थिः* exprp) {
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = exprp->dtypep()->skipRefp();
        सचे (!dtypep->isIntegralOrPacked()) {
            nodep->v3error("Expected numeric type, but got a " << dtypep->prettyDTypeNameQ()
                                                               << " data type");
        }
    }

    Ast_Sen_वृक्षः* new_Sen_वृक्षः(Ast_ग्रन्थिः* nodep, Ast_Sen_वृक्षः* useTreep = nullptr,
                           Ast_ग्रन्थिः_Cover_Or_Assert* cassertp = nullptr) {
        // Create sentree based on clocked or default clock
        // Return nullptr for always
        सचे (useTreep) पटिनिवत्त useTreep;
        Ast_Sen_वृक्षः* newp = nullptr;
        AstSenItem* senip = m_senip;
        तार्किक fromAlways = मिच्छा;
        सचे (!senip && m_defaultClockingp) senip = m_defaultClockingp->sentreep()->sensesp();
        सचे (!senip) {
            senip = m_seniAlwaysp;
            fromAlways = सच्च;
        }
        सचे (!senip) {
            nodep->v3error("Concurrent assertion has no clock (IEEE 1800-2023 16.16)\n"
                           << nodep->warnMore()
                           << "... Suggest provide a clocking event, a default"
                              " clocking, or a clocked procedural context");
            newp = नूतन Ast_Sen_वृक्षः{nodep->fileline(), nullptr};
        } अञ्ञथा {
            सचे (cassertp && fromAlways) cassertp->senFromAlways(सच्च);
            newp = नूतन Ast_Sen_वृक्षः{nodep->fileline(), senip->clone_वृक्षः(सच्च)};
        }
        पटिनिवत्त newp;
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* getSequenceBodyExprp(ध्रुव AstSequence* ध्रुव seqp) ध्रुव {
        // The statements in AstSequence are optional AstVar (ports) followed by body expr.
        Ast_ग्रन्थिः* bodyp = seqp->stmtsp();
        याव (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        पटिनिवत्त VN_CAST(bodyp, ग्रन्थिः_व्यञ्जकम्);
    }
    AstPropSpec* getPropertyExprp(ध्रुव AstProperty* ध्रुव propp) {
        // Statements in AstProperty: AstVar (ports/local vars),
        // AstInitialStaticStmt/AstInitialAutomaticStmt (var init), AstPropSpec (body).
        Ast_ग्रन्थिः* propExprp = propp->stmtsp();
        याव (propExprp
               && (VN_IS(propExprp, Var) || VN_IS(propExprp, InitialStaticStmt)
                   || VN_IS(propExprp, InitialAutomaticStmt))) {
            propExprp = propExprp->nextp();
        }
        पटिनिवत्त VN_CAST(propExprp, PropSpec);
    }
    शून्य substituteSequenceCall(AstFuncRef* funcrefp, AstSequence* seqp) {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Inline the sequence body at the call site, replacing the FuncRef
        Ast_ग्रन्थिः_व्यञ्जकम्* bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        // Clone the body expression since the sequence may be referenced multiple times
        Ast_ग्रन्थिः_व्यञ्जकम्* clonedp = bodyExprp->clone_वृक्षः(मिच्छा);
        // Build substitution map, then do a single traversal to replace all formals
        // (textual substitution per IEEE 16.8.2).
        ध्रुव V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<ध्रुव AstVar*, Ast_ग्रन्थिः_व्यञ्जकम्*> portMap;
        अत्थाय (ध्रुव auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            ध्रुव auto it = portMap.find(refp->varp());
            सचे (it != portMap.end()) {
                refp->replaceWith(it->second->clone_वृक्षः(मिच्छा));
                VL_DO_DANGLING(प्रक्षिप_Deletep(refp), refp);
            }
        });
        अत्थाय (ध्रुव auto& tconnect : tconnects) {
            प्रक्षिप_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        // Replace the FuncRef with the inlined body
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(funcrefp), funcrefp);
        // Clear referenced flag; sequences with isReferenced==false are deleted in assertPreAll
        seqp->isReferenced(मिच्छा);
    }
    AstPropSpec* clonePropertyBody(AstFuncRef* funcrefp, ध्रुव AstProperty* propp) {
        AstPropSpec* propExprp = getPropertyExprp(propp);
        // Substitute inner property call before copying in order to not doing the same for
        // each call of outer property call.
        propExprp = substitutePropertyCall(propExprp);
        // Clone subtree after substitution. It is needed, because property might be called
        // multiple times with different arguments.
        propExprp = propExprp->clone_वृक्षः(मिच्छा);
        // Build substitution maps for formal arguments and property-local
        // variables, then perform a single foreach to apply all replacements.
        // Map port vars to their actual argument expressions
        ध्रुव V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propp->stmtsp());
        std::unordered_map<ध्रुव AstVar*, Ast_ग्रन्थिः_व्यञ्जकम्*> portMap;
        अत्थाय (ध्रुव auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }

        // Promote property-local variables (non-port vars, IEEE 16.10) to
        // module-level __Vpropvar temps. Cross-cycle persistence is handled
        // by the match item lowering in visit(AstImplication*).
        std::unordered_map<ध्रुव AstVar*, AstVar*> localVarMap;
        अत्थाय (Ast_ग्रन्थिः* stmtp = propp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            सचे (AstVar* ध्रुव varp = VN_CAST(stmtp, Var)) {
                सचे (!varp->isIO()) {
                    ध्रुव string newName = m_propVarNames.get(varp);
                    AstVar* ध्रुव newVarp = नूतन AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->addStmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        // Single traversal: substitute ports and update local var references
        propExprp->foreach([&](AstVarRef* refp) {
            ध्रुव auto portIt = portMap.find(refp->varp());
            सचे (portIt != portMap.end()) {
                refp->replaceWith(portIt->second->clone_वृक्षः(मिच्छा));
                VL_DO_DANGLING(प्रक्षिप_Deletep(refp), refp);
                पटिनिवत्त;
            }
            ध्रुव auto localIt = localVarMap.find(refp->varp());
            सचे (localIt != localVarMap.end()) refp->varp(localIt->second);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        // Clean up argument expressions
        अत्थाय (ध्रुव auto& tconnect : tconnects) {
            प्रक्षिप_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(propExprp, funcrefp);
        पटिनिवत्त propExprp;
    }
    AstPropSpec* substitutePropertyCall(AstPropSpec* nodep) {
        सचे (AstFuncRef* ध्रुव funcrefp = VN_CAST(nodep->propp(), FuncRef)) {
            सचे (ध्रुव AstProperty* ध्रुव propp = VN_CAST(funcrefp->taskp(), Property)) {
                AstPropSpec* ध्रुव propExprp = clonePropertyBody(funcrefp, propp);

                // Handle case with 2 disable iff statement (IEEE 1800-2023 16.12.1)
                सचे (nodep->disablep() && propExprp->disablep()) {
                    nodep->v3error("disable iff expression before property call and in its "
                                   "body is not legal");
                    प्रक्षिप_Deletep(propExprp->disablep()->unlinkFrBack());
                }
                // If disable iff is in outer property, move it to inner
                सचे (nodep->disablep()) {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव disablep = nodep->disablep()->unlinkFrBack();
                    propExprp->disablep(disablep);
                }

                सचे (nodep->sensesp() && propExprp->sensesp()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Clock event before property call and in its body");
                    प्रक्षिप_Deletep(propExprp->sensesp()->unlinkFrBack());
                }
                // If clock event is in outer property, move it to inner
                सचे (nodep->sensesp()) {
                    AstSenItem* ध्रुव sensesp = nodep->sensesp();
                    sensesp->unlinkFrBack();
                    propExprp->sensesp(sensesp);
                }

                // Now substitute property reference with property body
                nodep->replaceWith(propExprp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                पटिनिवत्त propExprp;
            }
        }
        पटिनिवत्त nodep;
    }

    शून्य substituteNestedPropertyCall(AstFuncRef* funcrefp, ध्रुव AstProperty* propp) {
        AstPropSpec* ध्रुव specp = clonePropertyBody(funcrefp, propp);
        Ast_ग्रन्थिः* replp = nullptr;
        सचे (specp->disablep()) {
            funcrefp->v3error("Illegal 'disable iff' in a nested property instance"
                              " (IEEE 1800-2023 16.12.1)");
        } अञ्ञथा सचे (specp->sensesp()) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: clock event in a nested property instance");
        } अञ्ञथा सचे (specp->propp()->exists(
                       [](ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* np) { पटिनिवत्त np->isMultiCycleSva(); })) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: multi-cycle body in a nested property instance");
        } अञ्ञथा {
            replp = specp->propp()->unlinkFrBack();
        }
        सचे (!replp) replp = नूतन AstConst{funcrefp->fileline(), AstConst::BitFalse{}};
        funcrefp->replaceWith(replp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(funcrefp), funcrefp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(specp), specp);
    }

    // VISITORS
    //========== Statements
    शून्य visit(AstClocking* ध्रुव nodep) override {
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        UINFO(8, "   CLOCKING" << nodep);
        सचे (nodep->isDefault()) {
            सचे (m_defaultClockingp) {
                nodep->v3error("Only one default clocking block allowed per module"
                               " (IEEE 1800-2023 14.12)");
            } अञ्ञथा {
                m_defaultClockingp = nodep;
                m_defaultClkEvtVarp = nodep->ensureEventp();
            }
        }
        iterateChildren(nodep);
        सचे (nodep->eventp()) nodep->addNextHere(nodep->eventp()->unlinkFrBack());
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
    }
    शून्य visit(AstModportClockingRef* ध्रुव nodep) override {
        // It has to be converted to a list of ModportClockingVarRefs,
        // because clocking blocks are removed in this pass
        अत्थाय (Ast_ग्रन्थिः* itemp = nodep->clockingp()->itemsp(); itemp; itemp = itemp->nextp()) {
            सचे (AstClockingItem* ध्रुव citemp = VN_CAST(itemp, ClockingItem)) {
                सचे (AstVar* ध्रुव varp
                    = citemp->varp() ? citemp->varp() : VN_AS(citemp->user1p(), Var)) {
                    AstModportVarRef* ध्रुव modVarp = नूतन AstModportVarRef{
                        nodep->fileline(), varp->name(), citemp->direction()};
                    modVarp->varp(varp);
                    nodep->addNextHere(modVarp);
                    सचे (citemp->direction().isOutput()) {
                        // Drives through the modport set the drive flag
                        AstVar* ध्रुव drivenVarp = getCreateDrivenVarp(citemp);
                        AstModportVarRef* ध्रुव drivenRefp = नूतन AstModportVarRef{
                            nodep->fileline(), drivenVarp->name(), VDirection::OUTPUT};
                        drivenRefp->varp(drivenVarp);
                        nodep->addNextHere(drivenRefp);
                    }
                }
            }
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
    }
    शून्य visit(AstClockingItem* ध्रुव nodep) override {
        // Get a ref to the sampled/driven variable
        AstVar* ध्रुव varp = nodep->varp();
        सचे (!varp) {
            // Unused item
            पटिनिवत्त;
        }
        // Flag set by drives of an output clockvar, possibly visited before this item
        सचे (nodep->direction().isOutput()) m_modp->addStmtsp(getCreateDrivenVarp(nodep));
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        V3Const::constifyEdit(nodep->skewp());
        सचे (!VN_IS(nodep->skewp(), Const)) {
            nodep->skewp()->v3error("Skew must be constant (IEEE 1800-2023 14.4)");
            पटिनिवत्त;
        }
        AstConst* ध्रुव skewp = VN_AS(nodep->skewp(), Const);
        सचे (skewp->अङ्कः().isNegative()) skewp->v3error("Skew cannot be negative");
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = nodep->exprp();
        varp->name(m_clockingp->name() + "__DOT__" + varp->name());
        m_clockingp->addNextHere(varp->unlinkFrBack());
        nodep->user1p(varp);
        varp->user1p(nodep);
        सचे (nodep->direction() == VDirection::OUTPUT) {
            exprp->foreach([](ध्रुव Ast_ग्रन्थिः_Var_Ref* varrefp) {
                // Prevent confusing BLKANDNBLK warnings on clockvars due to generated assignments
                varrefp->fileline()->warnOff(V3ErrorCode::BLKANDNBLK, सच्च);
                // Mark the driven signal so later passes (e.g. V3Inst port
                // connection) know the drive flows out from inside the cell.
                // Item expressions are plain signal references, so only the
                // driven signal is marked, never index selects.
                varrefp->varp()->isClockingDriven(सच्च);
            });
            AstVarRef* ध्रुव skewedReadRefp = नूतन AstVarRef{flp, varp, VAccess::READ};
            skewedReadRefp->user1(सच्च);
            // Initialize the clockvar
            AstVarRef* ध्रुव skewedWriteRefp = नूतन AstVarRef{flp, varp, VAccess::WRITE};
            skewedWriteRefp->user1(सच्च);
            AstInitialStatic* ध्रुव initClockvarp = नूतन AstInitialStatic{
                flp, नूतन AstAssign{flp, skewedWriteRefp, exprp->clone_वृक्षः_Pure(मिच्छा)}};
            m_modp->addStmtsp(initClockvarp);
            // Each drive sets a flag, so the signal is also assigned when driven with the same
            // value again, e.g. after another assignment to the signal (IEEE 1800-2023 14.16.2)
            AstVar* ध्रुव drivenVarp = getCreateDrivenVarp(nodep);
            m_modp->addStmtsp(नूतन AstInitialStatic{
                flp, नूतन AstAssign{flp, नूतन AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                   नूतन AstConst{flp, AstConst::BitFalse{}}}});
            // Assign the clockvar to the actual var if it was driven
            AstAssign* ध्रुव assignp
                = नूतन AstAssign{flp, exprp->clone_वृक्षः_Pure(मिच्छा), skewedReadRefp};
            AstIf* ध्रुव ifp
                = नूतन AstIf{flp, नूतन AstVarRef{flp, drivenVarp, VAccess::READ},
                            नूतन AstAssign{flp, नूतन AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                          नूतन AstConst{flp, AstConst::BitFalse{}}}};
            ifp->addThensp(assignp);
            // Only in the time step of the clocking event
            AstCMethodHard* ध्रुव trigp = नूतन AstCMethodHard{
                nodep->fileline(), नूतन AstVarRef{flp, m_clockingp->ensureEventp(), VAccess::READ},
                VCMethod::EVENT_IS_TRIGGERED};
            trigp->dtypeSetBit();
            ifp->condp(नूतन AstLogAnd{flp, ifp->condp()->unlinkFrBack(), trigp});
            // A skew rounding to no time precision ticks is a zero skew
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव ticksp
                = V3Timing::delayToTicks(skewp->clone_वृक्षः(मिच्छा), m_modp->timeunit());
            ध्रुव तार्किक zeroSkew = ticksp->isZero();
            VL_DO_DANGLING(ticksp->delete_वृक्षः(), ticksp);
            सचे (zeroSkew || !skewp->fileline()->timingOn()) {
                // Drive the var in Re-NBA (IEEE 1800-2023 14.16), ignoring skews where timing is
                // turned off
                Ast_Sen_वृक्षः* senTreep = m_clockingp->sentreep()->clone_वृक्षः(मिच्छा);
                senTreep->addSensesp(नूतन AstSenItem{
                    flp, VEdgeType::ET_CHANGED, नूतन AstVarRef{flp, drivenVarp, VAccess::READ}});
                m_clockingp->addNextHere(नूतन AstAlwaysReactive{flp, senTreep, ifp});
            } अञ्ञथा {
                // Fork only the assignment, so this AlwaysPostponed can be retriggered
                // before the assignment happens
                AstFork* ध्रुव forkp = नूतन AstFork{flp, VJoinType::JOIN_NONE};
                forkp->immediateStart(सच्च);
                forkp->addForksp(नूतन AstBegin{flp, "", assignp->unlinkFrBack(), सच्च});
                ifp->addThensp(forkp);
                // Read the clockvar in Postponed, so a drive late in the time step is applied
                // at this event plus the skew (IEEE 1800-2023 14.16)
                m_clockingp->addNextHere(नूतन AstAlwaysPostponed{flp, ifp});
                सचे (v3Global.opt.timing().isSetTrue()) {
                    AstDelay* ध्रुव delayp = नूतन AstDelay{flp, skewp->unlinkFrBack(), मिच्छा};
                    delayp->timeunit(m_modp->timeunit());
                    assignp->timingControlp(delayp);
                } अञ्ञथा सचे (v3Global.opt.timing().isSetFalse()) {
                    nodep->v3warn(E_NOTIMING,
                                  "Clocking output skew greater than #0 requires --timing");
                } अञ्ञथा {
                    nodep->v3warn(E_NEEDTIMINGOPT,
                                  "Use --timing or --no-timing to specify how "
                                  "clocking output skew greater than #0 should be handled");
                }
            }
        } अञ्ञथा सचे (nodep->direction() == VDirection::निवेशः) {
            // Ref to the clockvar
            AstVarRef* ध्रुव refp = नूतन AstVarRef{flp, varp, VAccess::WRITE};
            refp->user1(सच्च);
            सचे (skewp->अङ्कः().is1Step()) {
                // #1step means the value that is sampled is always the signal's last value
                // before the clock edge (IEEE 1800-2023 14.4)
                AstSampled* ध्रुव sampledp
                    = नूतन AstSampled{flp, exprp->clone_वृक्षः_Pure(मिच्छा), exprp->dtypep(), सच्च};
                AstAssign* ध्रुव assignp = नूतन AstAssign{flp, refp, sampledp};
                m_clockingp->addNextHere(नूतन AstAlways{
                    flp, VAlwaysKwd::ALWAYS, m_clockingp->sentreep()->clone_वृक्षः(मिच्छा), assignp});
            } अञ्ञथा सचे (skewp->isZero()) {
                // #0 means the var has to be sampled in Observed (IEEE 1800-2023 14.13)
                AstAssign* ध्रुव assignp = नूतन AstAssign{flp, refp, exprp->clone_वृक्षः_Pure(मिच्छा)};
                m_clockingp->addNextHere(नूतन AstAlwaysObserved{
                    flp, m_clockingp->sentreep()->clone_वृक्षः(मिच्छा), assignp});
            } अञ्ञथा {
                // Create a queue where we'll store sampled values with timestamps
                AstSampleQueueDType* ध्रुव queueDtp
                    = नूतन AstSampleQueueDType{flp, exprp->dtypep()};
                m_netlistp->typeTablep()->addTypesp(queueDtp);
                AstVar* ध्रुव queueVarp
                    = नूतन AstVar{flp, VVarType::MODULETEMP, "__Vqueue__" + varp->name(), queueDtp};
                queueVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_clockingp->addNextHere(queueVarp);
                // Create a process like this:
                //     always queue.push(<sampled var>);
                AstCMethodHard* ध्रुव pushp = नूतन AstCMethodHard{
                    flp, नूतन AstVarRef{flp, queueVarp, VAccess::WRITE}, VCMethod::DYN_प्रक्षिप,
                    नूतन AstTime{nodep->fileline(), m_modp->timeunit()}};
                pushp->addPinsp(exprp->clone_वृक्षः_Pure(मिच्छा));
                pushp->dtypeSetVoid();
                m_clockingp->addNextHere(
                    नूतन AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, pushp->makeStmt()});
                // Create a process like this:
                //     always @<clocking event> queue.pop(<skew>, /*out*/<skewed var>);
                AstCMethodHard* ध्रुव popp = नूतन AstCMethodHard{
                    flp, नूतन AstVarRef{flp, queueVarp, VAccess::READWRITE}, VCMethod::DYN_निष्कासय,
                    नूतन AstTime{nodep->fileline(), m_modp->timeunit()}};
                popp->addPinsp(skewp->unlinkFrBack());
                refp->access(VAccess::READWRITE);  // Only conditionally assigned
                popp->addPinsp(refp);
                popp->dtypeSetVoid();
                m_clockingp->addNextHere(नूतन AstAlways{flp, VAlwaysKwd::ALWAYS,
                                                       m_clockingp->sentreep()->clone_वृक्षः(मिच्छा),
                                                       popp->makeStmt()});
            }
        } अञ्ञथा {
            nodep->v3fatalSrc("Invalid direction");
        }
    }
    शून्य visit(AstDelay* nodep) override {
        m_hasCycleDelay |= nodep->isCycleDelay();
        // Only cycle delays are relevant in this stage; also only process once
        सचे (!nodep->isCycleDelay()) {
            सचे (m_inSynchDrive) {
                nodep->v3error("Only cycle delays can be used in synchronous drives"
                               " (IEEE 1800-2023 14.16)");
            }
            iterateChildren(nodep);
            पटिनिवत्त;
        }
        सचे (m_inAssign && !m_inSynchDrive) {
            nodep->v3error("Cycle delays not allowed as intra-assignment delays"
                           " (IEEE 1800-2023 14.11)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्षः(), nodep);
            पटिनिवत्त;
        }
        सचे (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBackWithNext());
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* valuep = V3Const::constifyEdit(nodep->lhsp()->unlinkFrBack());
        ध्रुव AstConst* ध्रुव constp = VN_CAST(valuep, Const);
        सचे (!constp && m_pexprp) {
            // V3AssertNfa handles non-const delays in sequences before this pass and
            // replaces the property; this branch should never be reached.
            nodep->v3fatalSrc("Non-constant cycle delay in assertion: "
                              "should have been caught by V3AssertNfa");
        } अञ्ञथा सचे (constp && constp->isZero()) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(valuep), valuep);
            सचे (m_inSynchDrive) {
                // ##0 has no effect in synchronous drives (IEEE 1800-2023 14.11)
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
                पटिनिवत्त;
            }
            सचे (m_pexprp) {
                // ##0 in sequence context = zero delay = same clock tick
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
                पटिनिवत्त;
            }
            // Procedural ##0: synchronize with default clocking event (IEEE 1800-2023 14.11)
            सचे (!m_defaultClockingp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
                पटिनिवत्त;
            }
            nodep->replaceWith(newCycleDelayZerop(flp));
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            पटिनिवत्त;
        }
        Ast_Sen_वृक्षः* sentreep = nullptr;
        सचे (!m_drives.empty()) {
            सचे (m_inConcatAssign) {
                // Also the clockvars may be of different clocking blocks
                nodep->v3error("Cycle delays cannot be used in synchronous drives to a"
                               " concatenation (IEEE 1800-2023 14.16)");
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(प्रक्षिप_Deletep(valuep), valuep);
                पटिनिवत्त;
            }
            // Count the cycles of the driven clockvar's clocking block (IEEE 1800-2023 14.16)
            sentreep = newDriveSentreep(nodep, m_drives.front());
            सचे (!sentreep) {
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(प्रक्षिप_Deletep(valuep), valuep);
                पटिनिवत्त;
            }
        } अञ्ञथा सचे (!m_defaultClockingp) {
            सचे (!m_pexprp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(प्रक्षिप_Deletep(valuep), valuep);
                पटिनिवत्त;
            }
            sentreep = नूतन Ast_Sen_वृक्षः{flp, m_senip->clone_वृक्षः(मिच्छा)};
        } अञ्ञथा {
            sentreep = m_defaultClockingp->sentreep()->clone_वृक्षः(मिच्छा);
        }
        AstEventControl* ध्रुव controlp
            = नूतन AstEventControl{nodep->fileline(), sentreep, nullptr};
        ध्रुव std::string delayName = m_cycleDlyNames.get(nodep);
        Ast_ग्रन्थिः_व्यञ्जकम्* throughoutp
            = nodep->throughoutp() ? nodep->throughoutp()->unlinkFrBack() : nullptr;

        AstVar* ध्रुव cntVarp = नूतन AstVar{flp, VVarType::BLOCKTEMP, delayName + "__counter",
                                           nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstBegin* ध्रुव beginp = नूतन AstBegin{flp, delayName + "__block", cntVarp, सच्च};
        // A non-constant delay is counted at runtime. The count is integral of 64 bits, as
        // V3Width makes delays, or real, which is rounded (IEEE 1800-2023 6.12.1)
        सचे (valuep->isDouble()) {
            valuep = नूतन AstRToIRoundS{flp, valuep};
            valuep->dtypeFrom(cntVarp);
        } अञ्ञथा {
            valuep = नूतन AstSel{flp, valuep, 0, cntVarp->width()};
        }
        beginp->addStmtsp(नूतन AstAssign{flp, नूतन AstVarRef{flp, cntVarp, VAccess::WRITE}, valuep});
        // If the count is zero, a synchronous drive waits for none, like with a constant '##0',
        // and a procedural delay waits like a procedural '##0' (IEEE 1800-2023 14.11)
        सचे (!constp && !m_inSynchDrive) {
            beginp->addStmtsp(नूतन AstIf{
                flp,
                नूतन AstEq{flp, नूतन AstVarRef{flp, cntVarp, VAccess::READ}, नूतन AstConst{flp, 0}},
                newCycleDelayZerop(flp)});
        }

        // Throughout: create flag tracking whether condition held every tick
        AstVar* throughoutOkp = nullptr;
        सचे (throughoutp) {
            throughoutOkp = नूतन AstVar{flp, VVarType::BLOCKTEMP, delayName + "__throughoutOk",
                                       nodep->findBasicDType(VBasicDTypeKwd::LOGIC_IMPLICIT)};
            throughoutOkp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->addStmtsp(throughoutOkp);
            beginp->addStmtsp(नूतन AstAssign{flp, नूतन AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            नूतन AstConst{flp, AstConst::BitTrue{}}});
            // Check condition at tick 0 (sequence start, before entering loop)
            beginp->addStmtsp(
                नूतन AstIf{flp, नूतन AstLogNot{flp, throughoutp->clone_वृक्षः_Pure(मिच्छा)},
                          नूतन AstAssign{flp, नूतन AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                        नूतन AstConst{flp, AstConst::BitFalse{}}}});
        }

        {
            AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
            // When throughout is present, exit loop early if condition fails
            Ast_ग्रन्थिः_व्यञ्जकम्* loopCondp
                = नूतन AstGt{flp, नूतन AstVarRef{flp, cntVarp, VAccess::READ}, नूतन AstConst{flp, 0}};
            सचे (throughoutOkp) {
                loopCondp = नूतन AstLogAnd{flp, loopCondp,
                                          नूतन AstVarRef{flp, throughoutOkp, VAccess::READ}};
            }
            loopp->addStmtsp(नूतन AstLoopTest{flp, loopp, loopCondp});
            loopp->addStmtsp(controlp);
            loopp->addStmtsp(
                नूतन AstAssign{flp, नूतन AstVarRef{flp, cntVarp, VAccess::WRITE},
                              नूतन AstSub{flp, नूतन AstVarRef{flp, cntVarp, VAccess::READ},
                                         नूतन AstConst{flp, 1}}});
            // Check throughout condition at each tick during delay (IEEE 1800-2023 16.9.9)
            सचे (throughoutp) {
                loopp->addStmtsp(
                    नूतन AstIf{flp, नूतन AstLogNot{flp, throughoutp},
                              नूतन AstAssign{flp, नूतन AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            नूतन AstConst{flp, AstConst::BitFalse{}}}});
            }
            beginp->addStmtsp(loopp);
        }
        // Compose wrappers on remaining sequence: throughout gate (inner), disable iff (outer)
        Ast_ग्रन्थिः* remainp = nodep->nextp() ? nodep->nextp()->unlinkFrBackWithNext() : nullptr;
        सचे (throughoutOkp) {
            // If condition failed during delay, fail assertion
            remainp = नूतन AstIf{flp, नूतन AstVarRef{flp, throughoutOkp, VAccess::READ}, remainp,
                                नूतन AstPExprClause{flp, /*pass=*/मिच्छा}};
        }
        सचे (m_disableSeqIfp && remainp) {
            AstIf* ध्रुव disableSeqIfp = m_disableSeqIfp->clone_वृक्षः(मिच्छा);
            disableSeqIfp->addThensp(remainp);
            remainp = disableSeqIfp;
        }
        सचे (remainp) {
            सचे (throughoutOkp) {
                // throughoutOkp is declared in beginp scope -- check must be inside it
                beginp->addStmtsp(remainp);
            } अञ्ञथा {
                nodep->addNextHere(remainp);
            }
        }
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }
    शून्य visit(Ast_Sen_वृक्षः* nodep) override {
        सचे (m_inSynchDrive) {
            nodep->v3error("Event controls cannot be used in "
                           "synchronous drives (IEEE 1800-2023 14.16)");
        }

        ध्रुव AstSampled* sampledp;
        सचे (nodep->exists([&sampledp](ध्रुव AstSampled* ध्रुव sp) {
                sampledp = sp;
                पटिनिवत्त सच्च;
            })) {
            sampledp->v3warn(E_UNSUPPORTED, "Unsupported: $sampled inside sensitivity list");
        }
    }
    शून्य visit(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        UINFO(8, " -varref:  " << nodep);
        UINFO(8, " -varref-var-back:  " << nodep->varp()->backp());
        UINFO(8, " -varref-var-user1:  " << nodep->varp()->user1p());
        सचे (AstClockingItem* ध्रुव itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->firstAbovep(),
                ClockingItem)) {
            सचे (nodep->user1()) पटिनिवत्त;

            // ensure linking still works, this has to be done only once
            सचे (AstVarXRef* xrefp = VN_CAST(nodep, VarXRef)) {
                UINFO(8, " -clockvarxref-in:  " << xrefp);
                string dotted = xrefp->dotted();
                ध्रुव size_t dotPos = dotted.rfind('.');
                dotted.erase(dotPos, string::npos);
                xrefp->dotted(dotted);
                UINFO(8, " -clockvarxref-out: " << xrefp);
                m_xrefsp.emplace_back(xrefp);
            }

            सचे (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            सचे (nodep->access().isWriteOrRW()) {
                सचे (itemp->direction() == VDirection::OUTPUT) {
                    सचे (!m_inAssignDlyLhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    सचे (m_inAssign) {
                        m_inSynchDrive = सच्च;
                        m_drives.प्रक्षिप_back({itemp, nodep});
                    }
                } अञ्ञथा सचे (itemp->direction() == VDirection::निवेशः) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
            nodep->user1(सच्च);
        }
    }
    शून्य visit(AstMemberSel* nodep) override {
        सचे (AstClockingItem* ध्रुव itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->firstAbovep(),
                ClockingItem)) {
            सचे (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            सचे (nodep->access().isWriteOrRW()) {
                सचे (itemp->direction() == VDirection::OUTPUT) {
                    सचे (!m_inAssignDlyLhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    सचे (m_inAssign) {
                        m_inSynchDrive = सच्च;
                        m_drives.प्रक्षिप_back({itemp, nodep});
                    }
                } अञ्ञथा सचे (itemp->direction() == VDirection::निवेशः) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
        }
    }
    शून्य visit(Ast_ग्रन्थिः_Assign* nodep) override {
        सचे (nodep->user1()) पटिनिवत्त;
        VL_RESTORER(m_inAssign);
        VL_RESTORER(m_inSynchDrive);
        VL_RESTORER(m_inConcatAssign);
        VL_RESTORER_CLEAR(m_drives);
        m_inAssign = सच्च;
        m_inSynchDrive = मिच्छा;
        {
            // The braces of a concatenation are a replication, by one
            ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* lhsp = nodep->lhsp();
            सचे (ध्रुव AstReplicate* ध्रुव repp = VN_CAST(lhsp, Replicate)) lhsp = repp->srcp();
            m_inConcatAssign = VN_IS(lhsp, Concat);
        }
        {
            VL_RESTORER(m_inAssignDlyLhs);
            m_inAssignDlyLhs = VN_IS(nodep, AssignDly);
            iterate(nodep->lhsp());
        }
        iterate(nodep->rhsp());
        सचे (nodep->timingControlp()) iterate(nodep->timingControlp());
        // A drive without a cycle delay, also after removing '##0', updates the clockvar now
        सचे (m_inSynchDrive && !nodep->timingControlp()) {
            AstAssign* ध्रुव assignp = नूतन AstAssign{
                nodep->fileline(), nodep->lhsp()->unlinkFrBack(), nodep->rhsp()->unlinkFrBack()};
            assignp->user1(सच्च);
            nodep->replaceWith(assignp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            nodep = assignp;
        }
        // Note the drives when they take effect, after the cycle delay if any. Each clockvar
        // of the LHS is driven, e.g. in a concatenation which other simulators accept, although
        // IEEE 1800-2023 14.16 does not allow it
        Ast_ग्रन्थिः* setsp = nullptr;
        अत्थाय (ध्रुव SynchDrive& drive : m_drives) {
            setsp = Ast_ग्रन्थिः::addNext(setsp, newDrivenSetp(nodep->fileline(), drive));
        }
        सचे (!setsp) पटिनिवत्त;
        सचे (AstBegin* ध्रुव beginp = VN_CAST(nodep->timingControlp(), Begin)) {
            beginp->addStmtsp(setsp);
        } अञ्ञथा {
            nodep->addNextHere(setsp);
        }
    }
    शून्य visit(AstAlways* nodep) override {
        iterateAndNextNull(nodep->sentreep());
        सचे (nodep->sentreep()) m_seniAlwaysp = nodep->sentreep()->sensesp();
        iterateAndNextNull(nodep->stmtsp());
        m_seniAlwaysp = nullptr;
    }

    शून्य visit(Ast_ग्रन्थिः_Cover_Or_Assert* nodep) override {
        सचे (nodep->sentreep()) पटिनिवत्त;  // Already processed

        VL_RESTORER(m_senip);
        VL_RESTORER(m_disablep);
        m_senip = nullptr;
        m_disablep = nullptr;

        // Find Clocking's buried under nodep->exprsp
        iterateChildren(nodep);
        सचे (!nodep->immediate()) nodep->sentreep(new_Sen_वृक्षः(nodep, nullptr, nodep));
    }
    शून्य visit(AstFalling* nodep) override {
        सचे (nodep->user1SetOnce()) पटिनिवत्त;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* exprp = nodep->exprp()->unlinkFrBack();
        सचे (exprp->width() > 1) exprp = नूतन AstSel{fl, exprp, 0, 1};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव futurep = नूतन AstFuture{fl, exprp, new_Sen_वृक्षः(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = नूतन AstAnd{fl, exprp->clone_वृक्षः_Pure(मिच्छा), नूतन AstNot{fl, futurep}};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य visit(AstFell* nodep) override {
        सचे (nodep->user1SetOnce()) पटिनिवत्त;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* exprp = nodep->exprp()->unlinkFrBack();
        सचे (exprp->width() > 1) exprp = नूतन AstSel{fl, exprp, 0, 1};
        Ast_Sen_वृक्षः* sentreep = nodep->sentreep();
        सचे (sentreep) sentreep->unlinkFrBack();
        AstPast* ध्रुव pastp = नूतन AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_वृक्षः(nodep, sentreep));
        exprp = नूतन AstAnd{fl, pastp, नूतन AstNot{fl, exprp->clone_वृक्षः_Pure(मिच्छा)}};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य visit(AstFuture* nodep) override {
        सचे (nodep->user1SetOnce()) पटिनिवत्त;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        Ast_Sen_वृक्षः* ध्रुव sentreep = nodep->sentreep();
        सचे (sentreep) VL_DO_DANGLING(प्रक्षिप_Deletep(sentreep->unlinkFrBack()), sentreep);
        nodep->sentreep(new_Sen_वृक्षः(nodep));
    }
    शून्य visit(AstPast* nodep) override {
        checkSamplingFuncDType(nodep, nodep->exprp());
        सचे (nodep->sentreep()) पटिनिवत्त;  // Already processed
        iterateChildren(nodep);
        nodep->sentreep(new_Sen_वृक्षः(nodep));
    }
    शून्य visit(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- Lower standalone exact [*N] (N >= 2) via saturating counter.
        // Range/unbounded forms and SExpr-contained forms are lowered by V3AssertNfa.
        iterateChildren(nodep);
        // V3AssertNfa handles unbounded/ranged forms upstream, so this fast-path
        // is effectively unreachable when NFA is enabled.
        सचे (nodep->unbounded() || nodep->maxCountp()) पटिनिवत्त;  // LCOV_EXCL_LINE
        ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep->countp(), Const);
        सचे (VL_UNLIKELY(!constp || constp->toSInt() < 1)) {
            nodep->v3fatalSrc("Consecutive repetition count must be a positive constant"
                              " (should have been caught by V3Width)");
            पटिनिवत्त;
        }
        ध्रुव पूर्णाङ्क n = constp->toSInt();
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = nodep->exprp()->unlinkFrBack();
        सचे (n == 1) {
            nodep->replaceWith(exprp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            पटिनिवत्त;
        }
        // Saturating counter: if (expr) cnt <= min(cnt+1, N); else cnt <= 0;
        AstVar* ध्रुव cntVarp = नूतन AstVar{flp, VVarType::MODULETEMP, m_consRepNames.get(""),
                                           nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(cntVarp);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव व्यञ्जकम्_Clonep = exprp->clone_वृक्षः_Pure(मिच्छा);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव saturatingIncrp = नूतन AstCond{
            flp,
            नूतन AstLt{flp, नूतन AstVarRef{flp, cntVarp, VAccess::READ},
                      नूतन AstConst{flp, static_cast<uint32_t>(n)}},
            नूतन AstAdd{flp, नूतन AstVarRef{flp, cntVarp, VAccess::READ}, नूतन AstConst{flp, 1u}},
            नूतन AstConst{flp, static_cast<uint32_t>(n)}};
        AstAssignDly* ध्रुव incrAssignp
            = नूतन AstAssignDly{flp, नूतन AstVarRef{flp, cntVarp, VAccess::WRITE}, saturatingIncrp};
        AstAssignDly* ध्रुव resetAssignp = नूतन AstAssignDly{
            flp, नूतन AstVarRef{flp, cntVarp, VAccess::WRITE}, नूतन AstConst{flp, 0u}};
        AstIf* ध्रुव ifp = नूतन AstIf{flp, व्यञ्जकम्_Clonep, incrAssignp, resetAssignp};
        Ast_Sen_वृक्षः* ध्रुव senTreep = new_Sen_वृक्षः(nodep);
        AstAlways* ध्रुव alwaysp = नूतन AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, ifp};
        cntVarp->addNextHere(alwaysp);
        // Match: cnt >= N-1 (previous cycles via NBA) && expr (current cycle)
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव cntCheckp = नूतन AstGte{flp, नूतन AstVarRef{flp, cntVarp, VAccess::READ},
                                                  नूतन AstConst{flp, static_cast<uint32_t>(n - 1)}};
        cntCheckp->dtypeSetBit();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव matchp = नूतन AstAnd{flp, cntCheckp, exprp};
        matchp->dtypeSetBit();
        nodep->replaceWith(matchp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य visit(AstRising* nodep) override {
        सचे (nodep->user1SetOnce()) पटिनिवत्त;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* exprp = nodep->exprp()->unlinkFrBack();
        सचे (exprp->width() > 1) exprp = नूतन AstSel{fl, exprp, 0, 1};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव futurep = नूतन AstFuture{fl, exprp, new_Sen_वृक्षः(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = नूतन AstAnd{fl, नूतन AstNot{fl, exprp->clone_वृक्षः_Pure(मिच्छा)}, futurep};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य visit(AstRose* nodep) override {
        सचे (nodep->user1SetOnce()) पटिनिवत्त;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* exprp = nodep->exprp()->unlinkFrBack();
        सचे (exprp->width() > 1) exprp = नूतन AstSel{fl, exprp, 0, 1};
        Ast_Sen_वृक्षः* sentreep = nodep->sentreep();
        सचे (sentreep) sentreep->unlinkFrBack();
        AstPast* ध्रुव pastp = नूतन AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_वृक्षः(nodep, sentreep));
        exprp = नूतन AstAnd{fl, नूतन AstNot{fl, pastp}, exprp->clone_वृक्षः_Pure(मिच्छा)};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    स्थिर AstAssocArrayDType* getProcessAssocArrayType(File_पङ्क्तिः* ध्रुव flp) {
        // Type of __VassertsActive___x[std::process]
        Ast_ग्रन्थिः_DType* valp
            = v3Global.rootp()->typeTablep()->findBasicDType(flp, VBasicDTypeKwd::BIT);
        AstClassRefDType* keyp
            = नूतन AstClassRefDType{flp, v3Global.rootp()->stdPackageProcessp(), nullptr};
        keyp->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        v3Global.rootp()->typeTablep()->addTypesp(keyp);
        AstAssocArrayDType* ध्रुव typep = नूतन AstAssocArrayDType{flp, valp, keyp};
        typep->dtypep(typep);
        v3Global.rootp()->typeTablep()->addTypesp(typep);
        पटिनिवत्त typep;
    }
    स्थिर Ast_Stmt_व्यञ्जकम्* getProcessAssocArrayDelete(AstVarRef* ध्रुव refp) {
        // Constructs refp.delete(std::process::self()) statement
        File_पङ्क्तिः* ध्रुव flp = refp->fileline();
        AstCMethodHard* ध्रुव deletep = नूतन AstCMethodHard{
            flp, refp, VCMethod::ASSOC_ERASE, v3Global.rootp()->stdPackageProcessSelfp(flp)};
        deletep->dtypep(refp->findVoidDType());
        पटिनिवत्त नूतन Ast_Stmt_व्यञ्जकम्{flp, deletep};
    }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* getProcessAssocArraySize(AstVarRef* ध्रुव refp) {
        // Constructs refp.size() statement
        AstCMethodHard* ध्रुव sizep
            = नूतन AstCMethodHard{refp->fileline(), refp, VCMethod::ASSOC_SIZE};
        sizep->dtypep(refp->findBasicDType(VBasicDTypeKwd::UINT32));
        पटिनिवत्त sizep;
    }
    शून्य visit(AstSEventually* nodep) override {
        UASSERT(v3Global.rootp()->stdPackagep(), "Should be imported");

        Ast_Sen_वृक्षः* ध्रुव sentreep = new_Sen_वृक्षः(nodep);
        सचे (!sentreep->sensesp()) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(sentreep), sentreep);
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            पटिनिवत्त;
        }

        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();

        // Track active assertions
        AstVar* ध्रुव activep = नूतन AstVar{flp, VVarType::MODULETEMP, m_activeNames.get(""),
                                           getProcessAssocArrayType(flp)};
        activep->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(activep);

        // Assertion condition check
        AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
        AstSampled* ध्रुव condp
            = नूतन AstSampled{flp, nodep->exprp()->unlinkFrBack(), nullptr, सच्च};
        loopp->addStmtsp(नूतन AstLoopTest{flp, loopp, नूतन AstLogNot{flp, condp}});
        loopp->addStmtsp(नूतन AstEventControl{flp, sentreep, nullptr});

        // Add assertion to the active set
        AstAssocSel* ध्रुव selp = नूतन AstAssocSel{flp, नूतन AstVarRef{flp, activep, VAccess::WRITE},
                                                  v3Global.rootp()->stdPackageProcessSelfp(flp)};
        AstAssign* ध्रुव incrementp = नूतन AstAssign{flp, selp, नूतन AstConst{flp, 1}};
        AstPExprClause* ध्रुव clausep = नूतन AstPExprClause{flp};
        Ast_Stmt_व्यञ्जकम्* ध्रुव deletep
            = getProcessAssocArrayDelete(नूतन AstVarRef{flp, activep, VAccess::WRITE});

        // Main assertion block
        AstBegin* ध्रुव bodyp = नूतन AstBegin{flp, m_blockNames.get(""), nullptr, सच्च};
        bodyp->addStmtsp(incrementp);
        bodyp->addStmtsp(loopp);
        bodyp->addStmtsp(clausep);
        bodyp->addStmtsp(deletep);

        // Validate assertion condition for each active assert
        AstVar* ध्रुव activeCountp = नूतन AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        activeCountp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstAssign* ध्रुव initActiveCountp
            = नूतन AstAssign{flp, नूतन AstVarRef{flp, activeCountp, VAccess::WRITE},
                            getProcessAssocArraySize(नूतन AstVarRef{flp, activep, VAccess::READ})};
        AstLoop* ध्रुव finalLoopp = नूतन AstLoop{flp};
        AstIf* ध्रुव finalBodypCondp
            = नूतन AstIf{flp, condp->clone_वृक्षः_Pure(मिच्छा), नूतन AstPExprClause{flp},
                        नूतन AstPExprClause{flp, मिच्छा}};
        finalLoopp->addStmtsp(
            नूतन AstLoopTest{flp, finalLoopp,
                            नूतन AstNeq{flp, नूतन AstVarRef{flp, activeCountp, VAccess::READ},
                                       नूतन AstConst{flp, 0}}});
        finalLoopp->addStmtsp(finalBodypCondp);
        finalLoopp->addStmtsp(
            नूतन AstAssign{flp, नूतन AstVarRef{flp, activeCountp, VAccess::WRITE},
                          नूतन AstSub{flp, नूतन AstVarRef{flp, activeCountp, VAccess::READ},
                                     नूतन AstConst{flp, 1}}});

        // Final assertion block
        AstBegin* ध्रुव finalp = नूतन AstBegin{flp, m_blockNames.get(""), nullptr, सच्च};
        finalp->addStmtsp(activeCountp);
        finalp->addStmtsp(initActiveCountp);
        finalp->addStmtsp(finalLoopp);

        VL_RESTORER(m_pexprp);
        m_pexprp = नूतन AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
        VL_RESTORER(m_hasCycleDelay);
        m_hasCycleDelay = मिच्छा;
        iterate(bodyp);
        iterate(finalp);

        सचे (m_hasCycleDelay) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in s_eventually");
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            VL_DO_DANGLING(m_pexprp->delete_वृक्षः(), m_pexprp);
            पटिनिवत्त;
        }

        nodep->replaceWith(m_pexprp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य visit(AstStable* nodep) override {
        सचे (nodep->user1SetOnce()) पटिनिवत्त;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* exprp = nodep->exprp()->unlinkFrBack();
        Ast_Sen_वृक्षः* sentreep = nodep->sentreep();
        सचे (sentreep) sentreep->unlinkFrBack();
        AstPast* ध्रुव pastp = नूतन AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_वृक्षः(nodep, sentreep));
        exprp = नूतन AstEq{fl, pastp, exprp->clone_वृक्षः_Pure(मिच्छा)};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य visit(AstSteady* nodep) override {
        सचे (nodep->user1SetOnce()) पटिनिवत्त;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* exprp = nodep->exprp()->unlinkFrBack();
        सचे (exprp->width() > 1) exprp = नूतन AstSel{fl, exprp, 0, 1};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव futurep = नूतन AstFuture{fl, exprp, new_Sen_वृक्षः(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = नूतन AstEq{fl, exprp->clone_वृक्षः_Pure(मिच्छा), futurep};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य visit(AstSampled* nodep) override {
        iterateChildren(nodep);
        सचे (!nodep->internal()) checkSamplingFuncDType(nodep, nodep->exprp());
    }

    // Validate repetition count: must be a non-negative elaboration-time constant.
    // Shared by goto [->N] and nonconsecutive [=N] repetition.
    // On error, replaces nodep with BitFalse placeholder and returns nullptr.
    ध्रुव AstConst* validate_Rep_गणनम्(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः_व्यञ्जकम्*& countp) {
        countp = V3Const::constifyEdit(countp);
        ध्रुव AstConst* ध्रुव constp = VN_CAST(countp, Const);
        सचे (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(प्रक्षिप_Deletep(countp), countp);
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            पटिनिवत्त nullptr;
        }
        सचे (constp->toSInt() < 0) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(प्रक्षिप_Deletep(countp), countp);
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            पटिनिवत्त nullptr;
        }
        सचे (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(प्रक्षिप_Deletep(countp), countp);
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            पटिनिवत्त nullptr;
        }
        पटिनिवत्त constp;
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
    AstPExpr* createRepPExpr(File_पङ्क्तिः* flp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_ग्रन्थिः_व्यञ्जकम्* countp,
                             Ast_ग्रन्थिः_व्यञ्जकम्* rhsp, तार्किक isOverlapped, तार्किक isNonConsec) {
        AstSenItem* ध्रुव sensesp = m_senip;
        UASSERT_OBJ(sensesp, exprp, "Repetition requires a clock");

        ध्रुव std::string name
            = isNonConsec ? m_nonConsRepNames.get(exprp) : m_gotoRepNames.get(exprp);
        AstVar* ध्रुव cntVarp = नूतन AstVar{flp, VVarType::BLOCKTEMP, name + "__counter",
                                           exprp->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstBegin* ध्रुव beginp = नूतन AstBegin{flp, name + "__block", cntVarp, सच्च};
        beginp->addStmtsp(
            नूतन AstAssign{flp, नूतन AstVarRef{flp, cntVarp, VAccess::WRITE}, नूतन AstConst{flp, 0}});

        // Counting loop: find N occurrences of expr (shared by goto and nonconsec)
        AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
        loopp->addStmtsp(नूतन AstLoopTest{flp, loopp,
                                         नूतन AstLt{flp, नूतन AstVarRef{flp, cntVarp, VAccess::READ},
                                                   countp->clone_वृक्षः_Pure(मिच्छा)}});
        // if (expr) cnt++ -- sampled is applied to whole property expr by V3Assert
        loopp->addStmtsp(
            नूतन AstIf{flp, exprp,
                      नूतन AstAssign{flp, नूतन AstVarRef{flp, cntVarp, VAccess::WRITE},
                                    नूतन AstAdd{flp, नूतन AstVarRef{flp, cntVarp, VAccess::READ},
                                               नूतन AstConst{flp, 1}}}});
        loopp->addStmtsp(नूतन AstIf{
            flp, नूतन AstLt{flp, नूतन AstVarRef{flp, cntVarp, VAccess::READ}, countp},
            नूतन AstEventControl{flp, नूतन Ast_Sen_वृक्षः{flp, sensesp->clone_वृक्षः(मिच्छा)}, nullptr}});

        beginp->addStmtsp(loopp);

        सचे (isNonConsec && rhsp) {
            // IEEE 16.9.2: b[=N] = b[->N] ##1 !b[*0:$]
            // Trailing window: ##1 advance, then scan !expr cycles checking rhs.
            // Window closes when expr becomes true again (fail if rhs was never true).
            beginp->addStmtsp(नूतन AstEventControl{
                flp, नूतन Ast_Sen_वृक्षः{flp, sensesp->clone_वृक्षः(मिच्छा)}, nullptr});  // ##1
            सचे (!isOverlapped) {
                beginp->addStmtsp(नूतन AstEventControl{
                    flp, नूतन Ast_Sen_वृक्षः{flp, sensesp->clone_वृक्षः(मिच्छा)}, nullptr});  // |=>
            }
            // Window loop: check rhs at each !expr cycle (done variable for termination)
            AstVar* ध्रुव doneVarp
                = नूतन AstVar{flp, VVarType::BLOCKTEMP, name + "__done", exprp->findBitDType()};
            doneVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->addStmtsp(doneVarp);
            beginp->addStmtsp(नूतन AstAssign{flp, नूतन AstVarRef{flp, doneVarp, VAccess::WRITE},
                                            नूतन AstConst{flp, AstConst::BitFalse{}}});
            auto setDone = [&]() {
                पटिनिवत्त नूतन AstAssign{flp, नूतन AstVarRef{flp, doneVarp, VAccess::WRITE},
                                     नूतन AstConst{flp, AstConst::BitTrue{}}};
            };
            AstLoop* ध्रुव windowp = नूतन AstLoop{flp};
            // LoopTest: continue while !done
            windowp->addStmtsp(नूतन AstLoopTest{
                flp, windowp, नूतन AstNot{flp, नूतन AstVarRef{flp, doneVarp, VAccess::READ}}});
            // if (expr) { fail; done = 1; } -- window closed, expr true again
            Ast_ग्रन्थिः* ध्रुव failsp = नूतन AstPExprClause{flp, मिच्छा};
            failsp->addNext(setDone());
            windowp->addStmtsp(नूतन AstIf{flp, exprp->clone_वृक्षः_Pure(मिच्छा), failsp});
            // if (rhs) { pass; done = 1; } -- consequent true at this !expr endpoint
            Ast_ग्रन्थिः* ध्रुव passsp = नूतन AstPExprClause{flp, सच्च};
            passsp->addNext(setDone());
            windowp->addStmtsp(नूतन AstIf{flp, rhsp, passsp});
            // @(clk) -- advance to next cycle in window
            windowp->addStmtsp(
                नूतन AstEventControl{flp, नूतन Ast_Sen_वृक्षः{flp, sensesp->clone_वृक्षः(मिच्छा)}, nullptr});
            beginp->addStmtsp(windowp);
        } अञ्ञथा सचे (isNonConsec) {
            // Standalone nonconsec: ##1 into window, then pass
            beginp->addStmtsp(
                नूतन AstEventControl{flp, नूतन Ast_Sen_वृक्षः{flp, sensesp->clone_वृक्षः(मिच्छा)}, nullptr});
            beginp->addStmtsp(नूतन AstPExprClause{flp, सच्च});
        } अञ्ञथा सचे (rhsp) {
            // Goto rep: check consequent once at match endpoint
            सचे (!isOverlapped) {
                beginp->addStmtsp(नूतन AstEventControl{
                    flp, नूतन Ast_Sen_वृक्षः{flp, sensesp->clone_वृक्षः(मिच्छा)}, nullptr});
            }
            beginp->addStmtsp(नूतन AstIf{flp, rhsp, नूतन AstPExprClause{flp, सच्च},
                                        नूतन AstPExprClause{flp, मिच्छा}});
        } अञ्ञथा {
            // Standalone goto: pass after counting
            beginp->addStmtsp(नूतन AstPExprClause{flp, सच्च});
        }

        पटिनिवत्त नूतन AstPExpr{flp, beginp, exprp->findBitDType()};
    }

    // Reject [=M:N] nonconsecutive range that reached V3AssertPre.
    // [->M:N] is fully handled upstream (NFA path); only [=M:N] still lands
    // here because AstSNonConsRep lowering is not yet implemented there.
    // Replaces the node with BitFalse so lowering continues after the warning.
    तार्किक rejectNonconsecRange(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः_व्यञ्जकम्* maxCountp) {
        सचे (!maxCountp) पटिनिवत्त मिच्छा;
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: [=M:N] nonconsecutive range repetition"
                                     " (IEEE 1800-2023 16.9.2)");
        nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}});
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        पटिनिवत्त सच्च;
    }

    शून्य visit(AstSGotoRep* nodep) override {
        // Standalone goto rep (not inside implication antecedent)
        iterateChildren(nodep);
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* countp = nodep->countp()->unlinkFrBack();
        सचे (!validate_Rep_गणनम्(nodep, countp)) पटिनिवत्त;
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = nodep->exprp()->unlinkFrBack();
        AstPExpr* ध्रुव pexprp = createRepPExpr(flp, exprp, countp, nullptr, सच्च, मिच्छा);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    शून्य visit(AstSNonConsRep* nodep) override {
        // Standalone nonconsecutive rep (not inside implication antecedent)
        iterateChildren(nodep);
        सचे (rejectNonconsecRange(nodep, nodep->maxCountp())) पटिनिवत्त;
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* countp = nodep->countp()->unlinkFrBack();
        सचे (!validate_Rep_गणनम्(nodep, countp)) पटिनिवत्त;
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = nodep->exprp()->unlinkFrBack();
        AstPExpr* ध्रुव pexprp = createRepPExpr(flp, exprp, countp, nullptr, सच्च, सच्च);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    शून्य visit(AstFuncRef* nodep) override {
        // IEEE 1800-2023 16.8: Inline sequence instances wherever they appear
        // in the expression tree (inside implications, boolean ops, nested refs, etc.)
        सचे (AstSequence* ध्रुव seqp = VN_CAST(nodep->taskp(), Sequence)) {
            substituteSequenceCall(nodep, seqp);
            // The FuncRef has been replaced; do not access nodep after this point.
            // The replacement node will be visited by the parent's iterateChildren.
            पटिनिवत्त;
        }
        सचे (ध्रुव AstProperty* ध्रुव propp = VN_CAST(nodep->taskp(), Property)) {
            substituteNestedPropertyCall(nodep, propp);
            पटिनिवत्त;
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstImplication* nodep) override {
        सचे (nodep->sentreep()) पटिनिवत्त;  // Already processed

        // Top-level followed-by is claimed by V3AssertNfa; anything reaching here
        // is nested inside iff/implies/or. IEEE 1800-2023 16.12.9 permits the
        // composition, but silent lowering would drop non-vacuous-fail semantics.
        सचे (nodep->isFollowedBy()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: followed-by (#-# / #=#) nested inside property operator"
                          " (iff/implies/or) (IEEE 1800-2023 16.12.9)");
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            पटिनिवत्त;
        }

        // Handle goto repetition as antecedent before iterateChildren,
        // so the standalone AstSGotoRep visitor doesn't process it
        सचे (AstSGotoRep* ध्रुव gotop = VN_CAST(nodep->lhsp(), SGotoRep)) {
            iterateChildren(gotop);
            iterateAndNextNull(nodep->rhsp());
            File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
            Ast_ग्रन्थिः_व्यञ्जकम्* countp = gotop->countp()->unlinkFrBack();
            सचे (!validate_Rep_गणनम्(nodep, countp)) पटिनिवत्त;
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = gotop->exprp()->unlinkFrBack();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp()->unlinkFrBack();
            AstPExpr* ध्रुव pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), मिच्छा);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            पटिनिवत्त;
        }

        // Handle nonconsecutive repetition as antecedent before iterateChildren,
        // so the standalone AstSNonConsRep visitor doesn't process it
        सचे (AstSNonConsRep* ध्रुव ncrp = VN_CAST(nodep->lhsp(), SNonConsRep)) {
            iterateChildren(ncrp);
            iterateAndNextNull(nodep->rhsp());
            सचे (rejectNonconsecRange(nodep, ncrp->maxCountp())) पटिनिवत्त;
            File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
            Ast_ग्रन्थिः_व्यञ्जकम्* countp = ncrp->countp()->unlinkFrBack();
            सचे (!validate_Rep_गणनम्(nodep, countp)) पटिनिवत्त;
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = ncrp->exprp()->unlinkFrBack();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp()->unlinkFrBack();
            AstPExpr* ध्रुव pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), सच्च);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            पटिनिवत्त;
        }

        iterateChildren(nodep);

        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_ग्रन्थिः_व्यञ्जकम्* lhsp = nodep->lhsp()->unlinkFrBack();

        // Lower sequence match items (IEEE 16.11): (expr, var = val, ...) |-> / |=>
        सचे (Ast_व्यञ्जकम्_Stmt* ध्रुव व्यञ्जकम्_Stmtp = VN_CAST(lhsp, व्यञ्जकम्_Stmt)) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव antExprp = व्यञ्जकम्_Stmtp->resultp()->unlinkFrBack();

            सचे (nodep->isOverlapped()) {
                // |-> : assign to __Vpropvar via always_comb (continuous).
                // The assign evaluates RHS once; V3Sampled snapshots the
                // result so all consequent refs read the same value.
                Ast_ग्रन्थिः* matchAssignsp = nullptr;
                अत्थाय (Ast_ग्रन्थिः* stmtp = व्यञ्जकम्_Stmtp->stmtsp(); stmtp;) {
                    Ast_ग्रन्थिः* ध्रुव nextp = stmtp->nextp();
                    सचे (AstAssign* ध्रुव assignp = VN_CAST(stmtp, Assign)) {
                        assignp->unlinkFrBack();
                        matchAssignsp = Ast_ग्रन्थिः::addNextNull(matchAssignsp, assignp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(प्रक्षिप_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                सचे (matchAssignsp) {
                    AstAlways* ध्रुव alwaysp
                        = नूतन AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, matchAssignsp};
                    m_modp->addStmtsp(alwaysp);
                }
            } अञ्ञथा {
                // |=> : assign to __Vpropvar via NBA in a clocked always block.
                // The NBA commits before the next cycle's sampled snapshot,
                // so the consequent (which already references __Vpropvar)
                // sees the captured value.
                Ast_ग्रन्थिः* matchAssignsp = nullptr;
                अत्थाय (Ast_ग्रन्थिः* stmtp = व्यञ्जकम्_Stmtp->stmtsp(); stmtp;) {
                    Ast_ग्रन्थिः* ध्रुव nextp = stmtp->nextp();
                    सचे (AstAssign* ध्रुव assignp = VN_CAST(stmtp, Assign)) {
                        assignp->unlinkFrBack();
                        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव assignLhsp = assignp->lhsp()->unlinkFrBack();
                        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव assignRhsp = assignp->rhsp()->unlinkFrBack();
                        AstAssignDly* ध्रुव dlyp = नूतन AstAssignDly{flp, assignLhsp, assignRhsp};
                        VL_DO_DANGLING(प्रक्षिप_Deletep(assignp), assignp);
                        matchAssignsp = Ast_ग्रन्थिः::addNextNull(matchAssignsp, dlyp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(प्रक्षिप_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                सचे (matchAssignsp) {
                    AstIf* ध्रुव condp
                        = नूतन AstIf{flp, antExprp->clone_वृक्षः_Pure(मिच्छा), matchAssignsp};
                    AstAlways* ध्रुव alwaysp
                        = नूतन AstAlways{flp, VAlwaysKwd::ALWAYS, new_Sen_वृक्षः(nodep), condp};
                    m_modp->addStmtsp(alwaysp);
                }
            }
        }

        सचे (AstPExpr* ध्रुव pexprp = VN_CAST(rhsp, PExpr)) {
            // Implication with sequence expression on RHS (IEEE 1800-2023 16.11, 16.12.7).
            // The PExpr was lowered from the property expression earlier in this pass.
            // Wrap the PExpr body with the antecedent check so the sequence only
            // starts when the antecedent holds.
            Ast_ग्रन्थिः_व्यञ्जकम्* condp;
            सचे (nodep->isOverlapped()) {
                // Overlapped implication (|->): check antecedent on same cycle.
                // disable iff is applied at the assertion level, not at the
                // antecedent gate, matching the existing non-PExpr overlapped path.
                condp = lhsp;
            } अञ्ञथा {
                // Non-overlapped implication (|=>): check antecedent from previous cycle
                सचे (m_disablep) {
                    lhsp
                        = नूतन AstAnd{flp, नूतन AstNot{flp, m_disablep->clone_वृक्षः_Pure(मिच्छा)}, lhsp};
                }
                AstPast* ध्रुव pastp
                    = नूतन AstPast{flp, lhsp, nullptr, nullptr, /* propertyTiming */ सच्च};
                pastp->dtypeFrom(lhsp);
                pastp->sentreep(new_Sen_वृक्षः(nodep));
                condp = pastp;
            }
            // Wrap existing PExpr body: if (antecedent) { <original body> } else { /* vacuous pass
            // */ }
            // Only the statements are guarded; declarations stay directly in the block. After
            // V3Fork moves the block's statements into a task, V3Task only handles variables
            // that are direct statements of the task.
            AstBegin* ध्रुव bodyp = pexprp->bodyp();
            AstIf* ध्रुव guardp = नूतन AstIf{flp, condp};
            अत्थाय (Ast_ग्रन्थिः* stmtp = bodyp->stmtsp(); stmtp;) {
                Ast_ग्रन्थिः* ध्रुव nextp = stmtp->nextp();
                सचे (!VN_IS(stmtp, Var)) guardp->addThensp(stmtp->unlinkFrBack());
                stmtp = nextp;
            }
            bodyp->addStmtsp(guardp);
            nodep->replaceWith(pexprp);
            // Don't iterate pexprp here -- it was already iterated when created
            // (in visit(AstSExpr*)), so delays and disable iff are already processed.
        } अञ्ञथा सचे (nodep->isOverlapped()) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp
                = m_underCover ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstLogAnd{flp, lhsp, rhsp})
                               : नूतन AstLogOr{flp, नूतन AstLogNot{flp, lhsp}, rhsp};
            nodep->replaceWith(exprp);
        } अञ्ञथा {
            सचे (m_disablep) {
                lhsp = नूतन AstAnd{flp, नूतन AstNot{flp, m_disablep->clone_वृक्षः_Pure(मिच्छा)}, lhsp};
            }

            AstPast* ध्रुव pastp
                = नूतन AstPast{flp, lhsp, nullptr, nullptr, /* propertyTiming */ सच्च};
            pastp->dtypeFrom(lhsp);
            pastp->sentreep(new_Sen_वृक्षः(nodep));
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp
                = m_underCover ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstAnd{flp, pastp, rhsp})
                               : नूतन AstOr{flp, नूतन AstNot{flp, pastp}, rhsp};
            exprp->dtypeSetBit();
            nodep->replaceWith(exprp);
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य visit(AstUntil* nodep) override {
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        UASSERT_OBJ(
            !m_pexprp, nodep,
            "'" << nodep->verilogKwd()
                << "' in complex property expression should have been rejected by V3AssertNfa");
        सचे (nodep->isStrong()
            && (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue())) {
            nodep->v3warn(E_NOTIMING, nodep->verilogKwd() << " requires --timing");
            nodep->replaceWith(नूतन AstConst{flp, AstConst::BitFalse{}});
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            पटिनिवत्त;
        }
        सचे (nodep->isStrong()) {
            // p s_until q / p s_until_with q: q must eventually be true. Until then, p must
            // be true on every sampled tick. For s_until, check q first: when q is true on
            // this tick, p is not required. For s_until_with, p must be true on the q tick too.
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rawLhsp = nodep->lhsp()->unlinkFrBack();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rawRhsp = nodep->rhsp()->unlinkFrBack();
            AstSampled* ध्रुव lhsp = नूतन AstSampled{flp, rawLhsp, rawLhsp->dtypep(), सच्च};
            AstSampled* ध्रुव rhsp = नूतन AstSampled{flp, rawRhsp, rawRhsp->dtypep(), सच्च};
            Ast_ग्रन्थिः_व्यञ्जकम्* finalCondp = rhsp->clone_वृक्षः_Pure(मिच्छा);
            सचे (nodep->isOverlapping()) {
                finalCondp = नूतन AstLogAnd{flp, lhsp->clone_वृक्षः_Pure(मिच्छा), finalCondp};
            }

            // Track active assertion attempts. Only the count is needed to emit final failures.
            AstVar* ध्रुव activep = नूतन AstVar{flp, VVarType::MODULETEMP, m_activeNames.get(""),
                                               nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            activep->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(activep);

            AstVar* ध्रुव donep
                = नूतन AstVar{flp, VVarType::BLOCKTEMP, "__VassertDone", nodep->findBitDType()};
            donep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            auto setDone = [&]() {
                पटिनिवत्त नूतन AstAssign{flp, नूतन AstVarRef{flp, donep, VAccess::WRITE},
                                     नूतन AstConst{flp, AstConst::BitTrue{}}};
            };

            AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
            AstAssign* ध्रुव decrementVar = नूतन AstAssign{
                flp, नूतन AstVarRef{flp, activep, VAccess::WRITE},
                नूतन AstSub{flp, नूतन AstVarRef{flp, activep, VAccess::READ}, नूतन AstConst{flp, 1}}};
            loopp->addStmtsp(नूतन AstLoopTest{
                flp, loopp, नूतन AstLogNot{flp, नूतन AstVarRef{flp, donep, VAccess::READ}}});
            {
                Ast_ग्रन्थिः* ध्रुव passp = नूतन AstPExprClause{flp};
                passp->addNext(decrementVar);
                passp->addNext(setDone());
                Ast_ग्रन्थिः_व्यञ्जकम्* passCondp = rhsp;
                सचे (nodep->isOverlapping()) {
                    passCondp = नूतन AstLogAnd{flp, lhsp->clone_वृक्षः_Pure(मिच्छा), passCondp};
                }
                loopp->addStmtsp(नूतन AstIf{flp, passCondp, passp});
            }
            {
                Ast_ग्रन्थिः* ध्रुव failp = नूतन AstPExprClause{flp, मिच्छा};
                failp->addNext(decrementVar->clone_वृक्षः(मिच्छा));
                failp->addNext(setDone());
                loopp->addStmtsp(नूतन AstIf{
                    flp,
                    नूतन AstLogAnd{flp,
                                  नूतन AstLogNot{flp, नूतन AstVarRef{flp, donep, VAccess::READ}},
                                  नूतन AstLogNot{flp, lhsp}},
                    failp});
            }
            AstDelay* ध्रुव delayp = नूतन AstDelay{flp, नूतन AstConst{flp, 1}, मिच्छा};
            delayp->timeunit(m_modp->timeunit());
            loopp->addStmtsp(नूतन AstIf{
                flp, नूतन AstLogNot{flp, नूतन AstVarRef{flp, donep, VAccess::READ}}, delayp});

            // Main assertion block
            AstBegin* ध्रुव bodyp = नूतन AstBegin{flp, m_blockNames.get(""), nullptr, सच्च};
            bodyp->addStmtsp(donep);
            bodyp->addStmtsp(नूतन AstAssign{flp, नूतन AstVarRef{flp, donep, VAccess::WRITE},
                                           नूतन AstConst{flp, AstConst::BitFalse{}}});
            File_पङ्क्तिः* ध्रुव flp = activep->fileline();
            bodyp->addStmtsp(
                नूतन AstAssign{flp, नूतन AstVarRef{flp, activep, VAccess::WRITE},
                              नूतन AstAdd{flp, नूतन AstVarRef{flp, activep, VAccess::READ},
                                         नूतन AstConst{flp, 1}}});
            bodyp->addStmtsp(loopp);

            // Validate assertion condition for each active assert
            AstVar* ध्रुव activeCountp = नूतन AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                    nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            activeCountp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

            AstAssign* ध्रुव initActiveCountp
                = नूतन AstAssign{flp, नूतन AstVarRef{flp, activeCountp, VAccess::WRITE},
                                नूतन AstVarRef{flp, activep, VAccess::READ}};
            AstLoop* ध्रुव finalLoopp = नूतन AstLoop{flp};
            finalLoopp->addStmtsp(
                नूतन AstLoopTest{flp, finalLoopp,
                                नूतन AstNeq{flp, नूतन AstVarRef{flp, activeCountp, VAccess::READ},
                                           नूतन AstConst{flp, 0}}});
            finalLoopp->addStmtsp(नूतन AstIf{flp, finalCondp, नूतन AstPExprClause{flp},
                                            नूतन AstPExprClause{flp, मिच्छा}});
            finalLoopp->addStmtsp(
                नूतन AstAssign{flp, नूतन AstVarRef{flp, activeCountp, VAccess::WRITE},
                              नूतन AstSub{flp, नूतन AstVarRef{flp, activeCountp, VAccess::READ},
                                         नूतन AstConst{flp, 1}}});

            // Final assertion block
            AstBegin* ध्रुव finalp = नूतन AstBegin{flp, m_blockNames.get(""), nullptr, सच्च};
            finalp->addStmtsp(activeCountp);
            finalp->addStmtsp(initActiveCountp);
            finalp->addStmtsp(finalLoopp);

            AstPExpr* ध्रुव pexprp = नूतन AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
            VL_RESTORER(m_pexprp);
            VL_RESTORER(m_hasCycleDelay);
            m_pexprp = pexprp;
            m_hasCycleDelay = मिच्छा;
            iterate(bodyp);
            iterate(finalp);

            UASSERT_OBJ(!m_hasCycleDelay, nodep,
                        "s_until cycle delay should have been handled by V3AssertNfa");

            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            पटिनिवत्त;
        }
        AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp()->unlinkFrBack();
        AstLogAnd* ध्रुव loopCondp = नूतन AstLogAnd{flp, lhsp, नूतन AstLogNot{flp, rhsp}};
        loopp->addStmtsp(नूतन AstLoopTest{flp, loopp, loopCondp});
        loopp->addStmtsp(नूतन AstEventControl{flp, new_Sen_वृक्षः(nodep), nullptr});

        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsCopyp = rhsp->clone_वृक्षः_Pure(मिच्छा);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव passCondp
            = nodep->isOverlapping() ? नूतन AstLogAnd{flp, lhsp->clone_वृक्षः_Pure(मिच्छा), rhsCopyp}
                                     : rhsCopyp;
        AstBegin* ध्रुव beginp = नूतन AstBegin{flp, m_blockNames.get(""), loopp, सच्च};
        beginp->addStmtsp(
            नूतन AstIf{flp, passCondp, नूतन AstPExprClause{flp}, नूतन AstPExprClause{flp, मिच्छा}});

        AstPExpr* ध्रुव pexprp = नूतन AstPExpr{flp, beginp, nodep->dtypep()};
        pexprp->user1(1);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    शून्य visit(AstDefaultDisable* nodep) override {
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
    }
    शून्य visit(AstInferredDisable* nodep) override {
        Ast_ग्रन्थिः* newp;
        सचे (m_defaultDisablep) {
            newp = m_defaultDisablep->condp()->clone_वृक्षः_Pure(सच्च);
        } अञ्ञथा {
            newp = नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    शून्य visit(AstPropSpec* nodep) override {
        nodep = substitutePropertyCall(nodep);
        iterateAndNextNull(nodep->sensesp());
        सचे (m_senip && m_senip != nodep->sensesp())
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Only one PSL clock allowed per assertion");
        ध्रुव AstCover* ध्रुव coverp = VN_CAST(nodep->backp(), Cover);
        ध्रुव तार्किक seqEvent = coverp && coverp->isSeqEvent();
        सचे (!nodep->disablep() && m_defaultDisablep && !seqEvent) {
            nodep->disablep(m_defaultDisablep->condp()->clone_वृक्षः_Pure(सच्च));
        }
        m_disablep = nodep->disablep();
        // Unlink and just keep a pointer to it, convert to sentree as needed
        m_senip = nodep->sensesp();
        iterateNull(nodep->disablep());
        सचे (!VN_AS(nodep->backp(), ग्रन्थिः_Cover_Or_Assert)->immediate()) {
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव propDtp = nodep->propp()->dtypep();
            nodep->propp(नूतन AstSampled{nodep->fileline(), nodep->propp()->unlinkFrBack(),
                                        propDtp->dtypep(), सच्च});
        }
        // cover counts non-vacuous matches only (IEEE 1800-2023 16.15.2), so an
        // implication antecedent must hold; assert passes vacuously instead.
        VL_RESTORER(m_underCover);
        m_underCover = VN_IS(nodep->backp(), Cover);
        iterate(nodep->propp());
        iterateNull(nodep->matchCountp());
    }
    शून्य visit(AstPExpr* nodep) override {
        // V3AssertNfa handles multi-cycle property expressions before this pass,
        // so the following unsupported paths are defensive and typically unreached.
        सचे (m_pexprp && m_pexprp->user1()) {  // LCOV_EXCL_START
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Complex property expression inside 'until''");
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            पटिनिवत्त;
        }  // LCOV_EXCL_STOP
        सचे (AstLogNot* ध्रुव notp = VN_CAST(nodep->backp(), LogNot)) {
            notp->replaceWith(nodep->unlinkFrBack());
            VL_DO_DANGLING(प्रक्षिप_Deletep(notp), notp);
            iterate(nodep);
            पटिनिवत्त;
        }
        // Sequence expression as antecedent of implication is not yet supported
        सचे (AstImplication* ध्रुव implp = VN_CAST(nodep->backp(), Implication)) {
            सचे (implp->lhsp() == nodep) {  // LCOV_EXCL_START
                implp->v3warn(E_UNSUPPORTED,
                              "Unsupported: Implication with sequence expression as antecedent");
                nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                पटिनिवत्त;
            }  // LCOV_EXCL_STOP
        }
        VL_RESTORER(m_pexprp);
        VL_RESTORER(m_disableSeqIfp);
        m_pexprp = nodep;

        सचे (m_disablep) {
            ध्रुव AstSampled* sampledp = nullptr;
            सचे (m_disablep->exists([&sampledp](ध्रुव AstSampled* ध्रुव sp) {
                    sampledp = sp;
                    पटिनिवत्त सच्च;
                })) {
                sampledp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: $sampled inside disabled condition of a sequence");
                m_disablep = नूतन AstConst{m_disablep->fileline(), AstConst::BitFalse{}};
                // always a copy is used, so remove it now
                प्रक्षिप_Deletep(m_disablep);  // m_disablep used later
            }
            File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
            // Add counter which counts times the condition turned true
            AstVar* ध्रुव disableCntp
                = नूतन AstVar{flp, VVarType::MODULETEMP, m_disableCntNames.get(""),
                             nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            disableCntp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(disableCntp);
            AstVarRef* ध्रुव readCntRefp = नूतन AstVarRef{flp, disableCntp, VAccess::READ};
            AstVarRef* ध्रुव writeCntRefp = नूतन AstVarRef{flp, disableCntp, VAccess::WRITE};
            AstAssign* ध्रुव incrStmtp = नूतन AstAssign{
                flp, writeCntRefp, नूतन AstAdd{flp, readCntRefp, नूतन AstConst{flp, 1}}};
            AstAlways* ध्रुव alwaysp
                = नूतन AstAlways{flp, VAlwaysKwd::ALWAYS,
                                नूतन Ast_Sen_वृक्षः{flp, नूतन AstSenItem{flp, VEdgeType::ET_POSEDGE,
                                                                   m_disablep->clone_वृक्षः(मिच्छा)}},
                                incrStmtp};
            disableCntp->addNextHere(alwaysp);

            // Store value of that counter at the beginning of sequence evaluation
            AstBegin* ध्रुव bodyp = nodep->bodyp();
            AstVar* ध्रुव initialCntp = नूतन AstVar{flp, VVarType::BLOCKTEMP, "__VinitialCnt",
                                                   nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            initialCntp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            AstAssign* ध्रुव assignp
                = नूतन AstAssign{flp, नूतन AstVarRef{flp, initialCntp, VAccess::WRITE},
                                readCntRefp->clone_वृक्षः(मिच्छा)};
            // Prepend to the sequence body to keep statement list structure valid.
            UASSERT_OBJ(!bodyp->name().empty(), bodyp, "Sequence body should be named");
            Ast_ग्रन्थिः* ध्रुव newStmtsp = Ast_ग्रन्थिः::addNext<Ast_ग्रन्थिः, Ast_ग्रन्थिः>(initialCntp, assignp);
            bodyp->prependStmtsp(newStmtsp);

            m_disableSeqIfp
                = नूतन AstIf{flp, नूतन AstEq{flp, नूतन AstVarRef{flp, initialCntp, VAccess::READ},
                                           readCntRefp->clone_वृक्षः(मिच्छा)}};
            // Delete it, because it is always copied before insetion to the AST
            प्रक्षिप_Deletep(m_disableSeqIfp);  // m_disableSeqIfp used later
        }
        iterateChildren(nodep);
    }
    शून्य visit(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultClkEvtVarp);
        VL_RESTORER(m_defaultDisablep);
        VL_RESTORER(m_modp);
        सचे (!VN_IS(nodep, Class)) {
            // A class nested in a module is in the scope of the module's default clocking
            m_defaultClockingp = nullptr;
            m_defaultClkEvtVarp = nullptr;
        }
        m_defaultDisablep = nodep->defaultDisablep();
        m_modp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    शून्य visit(AstProperty* nodep) override {
        // The body will be visited when will be substituted in place of property reference
        // (AstFuncRef)
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
    }
    शून्य visit(AstSequence* nodep) override {
        // Sequence declarations are not visited directly; their bodies are inlined
        // at call sites by visit(AstFuncRef*). Collect for post-traversal cleanup.
        m_seqsToCleanup.प्रक्षिप_back(nodep);
    }
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit AssertPreVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        { AssertPreClockingVisitor{nodep}; }
        // Process
        iterate(nodep);
        // Fix up varref names
        अत्थाय (AstVarXRef* xrefp : m_xrefsp) xrefp->name(xrefp->varp()->name());
        // Clean up sequence declarations after inlining.
        // Referenced sequences that were inlined have isReferenced cleared.
        // Remaining referenced sequences are in unsupported contexts (e.g. @seq event).
        अत्थाय (AstSequence* seqp : m_seqsToCleanup) {
            सचे (seqp->isReferenced()) {
                seqp->v3warn(E_UNSUPPORTED,
                             "Unsupported: sequence referenced outside assertion property");
            } अञ्ञथा {
                VL_DO_DANGLING(seqp->unlinkFrBack()->delete_वृक्षः(), seqp);
            }
        }
    }
    ~AssertPreVisitor() override = स्वभाव;
};

//######################################################################
// Top Assert class

शून्य V3AssertPre::assertPreAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { AssertPreVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("assertpre", 0, dump_वृक्षः_Either_Level() >= 3);
}
