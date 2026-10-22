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

वर्ग AssertPreClockingVisitor final : सार्वजनिक VNVisitorConst {
    // Captures the clocking block of each clocking item before AssertPreVisitor, which needs it
    // at synchronous drives, often visited before the clocking block, e.g. in a parent module
    // NODE STATE
    // AstClockingItem::user3p()  // AstClocking*. Clocking block of the item, user3 claimed by
    //                            //               AssertPreVisitor

    // VISITORS
    शून्य visit(AstNetlist* nodep) override { iterateAndNextConstNull(nodep->modulesp()); }
    शून्य visit(Ast_गाठ_Module* nodep) override { iterateAndNextConstNull(nodep->stmtsp()); }
    शून्य visit(AstGenBlock* nodep) override { iterateAndNextConstNull(nodep->itemsp()); }
    शून्य visit(AstClocking* nodep) override {
        साठी (Ast_गाठ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            जर (AstClockingItem* स्थिरांक citemp = VN_CAST(itemp, ClockingItem)) {
                citemp->user3p(nodep);
            }
        }
    }
    // Clocking blocks are only in modules and their generate blocks
    शून्य visit(Ast_गाठ*) override {}

सार्वजनिक:
    // CONSTRUCTORS
    explicit AssertPreClockingVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~AssertPreClockingVisitor() override = पूर्वनिर्धारित;
};

//######################################################################
// Assert class functions

