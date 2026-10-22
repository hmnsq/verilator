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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3AssertPre.h"

#समावेश "V3Assert.h"
#समावेश "V3Const.h"
#समावेश "V3Task.h"
#समावेश "V3Timing.h"
#समावेश "V3UniqueNames.h"

#समावेश <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clocking blocks of clocking items

श्रेणी AssertPreClockingVisitor final : खुला VNVisitorConst {
    // Captures the clocking block of each clocking item before AssertPreVisitor, which needs it
    // at synchronous drives, often visited before the clocking block, e.g. in a parent module
    // NODE STATE
    // AstClockingItem::user3p()  // AstClocking*. Clocking block of the item, user3 claimed by
    //                            //               AssertPreVisitor

    // VISITORS
    व्योम visit(AstNetlist* nodep) override { iterate_And_अग्र_Const_Null(nodep->modulesp()); }
    व्योम visit(Ast_गाँठ_Module* nodep) override { iterate_And_अग्र_Const_Null(nodep->stmtsp()); }
    व्योम visit(AstGenBlock* nodep) override { iterate_And_अग्र_Const_Null(nodep->itemsp()); }
    व्योम visit(AstClocking* nodep) override {
        क्रम (Ast_गाँठ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            अगर (AstClockingItem* स्थिर citemp = VN_CAST(itemp, ClockingItem)) {
                citemp->user3p(nodep);
            }
        }
    }
    // Clocking blocks are only in modules and their generate blocks
    व्योम visit(Ast_गाँठ*) override {}

खुला:
    // CONSTRUCTORS
    explicit AssertPreClockingVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~AssertPreClockingVisitor() override = शेष;
};

//######################################################################
// Assert class functions