वर्ग AssertPreVisitor final : सार्वजनिक VNVisitor {
    // Removes clocks and other pre-optimizations
    // Eventually inlines calls to sequences, properties, etc.
    // We're not parsing the tree, or anything more complicated.
खाजगी:
    // TYPES
    संरचना SynchDrive final {
        AstClockingItem* itemp;  // Clocking item of the driven clockvar
        Ast_गाठ_अभिव्यक्ती* refp;  // Reference to the driven clockvar
    };

    // NODE STATE
    // AstClockingItem::user1p()         // AstVar*.      varp() of ClockingItem after unlink
    // AstClockingItem::user2p()         // AstVar*.      Flag set by drives of output clockvar
    // AstClockingItem::user3p()         // AstClocking*. Clocking block of the item, captured
    //                                   //               by AssertPreClockingVisitor
    // AstPExpr::user1()                 // bool.         Created from AstUntil
    स्थिरांक VNUser1InUse m_inuser1;
    स्थिरांक VNUser2InUse m_inuser2;
    स्थिरांक VNUser3InUse m_inuser3;
    // STATE
    // Current context:
    AstNetlist* स्थिरांक m_netlistp = nullptr;  // Current netlist
    Ast_गाठ_Module* m_modp = nullptr;  // Current module
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
    Ast_गाठ_अभिव्यक्ती* m_disablep = nullptr;  // Last disable
    AstIf* m_disableSeqIfp = nullptr;  // Used for handling disable iff in sequences
    AstPExpr* m_pexprp = nullptr;  // Last AstPExpr
    तार्किक m_underCover = असत्य;  // True if the enclosing assertion is a cover
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
    तार्किक m_inAssign = असत्य;  // True if in an AssignNode
    तार्किक m_inAssignDlyLhs = असत्य;  // True if in AssignDly's LHS
    तार्किक m_inSynchDrive = असत्य;  // True if in synchronous drive
    तार्किक m_inConcatAssign = असत्य;  // True if in assignment to a concatenation
    std::vector<SynchDrive> m_drives;  // Clockvars written by the synchronous drive
    तार्किक m_hasCycleDelay = असत्य;  // True if node has cycle delay beneath
    std::vector<AstVarXRef*> m_xrefsp;  // list of xrefs that need name fixup
    std::vector<AstSequence*> m_seqsToCleanup;  // Sequences to clean up after traversal

    // METHODS

    // Flag set by each drive of an output clockvar, added to the module with its clocking item
    AstVar* getCreateDrivenVarp(AstClockingItem* itemp) {
        जर (!itemp->user2p()) {
            AstVar* स्थिरांक varp = नवीन AstVar{itemp->fileline(), VVarType::MODULETEMP,
                                            m_drivenNames.get(itemp), itemp->findBitDType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            itemp->user2p(varp);
        }
        परत VN_AS(itemp->user2p(), Var);
    }
    // Reference to a variable in the scope of a driven clockvar, made like the clockvar's
    स्थिर Ast_गाठ_अभिव्यक्ती* newDriveRefp(File_ओळ* flp, स्थिरांक SynchDrive& drive, AstVar* varp,
                                     स्थिरांक VAccess& access) {
        जर (स्थिरांक AstVarXRef* स्थिरांक xrefp = VN_CAST(drive.refp, VarXRef)) {
            परत नवीन AstVarXRef{flp, varp, xrefp->dotted(), access};
        } अन्यथा जर (स्थिरांक AstMemberSel* स्थिरांक selp = VN_CAST(drive.refp, MemberSel)) {
            // The interface expression is evaluated again, so it must not have side effects;
            // cloneTreePure warns if it has any
            AstMemberSel* स्थिरांक newSelp
                = नवीन AstMemberSel{flp, selp->fromp()->clone_वृक्ष_Pure(असत्य), varp};
            newSelp->access(access);
            परत newSelp;
        }
        परत नवीन AstVarRef{flp, varp, access};
    }
    // Assignment setting the drive flag of a driven clockvar
    AstAssign* newDrivenSetp(File_ओळ* flp, स्थिरांक SynchDrive& drive) {
        AstVar* स्थिरांक varp = getCreateDrivenVarp(drive.itemp);
        AstAssign* स्थिरांक setp = नवीन AstAssign{flp, newDriveRefp(flp, drive, varp, VAccess::WRITE),
                                              नवीन AstConst{flp, AstConst::BitTrue{}}};
        setp->user1(सत्य);
        परत setp;
    }
    // Clocking block of a clocking item
    स्थिर AstClocking* clockingOf(स्थिरांक AstClockingItem* itemp) {
        UASSERT_OBJ(itemp->user3p(), itemp, "Clocking item not captured in a clocking block");
        परत VN_AS(itemp->user3p(), Clocking);
    }
    // Clocking event of a driven clockvar, referenced from the drive like the clockvar, or
    // nullptr if unsupported, which is reported on the drive's cycle delay
    Ast_Sen_वृक्ष* newDriveSentreep(AstDelay* delayp, स्थिरांक SynchDrive& drive) {
        Ast_Sen_वृक्ष* स्थिरांक origp = clockingOf(drive.itemp)->sentreep();
        जर (VN_IS(drive.refp, VarRef)) परत origp->clone_वृक्ष(असत्य);
        जर (origp->exists([](स्थिरांक AstVarXRef*) { परत सत्य; })) {
            // The reference is relative to the clocking block
            delayp->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in synchronous drive to"
                                          " clockvar in another scope, whose clocking event"
                                          " has a hierarchical reference");
            परत nullptr;
        }
        स्थिरांक AstMemberSel* स्थिरांक selp = VN_CAST(drive.refp, MemberSel);
        // Processes outside classes evaluate their events also when not waiting on them,
        // possibly before the interface reference is set, so the event is guarded against a
        // null reference, which needs an integral value; scheduling does not guard it
        स्थिरांक तार्किक guardNull = selp && !VN_IS(m_modp, Class);
        जर (guardNull) {
            साठी (AstSenItem* itemp = origp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                जर (!itemp->sensp()->dtypep()->skipRefp()->isIntegralOrPacked()) {
                    delayp->v3warn(E_UNSUPPORTED,
                                   "Unsupported: cycle delay in synchronous drive through"
                                   " an interface reference outside a class, to clocking"
                                   " block with non-integral clocking event");
                    परत nullptr;
                }
            }
        }
        // Interface declaring the clocking block, if driven through an interface reference
        AstIface* स्थिरांक ifacep
            = selp ? VN_AS(selp->fromp()->dtypep()->skipRefp(), IfaceRefDType)->ifaceViaCellp()
                   : nullptr;
        Ast_Sen_वृक्ष* स्थिरांक sentreep = origp->clone_वृक्ष(असत्य);
        std::vector<AstVarRef*> refps;
        sentreep->foreach([&](AstVarRef* refp) {
            जर (!refp->classOrPackagep()) refps.ढकला_back(refp);
        });
        साठी (AstVarRef* स्थिरांक refp : refps) {
            AstVar* स्थिरांक varp = refp->varp();
            जर (ifacep) {
                // Mark the variable as read through an interface reference, as V3Width does for
                // a member select. The mark is the interface declaring the variable, whose
                // instances all share it, so drives through references to any instances agree.
                UASSERT_OBJ(!varp->sensIfacep() || varp->sensIfacep() == ifacep, refp,
                            "Variable read through references to different interfaces");
                varp->sensIfacep(ifacep);
            }
            refp->replaceWith(newDriveRefp(refp->fileline(), drive, varp, refp->access()));
            VL_DO_DANGLING(ढकला_Deletep(refp), refp);
        }
        जर (guardNull) {
            साठी (AstSenItem* itemp = sentreep->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                guardNullIface(itemp, selp->fromp());
            }
        }
        परत sentreep;
    }
    // Make an integral clocking event through an interface reference false while the reference
    // is null
    स्थिर शून्य guardNullIface(AstSenItem* sensesp, Ast_गाठ_अभिव्यक्ती* fromp) {
        File_ओळ* स्थिरांक flp = sensesp->fileline();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक sensp = sensesp->sensp()->unlinkFrBack();
        sensesp->sensp(नवीन AstCond{flp, newNotNullp(flp, fromp), sensp,
                                   नवीन AstConst{flp, AstConst::DTyped{}, sensp->dtypep()}});
        जर (Ast_गाठ_अभिव्यक्ती* स्थिरांक condp = sensesp->condp()) {
            condp->unlinkFrBack();
            sensesp->condp(नवीन AstLogAnd{flp, newNotNullp(flp, fromp), condp});
        }
    }
    स्थिर Ast_गाठ_अभिव्यक्ती* newNotNullp(File_ओळ* flp, Ast_गाठ_अभिव्यक्ती* fromp) {
        परत नवीन AstNeq{flp, fromp->clone_वृक्ष_Pure(असत्य), नवीन AstConst{flp, AstConst::Null{}}};
    }
    // Procedural '##0': if the default clocking event has not yet occurred in this time step,
    // wait for it; otherwise continue without suspension (IEEE 1800-2023 14.11)
    AstIf* newCycleDelayZerop(File_ओळ* flp) स्थिरांक {
        AstVar* स्थिरांक evtVarp = m_defaultClkEvtVarp;
        UASSERT_OBJ(evtVarp, flp, "Default clocking event var not pre-created");
        AstCMethodHard* स्थिरांक isTriggeredp = नवीन AstCMethodHard{
            flp, नवीन AstVarRef{flp, evtVarp, VAccess::READ}, VCMethod::EVENT_IS_TRIGGERED};
        isTriggeredp->dtypeSetBit();
        AstEventControl* स्थिरांक waitp = नवीन AstEventControl{
            flp,
            नवीन Ast_Sen_वृक्ष{flp, नवीन AstSenItem{flp, VEdgeType::ET_EVENT,
                                               नवीन AstVarRef{flp, evtVarp, VAccess::READ}}},
            nullptr};
        परत नवीन AstIf{flp, नवीन AstNot{flp, isTriggeredp}, waitp};
    }

    स्थिर शून्य checkSamplingFuncDType(Ast_गाठ_अभिव्यक्ती* nodep, स्थिरांक Ast_गाठ* exprp) {
        स्थिरांक Ast_गाठ_DType* स्थिरांक dtypep = exprp->dtypep()->skipRefp();
        जर (!dtypep->isIntegralOrPacked()) {
            nodep->v3error("Expected numeric type, but got a " << dtypep->prettyDTypeNameQ()
                                                               << " data type");
        }
    }

    Ast_Sen_वृक्ष* new_Sen_वृक्ष(Ast_गाठ* nodep, Ast_Sen_वृक्ष* useTreep = nullptr,
                           Ast_गाठ_Cover_Or_Assert* cassertp = nullptr) {
        // Create sentree based on clocked or default clock
        // Return nullptr for always
        जर (useTreep) परत useTreep;
        Ast_Sen_वृक्ष* newp = nullptr;
        AstSenItem* senip = m_senip;
        तार्किक fromAlways = असत्य;
        जर (!senip && m_defaultClockingp) senip = m_defaultClockingp->sentreep()->sensesp();
        जर (!senip) {
            senip = m_seniAlwaysp;
            fromAlways = सत्य;
        }
        जर (!senip) {
            nodep->v3error("Concurrent assertion has no clock (IEEE 1800-2023 16.16)\n"
                           << nodep->warnMore()
                           << "... Suggest provide a clocking event, a default"
                              " clocking, or a clocked procedural context");
            newp = नवीन Ast_Sen_वृक्ष{nodep->fileline(), nullptr};
        } अन्यथा {
            जर (cassertp && fromAlways) cassertp->senFromAlways(सत्य);
            newp = नवीन Ast_Sen_वृक्ष{nodep->fileline(), senip->clone_वृक्ष(सत्य)};
        }
        परत newp;
    }
    Ast_गाठ_अभिव्यक्ती* getSequenceBodyExprp(स्थिरांक AstSequence* स्थिरांक seqp) स्थिरांक {
        // The statements in AstSequence are optional AstVar (ports) followed by body expr.
        Ast_गाठ* bodyp = seqp->stmtsp();
        पर्यंत (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        परत VN_CAST(bodyp, गाठ_अभिव्यक्ती);
    }
    AstPropSpec* getPropertyExprp(स्थिरांक AstProperty* स्थिरांक propp) {
        // Statements in AstProperty: AstVar (ports/local vars),
        // AstInitialStaticStmt/AstInitialAutomaticStmt (var init), AstPropSpec (body).
        Ast_गाठ* propExprp = propp->stmtsp();
        पर्यंत (propExprp
               && (VN_IS(propExprp, Var) || VN_IS(propExprp, InitialStaticStmt)
                   || VN_IS(propExprp, InitialAutomaticStmt))) {
            propExprp = propExprp->nextp();
        }
        परत VN_CAST(propExprp, PropSpec);
    }
    शून्य substituteSequenceCall(AstFuncRef* funcrefp, AstSequence* seqp) {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Inline the sequence body at the call site, replacing the FuncRef
        Ast_गाठ_अभिव्यक्ती* bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        // Clone the body expression since the sequence may be referenced multiple times
        Ast_गाठ_अभिव्यक्ती* clonedp = bodyExprp->clone_वृक्ष(असत्य);
        // Build substitution map, then do a single traversal to replace all formals
        // (textual substitution per IEEE 16.8.2).
        स्थिरांक V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<स्थिरांक AstVar*, Ast_गाठ_अभिव्यक्ती*> portMap;
        साठी (स्थिरांक स्वतः& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            स्थिरांक स्वतः it = portMap.find(refp->varp());
            जर (it != portMap.end()) {
                refp->replaceWith(it->second->clone_वृक्ष(असत्य));
                VL_DO_DANGLING(ढकला_Deletep(refp), refp);
            }
        });
        साठी (स्थिरांक स्वतः& tconnect : tconnects) {
            ढकला_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        // Replace the FuncRef with the inlined body
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(ढकला_Deletep(funcrefp), funcrefp);
        // Clear referenced flag; sequences with isReferenced==false are deleted in assertPreAll
        seqp->isReferenced(असत्य);
    }
    AstPropSpec* clonePropertyBody(AstFuncRef* funcrefp, स्थिरांक AstProperty* propp) {
        AstPropSpec* propExprp = getPropertyExprp(propp);
        // Substitute inner property call before copying in order to not doing the same for
        // each call of outer property call.
        propExprp = substitutePropertyCall(propExprp);
        // Clone subtree after substitution. It is needed, because property might be called
        // multiple times with different arguments.
        propExprp = propExprp->clone_वृक्ष(असत्य);
        // Build substitution maps for formal arguments and property-local
        // variables, then perform a single foreach to apply all replacements.
        // Map port vars to their actual argument expressions
        स्थिरांक V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propp->stmtsp());
        std::unordered_map<स्थिरांक AstVar*, Ast_गाठ_अभिव्यक्ती*> portMap;
        साठी (स्थिरांक स्वतः& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }

        // Promote property-local variables (non-port vars, IEEE 16.10) to
        // module-level __Vpropvar temps. Cross-cycle persistence is handled
        // by the match item lowering in visit(AstImplication*).
        std::unordered_map<स्थिरांक AstVar*, AstVar*> localVarMap;
        साठी (Ast_गाठ* stmtp = propp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            जर (AstVar* स्थिरांक varp = VN_CAST(stmtp, Var)) {
                जर (!varp->isIO()) {
                    स्थिरांक string newName = m_propVarNames.get(varp);
                    AstVar* स्थिरांक newVarp = नवीन AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->addStmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        // Single traversal: substitute ports and update local var references
        propExprp->foreach([&](AstVarRef* refp) {
            स्थिरांक स्वतः portIt = portMap.find(refp->varp());
            जर (portIt != portMap.end()) {
                refp->replaceWith(portIt->second->clone_वृक्ष(असत्य));
                VL_DO_DANGLING(ढकला_Deletep(refp), refp);
                परत;
            }
            स्थिरांक स्वतः localIt = localVarMap.find(refp->varp());
            जर (localIt != localVarMap.end()) refp->varp(localIt->second);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        // Clean up argument expressions
        साठी (स्थिरांक स्वतः& tconnect : tconnects) {
            ढकला_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(propExprp, funcrefp);
        परत propExprp;
    }
    AstPropSpec* substitutePropertyCall(AstPropSpec* nodep) {
        जर (AstFuncRef* स्थिरांक funcrefp = VN_CAST(nodep->propp(), FuncRef)) {
            जर (स्थिरांक AstProperty* स्थिरांक propp = VN_CAST(funcrefp->taskp(), Property)) {
                AstPropSpec* स्थिरांक propExprp = clonePropertyBody(funcrefp, propp);

                // Handle case with 2 disable iff statement (IEEE 1800-2023 16.12.1)
                जर (nodep->disablep() && propExprp->disablep()) {
                    nodep->v3error("disable iff expression before property call and in its "
                                   "body is not legal");
                    ढकला_Deletep(propExprp->disablep()->unlinkFrBack());
                }
                // If disable iff is in outer property, move it to inner
                जर (nodep->disablep()) {
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक disablep = nodep->disablep()->unlinkFrBack();
                    propExprp->disablep(disablep);
                }

                जर (nodep->sensesp() && propExprp->sensesp()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Clock event before property call and in its body");
                    ढकला_Deletep(propExprp->sensesp()->unlinkFrBack());
                }
                // If clock event is in outer property, move it to inner
                जर (nodep->sensesp()) {
                    AstSenItem* स्थिरांक sensesp = nodep->sensesp();
                    sensesp->unlinkFrBack();
                    propExprp->sensesp(sensesp);
                }

                // Now substitute property reference with property body
                nodep->replaceWith(propExprp);
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                परत propExprp;
            }
        }
        परत nodep;
    }

    शून्य substituteNestedPropertyCall(AstFuncRef* funcrefp, स्थिरांक AstProperty* propp) {
        AstPropSpec* स्थिरांक specp = clonePropertyBody(funcrefp, propp);
        Ast_गाठ* replp = nullptr;
        जर (specp->disablep()) {
            funcrefp->v3error("Illegal 'disable iff' in a nested property instance"
                              " (IEEE 1800-2023 16.12.1)");
        } अन्यथा जर (specp->sensesp()) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: clock event in a nested property instance");
        } अन्यथा जर (specp->propp()->exists(
                       [](स्थिरांक Ast_गाठ_अभिव्यक्ती* np) { परत np->isMultiCycleSva(); })) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: multi-cycle body in a nested property instance");
        } अन्यथा {
            replp = specp->propp()->unlinkFrBack();
        }
        जर (!replp) replp = नवीन AstConst{funcrefp->fileline(), AstConst::BitFalse{}};
        funcrefp->replaceWith(replp);
        VL_DO_DANGLING(ढकला_Deletep(funcrefp), funcrefp);
        VL_DO_DANGLING(ढकला_Deletep(specp), specp);
    }

    // VISITORS
    //========== Statements
    शून्य visit(AstClocking* स्थिरांक nodep) override {
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        UINFO(8, "   CLOCKING" << nodep);
        जर (nodep->isDefault()) {
            जर (m_defaultClockingp) {
                nodep->v3error("Only one default clocking block allowed per module"
                               " (IEEE 1800-2023 14.12)");
            } अन्यथा {
                m_defaultClockingp = nodep;
                m_defaultClkEvtVarp = nodep->ensureEventp();
            }
        }
        iterateChildren(nodep);
        जर (nodep->eventp()) nodep->addNextHere(nodep->eventp()->unlinkFrBack());
        VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
    }
    शून्य visit(AstModportClockingRef* स्थिरांक nodep) override {
        // It has to be converted to a list of ModportClockingVarRefs,
        // because clocking blocks are removed in this pass
        साठी (Ast_गाठ* itemp = nodep->clockingp()->itemsp(); itemp; itemp = itemp->nextp()) {
            जर (AstClockingItem* स्थिरांक citemp = VN_CAST(itemp, ClockingItem)) {
                जर (AstVar* स्थिरांक varp
                    = citemp->varp() ? citemp->varp() : VN_AS(citemp->user1p(), Var)) {
                    AstModportVarRef* स्थिरांक modVarp = नवीन AstModportVarRef{
                        nodep->fileline(), varp->name(), citemp->direction()};
                    modVarp->varp(varp);
                    nodep->addNextHere(modVarp);
                    जर (citemp->direction().isOutput()) {
                        // Drives through the modport set the drive flag
                        AstVar* स्थिरांक drivenVarp = getCreateDrivenVarp(citemp);
                        AstModportVarRef* स्थिरांक drivenRefp = नवीन AstModportVarRef{
                            nodep->fileline(), drivenVarp->name(), VDirection::OUTPUT};
                        drivenRefp->varp(drivenVarp);
                        nodep->addNextHere(drivenRefp);
                    }
                }
            }
        }
        VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
    }
    शून्य visit(AstClockingItem* स्थिरांक nodep) override {
        // Get a ref to the sampled/driven variable
        AstVar* स्थिरांक varp = nodep->varp();
        जर (!varp) {
            // Unused item
            परत;
        }
        // Flag set by drives of an output clockvar, possibly visited before this item
        जर (nodep->direction().isOutput()) m_modp->addStmtsp(getCreateDrivenVarp(nodep));
        File_ओळ* स्थिरांक flp = nodep->fileline();
        V3Const::constifyEdit(nodep->skewp());
        जर (!VN_IS(nodep->skewp(), Const)) {
            nodep->skewp()->v3error("Skew must be constant (IEEE 1800-2023 14.4)");
            परत;
        }
        AstConst* स्थिरांक skewp = VN_AS(nodep->skewp(), Const);
        जर (skewp->अंक().isNegative()) skewp->v3error("Skew cannot be negative");
        Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp = nodep->exprp();
        varp->name(m_clockingp->name() + "__DOT__" + varp->name());
        m_clockingp->addNextHere(varp->unlinkFrBack());
        nodep->user1p(varp);
        varp->user1p(nodep);
        जर (nodep->direction() == VDirection::OUTPUT) {
            exprp->foreach([](स्थिरांक Ast_गाठ_Var_Ref* varrefp) {
                // Prevent confusing BLKANDNBLK warnings on clockvars due to generated assignments
                varrefp->fileline()->warnOff(V3ErrorCode::BLKANDNBLK, सत्य);
                // Mark the driven signal so later passes (e.g. V3Inst port
                // connection) know the drive flows out from inside the cell.
                // Item expressions are plain signal references, so only the
                // driven signal is marked, never index selects.
                varrefp->varp()->isClockingDriven(सत्य);
            });
            AstVarRef* स्थिरांक skewedReadRefp = नवीन AstVarRef{flp, varp, VAccess::READ};
            skewedReadRefp->user1(सत्य);
            // Initialize the clockvar
            AstVarRef* स्थिरांक skewedWriteRefp = नवीन AstVarRef{flp, varp, VAccess::WRITE};
            skewedWriteRefp->user1(सत्य);
            AstInitialStatic* स्थिरांक initClockvarp = नवीन AstInitialStatic{
                flp, नवीन AstAssign{flp, skewedWriteRefp, exprp->clone_वृक्ष_Pure(असत्य)}};
            m_modp->addStmtsp(initClockvarp);
            // Each drive sets a flag, so the signal is also assigned when driven with the same
            // value again, e.g. after another assignment to the signal (IEEE 1800-2023 14.16.2)
            AstVar* स्थिरांक drivenVarp = getCreateDrivenVarp(nodep);
            m_modp->addStmtsp(नवीन AstInitialStatic{
                flp, नवीन AstAssign{flp, नवीन AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                   नवीन AstConst{flp, AstConst::BitFalse{}}}});
            // Assign the clockvar to the actual var if it was driven
            AstAssign* स्थिरांक assignp
                = नवीन AstAssign{flp, exprp->clone_वृक्ष_Pure(असत्य), skewedReadRefp};
            AstIf* स्थिरांक ifp
                = नवीन AstIf{flp, नवीन AstVarRef{flp, drivenVarp, VAccess::READ},
                            नवीन AstAssign{flp, नवीन AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                          नवीन AstConst{flp, AstConst::BitFalse{}}}};
            ifp->addThensp(assignp);
            // Only in the time step of the clocking event
            AstCMethodHard* स्थिरांक trigp = नवीन AstCMethodHard{
                nodep->fileline(), नवीन AstVarRef{flp, m_clockingp->ensureEventp(), VAccess::READ},
                VCMethod::EVENT_IS_TRIGGERED};
            trigp->dtypeSetBit();
            ifp->condp(नवीन AstLogAnd{flp, ifp->condp()->unlinkFrBack(), trigp});
            // A skew rounding to no time precision ticks is a zero skew
            Ast_गाठ_अभिव्यक्ती* स्थिरांक ticksp
                = V3Timing::delayToTicks(skewp->clone_वृक्ष(असत्य), m_modp->timeunit());
            स्थिरांक तार्किक zeroSkew = ticksp->isZero();
            VL_DO_DANGLING(ticksp->delete_वृक्ष(), ticksp);
            जर (zeroSkew || !skewp->fileline()->timingOn()) {
                // Drive the var in Re-NBA (IEEE 1800-2023 14.16), ignoring skews where timing is
                // turned off
                Ast_Sen_वृक्ष* senTreep = m_clockingp->sentreep()->clone_वृक्ष(असत्य);
                senTreep->addSensesp(नवीन AstSenItem{
                    flp, VEdgeType::ET_CHANGED, नवीन AstVarRef{flp, drivenVarp, VAccess::READ}});
                m_clockingp->addNextHere(नवीन AstAlwaysReactive{flp, senTreep, ifp});
            } अन्यथा {
                // Fork only the assignment, so this AlwaysPostponed can be retriggered
                // before the assignment happens
                AstFork* स्थिरांक forkp = नवीन AstFork{flp, VJoinType::JOIN_NONE};
                forkp->immediateStart(सत्य);
                forkp->addForksp(नवीन AstBegin{flp, "", assignp->unlinkFrBack(), सत्य});
                ifp->addThensp(forkp);
                // Read the clockvar in Postponed, so a drive late in the time step is applied
                // at this event plus the skew (IEEE 1800-2023 14.16)
                m_clockingp->addNextHere(नवीन AstAlwaysPostponed{flp, ifp});
                जर (v3Global.opt.timing().isSetTrue()) {
                    AstDelay* स्थिरांक delayp = नवीन AstDelay{flp, skewp->unlinkFrBack(), असत्य};
                    delayp->timeunit(m_modp->timeunit());
                    assignp->timingControlp(delayp);
                } अन्यथा जर (v3Global.opt.timing().isSetFalse()) {
                    nodep->v3warn(E_NOTIMING,
                                  "Clocking output skew greater than #0 requires --timing");
                } अन्यथा {
                    nodep->v3warn(E_NEEDTIMINGOPT,
                                  "Use --timing or --no-timing to specify how "
                                  "clocking output skew greater than #0 should be handled");
                }
            }
        } अन्यथा जर (nodep->direction() == VDirection::आदान) {
            // Ref to the clockvar
            AstVarRef* स्थिरांक refp = नवीन AstVarRef{flp, varp, VAccess::WRITE};
            refp->user1(सत्य);
            जर (skewp->अंक().is1Step()) {
                // #1step means the value that is sampled is always the signal's last value
                // before the clock edge (IEEE 1800-2023 14.4)
                AstSampled* स्थिरांक sampledp
                    = नवीन AstSampled{flp, exprp->clone_वृक्ष_Pure(असत्य), exprp->dtypep(), सत्य};
                AstAssign* स्थिरांक assignp = नवीन AstAssign{flp, refp, sampledp};
                m_clockingp->addNextHere(नवीन AstAlways{
                    flp, VAlwaysKwd::ALWAYS, m_clockingp->sentreep()->clone_वृक्ष(असत्य), assignp});
            } अन्यथा जर (skewp->isZero()) {
                // #0 means the var has to be sampled in Observed (IEEE 1800-2023 14.13)
                AstAssign* स्थिरांक assignp = नवीन AstAssign{flp, refp, exprp->clone_वृक्ष_Pure(असत्य)};
                m_clockingp->addNextHere(नवीन AstAlwaysObserved{
                    flp, m_clockingp->sentreep()->clone_वृक्ष(असत्य), assignp});
            } अन्यथा {
                // Create a queue where we'll store sampled values with timestamps
                AstSampleQueueDType* स्थिरांक queueDtp
                    = नवीन AstSampleQueueDType{flp, exprp->dtypep()};
                m_netlistp->typeTablep()->addTypesp(queueDtp);
                AstVar* स्थिरांक queueVarp
                    = नवीन AstVar{flp, VVarType::MODULETEMP, "__Vqueue__" + varp->name(), queueDtp};
                queueVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_clockingp->addNextHere(queueVarp);
                // Create a process like this:
                //     always queue.push(<sampled var>);
                AstCMethodHard* स्थिरांक pushp = नवीन AstCMethodHard{
                    flp, नवीन AstVarRef{flp, queueVarp, VAccess::WRITE}, VCMethod::DYN_ढकला,
                    नवीन AstTime{nodep->fileline(), m_modp->timeunit()}};
                pushp->addPinsp(exprp->clone_वृक्ष_Pure(असत्य));
                pushp->dtypeSetVoid();
                m_clockingp->addNextHere(
                    नवीन AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, pushp->makeStmt()});
                // Create a process like this:
                //     always @<clocking event> queue.pop(<skew>, /*out*/<skewed var>);
                AstCMethodHard* स्थिरांक popp = नवीन AstCMethodHard{
                    flp, नवीन AstVarRef{flp, queueVarp, VAccess::READWRITE}, VCMethod::DYN_काढा,
                    नवीन AstTime{nodep->fileline(), m_modp->timeunit()}};
                popp->addPinsp(skewp->unlinkFrBack());
                refp->access(VAccess::READWRITE);  // Only conditionally assigned
                popp->addPinsp(refp);
                popp->dtypeSetVoid();
                m_clockingp->addNextHere(नवीन AstAlways{flp, VAlwaysKwd::ALWAYS,
                                                       m_clockingp->sentreep()->clone_वृक्ष(असत्य),
                                                       popp->makeStmt()});
            }
        } अन्यथा {
            nodep->v3fatalSrc("Invalid direction");
        }
    }
    शून्य visit(AstDelay* nodep) override {
        m_hasCycleDelay |= nodep->isCycleDelay();
        // Only cycle delays are relevant in this stage; also only process once
        जर (!nodep->isCycleDelay()) {
            जर (m_inSynchDrive) {
                nodep->v3error("Only cycle delays can be used in synchronous drives"
                               " (IEEE 1800-2023 14.16)");
            }
            iterateChildren(nodep);
            परत;
        }
        जर (m_inAssign && !m_inSynchDrive) {
            nodep->v3error("Cycle delays not allowed as intra-assignment delays"
                           " (IEEE 1800-2023 14.11)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
            परत;
        }
        जर (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBackWithNext());
        File_ओळ* स्थिरांक flp = nodep->fileline();
        Ast_गाठ_अभिव्यक्ती* valuep = V3Const::constifyEdit(nodep->lhsp()->unlinkFrBack());
        स्थिरांक AstConst* स्थिरांक constp = VN_CAST(valuep, Const);
        जर (!constp && m_pexprp) {
            // V3AssertNfa handles non-const delays in sequences before this pass and
            // replaces the property; this branch should never be reached.
            nodep->v3fatalSrc("Non-constant cycle delay in assertion: "
                              "should have been caught by V3AssertNfa");
        } अन्यथा जर (constp && constp->isZero()) {
            VL_DO_DANGLING(ढकला_Deletep(valuep), valuep);
            जर (m_inSynchDrive) {
                // ##0 has no effect in synchronous drives (IEEE 1800-2023 14.11)
                VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
                परत;
            }
            जर (m_pexprp) {
                // ##0 in sequence context = zero delay = same clock tick
                VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
                परत;
            }
            // Procedural ##0: synchronize with default clocking event (IEEE 1800-2023 14.11)
            जर (!m_defaultClockingp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
                परत;
            }
            nodep->replaceWith(newCycleDelayZerop(flp));
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            परत;
        }
        Ast_Sen_वृक्ष* sentreep = nullptr;
        जर (!m_drives.empty()) {
            जर (m_inConcatAssign) {
                // Also the clockvars may be of different clocking blocks
                nodep->v3error("Cycle delays cannot be used in synchronous drives to a"
                               " concatenation (IEEE 1800-2023 14.16)");
                VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(ढकला_Deletep(valuep), valuep);
                परत;
            }
            // Count the cycles of the driven clockvar's clocking block (IEEE 1800-2023 14.16)
            sentreep = newDriveSentreep(nodep, m_drives.front());
            जर (!sentreep) {
                VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(ढकला_Deletep(valuep), valuep);
                परत;
            }
        } अन्यथा जर (!m_defaultClockingp) {
            जर (!m_pexprp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(ढकला_Deletep(valuep), valuep);
                परत;
            }
            sentreep = नवीन Ast_Sen_वृक्ष{flp, m_senip->clone_वृक्ष(असत्य)};
        } अन्यथा {
            sentreep = m_defaultClockingp->sentreep()->clone_वृक्ष(असत्य);
        }
        AstEventControl* स्थिरांक controlp
            = नवीन AstEventControl{nodep->fileline(), sentreep, nullptr};
        स्थिरांक std::string delayName = m_cycleDlyNames.get(nodep);
        Ast_गाठ_अभिव्यक्ती* throughoutp
            = nodep->throughoutp() ? nodep->throughoutp()->unlinkFrBack() : nullptr;

        AstVar* स्थिरांक cntVarp = नवीन AstVar{flp, VVarType::BLOCKTEMP, delayName + "__counter",
                                           nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstBegin* स्थिरांक beginp = नवीन AstBegin{flp, delayName + "__block", cntVarp, सत्य};
        // A non-constant delay is counted at runtime. The count is integral of 64 bits, as
        // V3Width makes delays, or real, which is rounded (IEEE 1800-2023 6.12.1)
        जर (valuep->isDouble()) {
            valuep = नवीन AstRToIRoundS{flp, valuep};
            valuep->dtypeFrom(cntVarp);
        } अन्यथा {
            valuep = नवीन AstSel{flp, valuep, 0, cntVarp->width()};
        }
        beginp->addStmtsp(नवीन AstAssign{flp, नवीन AstVarRef{flp, cntVarp, VAccess::WRITE}, valuep});
        // If the count is zero, a synchronous drive waits for none, like with a constant '##0',
        // and a procedural delay waits like a procedural '##0' (IEEE 1800-2023 14.11)
        जर (!constp && !m_inSynchDrive) {
            beginp->addStmtsp(नवीन AstIf{
                flp,
                नवीन AstEq{flp, नवीन AstVarRef{flp, cntVarp, VAccess::READ}, नवीन AstConst{flp, 0}},
                newCycleDelayZerop(flp)});
        }

        // Throughout: create flag tracking whether condition held every tick
        AstVar* throughoutOkp = nullptr;
        जर (throughoutp) {
            throughoutOkp = नवीन AstVar{flp, VVarType::BLOCKTEMP, delayName + "__throughoutOk",
                                       nodep->findBasicDType(VBasicDTypeKwd::LOGIC_IMPLICIT)};
            throughoutOkp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->addStmtsp(throughoutOkp);
            beginp->addStmtsp(नवीन AstAssign{flp, नवीन AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            नवीन AstConst{flp, AstConst::BitTrue{}}});
            // Check condition at tick 0 (sequence start, before entering loop)
            beginp->addStmtsp(
                नवीन AstIf{flp, नवीन AstLogNot{flp, throughoutp->clone_वृक्ष_Pure(असत्य)},
                          नवीन AstAssign{flp, नवीन AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                        नवीन AstConst{flp, AstConst::BitFalse{}}}});
        }

        {
            AstLoop* स्थिरांक loopp = नवीन AstLoop{flp};
            // When throughout is present, exit loop early if condition fails
            Ast_गाठ_अभिव्यक्ती* loopCondp
                = नवीन AstGt{flp, नवीन AstVarRef{flp, cntVarp, VAccess::READ}, नवीन AstConst{flp, 0}};
            जर (throughoutOkp) {
                loopCondp = नवीन AstLogAnd{flp, loopCondp,
                                          नवीन AstVarRef{flp, throughoutOkp, VAccess::READ}};
            }
            loopp->addStmtsp(नवीन AstLoopTest{flp, loopp, loopCondp});
            loopp->addStmtsp(controlp);
            loopp->addStmtsp(
                नवीन AstAssign{flp, नवीन AstVarRef{flp, cntVarp, VAccess::WRITE},
                              नवीन AstSub{flp, नवीन AstVarRef{flp, cntVarp, VAccess::READ},
                                         नवीन AstConst{flp, 1}}});
            // Check throughout condition at each tick during delay (IEEE 1800-2023 16.9.9)
            जर (throughoutp) {
                loopp->addStmtsp(
                    नवीन AstIf{flp, नवीन AstLogNot{flp, throughoutp},
                              नवीन AstAssign{flp, नवीन AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            नवीन AstConst{flp, AstConst::BitFalse{}}}});
            }
            beginp->addStmtsp(loopp);
        }
        // Compose wrappers on remaining sequence: throughout gate (inner), disable iff (outer)
        Ast_गाठ* remainp = nodep->nextp() ? nodep->nextp()->unlinkFrBackWithNext() : nullptr;
        जर (throughoutOkp) {
            // If condition failed during delay, fail assertion
            remainp = नवीन AstIf{flp, नवीन AstVarRef{flp, throughoutOkp, VAccess::READ}, remainp,
                                नवीन AstPExprClause{flp, /*pass=*/असत्य}};
        }
        जर (m_disableSeqIfp && remainp) {
            AstIf* स्थिरांक disableSeqIfp = m_disableSeqIfp->clone_वृक्ष(असत्य);
            disableSeqIfp->addThensp(remainp);
            remainp = disableSeqIfp;
        }
        जर (remainp) {
            जर (throughoutOkp) {
                // throughoutOkp is declared in beginp scope -- check must be inside it
                beginp->addStmtsp(remainp);
            } अन्यथा {
                nodep->addNextHere(remainp);
            }
        }
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
    शून्य visit(Ast_Sen_वृक्ष* nodep) override {
        जर (m_inSynchDrive) {
            nodep->v3error("Event controls cannot be used in "
                           "synchronous drives (IEEE 1800-2023 14.16)");
        }

        स्थिरांक AstSampled* sampledp;
        जर (nodep->exists([&sampledp](स्थिरांक AstSampled* स्थिरांक sp) {
                sampledp = sp;
                परत सत्य;
            })) {
            sampledp->v3warn(E_UNSUPPORTED, "Unsupported: $sampled inside sensitivity list");
        }
    }
    शून्य visit(Ast_गाठ_Var_Ref* nodep) override {
        UINFO(8, " -varref:  " << nodep);
        UINFO(8, " -varref-var-back:  " << nodep->varp()->backp());
        UINFO(8, " -varref-var-user1:  " << nodep->varp()->user1p());
        जर (AstClockingItem* स्थिरांक itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->firstAbovep(),
                ClockingItem)) {
            जर (nodep->user1()) परत;

            // ensure linking still works, this has to be done only once
            जर (AstVarXRef* xrefp = VN_CAST(nodep, VarXRef)) {
                UINFO(8, " -clockvarxref-in:  " << xrefp);
                string dotted = xrefp->dotted();
                स्थिरांक माप_प्रकार dotPos = dotted.rfind('.');
                dotted.erase(dotPos, string::npos);
                xrefp->dotted(dotted);
                UINFO(8, " -clockvarxref-out: " << xrefp);
                m_xrefsp.emplace_back(xrefp);
            }

            जर (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            जर (nodep->access().isWriteOrRW()) {
                जर (itemp->direction() == VDirection::OUTPUT) {
                    जर (!m_inAssignDlyLhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    जर (m_inAssign) {
                        m_inSynchDrive = सत्य;
                        m_drives.ढकला_back({itemp, nodep});
                    }
                } अन्यथा जर (itemp->direction() == VDirection::आदान) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
            nodep->user1(सत्य);
        }
    }
    शून्य visit(AstMemberSel* nodep) override {
        जर (AstClockingItem* स्थिरांक itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->firstAbovep(),
                ClockingItem)) {
            जर (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            जर (nodep->access().isWriteOrRW()) {
                जर (itemp->direction() == VDirection::OUTPUT) {
                    जर (!m_inAssignDlyLhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    जर (m_inAssign) {
                        m_inSynchDrive = सत्य;
                        m_drives.ढकला_back({itemp, nodep});
                    }
                } अन्यथा जर (itemp->direction() == VDirection::आदान) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
        }
    }
    शून्य visit(Ast_गाठ_Assign* nodep) override {
        जर (nodep->user1()) परत;
        VL_RESTORER(m_inAssign);
        VL_RESTORER(m_inSynchDrive);
        VL_RESTORER(m_inConcatAssign);
        VL_RESTORER_CLEAR(m_drives);
        m_inAssign = सत्य;
        m_inSynchDrive = असत्य;
        {
            // The braces of a concatenation are a replication, by one
            स्थिरांक Ast_गाठ_अभिव्यक्ती* lhsp = nodep->lhsp();
            जर (स्थिरांक AstReplicate* स्थिरांक repp = VN_CAST(lhsp, Replicate)) lhsp = repp->srcp();
            m_inConcatAssign = VN_IS(lhsp, Concat);
        }
        {
            VL_RESTORER(m_inAssignDlyLhs);
            m_inAssignDlyLhs = VN_IS(nodep, AssignDly);
            iterate(nodep->lhsp());
        }
        iterate(nodep->rhsp());
        जर (nodep->timingControlp()) iterate(nodep->timingControlp());
        // A drive without a cycle delay, also after removing '##0', updates the clockvar now
        जर (m_inSynchDrive && !nodep->timingControlp()) {
            AstAssign* स्थिरांक assignp = नवीन AstAssign{
                nodep->fileline(), nodep->lhsp()->unlinkFrBack(), nodep->rhsp()->unlinkFrBack()};
            assignp->user1(सत्य);
            nodep->replaceWith(assignp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
            nodep = assignp;
        }
        // Note the drives when they take effect, after the cycle delay if any. Each clockvar
        // of the LHS is driven, e.g. in a concatenation which other simulators accept, although
        // IEEE 1800-2023 14.16 does not allow it
        Ast_गाठ* setsp = nullptr;
        साठी (स्थिरांक SynchDrive& drive : m_drives) {
            setsp = Ast_गाठ::addNext(setsp, newDrivenSetp(nodep->fileline(), drive));
        }
        जर (!setsp) परत;
        जर (AstBegin* स्थिरांक beginp = VN_CAST(nodep->timingControlp(), Begin)) {
            beginp->addStmtsp(setsp);
        } अन्यथा {
            nodep->addNextHere(setsp);
        }
    }
    शून्य visit(AstAlways* nodep) override {
        iterateAndNextNull(nodep->sentreep());
        जर (nodep->sentreep()) m_seniAlwaysp = nodep->sentreep()->sensesp();
        iterateAndNextNull(nodep->stmtsp());
        m_seniAlwaysp = nullptr;
    }

    शून्य visit(Ast_गाठ_Cover_Or_Assert* nodep) override {
        जर (nodep->sentreep()) परत;  // Already processed

        VL_RESTORER(m_senip);
        VL_RESTORER(m_disablep);
        m_senip = nullptr;
        m_disablep = nullptr;

        // Find Clocking's buried under nodep->exprsp
        iterateChildren(nodep);
        जर (!nodep->immediate()) nodep->sentreep(new_Sen_वृक्ष(nodep, nullptr, nodep));
    }
    शून्य visit(AstFalling* nodep) override {
        जर (nodep->user1SetOnce()) परत;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_ओळ* स्थिरांक fl = nodep->fileline();
        Ast_गाठ_अभिव्यक्ती* exprp = nodep->exprp()->unlinkFrBack();
        जर (exprp->width() > 1) exprp = नवीन AstSel{fl, exprp, 0, 1};
        Ast_गाठ_अभिव्यक्ती* स्थिरांक futurep = नवीन AstFuture{fl, exprp, new_Sen_वृक्ष(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = नवीन AstAnd{fl, exprp->clone_वृक्ष_Pure(असत्य), नवीन AstNot{fl, futurep}};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    शून्य visit(AstFell* nodep) override {
        जर (nodep->user1SetOnce()) परत;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_ओळ* स्थिरांक fl = nodep->fileline();
        Ast_गाठ_अभिव्यक्ती* exprp = nodep->exprp()->unlinkFrBack();
        जर (exprp->width() > 1) exprp = नवीन AstSel{fl, exprp, 0, 1};
        Ast_Sen_वृक्ष* sentreep = nodep->sentreep();
        जर (sentreep) sentreep->unlinkFrBack();
        AstPast* स्थिरांक pastp = नवीन AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_वृक्ष(nodep, sentreep));
        exprp = नवीन AstAnd{fl, pastp, नवीन AstNot{fl, exprp->clone_वृक्ष_Pure(असत्य)}};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    शून्य visit(AstFuture* nodep) override {
        जर (nodep->user1SetOnce()) परत;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        Ast_Sen_वृक्ष* स्थिरांक sentreep = nodep->sentreep();
        जर (sentreep) VL_DO_DANGLING(ढकला_Deletep(sentreep->unlinkFrBack()), sentreep);
        nodep->sentreep(new_Sen_वृक्ष(nodep));
    }
    शून्य visit(AstPast* nodep) override {
        checkSamplingFuncDType(nodep, nodep->exprp());
        जर (nodep->sentreep()) परत;  // Already processed
        iterateChildren(nodep);
        nodep->sentreep(new_Sen_वृक्ष(nodep));
    }
    शून्य visit(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- Lower standalone exact [*N] (N >= 2) via saturating counter.
        // Range/unbounded forms and SExpr-contained forms are lowered by V3AssertNfa.
        iterateChildren(nodep);
        // V3AssertNfa handles unbounded/ranged forms upstream, so this fast-path
        // is effectively unreachable when NFA is enabled.
        जर (nodep->unbounded() || nodep->maxCountp()) परत;  // LCOV_EXCL_LINE
        स्थिरांक AstConst* स्थिरांक constp = VN_CAST(nodep->countp(), Const);
        जर (VL_UNLIKELY(!constp || constp->toSInt() < 1)) {
            nodep->v3fatalSrc("Consecutive repetition count must be a positive constant"
                              " (should have been caught by V3Width)");
            परत;
        }
        स्थिरांक पूर्णांक n = constp->toSInt();
        File_ओळ* स्थिरांक flp = nodep->fileline();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp = nodep->exprp()->unlinkFrBack();
        जर (n == 1) {
            nodep->replaceWith(exprp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            परत;
        }
        // Saturating counter: if (expr) cnt <= min(cnt+1, N); else cnt <= 0;
        AstVar* स्थिरांक cntVarp = नवीन AstVar{flp, VVarType::MODULETEMP, m_consRepNames.get(""),
                                           nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(cntVarp);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक अभिव्यक्ती_Clonep = exprp->clone_वृक्ष_Pure(असत्य);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक saturatingIncrp = नवीन AstCond{
            flp,
            नवीन AstLt{flp, नवीन AstVarRef{flp, cntVarp, VAccess::READ},
                      नवीन AstConst{flp, static_cast<uint32_t>(n)}},
            नवीन AstAdd{flp, नवीन AstVarRef{flp, cntVarp, VAccess::READ}, नवीन AstConst{flp, 1u}},
            नवीन AstConst{flp, static_cast<uint32_t>(n)}};
        AstAssignDly* स्थिरांक incrAssignp
            = नवीन AstAssignDly{flp, नवीन AstVarRef{flp, cntVarp, VAccess::WRITE}, saturatingIncrp};
        AstAssignDly* स्थिरांक resetAssignp = नवीन AstAssignDly{
            flp, नवीन AstVarRef{flp, cntVarp, VAccess::WRITE}, नवीन AstConst{flp, 0u}};
        AstIf* स्थिरांक ifp = नवीन AstIf{flp, अभिव्यक्ती_Clonep, incrAssignp, resetAssignp};
        Ast_Sen_वृक्ष* स्थिरांक senTreep = new_Sen_वृक्ष(nodep);
        AstAlways* स्थिरांक alwaysp = नवीन AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, ifp};
        cntVarp->addNextHere(alwaysp);
        // Match: cnt >= N-1 (previous cycles via NBA) && expr (current cycle)
        Ast_गाठ_अभिव्यक्ती* स्थिरांक cntCheckp = नवीन AstGte{flp, नवीन AstVarRef{flp, cntVarp, VAccess::READ},
                                                  नवीन AstConst{flp, static_cast<uint32_t>(n - 1)}};
        cntCheckp->dtypeSetBit();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक matchp = नवीन AstAnd{flp, cntCheckp, exprp};
        matchp->dtypeSetBit();
        nodep->replaceWith(matchp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    शून्य visit(AstRising* nodep) override {
        जर (nodep->user1SetOnce()) परत;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_ओळ* स्थिरांक fl = nodep->fileline();
        Ast_गाठ_अभिव्यक्ती* exprp = nodep->exprp()->unlinkFrBack();
        जर (exprp->width() > 1) exprp = नवीन AstSel{fl, exprp, 0, 1};
        Ast_गाठ_अभिव्यक्ती* स्थिरांक futurep = नवीन AstFuture{fl, exprp, new_Sen_वृक्ष(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = नवीन AstAnd{fl, नवीन AstNot{fl, exprp->clone_वृक्ष_Pure(असत्य)}, futurep};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    शून्य visit(AstRose* nodep) override {
        जर (nodep->user1SetOnce()) परत;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_ओळ* स्थिरांक fl = nodep->fileline();
        Ast_गाठ_अभिव्यक्ती* exprp = nodep->exprp()->unlinkFrBack();
        जर (exprp->width() > 1) exprp = नवीन AstSel{fl, exprp, 0, 1};
        Ast_Sen_वृक्ष* sentreep = nodep->sentreep();
        जर (sentreep) sentreep->unlinkFrBack();
        AstPast* स्थिरांक pastp = नवीन AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_वृक्ष(nodep, sentreep));
        exprp = नवीन AstAnd{fl, नवीन AstNot{fl, pastp}, exprp->clone_वृक्ष_Pure(असत्य)};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    स्थिर AstAssocArrayDType* getProcessAssocArrayType(File_ओळ* स्थिरांक flp) {
        // Type of __VassertsActive___x[std::process]
        Ast_गाठ_DType* valp
            = v3Global.rootp()->typeTablep()->findBasicDType(flp, VBasicDTypeKwd::BIT);
        AstClassRefDType* keyp
            = नवीन AstClassRefDType{flp, v3Global.rootp()->stdPackageProcessp(), nullptr};
        keyp->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        v3Global.rootp()->typeTablep()->addTypesp(keyp);
        AstAssocArrayDType* स्थिरांक typep = नवीन AstAssocArrayDType{flp, valp, keyp};
        typep->dtypep(typep);
        v3Global.rootp()->typeTablep()->addTypesp(typep);
        परत typep;
    }
    स्थिर Ast_Stmt_अभिव्यक्ती* getProcessAssocArrayDelete(AstVarRef* स्थिरांक refp) {
        // Constructs refp.delete(std::process::self()) statement
        File_ओळ* स्थिरांक flp = refp->fileline();
        AstCMethodHard* स्थिरांक deletep = नवीन AstCMethodHard{
            flp, refp, VCMethod::ASSOC_ERASE, v3Global.rootp()->stdPackageProcessSelfp(flp)};
        deletep->dtypep(refp->findVoidDType());
        परत नवीन Ast_Stmt_अभिव्यक्ती{flp, deletep};
    }
    स्थिर Ast_गाठ_अभिव्यक्ती* getProcessAssocArraySize(AstVarRef* स्थिरांक refp) {
        // Constructs refp.size() statement
        AstCMethodHard* स्थिरांक sizep
            = नवीन AstCMethodHard{refp->fileline(), refp, VCMethod::ASSOC_SIZE};
        sizep->dtypep(refp->findBasicDType(VBasicDTypeKwd::UINT32));
        परत sizep;
    }
    शून्य visit(AstSEventually* nodep) override {
        UASSERT(v3Global.rootp()->stdPackagep(), "Should be imported");

        Ast_Sen_वृक्ष* स्थिरांक sentreep = new_Sen_वृक्ष(nodep);
        जर (!sentreep->sensesp()) {
            VL_DO_DANGLING(ढकला_Deletep(sentreep), sentreep);
            nodep->replaceWith(नवीन AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            परत;
        }

        File_ओळ* स्थिरांक flp = nodep->fileline();

        // Track active assertions
        AstVar* स्थिरांक activep = नवीन AstVar{flp, VVarType::MODULETEMP, m_activeNames.get(""),
                                           getProcessAssocArrayType(flp)};
        activep->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(activep);

        // Assertion condition check
        AstLoop* स्थिरांक loopp = नवीन AstLoop{flp};
        AstSampled* स्थिरांक condp
            = नवीन AstSampled{flp, nodep->exprp()->unlinkFrBack(), nullptr, सत्य};
        loopp->addStmtsp(नवीन AstLoopTest{flp, loopp, नवीन AstLogNot{flp, condp}});
        loopp->addStmtsp(नवीन AstEventControl{flp, sentreep, nullptr});

        // Add assertion to the active set
        AstAssocSel* स्थिरांक selp = नवीन AstAssocSel{flp, नवीन AstVarRef{flp, activep, VAccess::WRITE},
                                                  v3Global.rootp()->stdPackageProcessSelfp(flp)};
        AstAssign* स्थिरांक incrementp = नवीन AstAssign{flp, selp, नवीन AstConst{flp, 1}};
        AstPExprClause* स्थिरांक clausep = नवीन AstPExprClause{flp};
        Ast_Stmt_अभिव्यक्ती* स्थिरांक deletep
            = getProcessAssocArrayDelete(नवीन AstVarRef{flp, activep, VAccess::WRITE});

        // Main assertion block
        AstBegin* स्थिरांक bodyp = नवीन AstBegin{flp, m_blockNames.get(""), nullptr, सत्य};
        bodyp->addStmtsp(incrementp);
        bodyp->addStmtsp(loopp);
        bodyp->addStmtsp(clausep);
        bodyp->addStmtsp(deletep);

        // Validate assertion condition for each active assert
        AstVar* स्थिरांक activeCountp = नवीन AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        activeCountp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstAssign* स्थिरांक initActiveCountp
            = नवीन AstAssign{flp, नवीन AstVarRef{flp, activeCountp, VAccess::WRITE},
                            getProcessAssocArraySize(नवीन AstVarRef{flp, activep, VAccess::READ})};
        AstLoop* स्थिरांक finalLoopp = नवीन AstLoop{flp};
        AstIf* स्थिरांक finalBodypCondp
            = नवीन AstIf{flp, condp->clone_वृक्ष_Pure(असत्य), नवीन AstPExprClause{flp},
                        नवीन AstPExprClause{flp, असत्य}};
        finalLoopp->addStmtsp(
            नवीन AstLoopTest{flp, finalLoopp,
                            नवीन AstNeq{flp, नवीन AstVarRef{flp, activeCountp, VAccess::READ},
                                       नवीन AstConst{flp, 0}}});
        finalLoopp->addStmtsp(finalBodypCondp);
        finalLoopp->addStmtsp(
            नवीन AstAssign{flp, नवीन AstVarRef{flp, activeCountp, VAccess::WRITE},
                          नवीन AstSub{flp, नवीन AstVarRef{flp, activeCountp, VAccess::READ},
                                     नवीन AstConst{flp, 1}}});

        // Final assertion block
        AstBegin* स्थिरांक finalp = नवीन AstBegin{flp, m_blockNames.get(""), nullptr, सत्य};
        finalp->addStmtsp(activeCountp);
        finalp->addStmtsp(initActiveCountp);
        finalp->addStmtsp(finalLoopp);

        VL_RESTORER(m_pexprp);
        m_pexprp = नवीन AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
        VL_RESTORER(m_hasCycleDelay);
        m_hasCycleDelay = असत्य;
        iterate(bodyp);
        iterate(finalp);

        जर (m_hasCycleDelay) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in s_eventually");
            nodep->replaceWith(नवीन AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            VL_DO_DANGLING(m_pexprp->delete_वृक्ष(), m_pexprp);
            परत;
        }

        nodep->replaceWith(m_pexprp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    शून्य visit(AstStable* nodep) override {
        जर (nodep->user1SetOnce()) परत;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_ओळ* स्थिरांक fl = nodep->fileline();
        Ast_गाठ_अभिव्यक्ती* exprp = nodep->exprp()->unlinkFrBack();
        Ast_Sen_वृक्ष* sentreep = nodep->sentreep();
        जर (sentreep) sentreep->unlinkFrBack();
        AstPast* स्थिरांक pastp = नवीन AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_वृक्ष(nodep, sentreep));
        exprp = नवीन AstEq{fl, pastp, exprp->clone_वृक्ष_Pure(असत्य)};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    शून्य visit(AstSteady* nodep) override {
        जर (nodep->user1SetOnce()) परत;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_ओळ* स्थिरांक fl = nodep->fileline();
        Ast_गाठ_अभिव्यक्ती* exprp = nodep->exprp()->unlinkFrBack();
        जर (exprp->width() > 1) exprp = नवीन AstSel{fl, exprp, 0, 1};
        Ast_गाठ_अभिव्यक्ती* स्थिरांक futurep = नवीन AstFuture{fl, exprp, new_Sen_वृक्ष(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = नवीन AstEq{fl, exprp->clone_वृक्ष_Pure(असत्य), futurep};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    शून्य visit(AstSampled* nodep) override {
        iterateChildren(nodep);
        जर (!nodep->internal()) checkSamplingFuncDType(nodep, nodep->exprp());
    }

    // Validate repetition count: must be a non-negative elaboration-time constant.
    // Shared by goto [->N] and nonconsecutive [=N] repetition.
    // On error, replaces nodep with BitFalse placeholder and returns nullptr.
    स्थिरांक AstConst* validate_Rep_मोजणी(Ast_गाठ* nodep, Ast_गाठ_अभिव्यक्ती*& countp) {
        countp = V3Const::constifyEdit(countp);
        स्थिरांक AstConst* स्थिरांक constp = VN_CAST(countp, Const);
        जर (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ढकला_Deletep(countp), countp);
            nodep->replaceWith(नवीन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            परत nullptr;
        }
        जर (constp->toSInt() < 0) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ढकला_Deletep(countp), countp);
            nodep->replaceWith(नवीन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            परत nullptr;
        }
        जर (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ढकला_Deletep(countp), countp);
            nodep->replaceWith(नवीन AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            परत nullptr;
        }
        परत constp;
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
    AstPExpr* createRepPExpr(File_ओळ* flp, Ast_गाठ_अभिव्यक्ती* exprp, Ast_गाठ_अभिव्यक्ती* countp,
                             Ast_गाठ_अभिव्यक्ती* rhsp, तार्किक isOverlapped, तार्किक isNonConsec) {
        AstSenItem* स्थिरांक sensesp = m_senip;
        UASSERT_OBJ(sensesp, exprp, "Repetition requires a clock");

        स्थिरांक std::string name
            = isNonConsec ? m_nonConsRepNames.get(exprp) : m_gotoRepNames.get(exprp);
        AstVar* स्थिरांक cntVarp = नवीन AstVar{flp, VVarType::BLOCKTEMP, name + "__counter",
                                           exprp->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstBegin* स्थिरांक beginp = नवीन AstBegin{flp, name + "__block", cntVarp, सत्य};
        beginp->addStmtsp(
            नवीन AstAssign{flp, नवीन AstVarRef{flp, cntVarp, VAccess::WRITE}, नवीन AstConst{flp, 0}});

        // Counting loop: find N occurrences of expr (shared by goto and nonconsec)
        AstLoop* स्थिरांक loopp = नवीन AstLoop{flp};
        loopp->addStmtsp(नवीन AstLoopTest{flp, loopp,
                                         नवीन AstLt{flp, नवीन AstVarRef{flp, cntVarp, VAccess::READ},
                                                   countp->clone_वृक्ष_Pure(असत्य)}});
        // if (expr) cnt++ -- sampled is applied to whole property expr by V3Assert
        loopp->addStmtsp(
            नवीन AstIf{flp, exprp,
                      नवीन AstAssign{flp, नवीन AstVarRef{flp, cntVarp, VAccess::WRITE},
                                    नवीन AstAdd{flp, नवीन AstVarRef{flp, cntVarp, VAccess::READ},
                                               नवीन AstConst{flp, 1}}}});
        loopp->addStmtsp(नवीन AstIf{
            flp, नवीन AstLt{flp, नवीन AstVarRef{flp, cntVarp, VAccess::READ}, countp},
            नवीन AstEventControl{flp, नवीन Ast_Sen_वृक्ष{flp, sensesp->clone_वृक्ष(असत्य)}, nullptr}});

        beginp->addStmtsp(loopp);

        जर (isNonConsec && rhsp) {
            // IEEE 16.9.2: b[=N] = b[->N] ##1 !b[*0:$]
            // Trailing window: ##1 advance, then scan !expr cycles checking rhs.
            // Window closes when expr becomes true again (fail if rhs was never true).
            beginp->addStmtsp(नवीन AstEventControl{
                flp, नवीन Ast_Sen_वृक्ष{flp, sensesp->clone_वृक्ष(असत्य)}, nullptr});  // ##1
            जर (!isOverlapped) {
                beginp->addStmtsp(नवीन AstEventControl{
                    flp, नवीन Ast_Sen_वृक्ष{flp, sensesp->clone_वृक्ष(असत्य)}, nullptr});  // |=>
            }
            // Window loop: check rhs at each !expr cycle (done variable for termination)
            AstVar* स्थिरांक doneVarp
                = नवीन AstVar{flp, VVarType::BLOCKTEMP, name + "__done", exprp->findBitDType()};
            doneVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->addStmtsp(doneVarp);
            beginp->addStmtsp(नवीन AstAssign{flp, नवीन AstVarRef{flp, doneVarp, VAccess::WRITE},
                                            नवीन AstConst{flp, AstConst::BitFalse{}}});
            स्वतः setDone = [&]() {
                परत नवीन AstAssign{flp, नवीन AstVarRef{flp, doneVarp, VAccess::WRITE},
                                     नवीन AstConst{flp, AstConst::BitTrue{}}};
            };
            AstLoop* स्थिरांक windowp = नवीन AstLoop{flp};
            // LoopTest: continue while !done
            windowp->addStmtsp(नवीन AstLoopTest{
                flp, windowp, नवीन AstNot{flp, नवीन AstVarRef{flp, doneVarp, VAccess::READ}}});
            // if (expr) { fail; done = 1; } -- window closed, expr true again
            Ast_गाठ* स्थिरांक failsp = नवीन AstPExprClause{flp, असत्य};
            failsp->addNext(setDone());
            windowp->addStmtsp(नवीन AstIf{flp, exprp->clone_वृक्ष_Pure(असत्य), failsp});
            // if (rhs) { pass; done = 1; } -- consequent true at this !expr endpoint
            Ast_गाठ* स्थिरांक passsp = नवीन AstPExprClause{flp, सत्य};
            passsp->addNext(setDone());
            windowp->addStmtsp(नवीन AstIf{flp, rhsp, passsp});
            // @(clk) -- advance to next cycle in window
            windowp->addStmtsp(
                नवीन AstEventControl{flp, नवीन Ast_Sen_वृक्ष{flp, sensesp->clone_वृक्ष(असत्य)}, nullptr});
            beginp->addStmtsp(windowp);
        } अन्यथा जर (isNonConsec) {
            // Standalone nonconsec: ##1 into window, then pass
            beginp->addStmtsp(
                नवीन AstEventControl{flp, नवीन Ast_Sen_वृक्ष{flp, sensesp->clone_वृक्ष(असत्य)}, nullptr});
            beginp->addStmtsp(नवीन AstPExprClause{flp, सत्य});
        } अन्यथा जर (rhsp) {
            // Goto rep: check consequent once at match endpoint
            जर (!isOverlapped) {
                beginp->addStmtsp(नवीन AstEventControl{
                    flp, नवीन Ast_Sen_वृक्ष{flp, sensesp->clone_वृक्ष(असत्य)}, nullptr});
            }
            beginp->addStmtsp(नवीन AstIf{flp, rhsp, नवीन AstPExprClause{flp, सत्य},
                                        नवीन AstPExprClause{flp, असत्य}});
        } अन्यथा {
            // Standalone goto: pass after counting
            beginp->addStmtsp(नवीन AstPExprClause{flp, सत्य});
        }

        परत नवीन AstPExpr{flp, beginp, exprp->findBitDType()};
    }

    // Reject [=M:N] nonconsecutive range that reached V3AssertPre.
    // [->M:N] is fully handled upstream (NFA path); only [=M:N] still lands
    // here because AstSNonConsRep lowering is not yet implemented there.
    // Replaces the node with BitFalse so lowering continues after the warning.
    तार्किक rejectNonconsecRange(Ast_गाठ* nodep, Ast_गाठ_अभिव्यक्ती* maxCountp) {
        जर (!maxCountp) परत असत्य;
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: [=M:N] nonconsecutive range repetition"
                                     " (IEEE 1800-2023 16.9.2)");
        nodep->replaceWith(नवीन AstConst{nodep->fileline(), AstConst::BitFalse{}});
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        परत सत्य;
    }

    शून्य visit(AstSGotoRep* nodep) override {
        // Standalone goto rep (not inside implication antecedent)
        iterateChildren(nodep);
        File_ओळ* स्थिरांक flp = nodep->fileline();
        Ast_गाठ_अभिव्यक्ती* countp = nodep->countp()->unlinkFrBack();
        जर (!validate_Rep_मोजणी(nodep, countp)) परत;
        Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp = nodep->exprp()->unlinkFrBack();
        AstPExpr* स्थिरांक pexprp = createRepPExpr(flp, exprp, countp, nullptr, सत्य, असत्य);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }

    शून्य visit(AstSNonConsRep* nodep) override {
        // Standalone nonconsecutive rep (not inside implication antecedent)
        iterateChildren(nodep);
        जर (rejectNonconsecRange(nodep, nodep->maxCountp())) परत;
        File_ओळ* स्थिरांक flp = nodep->fileline();
        Ast_गाठ_अभिव्यक्ती* countp = nodep->countp()->unlinkFrBack();
        जर (!validate_Rep_मोजणी(nodep, countp)) परत;
        Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp = nodep->exprp()->unlinkFrBack();
        AstPExpr* स्थिरांक pexprp = createRepPExpr(flp, exprp, countp, nullptr, सत्य, सत्य);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }

    शून्य visit(AstFuncRef* nodep) override {
        // IEEE 1800-2023 16.8: Inline sequence instances wherever they appear
        // in the expression tree (inside implications, boolean ops, nested refs, etc.)
        जर (AstSequence* स्थिरांक seqp = VN_CAST(nodep->taskp(), Sequence)) {
            substituteSequenceCall(nodep, seqp);
            // The FuncRef has been replaced; do not access nodep after this point.
            // The replacement node will be visited by the parent's iterateChildren.
            परत;
        }
        जर (स्थिरांक AstProperty* स्थिरांक propp = VN_CAST(nodep->taskp(), Property)) {
            substituteNestedPropertyCall(nodep, propp);
            परत;
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstImplication* nodep) override {
        जर (nodep->sentreep()) परत;  // Already processed

        // Top-level followed-by is claimed by V3AssertNfa; anything reaching here
        // is nested inside iff/implies/or. IEEE 1800-2023 16.12.9 permits the
        // composition, but silent lowering would drop non-vacuous-fail semantics.
        जर (nodep->isFollowedBy()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: followed-by (#-# / #=#) nested inside property operator"
                          " (iff/implies/or) (IEEE 1800-2023 16.12.9)");
            nodep->replaceWith(नवीन AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            परत;
        }

        // Handle goto repetition as antecedent before iterateChildren,
        // so the standalone AstSGotoRep visitor doesn't process it
        जर (AstSGotoRep* स्थिरांक gotop = VN_CAST(nodep->lhsp(), SGotoRep)) {
            iterateChildren(gotop);
            iterateAndNextNull(nodep->rhsp());
            File_ओळ* स्थिरांक flp = nodep->fileline();
            Ast_गाठ_अभिव्यक्ती* countp = gotop->countp()->unlinkFrBack();
            जर (!validate_Rep_मोजणी(nodep, countp)) परत;
            Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp = gotop->exprp()->unlinkFrBack();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = nodep->rhsp()->unlinkFrBack();
            AstPExpr* स्थिरांक pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), असत्य);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            परत;
        }

        // Handle nonconsecutive repetition as antecedent before iterateChildren,
        // so the standalone AstSNonConsRep visitor doesn't process it
        जर (AstSNonConsRep* स्थिरांक ncrp = VN_CAST(nodep->lhsp(), SNonConsRep)) {
            iterateChildren(ncrp);
            iterateAndNextNull(nodep->rhsp());
            जर (rejectNonconsecRange(nodep, ncrp->maxCountp())) परत;
            File_ओळ* स्थिरांक flp = nodep->fileline();
            Ast_गाठ_अभिव्यक्ती* countp = ncrp->countp()->unlinkFrBack();
            जर (!validate_Rep_मोजणी(nodep, countp)) परत;
            Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp = ncrp->exprp()->unlinkFrBack();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = nodep->rhsp()->unlinkFrBack();
            AstPExpr* स्थिरांक pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), सत्य);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            परत;
        }

        iterateChildren(nodep);

        File_ओळ* स्थिरांक flp = nodep->fileline();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* lhsp = nodep->lhsp()->unlinkFrBack();

        // Lower sequence match items (IEEE 16.11): (expr, var = val, ...) |-> / |=>
        जर (Ast_अभिव्यक्ती_Stmt* स्थिरांक अभिव्यक्ती_Stmtp = VN_CAST(lhsp, अभिव्यक्ती_Stmt)) {
            Ast_गाठ_अभिव्यक्ती* स्थिरांक antExprp = अभिव्यक्ती_Stmtp->resultp()->unlinkFrBack();

            जर (nodep->isOverlapped()) {
                // |-> : assign to __Vpropvar via always_comb (continuous).
                // The assign evaluates RHS once; V3Sampled snapshots the
                // result so all consequent refs read the same value.
                Ast_गाठ* matchAssignsp = nullptr;
                साठी (Ast_गाठ* stmtp = अभिव्यक्ती_Stmtp->stmtsp(); stmtp;) {
                    Ast_गाठ* स्थिरांक nextp = stmtp->nextp();
                    जर (AstAssign* स्थिरांक assignp = VN_CAST(stmtp, Assign)) {
                        assignp->unlinkFrBack();
                        matchAssignsp = Ast_गाठ::addNextNull(matchAssignsp, assignp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(ढकला_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                जर (matchAssignsp) {
                    AstAlways* स्थिरांक alwaysp
                        = नवीन AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, matchAssignsp};
                    m_modp->addStmtsp(alwaysp);
                }
            } अन्यथा {
                // |=> : assign to __Vpropvar via NBA in a clocked always block.
                // The NBA commits before the next cycle's sampled snapshot,
                // so the consequent (which already references __Vpropvar)
                // sees the captured value.
                Ast_गाठ* matchAssignsp = nullptr;
                साठी (Ast_गाठ* stmtp = अभिव्यक्ती_Stmtp->stmtsp(); stmtp;) {
                    Ast_गाठ* स्थिरांक nextp = stmtp->nextp();
                    जर (AstAssign* स्थिरांक assignp = VN_CAST(stmtp, Assign)) {
                        assignp->unlinkFrBack();
                        Ast_गाठ_अभिव्यक्ती* स्थिरांक assignLhsp = assignp->lhsp()->unlinkFrBack();
                        Ast_गाठ_अभिव्यक्ती* स्थिरांक assignRhsp = assignp->rhsp()->unlinkFrBack();
                        AstAssignDly* स्थिरांक dlyp = नवीन AstAssignDly{flp, assignLhsp, assignRhsp};
                        VL_DO_DANGLING(ढकला_Deletep(assignp), assignp);
                        matchAssignsp = Ast_गाठ::addNextNull(matchAssignsp, dlyp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(ढकला_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                जर (matchAssignsp) {
                    AstIf* स्थिरांक condp
                        = नवीन AstIf{flp, antExprp->clone_वृक्ष_Pure(असत्य), matchAssignsp};
                    AstAlways* स्थिरांक alwaysp
                        = नवीन AstAlways{flp, VAlwaysKwd::ALWAYS, new_Sen_वृक्ष(nodep), condp};
                    m_modp->addStmtsp(alwaysp);
                }
            }
        }

        जर (AstPExpr* स्थिरांक pexprp = VN_CAST(rhsp, PExpr)) {
            // Implication with sequence expression on RHS (IEEE 1800-2023 16.11, 16.12.7).
            // The PExpr was lowered from the property expression earlier in this pass.
            // Wrap the PExpr body with the antecedent check so the sequence only
            // starts when the antecedent holds.
            Ast_गाठ_अभिव्यक्ती* condp;
            जर (nodep->isOverlapped()) {
                // Overlapped implication (|->): check antecedent on same cycle.
                // disable iff is applied at the assertion level, not at the
                // antecedent gate, matching the existing non-PExpr overlapped path.
                condp = lhsp;
            } अन्यथा {
                // Non-overlapped implication (|=>): check antecedent from previous cycle
                जर (m_disablep) {
                    lhsp
                        = नवीन AstAnd{flp, नवीन AstNot{flp, m_disablep->clone_वृक्ष_Pure(असत्य)}, lhsp};
                }
                AstPast* स्थिरांक pastp
                    = नवीन AstPast{flp, lhsp, nullptr, nullptr, /* propertyTiming */ सत्य};
                pastp->dtypeFrom(lhsp);
                pastp->sentreep(new_Sen_वृक्ष(nodep));
                condp = pastp;
            }
            // Wrap existing PExpr body: if (antecedent) { <original body> } else { /* vacuous pass
            // */ }
            // Only the statements are guarded; declarations stay directly in the block. After
            // V3Fork moves the block's statements into a task, V3Task only handles variables
            // that are direct statements of the task.
            AstBegin* स्थिरांक bodyp = pexprp->bodyp();
            AstIf* स्थिरांक guardp = नवीन AstIf{flp, condp};
            साठी (Ast_गाठ* stmtp = bodyp->stmtsp(); stmtp;) {
                Ast_गाठ* स्थिरांक nextp = stmtp->nextp();
                जर (!VN_IS(stmtp, Var)) guardp->addThensp(stmtp->unlinkFrBack());
                stmtp = nextp;
            }
            bodyp->addStmtsp(guardp);
            nodep->replaceWith(pexprp);
            // Don't iterate pexprp here -- it was already iterated when created
            // (in visit(AstSExpr*)), so delays and disable iff are already processed.
        } अन्यथा जर (nodep->isOverlapped()) {
            Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp
                = m_underCover ? static_cast<Ast_गाठ_अभिव्यक्ती*>(नवीन AstLogAnd{flp, lhsp, rhsp})
                               : नवीन AstLogOr{flp, नवीन AstLogNot{flp, lhsp}, rhsp};
            nodep->replaceWith(exprp);
        } अन्यथा {
            जर (m_disablep) {
                lhsp = नवीन AstAnd{flp, नवीन AstNot{flp, m_disablep->clone_वृक्ष_Pure(असत्य)}, lhsp};
            }

            AstPast* स्थिरांक pastp
                = नवीन AstPast{flp, lhsp, nullptr, nullptr, /* propertyTiming */ सत्य};
            pastp->dtypeFrom(lhsp);
            pastp->sentreep(new_Sen_वृक्ष(nodep));
            Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp
                = m_underCover ? static_cast<Ast_गाठ_अभिव्यक्ती*>(नवीन AstAnd{flp, pastp, rhsp})
                               : नवीन AstOr{flp, नवीन AstNot{flp, pastp}, rhsp};
            exprp->dtypeSetBit();
            nodep->replaceWith(exprp);
        }
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    शून्य visit(AstUntil* nodep) override {
        File_ओळ* स्थिरांक flp = nodep->fileline();
        UASSERT_OBJ(
            !m_pexprp, nodep,
            "'" << nodep->verilogKwd()
                << "' in complex property expression should have been rejected by V3AssertNfa");
        जर (nodep->isStrong()
            && (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue())) {
            nodep->v3warn(E_NOTIMING, nodep->verilogKwd() << " requires --timing");
            nodep->replaceWith(नवीन AstConst{flp, AstConst::BitFalse{}});
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            परत;
        }
        जर (nodep->isStrong()) {
            // p s_until q / p s_until_with q: q must eventually be true. Until then, p must
            // be true on every sampled tick. For s_until, check q first: when q is true on
            // this tick, p is not required. For s_until_with, p must be true on the q tick too.
            Ast_गाठ_अभिव्यक्ती* स्थिरांक rawLhsp = nodep->lhsp()->unlinkFrBack();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक rawRhsp = nodep->rhsp()->unlinkFrBack();
            AstSampled* स्थिरांक lhsp = नवीन AstSampled{flp, rawLhsp, rawLhsp->dtypep(), सत्य};
            AstSampled* स्थिरांक rhsp = नवीन AstSampled{flp, rawRhsp, rawRhsp->dtypep(), सत्य};
            Ast_गाठ_अभिव्यक्ती* finalCondp = rhsp->clone_वृक्ष_Pure(असत्य);
            जर (nodep->isOverlapping()) {
                finalCondp = नवीन AstLogAnd{flp, lhsp->clone_वृक्ष_Pure(असत्य), finalCondp};
            }

            // Track active assertion attempts. Only the count is needed to emit final failures.
            AstVar* स्थिरांक activep = नवीन AstVar{flp, VVarType::MODULETEMP, m_activeNames.get(""),
                                               nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            activep->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(activep);

            AstVar* स्थिरांक donep
                = नवीन AstVar{flp, VVarType::BLOCKTEMP, "__VassertDone", nodep->findBitDType()};
            donep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            स्वतः setDone = [&]() {
                परत नवीन AstAssign{flp, नवीन AstVarRef{flp, donep, VAccess::WRITE},
                                     नवीन AstConst{flp, AstConst::BitTrue{}}};
            };

            AstLoop* स्थिरांक loopp = नवीन AstLoop{flp};
            AstAssign* स्थिरांक decrementVar = नवीन AstAssign{
                flp, नवीन AstVarRef{flp, activep, VAccess::WRITE},
                नवीन AstSub{flp, नवीन AstVarRef{flp, activep, VAccess::READ}, नवीन AstConst{flp, 1}}};
            loopp->addStmtsp(नवीन AstLoopTest{
                flp, loopp, नवीन AstLogNot{flp, नवीन AstVarRef{flp, donep, VAccess::READ}}});
            {
                Ast_गाठ* स्थिरांक passp = नवीन AstPExprClause{flp};
                passp->addNext(decrementVar);
                passp->addNext(setDone());
                Ast_गाठ_अभिव्यक्ती* passCondp = rhsp;
                जर (nodep->isOverlapping()) {
                    passCondp = नवीन AstLogAnd{flp, lhsp->clone_वृक्ष_Pure(असत्य), passCondp};
                }
                loopp->addStmtsp(नवीन AstIf{flp, passCondp, passp});
            }
            {
                Ast_गाठ* स्थिरांक failp = नवीन AstPExprClause{flp, असत्य};
                failp->addNext(decrementVar->clone_वृक्ष(असत्य));
                failp->addNext(setDone());
                loopp->addStmtsp(नवीन AstIf{
                    flp,
                    नवीन AstLogAnd{flp,
                                  नवीन AstLogNot{flp, नवीन AstVarRef{flp, donep, VAccess::READ}},
                                  नवीन AstLogNot{flp, lhsp}},
                    failp});
            }
            AstDelay* स्थिरांक delayp = नवीन AstDelay{flp, नवीन AstConst{flp, 1}, असत्य};
            delayp->timeunit(m_modp->timeunit());
            loopp->addStmtsp(नवीन AstIf{
                flp, नवीन AstLogNot{flp, नवीन AstVarRef{flp, donep, VAccess::READ}}, delayp});

            // Main assertion block
            AstBegin* स्थिरांक bodyp = नवीन AstBegin{flp, m_blockNames.get(""), nullptr, सत्य};
            bodyp->addStmtsp(donep);
            bodyp->addStmtsp(नवीन AstAssign{flp, नवीन AstVarRef{flp, donep, VAccess::WRITE},
                                           नवीन AstConst{flp, AstConst::BitFalse{}}});
            File_ओळ* स्थिरांक flp = activep->fileline();
            bodyp->addStmtsp(
                नवीन AstAssign{flp, नवीन AstVarRef{flp, activep, VAccess::WRITE},
                              नवीन AstAdd{flp, नवीन AstVarRef{flp, activep, VAccess::READ},
                                         नवीन AstConst{flp, 1}}});
            bodyp->addStmtsp(loopp);

            // Validate assertion condition for each active assert
            AstVar* स्थिरांक activeCountp = नवीन AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                    nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            activeCountp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

            AstAssign* स्थिरांक initActiveCountp
                = नवीन AstAssign{flp, नवीन AstVarRef{flp, activeCountp, VAccess::WRITE},
                                नवीन AstVarRef{flp, activep, VAccess::READ}};
            AstLoop* स्थिरांक finalLoopp = नवीन AstLoop{flp};
            finalLoopp->addStmtsp(
                नवीन AstLoopTest{flp, finalLoopp,
                                नवीन AstNeq{flp, नवीन AstVarRef{flp, activeCountp, VAccess::READ},
                                           नवीन AstConst{flp, 0}}});
            finalLoopp->addStmtsp(नवीन AstIf{flp, finalCondp, नवीन AstPExprClause{flp},
                                            नवीन AstPExprClause{flp, असत्य}});
            finalLoopp->addStmtsp(
                नवीन AstAssign{flp, नवीन AstVarRef{flp, activeCountp, VAccess::WRITE},
                              नवीन AstSub{flp, नवीन AstVarRef{flp, activeCountp, VAccess::READ},
                                         नवीन AstConst{flp, 1}}});

            // Final assertion block
            AstBegin* स्थिरांक finalp = नवीन AstBegin{flp, m_blockNames.get(""), nullptr, सत्य};
            finalp->addStmtsp(activeCountp);
            finalp->addStmtsp(initActiveCountp);
            finalp->addStmtsp(finalLoopp);

            AstPExpr* स्थिरांक pexprp = नवीन AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
            VL_RESTORER(m_pexprp);
            VL_RESTORER(m_hasCycleDelay);
            m_pexprp = pexprp;
            m_hasCycleDelay = असत्य;
            iterate(bodyp);
            iterate(finalp);

            UASSERT_OBJ(!m_hasCycleDelay, nodep,
                        "s_until cycle delay should have been handled by V3AssertNfa");

            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            परत;
        }
        AstLoop* स्थिरांक loopp = नवीन AstLoop{flp};
        Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = nodep->lhsp()->unlinkFrBack();
        AstLogAnd* स्थिरांक loopCondp = नवीन AstLogAnd{flp, lhsp, नवीन AstLogNot{flp, rhsp}};
        loopp->addStmtsp(नवीन AstLoopTest{flp, loopp, loopCondp});
        loopp->addStmtsp(नवीन AstEventControl{flp, new_Sen_वृक्ष(nodep), nullptr});

        Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsCopyp = rhsp->clone_वृक्ष_Pure(असत्य);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक passCondp
            = nodep->isOverlapping() ? नवीन AstLogAnd{flp, lhsp->clone_वृक्ष_Pure(असत्य), rhsCopyp}
                                     : rhsCopyp;
        AstBegin* स्थिरांक beginp = नवीन AstBegin{flp, m_blockNames.get(""), loopp, सत्य};
        beginp->addStmtsp(
            नवीन AstIf{flp, passCondp, नवीन AstPExprClause{flp}, नवीन AstPExprClause{flp, असत्य}});

        AstPExpr* स्थिरांक pexprp = नवीन AstPExpr{flp, beginp, nodep->dtypep()};
        pexprp->user1(1);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }

    शून्य visit(AstDefaultDisable* nodep) override {
        VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
    }
    शून्य visit(AstInferredDisable* nodep) override {
        Ast_गाठ* newp;
        जर (m_defaultDisablep) {
            newp = m_defaultDisablep->condp()->clone_वृक्ष_Pure(सत्य);
        } अन्यथा {
            newp = नवीन AstConst{nodep->fileline(), AstConst::BitFalse{}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }

    शून्य visit(AstPropSpec* nodep) override {
        nodep = substitutePropertyCall(nodep);
        iterateAndNextNull(nodep->sensesp());
        जर (m_senip && m_senip != nodep->sensesp())
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Only one PSL clock allowed per assertion");
        स्थिरांक AstCover* स्थिरांक coverp = VN_CAST(nodep->backp(), Cover);
        स्थिरांक तार्किक seqEvent = coverp && coverp->isSeqEvent();
        जर (!nodep->disablep() && m_defaultDisablep && !seqEvent) {
            nodep->disablep(m_defaultDisablep->condp()->clone_वृक्ष_Pure(सत्य));
        }
        m_disablep = nodep->disablep();
        // Unlink and just keep a pointer to it, convert to sentree as needed
        m_senip = nodep->sensesp();
        iterateNull(nodep->disablep());
        जर (!VN_AS(nodep->backp(), गाठ_Cover_Or_Assert)->immediate()) {
            स्थिरांक Ast_गाठ_DType* स्थिरांक propDtp = nodep->propp()->dtypep();
            nodep->propp(नवीन AstSampled{nodep->fileline(), nodep->propp()->unlinkFrBack(),
                                        propDtp->dtypep(), सत्य});
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
        जर (m_pexprp && m_pexprp->user1()) {  // LCOV_EXCL_START
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Complex property expression inside 'until''");
            nodep->replaceWith(नवीन AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            परत;
        }  // LCOV_EXCL_STOP
        जर (AstLogNot* स्थिरांक notp = VN_CAST(nodep->backp(), LogNot)) {
            notp->replaceWith(nodep->unlinkFrBack());
            VL_DO_DANGLING(ढकला_Deletep(notp), notp);
            iterate(nodep);
            परत;
        }
        // Sequence expression as antecedent of implication is not yet supported
        जर (AstImplication* स्थिरांक implp = VN_CAST(nodep->backp(), Implication)) {
            जर (implp->lhsp() == nodep) {  // LCOV_EXCL_START
                implp->v3warn(E_UNSUPPORTED,
                              "Unsupported: Implication with sequence expression as antecedent");
                nodep->replaceWith(नवीन AstConst{nodep->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                परत;
            }  // LCOV_EXCL_STOP
        }
        VL_RESTORER(m_pexprp);
        VL_RESTORER(m_disableSeqIfp);
        m_pexprp = nodep;

        जर (m_disablep) {
            स्थिरांक AstSampled* sampledp = nullptr;
            जर (m_disablep->exists([&sampledp](स्थिरांक AstSampled* स्थिरांक sp) {
                    sampledp = sp;
                    परत सत्य;
                })) {
                sampledp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: $sampled inside disabled condition of a sequence");
                m_disablep = नवीन AstConst{m_disablep->fileline(), AstConst::BitFalse{}};
                // always a copy is used, so remove it now
                ढकला_Deletep(m_disablep);  // m_disablep used later
            }
            File_ओळ* स्थिरांक flp = nodep->fileline();
            // Add counter which counts times the condition turned true
            AstVar* स्थिरांक disableCntp
                = नवीन AstVar{flp, VVarType::MODULETEMP, m_disableCntNames.get(""),
                             nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            disableCntp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(disableCntp);
            AstVarRef* स्थिरांक readCntRefp = नवीन AstVarRef{flp, disableCntp, VAccess::READ};
            AstVarRef* स्थिरांक writeCntRefp = नवीन AstVarRef{flp, disableCntp, VAccess::WRITE};
            AstAssign* स्थिरांक incrStmtp = नवीन AstAssign{
                flp, writeCntRefp, नवीन AstAdd{flp, readCntRefp, नवीन AstConst{flp, 1}}};
            AstAlways* स्थिरांक alwaysp
                = नवीन AstAlways{flp, VAlwaysKwd::ALWAYS,
                                नवीन Ast_Sen_वृक्ष{flp, नवीन AstSenItem{flp, VEdgeType::ET_POSEDGE,
                                                                   m_disablep->clone_वृक्ष(असत्य)}},
                                incrStmtp};
            disableCntp->addNextHere(alwaysp);

            // Store value of that counter at the beginning of sequence evaluation
            AstBegin* स्थिरांक bodyp = nodep->bodyp();
            AstVar* स्थिरांक initialCntp = नवीन AstVar{flp, VVarType::BLOCKTEMP, "__VinitialCnt",
                                                   nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            initialCntp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            AstAssign* स्थिरांक assignp
                = नवीन AstAssign{flp, नवीन AstVarRef{flp, initialCntp, VAccess::WRITE},
                                readCntRefp->clone_वृक्ष(असत्य)};
            // Prepend to the sequence body to keep statement list structure valid.
            UASSERT_OBJ(!bodyp->name().empty(), bodyp, "Sequence body should be named");
            Ast_गाठ* स्थिरांक newStmtsp = Ast_गाठ::addNext<Ast_गाठ, Ast_गाठ>(initialCntp, assignp);
            bodyp->prependStmtsp(newStmtsp);

            m_disableSeqIfp
                = नवीन AstIf{flp, नवीन AstEq{flp, नवीन AstVarRef{flp, initialCntp, VAccess::READ},
                                           readCntRefp->clone_वृक्ष(असत्य)}};
            // Delete it, because it is always copied before insetion to the AST
            ढकला_Deletep(m_disableSeqIfp);  // m_disableSeqIfp used later
        }
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाठ_Module* nodep) override {
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultClkEvtVarp);
        VL_RESTORER(m_defaultDisablep);
        VL_RESTORER(m_modp);
        जर (!VN_IS(nodep, Class)) {
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
        VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
    }
    शून्य visit(AstSequence* nodep) override {
        // Sequence declarations are not visited directly; their bodies are inlined
        // at call sites by visit(AstFuncRef*). Collect for post-traversal cleanup.
        m_seqsToCleanup.ढकला_back(nodep);
    }
    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit AssertPreVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        { AssertPreClockingVisitor{nodep}; }
        // Process
        iterate(nodep);
        // Fix up varref names
        साठी (AstVarXRef* xrefp : m_xrefsp) xrefp->name(xrefp->varp()->name());
        // Clean up sequence declarations after inlining.
        // Referenced sequences that were inlined have isReferenced cleared.
        // Remaining referenced sequences are in unsupported contexts (e.g. @seq event).
        साठी (AstSequence* seqp : m_seqsToCleanup) {
            जर (seqp->isReferenced()) {
                seqp->v3warn(E_UNSUPPORTED,
                             "Unsupported: sequence referenced outside assertion property");
            } अन्यथा {
                VL_DO_DANGLING(seqp->unlinkFrBack()->delete_वृक्ष(), seqp);
            }
        }
    }
    ~AssertPreVisitor() override = पूर्वनिर्धारित;
};

//######################################################################
// Top Assert class

शून्य V3AssertPre::assertPreAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { AssertPreVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("assertpre", 0, dump_वृक्ष_Either_Level() >= 3);
}