श्रेणी AssertPreVisitor final : खुला VNVisitor {
    // Removes clocks and other pre-optimizations
    // Eventually inlines calls to sequences, properties, etc.
    // We're not parsing the tree, or anything more complicated.
निजी:
    // TYPES
    काष्ठा SynchDrive final {
        AstClockingItem* itemp;  // Clocking item of the driven clockvar
        Ast_गाँठ_व्यंजक* refp;  // Reference to the driven clockvar
    };

    // NODE STATE
    // AstClockingItem::user1p()         // AstVar*.      varp() of ClockingItem after unlink
    // AstClockingItem::user2p()         // AstVar*.      Flag set by drives of output clockvar
    // AstClockingItem::user3p()         // AstClocking*. Clocking block of the item, captured
    //                                   //               by AssertPreClockingVisitor
    // AstPExpr::user1()                 // bool.         Created from AstUntil
    स्थिर VNUser1InUse m_inuser1;
    स्थिर VNUser2InUse m_inuser2;
    स्थिर VNUser3InUse m_inuser3;
    // STATE
    // Current context:
    AstNetlist* स्थिर m_netlistp = nullptr;  // Current netlist
    Ast_गाँठ_Module* m_modp = nullptr;  // Current module
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
    Ast_गाँठ_व्यंजक* m_disablep = nullptr;  // Last disable
    AstIf* m_disableSeqIfp = nullptr;  // Used for handling disable iff in sequences
    AstPExpr* m_pexprp = nullptr;  // Last AstPExpr
    bool m_underCover = false;  // True if the enclosing assertion is a cover
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
    bool m_inAssign = false;  // True if in an AssignNode
    bool m_inAssignDlyLhs = false;  // True if in AssignDly's LHS
    bool m_inSynchDrive = false;  // True if in synchronous drive
    bool m_inConcatAssign = false;  // True if in assignment to a concatenation
    std::vector<SynchDrive> m_drives;  // Clockvars written by the synchronous drive
    bool m_hasCycleDelay = false;  // True if node has cycle delay beneath
    std::vector<AstVarXRef*> m_xrefsp;  // list of xrefs that need name fixup
    std::vector<AstSequence*> m_seqsToCleanup;  // Sequences to clean up after traversal

    // METHODS

    // Flag set by each drive of an output clockvar, added to the module with its clocking item
    AstVar* getCreateDrivenVarp(AstClockingItem* itemp) {
        अगर (!itemp->user2p()) {
            AstVar* स्थिर varp = नया AstVar{itemp->fileline(), VVarType::MODULETEMP,
                                            m_drivenNames.लो(itemp), itemp->findBitDType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            itemp->user2p(varp);
        }
        वापस VN_AS(itemp->user2p(), Var);
    }
    // Reference to a variable in the scope of a driven clockvar, made like the clockvar's
    अटल Ast_गाँठ_व्यंजक* newDriveRefp(File_पंक्ति* flp, स्थिर SynchDrive& drive, AstVar* varp,
                                     स्थिर VAccess& access) {
        अगर (स्थिर AstVarXRef* स्थिर xrefp = VN_CAST(drive.refp, VarXRef)) {
            वापस नया AstVarXRef{flp, varp, xrefp->dotted(), access};
        } अन्यथा अगर (स्थिर AstMemberSel* स्थिर selp = VN_CAST(drive.refp, MemberSel)) {
            // The interface expression is evaluated again, so it must not have side effects;
            // cloneTreePure warns if it has any
            AstMemberSel* स्थिर newSelp
                = नया AstMemberSel{flp, selp->fromp()->clone_वृक्ष_Pure(false), varp};
            newSelp->access(access);
            वापस newSelp;
        }
        वापस नया AstVarRef{flp, varp, access};
    }
    // Assignment setting the drive flag of a driven clockvar
    AstAssign* newDrivenSetp(File_पंक्ति* flp, स्थिर SynchDrive& drive) {
        AstVar* स्थिर varp = getCreateDrivenVarp(drive.itemp);
        AstAssign* स्थिर setp = नया AstAssign{flp, newDriveRefp(flp, drive, varp, VAccess::WRITE),
                                              नया AstConst{flp, AstConst::BitTrue{}}};
        setp->user1(true);
        वापस setp;
    }
    // Clocking block of a clocking item
    अटल AstClocking* clockingOf(स्थिर AstClockingItem* itemp) {
        UASSERT_OBJ(itemp->user3p(), itemp, "Clocking item not captured in a clocking block");
        वापस VN_AS(itemp->user3p(), Clocking);
    }
    // Clocking event of a driven clockvar, referenced from the drive like the clockvar, or
    // nullptr if unsupported, which is reported on the drive's cycle delay
    Ast_Sen_वृक्ष* newDriveSentreep(AstDelay* delayp, स्थिर SynchDrive& drive) {
        Ast_Sen_वृक्ष* स्थिर origp = clockingOf(drive.itemp)->sentreep();
        अगर (VN_IS(drive.refp, VarRef)) वापस origp->clone_वृक्ष(false);
        अगर (origp->exists([](स्थिर AstVarXRef*) { वापस true; })) {
            // The reference is relative to the clocking block
            delayp->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in synchronous drive to"
                                          " clockvar in another scope, whose clocking event"
                                          " has a hierarchical reference");
            वापस nullptr;
        }
        स्थिर AstMemberSel* स्थिर selp = VN_CAST(drive.refp, MemberSel);
        // Processes outside classes evaluate their events also when not waiting on them,
        // possibly before the interface reference is set, so the event is guarded against a
        // null reference, which needs an integral value; scheduling does not guard it
        स्थिर bool guardNull = selp && !VN_IS(m_modp, Class);
        अगर (guardNull) {
            क्रम (AstSenItem* itemp = origp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                अगर (!itemp->sensp()->dtypep()->skipRefp()->isIntegralOrPacked()) {
                    delayp->v3warn(E_UNSUPPORTED,
                                   "Unsupported: cycle delay in synchronous drive through"
                                   " an interface reference outside a class, to clocking"
                                   " block with non-integral clocking event");
                    वापस nullptr;
                }
            }
        }
        // Interface declaring the clocking block, if driven through an interface reference
        AstIface* स्थिर ifacep
            = selp ? VN_AS(selp->fromp()->dtypep()->skipRefp(), IfaceRefDType)->ifaceViaCellp()
                   : nullptr;
        Ast_Sen_वृक्ष* स्थिर sentreep = origp->clone_वृक्ष(false);
        std::vector<AstVarRef*> refps;
        sentreep->foreach([&](AstVarRef* refp) {
            अगर (!refp->classOrPackagep()) refps.धकेलो_back(refp);
        });
        क्रम (AstVarRef* स्थिर refp : refps) {
            AstVar* स्थिर varp = refp->varp();
            अगर (ifacep) {
                // Mark the variable as read through an interface reference, as V3Width does for
                // a member select. The mark is the interface declaring the variable, whose
                // instances all share it, so drives through references to any instances agree.
                UASSERT_OBJ(!varp->sensIfacep() || varp->sensIfacep() == ifacep, refp,
                            "Variable read through references to different interfaces");
                varp->sensIfacep(ifacep);
            }
            refp->replaceWith(newDriveRefp(refp->fileline(), drive, varp, refp->access()));
            VL_DO_DANGLING(धकेलो_Deletep(refp), refp);
        }
        अगर (guardNull) {
            क्रम (AstSenItem* itemp = sentreep->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                guardNullIface(itemp, selp->fromp());
            }
        }
        वापस sentreep;
    }
    // Make an integral clocking event through an interface reference false while the reference
    // is null
    अटल व्योम guardNullIface(AstSenItem* sensesp, Ast_गाँठ_व्यंजक* fromp) {
        File_पंक्ति* स्थिर flp = sensesp->fileline();
        Ast_गाँठ_व्यंजक* स्थिर sensp = sensesp->sensp()->unlinkFrBack();
        sensesp->sensp(नया AstCond{flp, newNotNullp(flp, fromp), sensp,
                                   नया AstConst{flp, AstConst::DTyped{}, sensp->dtypep()}});
        अगर (Ast_गाँठ_व्यंजक* स्थिर condp = sensesp->condp()) {
            condp->unlinkFrBack();
            sensesp->condp(नया AstLogAnd{flp, newNotNullp(flp, fromp), condp});
        }
    }
    अटल Ast_गाँठ_व्यंजक* newNotNullp(File_पंक्ति* flp, Ast_गाँठ_व्यंजक* fromp) {
        वापस नया AstNeq{flp, fromp->clone_वृक्ष_Pure(false), नया AstConst{flp, AstConst::Null{}}};
    }
    // Procedural '##0': if the default clocking event has not yet occurred in this time step,
    // wait for it; otherwise continue without suspension (IEEE 1800-2023 14.11)
    AstIf* newCycleDelayZerop(File_पंक्ति* flp) स्थिर {
        AstVar* स्थिर evtVarp = m_defaultClkEvtVarp;
        UASSERT_OBJ(evtVarp, flp, "Default clocking event var not pre-created");
        AstCMethodHard* स्थिर isTriggeredp = नया AstCMethodHard{
            flp, नया AstVarRef{flp, evtVarp, VAccess::READ}, VCMethod::EVENT_IS_TRIGGERED};
        isTriggeredp->dtypeSetBit();
        AstEventControl* स्थिर waitp = नया AstEventControl{
            flp,
            नया Ast_Sen_वृक्ष{flp, नया AstSenItem{flp, VEdgeType::ET_EVENT,
                                               नया AstVarRef{flp, evtVarp, VAccess::READ}}},
            nullptr};
        वापस नया AstIf{flp, नया AstNot{flp, isTriggeredp}, waitp};
    }

    अटल व्योम checkSamplingFuncDType(Ast_गाँठ_व्यंजक* nodep, स्थिर Ast_गाँठ* exprp) {
        स्थिर Ast_गाँठ_DType* स्थिर dtypep = exprp->dtypep()->skipRefp();
        अगर (!dtypep->isIntegralOrPacked()) {
            nodep->v3error("Expected numeric type, but got a " << dtypep->prettyDTypeNameQ()
                                                               << " data type");
        }
    }

    Ast_Sen_वृक्ष* new_Sen_वृक्ष(Ast_गाँठ* nodep, Ast_Sen_वृक्ष* useTreep = nullptr,
                           Ast_गाँठ_Cover_Or_Assert* cassertp = nullptr) {
        // Create sentree based on clocked or default clock
        // Return nullptr for always
        अगर (useTreep) वापस useTreep;
        Ast_Sen_वृक्ष* newp = nullptr;
        AstSenItem* senip = m_senip;
        bool fromAlways = false;
        अगर (!senip && m_defaultClockingp) senip = m_defaultClockingp->sentreep()->sensesp();
        अगर (!senip) {
            senip = m_seniAlwaysp;
            fromAlways = true;
        }
        अगर (!senip) {
            nodep->v3error("Concurrent assertion has no clock (IEEE 1800-2023 16.16)\n"
                           << nodep->warnMore()
                           << "... Suggest provide a clocking event, a default"
                              " clocking, or a clocked procedural context");
            newp = नया Ast_Sen_वृक्ष{nodep->fileline(), nullptr};
        } अन्यथा {
            अगर (cassertp && fromAlways) cassertp->senFromAlways(true);
            newp = नया Ast_Sen_वृक्ष{nodep->fileline(), senip->clone_वृक्ष(true)};
        }
        वापस newp;
    }
    Ast_गाँठ_व्यंजक* getSequenceBodyExprp(स्थिर AstSequence* स्थिर seqp) स्थिर {
        // The statements in AstSequence are optional AstVar (ports) followed by body expr.
        Ast_गाँठ* bodyp = seqp->stmtsp();
        जबतक (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        वापस VN_CAST(bodyp, गाँठ_व्यंजक);
    }
    AstPropSpec* getPropertyExprp(स्थिर AstProperty* स्थिर propp) {
        // Statements in AstProperty: AstVar (ports/local vars),
        // AstInitialStaticStmt/AstInitialAutomaticStmt (var init), AstPropSpec (body).
        Ast_गाँठ* propExprp = propp->stmtsp();
        जबतक (propExprp
               && (VN_IS(propExprp, Var) || VN_IS(propExprp, InitialStaticStmt)
                   || VN_IS(propExprp, InitialAutomaticStmt))) {
            propExprp = propExprp->nextp();
        }
        वापस VN_CAST(propExprp, PropSpec);
    }
    व्योम substituteSequenceCall(AstFuncRef* funcrefp, AstSequence* seqp) {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Inline the sequence body at the call site, replacing the FuncRef
        Ast_गाँठ_व्यंजक* bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        // Clone the body expression since the sequence may be referenced multiple times
        Ast_गाँठ_व्यंजक* clonedp = bodyExprp->clone_वृक्ष(false);
        // Build substitution map, then do a single traversal to replace all formals
        // (textual substitution per IEEE 16.8.2).
        स्थिर V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<स्थिर AstVar*, Ast_गाँठ_व्यंजक*> portMap;
        क्रम (स्थिर स्वतः& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            स्थिर स्वतः it = portMap.find(refp->varp());
            अगर (it != portMap.end()) {
                refp->replaceWith(it->second->clone_वृक्ष(false));
                VL_DO_DANGLING(धकेलो_Deletep(refp), refp);
            }
        });
        क्रम (स्थिर स्वतः& tconnect : tconnects) {
            धकेलो_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        // Replace the FuncRef with the inlined body
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(धकेलो_Deletep(funcrefp), funcrefp);
        // Clear referenced flag; sequences with isReferenced==false are deleted in assertPreAll
        seqp->isReferenced(false);
    }
    AstPropSpec* clonePropertyBody(AstFuncRef* funcrefp, स्थिर AstProperty* propp) {
        AstPropSpec* propExprp = getPropertyExprp(propp);
        // Substitute inner property call before copying in order to not doing the same for
        // each call of outer property call.
        propExprp = substitutePropertyCall(propExprp);
        // Clone subtree after substitution. It is needed, because property might be called
        // multiple times with different arguments.
        propExprp = propExprp->clone_वृक्ष(false);
        // Build substitution maps for formal arguments and property-local
        // variables, then perform a single foreach to apply all replacements.
        // Map port vars to their actual argument expressions
        स्थिर V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propp->stmtsp());
        std::unordered_map<स्थिर AstVar*, Ast_गाँठ_व्यंजक*> portMap;
        क्रम (स्थिर स्वतः& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }

        // Promote property-local variables (non-port vars, IEEE 16.10) to
        // module-level __Vpropvar temps. Cross-cycle persistence is handled
        // by the match item lowering in visit(AstImplication*).
        std::unordered_map<स्थिर AstVar*, AstVar*> localVarMap;
        क्रम (Ast_गाँठ* stmtp = propp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            अगर (AstVar* स्थिर varp = VN_CAST(stmtp, Var)) {
                अगर (!varp->isIO()) {
                    स्थिर string newName = m_propVarNames.लो(varp);
                    AstVar* स्थिर newVarp = नया AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->addStmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        // Single traversal: substitute ports and update local var references
        propExprp->foreach([&](AstVarRef* refp) {
            स्थिर स्वतः portIt = portMap.find(refp->varp());
            अगर (portIt != portMap.end()) {
                refp->replaceWith(portIt->second->clone_वृक्ष(false));
                VL_DO_DANGLING(धकेलो_Deletep(refp), refp);
                वापस;
            }
            स्थिर स्वतः localIt = localVarMap.find(refp->varp());
            अगर (localIt != localVarMap.end()) refp->varp(localIt->second);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        // Clean up argument expressions
        क्रम (स्थिर स्वतः& tconnect : tconnects) {
            धकेलो_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(propExprp, funcrefp);
        वापस propExprp;
    }
    AstPropSpec* substitutePropertyCall(AstPropSpec* nodep) {
        अगर (AstFuncRef* स्थिर funcrefp = VN_CAST(nodep->propp(), FuncRef)) {
            अगर (स्थिर AstProperty* स्थिर propp = VN_CAST(funcrefp->taskp(), Property)) {
                AstPropSpec* स्थिर propExprp = clonePropertyBody(funcrefp, propp);

                // Handle case with 2 disable iff statement (IEEE 1800-2023 16.12.1)
                अगर (nodep->disablep() && propExprp->disablep()) {
                    nodep->v3error("disable iff expression before property call and in its "
                                   "body is not legal");
                    धकेलो_Deletep(propExprp->disablep()->unlinkFrBack());
                }
                // If disable iff is in outer property, move it to inner
                अगर (nodep->disablep()) {
                    Ast_गाँठ_व्यंजक* स्थिर disablep = nodep->disablep()->unlinkFrBack();
                    propExprp->disablep(disablep);
                }

                अगर (nodep->sensesp() && propExprp->sensesp()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Clock event before property call and in its body");
                    धकेलो_Deletep(propExprp->sensesp()->unlinkFrBack());
                }
                // If clock event is in outer property, move it to inner
                अगर (nodep->sensesp()) {
                    AstSenItem* स्थिर sensesp = nodep->sensesp();
                    sensesp->unlinkFrBack();
                    propExprp->sensesp(sensesp);
                }

                // Now substitute property reference with property body
                nodep->replaceWith(propExprp);
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                वापस propExprp;
            }
        }
        वापस nodep;
    }

    व्योम substituteNestedPropertyCall(AstFuncRef* funcrefp, स्थिर AstProperty* propp) {
        AstPropSpec* स्थिर specp = clonePropertyBody(funcrefp, propp);
        Ast_गाँठ* replp = nullptr;
        अगर (specp->disablep()) {
            funcrefp->v3error("Illegal 'disable iff' in a nested property instance"
                              " (IEEE 1800-2023 16.12.1)");
        } अन्यथा अगर (specp->sensesp()) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: clock event in a nested property instance");
        } अन्यथा अगर (specp->propp()->exists(
                       [](स्थिर Ast_गाँठ_व्यंजक* np) { वापस np->isMultiCycleSva(); })) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: multi-cycle body in a nested property instance");
        } अन्यथा {
            replp = specp->propp()->unlinkFrBack();
        }
        अगर (!replp) replp = नया AstConst{funcrefp->fileline(), AstConst::BitFalse{}};
        funcrefp->replaceWith(replp);
        VL_DO_DANGLING(धकेलो_Deletep(funcrefp), funcrefp);
        VL_DO_DANGLING(धकेलो_Deletep(specp), specp);
    }

    // VISITORS
    //========== Statements
    व्योम visit(AstClocking* स्थिर nodep) override {
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        UINFO(8, "   CLOCKING" << nodep);
        अगर (nodep->isDefault()) {
            अगर (m_defaultClockingp) {
                nodep->v3error("Only one default clocking block allowed per module"
                               " (IEEE 1800-2023 14.12)");
            } अन्यथा {
                m_defaultClockingp = nodep;
                m_defaultClkEvtVarp = nodep->ensureEventp();
            }
        }
        iterateChildren(nodep);
        अगर (nodep->eventp()) nodep->add_अग्र_Here(nodep->eventp()->unlinkFrBack());
        VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
    }
    व्योम visit(AstModportClockingRef* स्थिर nodep) override {
        // It has to be converted to a list of ModportClockingVarRefs,
        // because clocking blocks are removed in this pass
        क्रम (Ast_गाँठ* itemp = nodep->clockingp()->itemsp(); itemp; itemp = itemp->nextp()) {
            अगर (AstClockingItem* स्थिर citemp = VN_CAST(itemp, ClockingItem)) {
                अगर (AstVar* स्थिर varp
                    = citemp->varp() ? citemp->varp() : VN_AS(citemp->user1p(), Var)) {
                    AstModportVarRef* स्थिर modVarp = नया AstModportVarRef{
                        nodep->fileline(), varp->name(), citemp->direction()};
                    modVarp->varp(varp);
                    nodep->add_अग्र_Here(modVarp);
                    अगर (citemp->direction().isOutput()) {
                        // Drives through the modport set the drive flag
                        AstVar* स्थिर drivenVarp = getCreateDrivenVarp(citemp);
                        AstModportVarRef* स्थिर drivenRefp = नया AstModportVarRef{
                            nodep->fileline(), drivenVarp->name(), VDirection::OUTPUT};
                        drivenRefp->varp(drivenVarp);
                        nodep->add_अग्र_Here(drivenRefp);
                    }
                }
            }
        }
        VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
    }
    व्योम visit(AstClockingItem* स्थिर nodep) override {
        // Get a ref to the sampled/driven variable
        AstVar* स्थिर varp = nodep->varp();
        अगर (!varp) {
            // Unused item
            वापस;
        }
        // Flag set by drives of an output clockvar, possibly visited before this item
        अगर (nodep->direction().isOutput()) m_modp->addStmtsp(getCreateDrivenVarp(nodep));
        File_पंक्ति* स्थिर flp = nodep->fileline();
        V3Const::constifyEdit(nodep->skewp());
        अगर (!VN_IS(nodep->skewp(), Const)) {
            nodep->skewp()->v3error("Skew must be constant (IEEE 1800-2023 14.4)");
            वापस;
        }
        AstConst* स्थिर skewp = VN_AS(nodep->skewp(), Const);
        अगर (skewp->अंक().isNegative()) skewp->v3error("Skew cannot be negative");
        Ast_गाँठ_व्यंजक* स्थिर exprp = nodep->exprp();
        varp->name(m_clockingp->name() + "__DOT__" + varp->name());
        m_clockingp->add_अग्र_Here(varp->unlinkFrBack());
        nodep->user1p(varp);
        varp->user1p(nodep);
        अगर (nodep->direction() == VDirection::OUTPUT) {
            exprp->foreach([](स्थिर Ast_गाँठ_Var_Ref* varrefp) {
                // Prevent confusing BLKANDNBLK warnings on clockvars due to generated assignments
                varrefp->fileline()->warnOff(V3ErrorCode::BLKANDNBLK, true);
                // Mark the driven signal so later passes (e.g. V3Inst port
                // connection) know the drive flows out from inside the cell.
                // Item expressions are plain signal references, so only the
                // driven signal is marked, never index selects.
                varrefp->varp()->isClockingDriven(true);
            });
            AstVarRef* स्थिर skewedReadRefp = नया AstVarRef{flp, varp, VAccess::READ};
            skewedReadRefp->user1(true);
            // Initialize the clockvar
            AstVarRef* स्थिर skewedWriteRefp = नया AstVarRef{flp, varp, VAccess::WRITE};
            skewedWriteRefp->user1(true);
            AstInitialStatic* स्थिर initClockvarp = नया AstInitialStatic{
                flp, नया AstAssign{flp, skewedWriteRefp, exprp->clone_वृक्ष_Pure(false)}};
            m_modp->addStmtsp(initClockvarp);
            // Each drive sets a flag, so the signal is also assigned when driven with the same
            // value again, e.g. after another assignment to the signal (IEEE 1800-2023 14.16.2)
            AstVar* स्थिर drivenVarp = getCreateDrivenVarp(nodep);
            m_modp->addStmtsp(नया AstInitialStatic{
                flp, नया AstAssign{flp, नया AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                   नया AstConst{flp, AstConst::BitFalse{}}}});
            // Assign the clockvar to the actual var if it was driven
            AstAssign* स्थिर assignp
                = नया AstAssign{flp, exprp->clone_वृक्ष_Pure(false), skewedReadRefp};
            AstIf* स्थिर ifp
                = नया AstIf{flp, नया AstVarRef{flp, drivenVarp, VAccess::READ},
                            नया AstAssign{flp, नया AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                          नया AstConst{flp, AstConst::BitFalse{}}}};
            ifp->addThensp(assignp);
            // Only in the time step of the clocking event
            AstCMethodHard* स्थिर trigp = नया AstCMethodHard{
                nodep->fileline(), नया AstVarRef{flp, m_clockingp->ensureEventp(), VAccess::READ},
                VCMethod::EVENT_IS_TRIGGERED};
            trigp->dtypeSetBit();
            ifp->condp(नया AstLogAnd{flp, ifp->condp()->unlinkFrBack(), trigp});
            // A skew rounding to no time precision ticks is a zero skew
            Ast_गाँठ_व्यंजक* स्थिर ticksp
                = V3Timing::delayToTicks(skewp->clone_वृक्ष(false), m_modp->timeunit());
            स्थिर bool zeroSkew = ticksp->isZero();
            VL_DO_DANGLING(ticksp->delete_वृक्ष(), ticksp);
            अगर (zeroSkew || !skewp->fileline()->timingOn()) {
                // Drive the var in Re-NBA (IEEE 1800-2023 14.16), ignoring skews where timing is
                // turned off
                Ast_Sen_वृक्ष* senTreep = m_clockingp->sentreep()->clone_वृक्ष(false);
                senTreep->addSensesp(नया AstSenItem{
                    flp, VEdgeType::ET_CHANGED, नया AstVarRef{flp, drivenVarp, VAccess::READ}});
                m_clockingp->add_अग्र_Here(नया AstAlwaysReactive{flp, senTreep, ifp});
            } अन्यथा {
                // Fork only the assignment, so this AlwaysPostponed can be retriggered
                // before the assignment happens
                AstFork* स्थिर forkp = नया AstFork{flp, VJoinType::JOIN_NONE};
                forkp->immediateStart(true);
                forkp->addForksp(नया AstBegin{flp, "", assignp->unlinkFrBack(), true});
                ifp->addThensp(forkp);
                // Read the clockvar in Postponed, so a drive late in the time step is applied
                // at this event plus the skew (IEEE 1800-2023 14.16)
                m_clockingp->add_अग्र_Here(नया AstAlwaysPostponed{flp, ifp});
                अगर (v3Global.opt.timing().isSetTrue()) {
                    AstDelay* स्थिर delayp = नया AstDelay{flp, skewp->unlinkFrBack(), false};
                    delayp->timeunit(m_modp->timeunit());
                    assignp->timingControlp(delayp);
                } अन्यथा अगर (v3Global.opt.timing().isSetFalse()) {
                    nodep->v3warn(E_NOTIMING,
                                  "Clocking output skew greater than #0 requires --timing");
                } अन्यथा {
                    nodep->v3warn(E_NEEDTIMINGOPT,
                                  "Use --timing or --no-timing to specify how "
                                  "clocking output skew greater than #0 should be handled");
                }
            }
        } अन्यथा अगर (nodep->direction() == VDirection::निवेश) {
            // Ref to the clockvar
            AstVarRef* स्थिर refp = नया AstVarRef{flp, varp, VAccess::WRITE};
            refp->user1(true);
            अगर (skewp->अंक().is1Step()) {
                // #1step means the value that is sampled is always the signal's last value
                // before the clock edge (IEEE 1800-2023 14.4)
                AstSampled* स्थिर sampledp
                    = नया AstSampled{flp, exprp->clone_वृक्ष_Pure(false), exprp->dtypep(), true};
                AstAssign* स्थिर assignp = नया AstAssign{flp, refp, sampledp};
                m_clockingp->add_अग्र_Here(नया AstAlways{
                    flp, VAlwaysKwd::ALWAYS, m_clockingp->sentreep()->clone_वृक्ष(false), assignp});
            } अन्यथा अगर (skewp->isZero()) {
                // #0 means the var has to be sampled in Observed (IEEE 1800-2023 14.13)
                AstAssign* स्थिर assignp = नया AstAssign{flp, refp, exprp->clone_वृक्ष_Pure(false)};
                m_clockingp->add_अग्र_Here(नया AstAlwaysObserved{
                    flp, m_clockingp->sentreep()->clone_वृक्ष(false), assignp});
            } अन्यथा {
                // Create a queue where we'll store sampled values with timestamps
                AstSampleQueueDType* स्थिर queueDtp
                    = नया AstSampleQueueDType{flp, exprp->dtypep()};
                m_netlistp->typeTablep()->addTypesp(queueDtp);
                AstVar* स्थिर queueVarp
                    = नया AstVar{flp, VVarType::MODULETEMP, "__Vqueue__" + varp->name(), queueDtp};
                queueVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_clockingp->add_अग्र_Here(queueVarp);
                // Create a process like this:
                //     always queue.push(<sampled var>);
                AstCMethodHard* स्थिर pushp = नया AstCMethodHard{
                    flp, नया AstVarRef{flp, queueVarp, VAccess::WRITE}, VCMethod::DYN_धकेलो,
                    नया AstTime{nodep->fileline(), m_modp->timeunit()}};
                pushp->addPinsp(exprp->clone_वृक्ष_Pure(false));
                pushp->dtypeSetVoid();
                m_clockingp->add_अग्र_Here(
                    नया AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, pushp->makeStmt()});
                // Create a process like this:
                //     always @<clocking event> queue.pop(<skew>, /*out*/<skewed var>);
                AstCMethodHard* स्थिर popp = नया AstCMethodHard{
                    flp, नया AstVarRef{flp, queueVarp, VAccess::READWRITE}, VCMethod::DYN_निकालो,
                    नया AstTime{nodep->fileline(), m_modp->timeunit()}};
                popp->addPinsp(skewp->unlinkFrBack());
                refp->access(VAccess::READWRITE);  // Only conditionally assigned
                popp->addPinsp(refp);
                popp->dtypeSetVoid();
                m_clockingp->add_अग्र_Here(नया AstAlways{flp, VAlwaysKwd::ALWAYS,
                                                       m_clockingp->sentreep()->clone_वृक्ष(false),
                                                       popp->makeStmt()});
            }
        } अन्यथा {
            nodep->v3fatalSrc("Invalid direction");
        }
    }
    व्योम visit(AstDelay* nodep) override {
        m_hasCycleDelay |= nodep->isCycleDelay();
        // Only cycle delays are relevant in this stage; also only process once
        अगर (!nodep->isCycleDelay()) {
            अगर (m_inSynchDrive) {
                nodep->v3error("Only cycle delays can be used in synchronous drives"
                               " (IEEE 1800-2023 14.16)");
            }
            iterateChildren(nodep);
            वापस;
        }
        अगर (m_inAssign && !m_inSynchDrive) {
            nodep->v3error("Cycle delays not allowed as intra-assignment delays"
                           " (IEEE 1800-2023 14.11)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
            वापस;
        }
        अगर (nodep->stmtsp()) nodep->add_अग्र_Here(nodep->stmtsp()->unlink_Fr_Back_With_अग्र());
        File_पंक्ति* स्थिर flp = nodep->fileline();
        Ast_गाँठ_व्यंजक* valuep = V3Const::constifyEdit(nodep->lhsp()->unlinkFrBack());
        स्थिर AstConst* स्थिर constp = VN_CAST(valuep, Const);
        अगर (!constp && m_pexprp) {
            // V3AssertNfa handles non-const delays in sequences before this pass and
            // replaces the property; this branch should never be reached.
            nodep->v3fatalSrc("Non-constant cycle delay in assertion: "
                              "should have been caught by V3AssertNfa");
        } अन्यथा अगर (constp && constp->isZero()) {
            VL_DO_DANGLING(धकेलो_Deletep(valuep), valuep);
            अगर (m_inSynchDrive) {
                // ##0 has no effect in synchronous drives (IEEE 1800-2023 14.11)
                VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
                वापस;
            }
            अगर (m_pexprp) {
                // ##0 in sequence context = zero delay = same clock tick
                VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
                वापस;
            }
            // Procedural ##0: synchronize with default clocking event (IEEE 1800-2023 14.11)
            अगर (!m_defaultClockingp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
                वापस;
            }
            nodep->replaceWith(newCycleDelayZerop(flp));
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            वापस;
        }
        Ast_Sen_वृक्ष* sentreep = nullptr;
        अगर (!m_drives.empty()) {
            अगर (m_inConcatAssign) {
                // Also the clockvars may be of different clocking blocks
                nodep->v3error("Cycle delays cannot be used in synchronous drives to a"
                               " concatenation (IEEE 1800-2023 14.16)");
                VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(धकेलो_Deletep(valuep), valuep);
                वापस;
            }
            // Count the cycles of the driven clockvar's clocking block (IEEE 1800-2023 14.16)
            sentreep = newDriveSentreep(nodep, m_drives.front());
            अगर (!sentreep) {
                VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(धकेलो_Deletep(valuep), valuep);
                वापस;
            }
        } अन्यथा अगर (!m_defaultClockingp) {
            अगर (!m_pexprp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(धकेलो_Deletep(valuep), valuep);
                वापस;
            }
            sentreep = नया Ast_Sen_वृक्ष{flp, m_senip->clone_वृक्ष(false)};
        } अन्यथा {
            sentreep = m_defaultClockingp->sentreep()->clone_वृक्ष(false);
        }
        AstEventControl* स्थिर controlp
            = नया AstEventControl{nodep->fileline(), sentreep, nullptr};
        स्थिर std::string delayName = m_cycleDlyNames.लो(nodep);
        Ast_गाँठ_व्यंजक* throughoutp
            = nodep->throughoutp() ? nodep->throughoutp()->unlinkFrBack() : nullptr;

        AstVar* स्थिर cntVarp = नया AstVar{flp, VVarType::BLOCKTEMP, delayName + "__counter",
                                           nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstBegin* स्थिर beginp = नया AstBegin{flp, delayName + "__block", cntVarp, true};
        // A non-constant delay is counted at runtime. The count is integral of 64 bits, as
        // V3Width makes delays, or real, which is rounded (IEEE 1800-2023 6.12.1)
        अगर (valuep->isDouble()) {
            valuep = नया AstRToIRoundS{flp, valuep};
            valuep->dtypeFrom(cntVarp);
        } अन्यथा {
            valuep = नया AstSel{flp, valuep, 0, cntVarp->width()};
        }
        beginp->addStmtsp(नया AstAssign{flp, नया AstVarRef{flp, cntVarp, VAccess::WRITE}, valuep});
        // If the count is zero, a synchronous drive waits for none, like with a constant '##0',
        // and a procedural delay waits like a procedural '##0' (IEEE 1800-2023 14.11)
        अगर (!constp && !m_inSynchDrive) {
            beginp->addStmtsp(नया AstIf{
                flp,
                नया AstEq{flp, नया AstVarRef{flp, cntVarp, VAccess::READ}, नया AstConst{flp, 0}},
                newCycleDelayZerop(flp)});
        }

        // Throughout: create flag tracking whether condition held every tick
        AstVar* throughoutOkp = nullptr;
        अगर (throughoutp) {
            throughoutOkp = नया AstVar{flp, VVarType::BLOCKTEMP, delayName + "__throughoutOk",
                                       nodep->findBasicDType(VBasicDTypeKwd::LOGIC_IMPLICIT)};
            throughoutOkp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->addStmtsp(throughoutOkp);
            beginp->addStmtsp(नया AstAssign{flp, नया AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            नया AstConst{flp, AstConst::BitTrue{}}});
            // Check condition at tick 0 (sequence start, before entering loop)
            beginp->addStmtsp(
                नया AstIf{flp, नया AstLogNot{flp, throughoutp->clone_वृक्ष_Pure(false)},
                          नया AstAssign{flp, नया AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                        नया AstConst{flp, AstConst::BitFalse{}}}});
        }

        {
            AstLoop* स्थिर loopp = नया AstLoop{flp};
            // When throughout is present, exit loop early if condition fails
            Ast_गाँठ_व्यंजक* loopCondp
                = नया AstGt{flp, नया AstVarRef{flp, cntVarp, VAccess::READ}, नया AstConst{flp, 0}};
            अगर (throughoutOkp) {
                loopCondp = नया AstLogAnd{flp, loopCondp,
                                          नया AstVarRef{flp, throughoutOkp, VAccess::READ}};
            }
            loopp->addStmtsp(नया AstLoopTest{flp, loopp, loopCondp});
            loopp->addStmtsp(controlp);
            loopp->addStmtsp(
                नया AstAssign{flp, नया AstVarRef{flp, cntVarp, VAccess::WRITE},
                              नया AstSub{flp, नया AstVarRef{flp, cntVarp, VAccess::READ},
                                         नया AstConst{flp, 1}}});
            // Check throughout condition at each tick during delay (IEEE 1800-2023 16.9.9)
            अगर (throughoutp) {
                loopp->addStmtsp(
                    नया AstIf{flp, नया AstLogNot{flp, throughoutp},
                              नया AstAssign{flp, नया AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            नया AstConst{flp, AstConst::BitFalse{}}}});
            }
            beginp->addStmtsp(loopp);
        }
        // Compose wrappers on remaining sequence: throughout gate (inner), disable iff (outer)
        Ast_गाँठ* remainp = nodep->nextp() ? nodep->nextp()->unlink_Fr_Back_With_अग्र() : nullptr;
        अगर (throughoutOkp) {
            // If condition failed during delay, fail assertion
            remainp = नया AstIf{flp, नया AstVarRef{flp, throughoutOkp, VAccess::READ}, remainp,
                                नया AstPExprClause{flp, /*pass=*/false}};
        }
        अगर (m_disableSeqIfp && remainp) {
            AstIf* स्थिर disableSeqIfp = m_disableSeqIfp->clone_वृक्ष(false);
            disableSeqIfp->addThensp(remainp);
            remainp = disableSeqIfp;
        }
        अगर (remainp) {
            अगर (throughoutOkp) {
                // throughoutOkp is declared in beginp scope -- check must be inside it
                beginp->addStmtsp(remainp);
            } अन्यथा {
                nodep->add_अग्र_Here(remainp);
            }
        }
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
    व्योम visit(Ast_Sen_वृक्ष* nodep) override {
        अगर (m_inSynchDrive) {
            nodep->v3error("Event controls cannot be used in "
                           "synchronous drives (IEEE 1800-2023 14.16)");
        }

        स्थिर AstSampled* sampledp;
        अगर (nodep->exists([&sampledp](स्थिर AstSampled* स्थिर sp) {
                sampledp = sp;
                वापस true;
            })) {
            sampledp->v3warn(E_UNSUPPORTED, "Unsupported: $sampled inside sensitivity list");
        }
    }
    व्योम visit(Ast_गाँठ_Var_Ref* nodep) override {
        UINFO(8, " -varref:  " << nodep);
        UINFO(8, " -varref-var-back:  " << nodep->varp()->backp());
        UINFO(8, " -varref-var-user1:  " << nodep->varp()->user1p());
        अगर (AstClockingItem* स्थिर itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->firstAbovep(),
                ClockingItem)) {
            अगर (nodep->user1()) वापस;

            // ensure linking still works, this has to be done only once
            अगर (AstVarXRef* xrefp = VN_CAST(nodep, VarXRef)) {
                UINFO(8, " -clockvarxref-in:  " << xrefp);
                string dotted = xrefp->dotted();
                स्थिर माप_प्रकार dotPos = dotted.rfind('.');
                dotted.erase(dotPos, string::npos);
                xrefp->dotted(dotted);
                UINFO(8, " -clockvarxref-out: " << xrefp);
                m_xrefsp.emplace_back(xrefp);
            }

            अगर (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            अगर (nodep->access().isWriteOrRW()) {
                अगर (itemp->direction() == VDirection::OUTPUT) {
                    अगर (!m_inAssignDlyLhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    अगर (m_inAssign) {
                        m_inSynchDrive = true;
                        m_drives.धकेलो_back({itemp, nodep});
                    }
                } अन्यथा अगर (itemp->direction() == VDirection::निवेश) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
            nodep->user1(true);
        }
    }
    व्योम visit(AstMemberSel* nodep) override {
        अगर (AstClockingItem* स्थिर itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->firstAbovep(),
                ClockingItem)) {
            अगर (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            अगर (nodep->access().isWriteOrRW()) {
                अगर (itemp->direction() == VDirection::OUTPUT) {
                    अगर (!m_inAssignDlyLhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    अगर (m_inAssign) {
                        m_inSynchDrive = true;
                        m_drives.धकेलो_back({itemp, nodep});
                    }
                } अन्यथा अगर (itemp->direction() == VDirection::निवेश) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
        }
    }
    व्योम visit(Ast_गाँठ_Assign* nodep) override {
        अगर (nodep->user1()) वापस;
        VL_RESTORER(m_inAssign);
        VL_RESTORER(m_inSynchDrive);
        VL_RESTORER(m_inConcatAssign);
        VL_RESTORER_CLEAR(m_drives);
        m_inAssign = true;
        m_inSynchDrive = false;
        {
            // The braces of a concatenation are a replication, by one
            स्थिर Ast_गाँठ_व्यंजक* lhsp = nodep->lhsp();
            अगर (स्थिर AstReplicate* स्थिर repp = VN_CAST(lhsp, Replicate)) lhsp = repp->srcp();
            m_inConcatAssign = VN_IS(lhsp, Concat);
        }
        {
            VL_RESTORER(m_inAssignDlyLhs);
            m_inAssignDlyLhs = VN_IS(nodep, AssignDly);
            iterate(nodep->lhsp());
        }
        iterate(nodep->rhsp());
        अगर (nodep->timingControlp()) iterate(nodep->timingControlp());
        // A drive without a cycle delay, also after removing '##0', updates the clockvar now
        अगर (m_inSynchDrive && !nodep->timingControlp()) {
            AstAssign* स्थिर assignp = नया AstAssign{
                nodep->fileline(), nodep->lhsp()->unlinkFrBack(), nodep->rhsp()->unlinkFrBack()};
            assignp->user1(true);
            nodep->replaceWith(assignp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
            nodep = assignp;
        }
        // Note the drives when they take effect, after the cycle delay if any. Each clockvar
        // of the LHS is driven, e.g. in a concatenation which other simulators accept, although
        // IEEE 1800-2023 14.16 does not allow it
        Ast_गाँठ* setsp = nullptr;
        क्रम (स्थिर SynchDrive& drive : m_drives) {
            setsp = Ast_गाँठ::add_अग्र(setsp, newDrivenSetp(nodep->fileline(), drive));
        }
        अगर (!setsp) वापस;
        अगर (AstBegin* स्थिर beginp = VN_CAST(nodep->timingControlp(), Begin)) {
            beginp->addStmtsp(setsp);
        } अन्यथा {
            nodep->add_अग्र_Here(setsp);
        }
    }
    व्योम visit(AstAlways* nodep) override {
        iterate_And_अग्र_Null(nodep->sentreep());
        अगर (nodep->sentreep()) m_seniAlwaysp = nodep->sentreep()->sensesp();
        iterate_And_अग्र_Null(nodep->stmtsp());
        m_seniAlwaysp = nullptr;
    }

    व्योम visit(Ast_गाँठ_Cover_Or_Assert* nodep) override {
        अगर (nodep->sentreep()) वापस;  // Already processed

        VL_RESTORER(m_senip);
        VL_RESTORER(m_disablep);
        m_senip = nullptr;
        m_disablep = nullptr;

        // Find Clocking's buried under nodep->exprsp
        iterateChildren(nodep);
        अगर (!nodep->immediate()) nodep->sentreep(new_Sen_वृक्ष(nodep, nullptr, nodep));
    }
    व्योम visit(AstFalling* nodep) override {
        अगर (nodep->user1SetOnce()) वापस;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_पंक्ति* स्थिर fl = nodep->fileline();
        Ast_गाँठ_व्यंजक* exprp = nodep->exprp()->unlinkFrBack();
        अगर (exprp->width() > 1) exprp = नया AstSel{fl, exprp, 0, 1};
        Ast_गाँठ_व्यंजक* स्थिर futurep = नया AstFuture{fl, exprp, new_Sen_वृक्ष(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = नया AstAnd{fl, exprp->clone_वृक्ष_Pure(false), नया AstNot{fl, futurep}};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    व्योम visit(AstFell* nodep) override {
        अगर (nodep->user1SetOnce()) वापस;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_पंक्ति* स्थिर fl = nodep->fileline();
        Ast_गाँठ_व्यंजक* exprp = nodep->exprp()->unlinkFrBack();
        अगर (exprp->width() > 1) exprp = नया AstSel{fl, exprp, 0, 1};
        Ast_Sen_वृक्ष* sentreep = nodep->sentreep();
        अगर (sentreep) sentreep->unlinkFrBack();
        AstPast* स्थिर pastp = नया AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_वृक्ष(nodep, sentreep));
        exprp = नया AstAnd{fl, pastp, नया AstNot{fl, exprp->clone_वृक्ष_Pure(false)}};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    व्योम visit(AstFuture* nodep) override {
        अगर (nodep->user1SetOnce()) वापस;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        Ast_Sen_वृक्ष* स्थिर sentreep = nodep->sentreep();
        अगर (sentreep) VL_DO_DANGLING(धकेलो_Deletep(sentreep->unlinkFrBack()), sentreep);
        nodep->sentreep(new_Sen_वृक्ष(nodep));
    }
    व्योम visit(AstPast* nodep) override {
        checkSamplingFuncDType(nodep, nodep->exprp());
        अगर (nodep->sentreep()) वापस;  // Already processed
        iterateChildren(nodep);
        nodep->sentreep(new_Sen_वृक्ष(nodep));
    }
    व्योम visit(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- Lower standalone exact [*N] (N >= 2) via saturating counter.
        // Range/unbounded forms and SExpr-contained forms are lowered by V3AssertNfa.
        iterateChildren(nodep);
        // V3AssertNfa handles unbounded/ranged forms upstream, so this fast-path
        // is effectively unreachable when NFA is enabled.
        अगर (nodep->unbounded() || nodep->maxCountp()) वापस;  // LCOV_EXCL_LINE
        स्थिर AstConst* स्थिर constp = VN_CAST(nodep->countp(), Const);
        अगर (VL_UNLIKELY(!constp || constp->toSInt() < 1)) {
            nodep->v3fatalSrc("Consecutive repetition count must be a positive constant"
                              " (should have been caught by V3Width)");
            वापस;
        }
        स्थिर पूर्णांक n = constp->toSInt();
        File_पंक्ति* स्थिर flp = nodep->fileline();
        Ast_गाँठ_व्यंजक* स्थिर exprp = nodep->exprp()->unlinkFrBack();
        अगर (n == 1) {
            nodep->replaceWith(exprp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            वापस;
        }
        // Saturating counter: if (expr) cnt <= min(cnt+1, N); else cnt <= 0;
        AstVar* स्थिर cntVarp = नया AstVar{flp, VVarType::MODULETEMP, m_consRepNames.लो(""),
                                           nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(cntVarp);
        Ast_गाँठ_व्यंजक* स्थिर व्यंजक_Clonep = exprp->clone_वृक्ष_Pure(false);
        Ast_गाँठ_व्यंजक* स्थिर saturatingIncrp = नया AstCond{
            flp,
            नया AstLt{flp, नया AstVarRef{flp, cntVarp, VAccess::READ},
                      नया AstConst{flp, static_cast<uint32_t>(n)}},
            नया AstAdd{flp, नया AstVarRef{flp, cntVarp, VAccess::READ}, नया AstConst{flp, 1u}},
            नया AstConst{flp, static_cast<uint32_t>(n)}};
        AstAssignDly* स्थिर incrAssignp
            = नया AstAssignDly{flp, नया AstVarRef{flp, cntVarp, VAccess::WRITE}, saturatingIncrp};
        AstAssignDly* स्थिर resetAssignp = नया AstAssignDly{
            flp, नया AstVarRef{flp, cntVarp, VAccess::WRITE}, नया AstConst{flp, 0u}};
        AstIf* स्थिर ifp = नया AstIf{flp, व्यंजक_Clonep, incrAssignp, resetAssignp};
        Ast_Sen_वृक्ष* स्थिर senTreep = new_Sen_वृक्ष(nodep);
        AstAlways* स्थिर alwaysp = नया AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, ifp};
        cntVarp->add_अग्र_Here(alwaysp);
        // Match: cnt >= N-1 (previous cycles via NBA) && expr (current cycle)
        Ast_गाँठ_व्यंजक* स्थिर cntCheckp = नया AstGte{flp, नया AstVarRef{flp, cntVarp, VAccess::READ},
                                                  नया AstConst{flp, static_cast<uint32_t>(n - 1)}};
        cntCheckp->dtypeSetBit();
        Ast_गाँठ_व्यंजक* स्थिर matchp = नया AstAnd{flp, cntCheckp, exprp};
        matchp->dtypeSetBit();
        nodep->replaceWith(matchp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    व्योम visit(AstRising* nodep) override {
        अगर (nodep->user1SetOnce()) वापस;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_पंक्ति* स्थिर fl = nodep->fileline();
        Ast_गाँठ_व्यंजक* exprp = nodep->exprp()->unlinkFrBack();
        अगर (exprp->width() > 1) exprp = नया AstSel{fl, exprp, 0, 1};
        Ast_गाँठ_व्यंजक* स्थिर futurep = नया AstFuture{fl, exprp, new_Sen_वृक्ष(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = नया AstAnd{fl, नया AstNot{fl, exprp->clone_वृक्ष_Pure(false)}, futurep};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    व्योम visit(AstRose* nodep) override {
        अगर (nodep->user1SetOnce()) वापस;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_पंक्ति* स्थिर fl = nodep->fileline();
        Ast_गाँठ_व्यंजक* exprp = nodep->exprp()->unlinkFrBack();
        अगर (exprp->width() > 1) exprp = नया AstSel{fl, exprp, 0, 1};
        Ast_Sen_वृक्ष* sentreep = nodep->sentreep();
        अगर (sentreep) sentreep->unlinkFrBack();
        AstPast* स्थिर pastp = नया AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_वृक्ष(nodep, sentreep));
        exprp = नया AstAnd{fl, नया AstNot{fl, pastp}, exprp->clone_वृक्ष_Pure(false)};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    अटल AstAssocArrayDType* getProcessAssocArrayType(File_पंक्ति* स्थिर flp) {
        // Type of __VassertsActive___x[std::process]
        Ast_गाँठ_DType* valp
            = v3Global.rootp()->typeTablep()->findBasicDType(flp, VBasicDTypeKwd::BIT);
        AstClassRefDType* keyp
            = नया AstClassRefDType{flp, v3Global.rootp()->stdPackageProcessp(), nullptr};
        keyp->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        v3Global.rootp()->typeTablep()->addTypesp(keyp);
        AstAssocArrayDType* स्थिर typep = नया AstAssocArrayDType{flp, valp, keyp};
        typep->dtypep(typep);
        v3Global.rootp()->typeTablep()->addTypesp(typep);
        वापस typep;
    }
    अटल Ast_Stmt_व्यंजक* getProcessAssocArrayDelete(AstVarRef* स्थिर refp) {
        // Constructs refp.delete(std::process::self()) statement
        File_पंक्ति* स्थिर flp = refp->fileline();
        AstCMethodHard* स्थिर deletep = नया AstCMethodHard{
            flp, refp, VCMethod::ASSOC_ERASE, v3Global.rootp()->stdPackageProcessSelfp(flp)};
        deletep->dtypep(refp->findVoidDType());
        वापस नया Ast_Stmt_व्यंजक{flp, deletep};
    }
    अटल Ast_गाँठ_व्यंजक* get_Process_Assoc_Array_आकार(AstVarRef* स्थिर refp) {
        // Constructs refp.size() statement
        AstCMethodHard* स्थिर sizep
            = नया AstCMethodHard{refp->fileline(), refp, VCMethod::ASSOC_आकार};
        sizep->dtypep(refp->findBasicDType(VBasicDTypeKwd::UINT32));
        वापस sizep;
    }
    व्योम visit(AstSEventually* nodep) override {
        UASSERT(v3Global.rootp()->stdPackagep(), "Should be imported");

        Ast_Sen_वृक्ष* स्थिर sentreep = new_Sen_वृक्ष(nodep);
        अगर (!sentreep->sensesp()) {
            VL_DO_DANGLING(धकेलो_Deletep(sentreep), sentreep);
            nodep->replaceWith(नया AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            वापस;
        }

        File_पंक्ति* स्थिर flp = nodep->fileline();

        // Track active assertions
        AstVar* स्थिर activep = नया AstVar{flp, VVarType::MODULETEMP, m_activeNames.लो(""),
                                           getProcessAssocArrayType(flp)};
        activep->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(activep);

        // Assertion condition check
        AstLoop* स्थिर loopp = नया AstLoop{flp};
        AstSampled* स्थिर condp
            = नया AstSampled{flp, nodep->exprp()->unlinkFrBack(), nullptr, true};
        loopp->addStmtsp(नया AstLoopTest{flp, loopp, नया AstLogNot{flp, condp}});
        loopp->addStmtsp(नया AstEventControl{flp, sentreep, nullptr});

        // Add assertion to the active set
        AstAssocSel* स्थिर selp = नया AstAssocSel{flp, नया AstVarRef{flp, activep, VAccess::WRITE},
                                                  v3Global.rootp()->stdPackageProcessSelfp(flp)};
        AstAssign* स्थिर incrementp = नया AstAssign{flp, selp, नया AstConst{flp, 1}};
        AstPExprClause* स्थिर clausep = नया AstPExprClause{flp};
        Ast_Stmt_व्यंजक* स्थिर deletep
            = getProcessAssocArrayDelete(नया AstVarRef{flp, activep, VAccess::WRITE});

        // Main assertion block
        AstBegin* स्थिर bodyp = नया AstBegin{flp, m_blockNames.लो(""), nullptr, true};
        bodyp->addStmtsp(incrementp);
        bodyp->addStmtsp(loopp);
        bodyp->addStmtsp(clausep);
        bodyp->addStmtsp(deletep);

        // Validate assertion condition for each active assert
        AstVar* स्थिर activeCountp = नया AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        activeCountp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstAssign* स्थिर initActiveCountp
            = नया AstAssign{flp, नया AstVarRef{flp, activeCountp, VAccess::WRITE},
                            get_Process_Assoc_Array_आकार(नया AstVarRef{flp, activep, VAccess::READ})};
        AstLoop* स्थिर finalLoopp = नया AstLoop{flp};
        AstIf* स्थिर finalBodypCondp
            = नया AstIf{flp, condp->clone_वृक्ष_Pure(false), नया AstPExprClause{flp},
                        नया AstPExprClause{flp, false}};
        finalLoopp->addStmtsp(
            नया AstLoopTest{flp, finalLoopp,
                            नया AstNeq{flp, नया AstVarRef{flp, activeCountp, VAccess::READ},
                                       नया AstConst{flp, 0}}});
        finalLoopp->addStmtsp(finalBodypCondp);
        finalLoopp->addStmtsp(
            नया AstAssign{flp, नया AstVarRef{flp, activeCountp, VAccess::WRITE},
                          नया AstSub{flp, नया AstVarRef{flp, activeCountp, VAccess::READ},
                                     नया AstConst{flp, 1}}});

        // Final assertion block
        AstBegin* स्थिर finalp = नया AstBegin{flp, m_blockNames.लो(""), nullptr, true};
        finalp->addStmtsp(activeCountp);
        finalp->addStmtsp(initActiveCountp);
        finalp->addStmtsp(finalLoopp);

        VL_RESTORER(m_pexprp);
        m_pexprp = नया AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
        VL_RESTORER(m_hasCycleDelay);
        m_hasCycleDelay = false;
        iterate(bodyp);
        iterate(finalp);

        अगर (m_hasCycleDelay) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in s_eventually");
            nodep->replaceWith(नया AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            VL_DO_DANGLING(m_pexprp->delete_वृक्ष(), m_pexprp);
            वापस;
        }

        nodep->replaceWith(m_pexprp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    व्योम visit(AstStable* nodep) override {
        अगर (nodep->user1SetOnce()) वापस;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_पंक्ति* स्थिर fl = nodep->fileline();
        Ast_गाँठ_व्यंजक* exprp = nodep->exprp()->unlinkFrBack();
        Ast_Sen_वृक्ष* sentreep = nodep->sentreep();
        अगर (sentreep) sentreep->unlinkFrBack();
        AstPast* स्थिर pastp = नया AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_वृक्ष(nodep, sentreep));
        exprp = नया AstEq{fl, pastp, exprp->clone_वृक्ष_Pure(false)};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    व्योम visit(AstSteady* nodep) override {
        अगर (nodep->user1SetOnce()) वापस;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_पंक्ति* स्थिर fl = nodep->fileline();
        Ast_गाँठ_व्यंजक* exprp = nodep->exprp()->unlinkFrBack();
        अगर (exprp->width() > 1) exprp = नया AstSel{fl, exprp, 0, 1};
        Ast_गाँठ_व्यंजक* स्थिर futurep = नया AstFuture{fl, exprp, new_Sen_वृक्ष(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = नया AstEq{fl, exprp->clone_वृक्ष_Pure(false), futurep};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    व्योम visit(AstSampled* nodep) override {
        iterateChildren(nodep);
        अगर (!nodep->internal()) checkSamplingFuncDType(nodep, nodep->exprp());
    }

    // Validate repetition count: must be a non-negative elaboration-time constant.
    // Shared by goto [->N] and nonconsecutive [=N] repetition.
    // On error, replaces nodep with BitFalse placeholder and returns nullptr.
    स्थिर AstConst* validate_Rep_गिनती(Ast_गाँठ* nodep, Ast_गाँठ_व्यंजक*& countp) {
        countp = V3Const::constifyEdit(countp);
        स्थिर AstConst* स्थिर constp = VN_CAST(countp, Const);
        अगर (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(धकेलो_Deletep(countp), countp);
            nodep->replaceWith(नया AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            वापस nullptr;
        }
        अगर (constp->toSInt() < 0) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(धकेलो_Deletep(countp), countp);
            nodep->replaceWith(नया AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            वापस nullptr;
        }
        अगर (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(धकेलो_Deletep(countp), countp);
            nodep->replaceWith(नया AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            वापस nullptr;
        }
        वापस constp;
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
    AstPExpr* createRepPExpr(File_पंक्ति* flp, Ast_गाँठ_व्यंजक* exprp, Ast_गाँठ_व्यंजक* countp,
                             Ast_गाँठ_व्यंजक* rhsp, bool isOverlapped, bool isNonConsec) {
        AstSenItem* स्थिर sensesp = m_senip;
        UASSERT_OBJ(sensesp, exprp, "Repetition requires a clock");

        स्थिर std::string name
            = isNonConsec ? m_nonConsRepNames.लो(exprp) : m_gotoRepNames.लो(exprp);
        AstVar* स्थिर cntVarp = नया AstVar{flp, VVarType::BLOCKTEMP, name + "__counter",
                                           exprp->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstBegin* स्थिर beginp = नया AstBegin{flp, name + "__block", cntVarp, true};
        beginp->addStmtsp(
            नया AstAssign{flp, नया AstVarRef{flp, cntVarp, VAccess::WRITE}, नया AstConst{flp, 0}});

        // Counting loop: find N occurrences of expr (shared by goto and nonconsec)
        AstLoop* स्थिर loopp = नया AstLoop{flp};
        loopp->addStmtsp(नया AstLoopTest{flp, loopp,
                                         नया AstLt{flp, नया AstVarRef{flp, cntVarp, VAccess::READ},
                                                   countp->clone_वृक्ष_Pure(false)}});
        // if (expr) cnt++ -- sampled is applied to whole property expr by V3Assert
        loopp->addStmtsp(
            नया AstIf{flp, exprp,
                      नया AstAssign{flp, नया AstVarRef{flp, cntVarp, VAccess::WRITE},
                                    नया AstAdd{flp, नया AstVarRef{flp, cntVarp, VAccess::READ},
                                               नया AstConst{flp, 1}}}});
        loopp->addStmtsp(नया AstIf{
            flp, नया AstLt{flp, नया AstVarRef{flp, cntVarp, VAccess::READ}, countp},
            नया AstEventControl{flp, नया Ast_Sen_वृक्ष{flp, sensesp->clone_वृक्ष(false)}, nullptr}});

        beginp->addStmtsp(loopp);

        अगर (isNonConsec && rhsp) {
            // IEEE 16.9.2: b[=N] = b[->N] ##1 !b[*0:$]
            // Trailing window: ##1 advance, then scan !expr cycles checking rhs.
            // Window closes when expr becomes true again (fail if rhs was never true).
            beginp->addStmtsp(नया AstEventControl{
                flp, नया Ast_Sen_वृक्ष{flp, sensesp->clone_वृक्ष(false)}, nullptr});  // ##1
            अगर (!isOverlapped) {
                beginp->addStmtsp(नया AstEventControl{
                    flp, नया Ast_Sen_वृक्ष{flp, sensesp->clone_वृक्ष(false)}, nullptr});  // |=>
            }
            // Window loop: check rhs at each !expr cycle (done variable for termination)
            AstVar* स्थिर doneVarp
                = नया AstVar{flp, VVarType::BLOCKTEMP, name + "__done", exprp->findBitDType()};
            doneVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->addStmtsp(doneVarp);
            beginp->addStmtsp(नया AstAssign{flp, नया AstVarRef{flp, doneVarp, VAccess::WRITE},
                                            नया AstConst{flp, AstConst::BitFalse{}}});
            स्वतः setDone = [&]() {
                वापस नया AstAssign{flp, नया AstVarRef{flp, doneVarp, VAccess::WRITE},
                                     नया AstConst{flp, AstConst::BitTrue{}}};
            };
            AstLoop* स्थिर windowp = नया AstLoop{flp};
            // LoopTest: continue while !done
            windowp->addStmtsp(नया AstLoopTest{
                flp, windowp, नया AstNot{flp, नया AstVarRef{flp, doneVarp, VAccess::READ}}});
            // if (expr) { fail; done = 1; } -- window closed, expr true again
            Ast_गाँठ* स्थिर failsp = नया AstPExprClause{flp, false};
            failsp->add_अग्र(setDone());
            windowp->addStmtsp(नया AstIf{flp, exprp->clone_वृक्ष_Pure(false), failsp});
            // if (rhs) { pass; done = 1; } -- consequent true at this !expr endpoint
            Ast_गाँठ* स्थिर passsp = नया AstPExprClause{flp, true};
            passsp->add_अग्र(setDone());
            windowp->addStmtsp(नया AstIf{flp, rhsp, passsp});
            // @(clk) -- advance to next cycle in window
            windowp->addStmtsp(
                नया AstEventControl{flp, नया Ast_Sen_वृक्ष{flp, sensesp->clone_वृक्ष(false)}, nullptr});
            beginp->addStmtsp(windowp);
        } अन्यथा अगर (isNonConsec) {
            // Standalone nonconsec: ##1 into window, then pass
            beginp->addStmtsp(
                नया AstEventControl{flp, नया Ast_Sen_वृक्ष{flp, sensesp->clone_वृक्ष(false)}, nullptr});
            beginp->addStmtsp(नया AstPExprClause{flp, true});
        } अन्यथा अगर (rhsp) {
            // Goto rep: check consequent once at match endpoint
            अगर (!isOverlapped) {
                beginp->addStmtsp(नया AstEventControl{
                    flp, नया Ast_Sen_वृक्ष{flp, sensesp->clone_वृक्ष(false)}, nullptr});
            }
            beginp->addStmtsp(नया AstIf{flp, rhsp, नया AstPExprClause{flp, true},
                                        नया AstPExprClause{flp, false}});
        } अन्यथा {
            // Standalone goto: pass after counting
            beginp->addStmtsp(नया AstPExprClause{flp, true});
        }

        वापस नया AstPExpr{flp, beginp, exprp->findBitDType()};
    }

    // Reject [=M:N] nonconsecutive range that reached V3AssertPre.
    // [->M:N] is fully handled upstream (NFA path); only [=M:N] still lands
    // here because AstSNonConsRep lowering is not yet implemented there.
    // Replaces the node with BitFalse so lowering continues after the warning.
    bool rejectNonconsecRange(Ast_गाँठ* nodep, Ast_गाँठ_व्यंजक* maxCountp) {
        अगर (!maxCountp) वापस false;
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: [=M:N] nonconsecutive range repetition"
                                     " (IEEE 1800-2023 16.9.2)");
        nodep->replaceWith(नया AstConst{nodep->fileline(), AstConst::BitFalse{}});
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        वापस true;
    }

    व्योम visit(AstSGotoRep* nodep) override {
        // Standalone goto rep (not inside implication antecedent)
        iterateChildren(nodep);
        File_पंक्ति* स्थिर flp = nodep->fileline();
        Ast_गाँठ_व्यंजक* countp = nodep->countp()->unlinkFrBack();
        अगर (!validate_Rep_गिनती(nodep, countp)) वापस;
        Ast_गाँठ_व्यंजक* स्थिर exprp = nodep->exprp()->unlinkFrBack();
        AstPExpr* स्थिर pexprp = createRepPExpr(flp, exprp, countp, nullptr, true, false);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }

    व्योम visit(AstSNonConsRep* nodep) override {
        // Standalone nonconsecutive rep (not inside implication antecedent)
        iterateChildren(nodep);
        अगर (rejectNonconsecRange(nodep, nodep->maxCountp())) वापस;
        File_पंक्ति* स्थिर flp = nodep->fileline();
        Ast_गाँठ_व्यंजक* countp = nodep->countp()->unlinkFrBack();
        अगर (!validate_Rep_गिनती(nodep, countp)) वापस;
        Ast_गाँठ_व्यंजक* स्थिर exprp = nodep->exprp()->unlinkFrBack();
        AstPExpr* स्थिर pexprp = createRepPExpr(flp, exprp, countp, nullptr, true, true);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }

    व्योम visit(AstFuncRef* nodep) override {
        // IEEE 1800-2023 16.8: Inline sequence instances wherever they appear
        // in the expression tree (inside implications, boolean ops, nested refs, etc.)
        अगर (AstSequence* स्थिर seqp = VN_CAST(nodep->taskp(), Sequence)) {
            substituteSequenceCall(nodep, seqp);
            // The FuncRef has been replaced; do not access nodep after this point.
            // The replacement node will be visited by the parent's iterateChildren.
            वापस;
        }
        अगर (स्थिर AstProperty* स्थिर propp = VN_CAST(nodep->taskp(), Property)) {
            substituteNestedPropertyCall(nodep, propp);
            वापस;
        }
        iterateChildren(nodep);
    }
    व्योम visit(AstImplication* nodep) override {
        अगर (nodep->sentreep()) वापस;  // Already processed

        // Top-level followed-by is claimed by V3AssertNfa; anything reaching here
        // is nested inside iff/implies/or. IEEE 1800-2023 16.12.9 permits the
        // composition, but silent lowering would drop non-vacuous-fail semantics.
        अगर (nodep->isFollowedBy()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: followed-by (#-# / #=#) nested inside property operator"
                          " (iff/implies/or) (IEEE 1800-2023 16.12.9)");
            nodep->replaceWith(नया AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            वापस;
        }

        // Handle goto repetition as antecedent before iterateChildren,
        // so the standalone AstSGotoRep visitor doesn't process it
        अगर (AstSGotoRep* स्थिर gotop = VN_CAST(nodep->lhsp(), SGotoRep)) {
            iterateChildren(gotop);
            iterate_And_अग्र_Null(nodep->rhsp());
            File_पंक्ति* स्थिर flp = nodep->fileline();
            Ast_गाँठ_व्यंजक* countp = gotop->countp()->unlinkFrBack();
            अगर (!validate_Rep_गिनती(nodep, countp)) वापस;
            Ast_गाँठ_व्यंजक* स्थिर exprp = gotop->exprp()->unlinkFrBack();
            Ast_गाँठ_व्यंजक* स्थिर rhsp = nodep->rhsp()->unlinkFrBack();
            AstPExpr* स्थिर pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), false);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            वापस;
        }

        // Handle nonconsecutive repetition as antecedent before iterateChildren,
        // so the standalone AstSNonConsRep visitor doesn't process it
        अगर (AstSNonConsRep* स्थिर ncrp = VN_CAST(nodep->lhsp(), SNonConsRep)) {
            iterateChildren(ncrp);
            iterate_And_अग्र_Null(nodep->rhsp());
            अगर (rejectNonconsecRange(nodep, ncrp->maxCountp())) वापस;
            File_पंक्ति* स्थिर flp = nodep->fileline();
            Ast_गाँठ_व्यंजक* countp = ncrp->countp()->unlinkFrBack();
            अगर (!validate_Rep_गिनती(nodep, countp)) वापस;
            Ast_गाँठ_व्यंजक* स्थिर exprp = ncrp->exprp()->unlinkFrBack();
            Ast_गाँठ_व्यंजक* स्थिर rhsp = nodep->rhsp()->unlinkFrBack();
            AstPExpr* स्थिर pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), true);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            वापस;
        }

        iterateChildren(nodep);

        File_पंक्ति* स्थिर flp = nodep->fileline();
        Ast_गाँठ_व्यंजक* स्थिर rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_गाँठ_व्यंजक* lhsp = nodep->lhsp()->unlinkFrBack();

        // Lower sequence match items (IEEE 16.11): (expr, var = val, ...) |-> / |=>
        अगर (Ast_व्यंजक_Stmt* स्थिर व्यंजक_Stmtp = VN_CAST(lhsp, व्यंजक_Stmt)) {
            Ast_गाँठ_व्यंजक* स्थिर antExprp = व्यंजक_Stmtp->resultp()->unlinkFrBack();

            अगर (nodep->isOverlapped()) {
                // |-> : assign to __Vpropvar via always_comb (continuous).
                // The assign evaluates RHS once; V3Sampled snapshots the
                // result so all consequent refs read the same value.
                Ast_गाँठ* matchAssignsp = nullptr;
                क्रम (Ast_गाँठ* stmtp = व्यंजक_Stmtp->stmtsp(); stmtp;) {
                    Ast_गाँठ* स्थिर nextp = stmtp->nextp();
                    अगर (AstAssign* स्थिर assignp = VN_CAST(stmtp, Assign)) {
                        assignp->unlinkFrBack();
                        matchAssignsp = Ast_गाँठ::add_अग्र_Null(matchAssignsp, assignp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(धकेलो_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                अगर (matchAssignsp) {
                    AstAlways* स्थिर alwaysp
                        = नया AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, matchAssignsp};
                    m_modp->addStmtsp(alwaysp);
                }
            } अन्यथा {
                // |=> : assign to __Vpropvar via NBA in a clocked always block.
                // The NBA commits before the next cycle's sampled snapshot,
                // so the consequent (which already references __Vpropvar)
                // sees the captured value.
                Ast_गाँठ* matchAssignsp = nullptr;
                क्रम (Ast_गाँठ* stmtp = व्यंजक_Stmtp->stmtsp(); stmtp;) {
                    Ast_गाँठ* स्थिर nextp = stmtp->nextp();
                    अगर (AstAssign* स्थिर assignp = VN_CAST(stmtp, Assign)) {
                        assignp->unlinkFrBack();
                        Ast_गाँठ_व्यंजक* स्थिर assignLhsp = assignp->lhsp()->unlinkFrBack();
                        Ast_गाँठ_व्यंजक* स्थिर assignRhsp = assignp->rhsp()->unlinkFrBack();
                        AstAssignDly* स्थिर dlyp = नया AstAssignDly{flp, assignLhsp, assignRhsp};
                        VL_DO_DANGLING(धकेलो_Deletep(assignp), assignp);
                        matchAssignsp = Ast_गाँठ::add_अग्र_Null(matchAssignsp, dlyp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(धकेलो_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                अगर (matchAssignsp) {
                    AstIf* स्थिर condp
                        = नया AstIf{flp, antExprp->clone_वृक्ष_Pure(false), matchAssignsp};
                    AstAlways* स्थिर alwaysp
                        = नया AstAlways{flp, VAlwaysKwd::ALWAYS, new_Sen_वृक्ष(nodep), condp};
                    m_modp->addStmtsp(alwaysp);
                }
            }
        }

        अगर (AstPExpr* स्थिर pexprp = VN_CAST(rhsp, PExpr)) {
            // Implication with sequence expression on RHS (IEEE 1800-2023 16.11, 16.12.7).
            // The PExpr was lowered from the property expression earlier in this pass.
            // Wrap the PExpr body with the antecedent check so the sequence only
            // starts when the antecedent holds.
            Ast_गाँठ_व्यंजक* condp;
            अगर (nodep->isOverlapped()) {
                // Overlapped implication (|->): check antecedent on same cycle.
                // disable iff is applied at the assertion level, not at the
                // antecedent gate, matching the existing non-PExpr overlapped path.
                condp = lhsp;
            } अन्यथा {
                // Non-overlapped implication (|=>): check antecedent from previous cycle
                अगर (m_disablep) {
                    lhsp
                        = नया AstAnd{flp, नया AstNot{flp, m_disablep->clone_वृक्ष_Pure(false)}, lhsp};
                }
                AstPast* स्थिर pastp
                    = नया AstPast{flp, lhsp, nullptr, nullptr, /* propertyTiming */ true};
                pastp->dtypeFrom(lhsp);
                pastp->sentreep(new_Sen_वृक्ष(nodep));
                condp = pastp;
            }
            // Wrap existing PExpr body: if (antecedent) { <original body> } else { /* vacuous pass
            // */ }
            // Only the statements are guarded; declarations stay directly in the block. After
            // V3Fork moves the block's statements into a task, V3Task only handles variables
            // that are direct statements of the task.
            AstBegin* स्थिर bodyp = pexprp->bodyp();
            AstIf* स्थिर guardp = नया AstIf{flp, condp};
            क्रम (Ast_गाँठ* stmtp = bodyp->stmtsp(); stmtp;) {
                Ast_गाँठ* स्थिर nextp = stmtp->nextp();
                अगर (!VN_IS(stmtp, Var)) guardp->addThensp(stmtp->unlinkFrBack());
                stmtp = nextp;
            }
            bodyp->addStmtsp(guardp);
            nodep->replaceWith(pexprp);
            // Don't iterate pexprp here -- it was already iterated when created
            // (in visit(AstSExpr*)), so delays and disable iff are already processed.
        } अन्यथा अगर (nodep->isOverlapped()) {
            Ast_गाँठ_व्यंजक* स्थिर exprp
                = m_underCover ? static_cast<Ast_गाँठ_व्यंजक*>(नया AstLogAnd{flp, lhsp, rhsp})
                               : नया AstLogOr{flp, नया AstLogNot{flp, lhsp}, rhsp};
            nodep->replaceWith(exprp);
        } अन्यथा {
            अगर (m_disablep) {
                lhsp = नया AstAnd{flp, नया AstNot{flp, m_disablep->clone_वृक्ष_Pure(false)}, lhsp};
            }

            AstPast* स्थिर pastp
                = नया AstPast{flp, lhsp, nullptr, nullptr, /* propertyTiming */ true};
            pastp->dtypeFrom(lhsp);
            pastp->sentreep(new_Sen_वृक्ष(nodep));
            Ast_गाँठ_व्यंजक* स्थिर exprp
                = m_underCover ? static_cast<Ast_गाँठ_व्यंजक*>(नया AstAnd{flp, pastp, rhsp})
                               : नया AstOr{flp, नया AstNot{flp, pastp}, rhsp};
            exprp->dtypeSetBit();
            nodep->replaceWith(exprp);
        }
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    व्योम visit(AstUntil* nodep) override {
        File_पंक्ति* स्थिर flp = nodep->fileline();
        UASSERT_OBJ(
            !m_pexprp, nodep,
            "'" << nodep->verilogKwd()
                << "' in complex property expression should have been rejected by V3AssertNfa");
        अगर (nodep->isStrong()
            && (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue())) {
            nodep->v3warn(E_NOTIMING, nodep->verilogKwd() << " requires --timing");
            nodep->replaceWith(नया AstConst{flp, AstConst::BitFalse{}});
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            वापस;
        }
        अगर (nodep->isStrong()) {
            // p s_until q / p s_until_with q: q must eventually be true. Until then, p must
            // be true on every sampled tick. For s_until, check q first: when q is true on
            // this tick, p is not required. For s_until_with, p must be true on the q tick too.
            Ast_गाँठ_व्यंजक* स्थिर rawLhsp = nodep->lhsp()->unlinkFrBack();
            Ast_गाँठ_व्यंजक* स्थिर rawRhsp = nodep->rhsp()->unlinkFrBack();
            AstSampled* स्थिर lhsp = नया AstSampled{flp, rawLhsp, rawLhsp->dtypep(), true};
            AstSampled* स्थिर rhsp = नया AstSampled{flp, rawRhsp, rawRhsp->dtypep(), true};
            Ast_गाँठ_व्यंजक* finalCondp = rhsp->clone_वृक्ष_Pure(false);
            अगर (nodep->isOverlapping()) {
                finalCondp = नया AstLogAnd{flp, lhsp->clone_वृक्ष_Pure(false), finalCondp};
            }

            // Track active assertion attempts. Only the count is needed to emit final failures.
            AstVar* स्थिर activep = नया AstVar{flp, VVarType::MODULETEMP, m_activeNames.लो(""),
                                               nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            activep->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(activep);

            AstVar* स्थिर donep
                = नया AstVar{flp, VVarType::BLOCKTEMP, "__VassertDone", nodep->findBitDType()};
            donep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            स्वतः setDone = [&]() {
                वापस नया AstAssign{flp, नया AstVarRef{flp, donep, VAccess::WRITE},
                                     नया AstConst{flp, AstConst::BitTrue{}}};
            };

            AstLoop* स्थिर loopp = नया AstLoop{flp};
            AstAssign* स्थिर decrementVar = नया AstAssign{
                flp, नया AstVarRef{flp, activep, VAccess::WRITE},
                नया AstSub{flp, नया AstVarRef{flp, activep, VAccess::READ}, नया AstConst{flp, 1}}};
            loopp->addStmtsp(नया AstLoopTest{
                flp, loopp, नया AstLogNot{flp, नया AstVarRef{flp, donep, VAccess::READ}}});
            {
                Ast_गाँठ* स्थिर passp = नया AstPExprClause{flp};
                passp->add_अग्र(decrementVar);
                passp->add_अग्र(setDone());
                Ast_गाँठ_व्यंजक* passCondp = rhsp;
                अगर (nodep->isOverlapping()) {
                    passCondp = नया AstLogAnd{flp, lhsp->clone_वृक्ष_Pure(false), passCondp};
                }
                loopp->addStmtsp(नया AstIf{flp, passCondp, passp});
            }
            {
                Ast_गाँठ* स्थिर failp = नया AstPExprClause{flp, false};
                failp->add_अग्र(decrementVar->clone_वृक्ष(false));
                failp->add_अग्र(setDone());
                loopp->addStmtsp(नया AstIf{
                    flp,
                    नया AstLogAnd{flp,
                                  नया AstLogNot{flp, नया AstVarRef{flp, donep, VAccess::READ}},
                                  नया AstLogNot{flp, lhsp}},
                    failp});
            }
            AstDelay* स्थिर delayp = नया AstDelay{flp, नया AstConst{flp, 1}, false};
            delayp->timeunit(m_modp->timeunit());
            loopp->addStmtsp(नया AstIf{
                flp, नया AstLogNot{flp, नया AstVarRef{flp, donep, VAccess::READ}}, delayp});

            // Main assertion block
            AstBegin* स्थिर bodyp = नया AstBegin{flp, m_blockNames.लो(""), nullptr, true};
            bodyp->addStmtsp(donep);
            bodyp->addStmtsp(नया AstAssign{flp, नया AstVarRef{flp, donep, VAccess::WRITE},
                                           नया AstConst{flp, AstConst::BitFalse{}}});
            File_पंक्ति* स्थिर flp = activep->fileline();
            bodyp->addStmtsp(
                नया AstAssign{flp, नया AstVarRef{flp, activep, VAccess::WRITE},
                              नया AstAdd{flp, नया AstVarRef{flp, activep, VAccess::READ},
                                         नया AstConst{flp, 1}}});
            bodyp->addStmtsp(loopp);

            // Validate assertion condition for each active assert
            AstVar* स्थिर activeCountp = नया AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                    nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            activeCountp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

            AstAssign* स्थिर initActiveCountp
                = नया AstAssign{flp, नया AstVarRef{flp, activeCountp, VAccess::WRITE},
                                नया AstVarRef{flp, activep, VAccess::READ}};
            AstLoop* स्थिर finalLoopp = नया AstLoop{flp};
            finalLoopp->addStmtsp(
                नया AstLoopTest{flp, finalLoopp,
                                नया AstNeq{flp, नया AstVarRef{flp, activeCountp, VAccess::READ},
                                           नया AstConst{flp, 0}}});
            finalLoopp->addStmtsp(नया AstIf{flp, finalCondp, नया AstPExprClause{flp},
                                            नया AstPExprClause{flp, false}});
            finalLoopp->addStmtsp(
                नया AstAssign{flp, नया AstVarRef{flp, activeCountp, VAccess::WRITE},
                              नया AstSub{flp, नया AstVarRef{flp, activeCountp, VAccess::READ},
                                         नया AstConst{flp, 1}}});

            // Final assertion block
            AstBegin* स्थिर finalp = नया AstBegin{flp, m_blockNames.लो(""), nullptr, true};
            finalp->addStmtsp(activeCountp);
            finalp->addStmtsp(initActiveCountp);
            finalp->addStmtsp(finalLoopp);

            AstPExpr* स्थिर pexprp = नया AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
            VL_RESTORER(m_pexprp);
            VL_RESTORER(m_hasCycleDelay);
            m_pexprp = pexprp;
            m_hasCycleDelay = false;
            iterate(bodyp);
            iterate(finalp);

            UASSERT_OBJ(!m_hasCycleDelay, nodep,
                        "s_until cycle delay should have been handled by V3AssertNfa");

            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            वापस;
        }
        AstLoop* स्थिर loopp = नया AstLoop{flp};
        Ast_गाँठ_व्यंजक* स्थिर rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_गाँठ_व्यंजक* स्थिर lhsp = nodep->lhsp()->unlinkFrBack();
        AstLogAnd* स्थिर loopCondp = नया AstLogAnd{flp, lhsp, नया AstLogNot{flp, rhsp}};
        loopp->addStmtsp(नया AstLoopTest{flp, loopp, loopCondp});
        loopp->addStmtsp(नया AstEventControl{flp, new_Sen_वृक्ष(nodep), nullptr});

        Ast_गाँठ_व्यंजक* स्थिर rhsCopyp = rhsp->clone_वृक्ष_Pure(false);
        Ast_गाँठ_व्यंजक* स्थिर passCondp
            = nodep->isOverlapping() ? नया AstLogAnd{flp, lhsp->clone_वृक्ष_Pure(false), rhsCopyp}
                                     : rhsCopyp;
        AstBegin* स्थिर beginp = नया AstBegin{flp, m_blockNames.लो(""), loopp, true};
        beginp->addStmtsp(
            नया AstIf{flp, passCondp, नया AstPExprClause{flp}, नया AstPExprClause{flp, false}});

        AstPExpr* स्थिर pexprp = नया AstPExpr{flp, beginp, nodep->dtypep()};
        pexprp->user1(1);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }

    व्योम visit(AstDefaultDisable* nodep) override {
        VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
    }
    व्योम visit(AstInferredDisable* nodep) override {
        Ast_गाँठ* newp;
        अगर (m_defaultDisablep) {
            newp = m_defaultDisablep->condp()->clone_वृक्ष_Pure(true);
        } अन्यथा {
            newp = नया AstConst{nodep->fileline(), AstConst::BitFalse{}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }

    व्योम visit(AstPropSpec* nodep) override {
        nodep = substitutePropertyCall(nodep);
        iterate_And_अग्र_Null(nodep->sensesp());
        अगर (m_senip && m_senip != nodep->sensesp())
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Only one PSL clock allowed per assertion");
        स्थिर AstCover* स्थिर coverp = VN_CAST(nodep->backp(), Cover);
        स्थिर bool seqEvent = coverp && coverp->isSeqEvent();
        अगर (!nodep->disablep() && m_defaultDisablep && !seqEvent) {
            nodep->disablep(m_defaultDisablep->condp()->clone_वृक्ष_Pure(true));
        }
        m_disablep = nodep->disablep();
        // Unlink and just keep a pointer to it, convert to sentree as needed
        m_senip = nodep->sensesp();
        iterateNull(nodep->disablep());
        अगर (!VN_AS(nodep->backp(), गाँठ_Cover_Or_Assert)->immediate()) {
            स्थिर Ast_गाँठ_DType* स्थिर propDtp = nodep->propp()->dtypep();
            nodep->propp(नया AstSampled{nodep->fileline(), nodep->propp()->unlinkFrBack(),
                                        propDtp->dtypep(), true});
        }
        // cover counts non-vacuous matches only (IEEE 1800-2023 16.15.2), so an
        // implication antecedent must hold; assert passes vacuously instead.
        VL_RESTORER(m_underCover);
        m_underCover = VN_IS(nodep->backp(), Cover);
        iterate(nodep->propp());
        iterateNull(nodep->matchCountp());
    }
    व्योम visit(AstPExpr* nodep) override {
        // V3AssertNfa handles multi-cycle property expressions before this pass,
        // so the following unsupported paths are defensive and typically unreached.
        अगर (m_pexprp && m_pexprp->user1()) {  // LCOV_EXCL_START
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Complex property expression inside 'until''");
            nodep->replaceWith(नया AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            वापस;
        }  // LCOV_EXCL_STOP
        अगर (AstLogNot* स्थिर notp = VN_CAST(nodep->backp(), LogNot)) {
            notp->replaceWith(nodep->unlinkFrBack());
            VL_DO_DANGLING(धकेलो_Deletep(notp), notp);
            iterate(nodep);
            वापस;
        }
        // Sequence expression as antecedent of implication is not yet supported
        अगर (AstImplication* स्थिर implp = VN_CAST(nodep->backp(), Implication)) {
            अगर (implp->lhsp() == nodep) {  // LCOV_EXCL_START
                implp->v3warn(E_UNSUPPORTED,
                              "Unsupported: Implication with sequence expression as antecedent");
                nodep->replaceWith(नया AstConst{nodep->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                वापस;
            }  // LCOV_EXCL_STOP
        }
        VL_RESTORER(m_pexprp);
        VL_RESTORER(m_disableSeqIfp);
        m_pexprp = nodep;

        अगर (m_disablep) {
            स्थिर AstSampled* sampledp = nullptr;
            अगर (m_disablep->exists([&sampledp](स्थिर AstSampled* स्थिर sp) {
                    sampledp = sp;
                    वापस true;
                })) {
                sampledp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: $sampled inside disabled condition of a sequence");
                m_disablep = नया AstConst{m_disablep->fileline(), AstConst::BitFalse{}};
                // always a copy is used, so remove it now
                धकेलो_Deletep(m_disablep);  // m_disablep used later
            }
            File_पंक्ति* स्थिर flp = nodep->fileline();
            // Add counter which counts times the condition turned true
            AstVar* स्थिर disableCntp
                = नया AstVar{flp, VVarType::MODULETEMP, m_disableCntNames.लो(""),
                             nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            disableCntp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(disableCntp);
            AstVarRef* स्थिर readCntRefp = नया AstVarRef{flp, disableCntp, VAccess::READ};
            AstVarRef* स्थिर writeCntRefp = नया AstVarRef{flp, disableCntp, VAccess::WRITE};
            AstAssign* स्थिर incrStmtp = नया AstAssign{
                flp, writeCntRefp, नया AstAdd{flp, readCntRefp, नया AstConst{flp, 1}}};
            AstAlways* स्थिर alwaysp
                = नया AstAlways{flp, VAlwaysKwd::ALWAYS,
                                नया Ast_Sen_वृक्ष{flp, नया AstSenItem{flp, VEdgeType::ET_POSEDGE,
                                                                   m_disablep->clone_वृक्ष(false)}},
                                incrStmtp};
            disableCntp->add_अग्र_Here(alwaysp);

            // Store value of that counter at the beginning of sequence evaluation
            AstBegin* स्थिर bodyp = nodep->bodyp();
            AstVar* स्थिर initialCntp = नया AstVar{flp, VVarType::BLOCKTEMP, "__VinitialCnt",
                                                   nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            initialCntp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            AstAssign* स्थिर assignp
                = नया AstAssign{flp, नया AstVarRef{flp, initialCntp, VAccess::WRITE},
                                readCntRefp->clone_वृक्ष(false)};
            // Prepend to the sequence body to keep statement list structure valid.
            UASSERT_OBJ(!bodyp->name().empty(), bodyp, "Sequence body should be named");
            Ast_गाँठ* स्थिर newStmtsp = Ast_गाँठ::add_अग्र<Ast_गाँठ, Ast_गाँठ>(initialCntp, assignp);
            bodyp->prependStmtsp(newStmtsp);

            m_disableSeqIfp
                = नया AstIf{flp, नया AstEq{flp, नया AstVarRef{flp, initialCntp, VAccess::READ},
                                           readCntRefp->clone_वृक्ष(false)}};
            // Delete it, because it is always copied before insetion to the AST
            धकेलो_Deletep(m_disableSeqIfp);  // m_disableSeqIfp used later
        }
        iterateChildren(nodep);
    }
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultClkEvtVarp);
        VL_RESTORER(m_defaultDisablep);
        VL_RESTORER(m_modp);
        अगर (!VN_IS(nodep, Class)) {
            // A class nested in a module is in the scope of the module's default clocking
            m_defaultClockingp = nullptr;
            m_defaultClkEvtVarp = nullptr;
        }
        m_defaultDisablep = nodep->defaultDisablep();
        m_modp = nodep;
        iterateChildren(nodep);
    }
    व्योम visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    व्योम visit(AstProperty* nodep) override {
        // The body will be visited when will be substituted in place of property reference
        // (AstFuncRef)
        VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
    }
    व्योम visit(AstSequence* nodep) override {
        // Sequence declarations are not visited directly; their bodies are inlined
        // at call sites by visit(AstFuncRef*). Collect for post-traversal cleanup.
        m_seqsToCleanup.धकेलो_back(nodep);
    }
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    // CONSTRUCTORS
    explicit AssertPreVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        { AssertPreClockingVisitor{nodep}; }
        // Process
        iterate(nodep);
        // Fix up varref names
        क्रम (AstVarXRef* xrefp : m_xrefsp) xrefp->name(xrefp->varp()->name());
        // Clean up sequence declarations after inlining.
        // Referenced sequences that were inlined have isReferenced cleared.
        // Remaining referenced sequences are in unsupported contexts (e.g. @seq event).
        क्रम (AstSequence* seqp : m_seqsToCleanup) {
            अगर (seqp->isReferenced()) {
                seqp->v3warn(E_UNSUPPORTED,
                             "Unsupported: sequence referenced outside assertion property");
            } अन्यथा {
                VL_DO_DANGLING(seqp->unlinkFrBack()->delete_वृक्ष(), seqp);
            }
        }
    }
    ~AssertPreVisitor() override = शेष;
};

//######################################################################
// Top Assert class

व्योम V3AssertPre::assertPreAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { AssertPreVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("assertpre", 0, dump_वृक्ष_Either_Level() >= 3);
}
