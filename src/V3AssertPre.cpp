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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3AssertPre.h"

#அந்தர்பாவ "V3Assert.h"
#அந்தர்பாவ "V3Const.h"
#அந்தர்பாவ "V3Task.h"
#அந்தர்பாவ "V3Timing.h"
#அந்தர்பாவ "V3UniqueNames.h"

#அந்தர்பாவ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clocking blocks of clocking items

வகுப்பு AssertPreClockingVisitor final : பொது VNVisitorConst {
    // Captures the clocking block of each clocking item before AssertPreVisitor, which needs it
    // at synchronous drives, often visited before the clocking block, e.g. in a parent module
    // NODE STATE
    // AstClockingItem::user3p()  // AstClocking*. Clocking block of the item, user3 claimed by
    //                            //               AssertPreVisitor

    // VISITORS
    வெறுமை visit(AstNetlist* nodep) override { iterateAndNextConstNull(nodep->modulesp()); }
    வெறுமை visit(Ast_முனை_Module* nodep) override { iterateAndNextConstNull(nodep->stmtsp()); }
    வெறுமை visit(AstGenBlock* nodep) override { iterateAndNextConstNull(nodep->itemsp()); }
    வெறுமை visit(AstClocking* nodep) override {
        க்கு (Ast_முனை* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            எனில் (AstClockingItem* மாறிலி citemp = VN_CAST(itemp, ClockingItem)) {
                citemp->user3p(nodep);
            }
        }
    }
    // Clocking blocks are only in modules and their generate blocks
    வெறுமை visit(Ast_முனை*) override {}

பொது:
    // CONSTRUCTORS
    explicit AssertPreClockingVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~AssertPreClockingVisitor() override = இயல்பு;
};

//######################################################################
// Assert class functions

வகுப்பு AssertPreVisitor final : பொது VNVisitor {
    // Removes clocks and other pre-optimizations
    // Eventually inlines calls to sequences, properties, etc.
    // We're not parsing the tree, or anything more complicated.
தனிப்பட்ட:
    // TYPES
    கட்டமைப்பு SynchDrive final {
        AstClockingItem* itemp;  // Clocking item of the driven clockvar
        Ast_முனை_கோவை* refp;  // Reference to the driven clockvar
    };

    // NODE STATE
    // AstClockingItem::user1p()         // AstVar*.      varp() of ClockingItem after unlink
    // AstClockingItem::user2p()         // AstVar*.      Flag set by drives of output clockvar
    // AstClockingItem::user3p()         // AstClocking*. Clocking block of the item, captured
    //                                   //               by AssertPreClockingVisitor
    // AstPExpr::user1()                 // bool.         Created from AstUntil
    மாறிலி VNUser1InUse m_inuser1;
    மாறிலி VNUser2InUse m_inuser2;
    மாறிலி VNUser3InUse m_inuser3;
    // STATE
    // Current context:
    AstNetlist* மாறிலி m_netlistp = nullptr;  // Current netlist
    Ast_முனை_Module* m_modp = nullptr;  // Current module
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
    Ast_முனை_கோவை* m_disablep = nullptr;  // Last disable
    AstIf* m_disableSeqIfp = nullptr;  // Used for handling disable iff in sequences
    AstPExpr* m_pexprp = nullptr;  // Last AstPExpr
    தருக்க m_underCover = பொய்;  // True if the enclosing assertion is a cover
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
    தருக்க m_inAssign = பொய்;  // True if in an AssignNode
    தருக்க m_inAssignDlyLhs = பொய்;  // True if in AssignDly's LHS
    தருக்க m_inSynchDrive = பொய்;  // True if in synchronous drive
    தருக்க m_inConcatAssign = பொய்;  // True if in assignment to a concatenation
    std::vector<SynchDrive> m_drives;  // Clockvars written by the synchronous drive
    தருக்க m_hasCycleDelay = பொய்;  // True if node has cycle delay beneath
    std::vector<AstVarXRef*> m_xrefsp;  // list of xrefs that need name fixup
    std::vector<AstSequence*> m_seqsToCleanup;  // Sequences to clean up after traversal

    // METHODS

    // Flag set by each drive of an output clockvar, added to the module with its clocking item
    AstVar* getCreateDrivenVarp(AstClockingItem* itemp) {
        எனில் (!itemp->user2p()) {
            AstVar* மாறிலி varp = புதிய AstVar{itemp->fileline(), VVarType::MODULETEMP,
                                            m_drivenNames.get(itemp), itemp->findBitDType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            itemp->user2p(varp);
        }
        திரும்பு VN_AS(itemp->user2p(), Var);
    }
    // Reference to a variable in the scope of a driven clockvar, made like the clockvar's
    நிலையான Ast_முனை_கோவை* newDriveRefp(File_வரி* flp, மாறிலி SynchDrive& drive, AstVar* varp,
                                     மாறிலி VAccess& access) {
        எனில் (மாறிலி AstVarXRef* மாறிலி xrefp = VN_CAST(drive.refp, VarXRef)) {
            திரும்பு புதிய AstVarXRef{flp, varp, xrefp->dotted(), access};
        } இல்லையெனில் எனில் (மாறிலி AstMemberSel* மாறிலி selp = VN_CAST(drive.refp, MemberSel)) {
            // The interface expression is evaluated again, so it must not have side effects;
            // cloneTreePure warns if it has any
            AstMemberSel* மாறிலி newSelp
                = புதிய AstMemberSel{flp, selp->fromp()->clone_மரம்_Pure(பொய்), varp};
            newSelp->access(access);
            திரும்பு newSelp;
        }
        திரும்பு புதிய AstVarRef{flp, varp, access};
    }
    // Assignment setting the drive flag of a driven clockvar
    AstAssign* newDrivenSetp(File_வரி* flp, மாறிலி SynchDrive& drive) {
        AstVar* மாறிலி varp = getCreateDrivenVarp(drive.itemp);
        AstAssign* மாறிலி setp = புதிய AstAssign{flp, newDriveRefp(flp, drive, varp, VAccess::WRITE),
                                              புதிய AstConst{flp, AstConst::BitTrue{}}};
        setp->user1(உண்மை);
        திரும்பு setp;
    }
    // Clocking block of a clocking item
    நிலையான AstClocking* clockingOf(மாறிலி AstClockingItem* itemp) {
        UASSERT_OBJ(itemp->user3p(), itemp, "Clocking item not captured in a clocking block");
        திரும்பு VN_AS(itemp->user3p(), Clocking);
    }
    // Clocking event of a driven clockvar, referenced from the drive like the clockvar, or
    // nullptr if unsupported, which is reported on the drive's cycle delay
    Ast_Sen_மரம்* newDriveSentreep(AstDelay* delayp, மாறிலி SynchDrive& drive) {
        Ast_Sen_மரம்* மாறிலி origp = clockingOf(drive.itemp)->sentreep();
        எனில் (VN_IS(drive.refp, VarRef)) திரும்பு origp->clone_மரம்(பொய்);
        எனில் (origp->exists([](மாறிலி AstVarXRef*) { திரும்பு உண்மை; })) {
            // The reference is relative to the clocking block
            delayp->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in synchronous drive to"
                                          " clockvar in another scope, whose clocking event"
                                          " has a hierarchical reference");
            திரும்பு nullptr;
        }
        மாறிலி AstMemberSel* மாறிலி selp = VN_CAST(drive.refp, MemberSel);
        // Processes outside classes evaluate their events also when not waiting on them,
        // possibly before the interface reference is set, so the event is guarded against a
        // null reference, which needs an integral value; scheduling does not guard it
        மாறிலி தருக்க guardNull = selp && !VN_IS(m_modp, Class);
        எனில் (guardNull) {
            க்கு (AstSenItem* itemp = origp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                எனில் (!itemp->sensp()->dtypep()->skipRefp()->isIntegralOrPacked()) {
                    delayp->v3warn(E_UNSUPPORTED,
                                   "Unsupported: cycle delay in synchronous drive through"
                                   " an interface reference outside a class, to clocking"
                                   " block with non-integral clocking event");
                    திரும்பு nullptr;
                }
            }
        }
        // Interface declaring the clocking block, if driven through an interface reference
        AstIface* மாறிலி ifacep
            = selp ? VN_AS(selp->fromp()->dtypep()->skipRefp(), IfaceRefDType)->ifaceViaCellp()
                   : nullptr;
        Ast_Sen_மரம்* மாறிலி sentreep = origp->clone_மரம்(பொய்);
        std::vector<AstVarRef*> refps;
        sentreep->foreach([&](AstVarRef* refp) {
            எனில் (!refp->classOrPackagep()) refps.தள்ளு_back(refp);
        });
        க்கு (AstVarRef* மாறிலி refp : refps) {
            AstVar* மாறிலி varp = refp->varp();
            எனில் (ifacep) {
                // Mark the variable as read through an interface reference, as V3Width does for
                // a member select. The mark is the interface declaring the variable, whose
                // instances all share it, so drives through references to any instances agree.
                UASSERT_OBJ(!varp->sensIfacep() || varp->sensIfacep() == ifacep, refp,
                            "Variable read through references to different interfaces");
                varp->sensIfacep(ifacep);
            }
            refp->replaceWith(newDriveRefp(refp->fileline(), drive, varp, refp->access()));
            VL_DO_DANGLING(தள்ளு_Deletep(refp), refp);
        }
        எனில் (guardNull) {
            க்கு (AstSenItem* itemp = sentreep->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                guardNullIface(itemp, selp->fromp());
            }
        }
        திரும்பு sentreep;
    }
    // Make an integral clocking event through an interface reference false while the reference
    // is null
    நிலையான வெறுமை guardNullIface(AstSenItem* sensesp, Ast_முனை_கோவை* fromp) {
        File_வரி* மாறிலி flp = sensesp->fileline();
        Ast_முனை_கோவை* மாறிலி sensp = sensesp->sensp()->unlinkFrBack();
        sensesp->sensp(புதிய AstCond{flp, newNotNullp(flp, fromp), sensp,
                                   புதிய AstConst{flp, AstConst::DTyped{}, sensp->dtypep()}});
        எனில் (Ast_முனை_கோவை* மாறிலி condp = sensesp->condp()) {
            condp->unlinkFrBack();
            sensesp->condp(புதிய AstLogAnd{flp, newNotNullp(flp, fromp), condp});
        }
    }
    நிலையான Ast_முனை_கோவை* newNotNullp(File_வரி* flp, Ast_முனை_கோவை* fromp) {
        திரும்பு புதிய AstNeq{flp, fromp->clone_மரம்_Pure(பொய்), புதிய AstConst{flp, AstConst::Null{}}};
    }
    // Procedural '##0': if the default clocking event has not yet occurred in this time step,
    // wait for it; otherwise continue without suspension (IEEE 1800-2023 14.11)
    AstIf* newCycleDelayZerop(File_வரி* flp) மாறிலி {
        AstVar* மாறிலி evtVarp = m_defaultClkEvtVarp;
        UASSERT_OBJ(evtVarp, flp, "Default clocking event var not pre-created");
        AstCMethodHard* மாறிலி isTriggeredp = புதிய AstCMethodHard{
            flp, புதிய AstVarRef{flp, evtVarp, VAccess::READ}, VCMethod::EVENT_IS_TRIGGERED};
        isTriggeredp->dtypeSetBit();
        AstEventControl* மாறிலி waitp = புதிய AstEventControl{
            flp,
            புதிய Ast_Sen_மரம்{flp, புதிய AstSenItem{flp, VEdgeType::ET_EVENT,
                                               புதிய AstVarRef{flp, evtVarp, VAccess::READ}}},
            nullptr};
        திரும்பு புதிய AstIf{flp, புதிய AstNot{flp, isTriggeredp}, waitp};
    }

    நிலையான வெறுமை checkSamplingFuncDType(Ast_முனை_கோவை* nodep, மாறிலி Ast_முனை* exprp) {
        மாறிலி Ast_முனை_DType* மாறிலி dtypep = exprp->dtypep()->skipRefp();
        எனில் (!dtypep->isIntegralOrPacked()) {
            nodep->v3error("Expected numeric type, but got a " << dtypep->prettyDTypeNameQ()
                                                               << " data type");
        }
    }

    Ast_Sen_மரம்* new_Sen_மரம்(Ast_முனை* nodep, Ast_Sen_மரம்* useTreep = nullptr,
                           Ast_முனை_Cover_Or_Assert* cassertp = nullptr) {
        // Create sentree based on clocked or default clock
        // Return nullptr for always
        எனில் (useTreep) திரும்பு useTreep;
        Ast_Sen_மரம்* newp = nullptr;
        AstSenItem* senip = m_senip;
        தருக்க fromAlways = பொய்;
        எனில் (!senip && m_defaultClockingp) senip = m_defaultClockingp->sentreep()->sensesp();
        எனில் (!senip) {
            senip = m_seniAlwaysp;
            fromAlways = உண்மை;
        }
        எனில் (!senip) {
            nodep->v3error("Concurrent assertion has no clock (IEEE 1800-2023 16.16)\n"
                           << nodep->warnMore()
                           << "... Suggest provide a clocking event, a default"
                              " clocking, or a clocked procedural context");
            newp = புதிய Ast_Sen_மரம்{nodep->fileline(), nullptr};
        } இல்லையெனில் {
            எனில் (cassertp && fromAlways) cassertp->senFromAlways(உண்மை);
            newp = புதிய Ast_Sen_மரம்{nodep->fileline(), senip->clone_மரம்(உண்மை)};
        }
        திரும்பு newp;
    }
    Ast_முனை_கோவை* getSequenceBodyExprp(மாறிலி AstSequence* மாறிலி seqp) மாறிலி {
        // The statements in AstSequence are optional AstVar (ports) followed by body expr.
        Ast_முனை* bodyp = seqp->stmtsp();
        போது (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        திரும்பு VN_CAST(bodyp, முனை_கோவை);
    }
    AstPropSpec* getPropertyExprp(மாறிலி AstProperty* மாறிலி propp) {
        // Statements in AstProperty: AstVar (ports/local vars),
        // AstInitialStaticStmt/AstInitialAutomaticStmt (var init), AstPropSpec (body).
        Ast_முனை* propExprp = propp->stmtsp();
        போது (propExprp
               && (VN_IS(propExprp, Var) || VN_IS(propExprp, InitialStaticStmt)
                   || VN_IS(propExprp, InitialAutomaticStmt))) {
            propExprp = propExprp->nextp();
        }
        திரும்பு VN_CAST(propExprp, PropSpec);
    }
    வெறுமை substituteSequenceCall(AstFuncRef* funcrefp, AstSequence* seqp) {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Inline the sequence body at the call site, replacing the FuncRef
        Ast_முனை_கோவை* bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        // Clone the body expression since the sequence may be referenced multiple times
        Ast_முனை_கோவை* clonedp = bodyExprp->clone_மரம்(பொய்);
        // Build substitution map, then do a single traversal to replace all formals
        // (textual substitution per IEEE 16.8.2).
        மாறிலி V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<மாறிலி AstVar*, Ast_முனை_கோவை*> portMap;
        க்கு (மாறிலி auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            மாறிலி auto it = portMap.find(refp->varp());
            எனில் (it != portMap.end()) {
                refp->replaceWith(it->second->clone_மரம்(பொய்));
                VL_DO_DANGLING(தள்ளு_Deletep(refp), refp);
            }
        });
        க்கு (மாறிலி auto& tconnect : tconnects) {
            தள்ளு_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        // Replace the FuncRef with the inlined body
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(தள்ளு_Deletep(funcrefp), funcrefp);
        // Clear referenced flag; sequences with isReferenced==false are deleted in assertPreAll
        seqp->isReferenced(பொய்);
    }
    AstPropSpec* clonePropertyBody(AstFuncRef* funcrefp, மாறிலி AstProperty* propp) {
        AstPropSpec* propExprp = getPropertyExprp(propp);
        // Substitute inner property call before copying in order to not doing the same for
        // each call of outer property call.
        propExprp = substitutePropertyCall(propExprp);
        // Clone subtree after substitution. It is needed, because property might be called
        // multiple times with different arguments.
        propExprp = propExprp->clone_மரம்(பொய்);
        // Build substitution maps for formal arguments and property-local
        // variables, then perform a single foreach to apply all replacements.
        // Map port vars to their actual argument expressions
        மாறிலி V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propp->stmtsp());
        std::unordered_map<மாறிலி AstVar*, Ast_முனை_கோவை*> portMap;
        க்கு (மாறிலி auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }

        // Promote property-local variables (non-port vars, IEEE 16.10) to
        // module-level __Vpropvar temps. Cross-cycle persistence is handled
        // by the match item lowering in visit(AstImplication*).
        std::unordered_map<மாறிலி AstVar*, AstVar*> localVarMap;
        க்கு (Ast_முனை* stmtp = propp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            எனில் (AstVar* மாறிலி varp = VN_CAST(stmtp, Var)) {
                எனில் (!varp->isIO()) {
                    மாறிலி string newName = m_propVarNames.get(varp);
                    AstVar* மாறிலி newVarp = புதிய AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->addStmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        // Single traversal: substitute ports and update local var references
        propExprp->foreach([&](AstVarRef* refp) {
            மாறிலி auto portIt = portMap.find(refp->varp());
            எனில் (portIt != portMap.end()) {
                refp->replaceWith(portIt->second->clone_மரம்(பொய்));
                VL_DO_DANGLING(தள்ளு_Deletep(refp), refp);
                திரும்பு;
            }
            மாறிலி auto localIt = localVarMap.find(refp->varp());
            எனில் (localIt != localVarMap.end()) refp->varp(localIt->second);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        // Clean up argument expressions
        க்கு (மாறிலி auto& tconnect : tconnects) {
            தள்ளு_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(propExprp, funcrefp);
        திரும்பு propExprp;
    }
    AstPropSpec* substitutePropertyCall(AstPropSpec* nodep) {
        எனில் (AstFuncRef* மாறிலி funcrefp = VN_CAST(nodep->propp(), FuncRef)) {
            எனில் (மாறிலி AstProperty* மாறிலி propp = VN_CAST(funcrefp->taskp(), Property)) {
                AstPropSpec* மாறிலி propExprp = clonePropertyBody(funcrefp, propp);

                // Handle case with 2 disable iff statement (IEEE 1800-2023 16.12.1)
                எனில் (nodep->disablep() && propExprp->disablep()) {
                    nodep->v3error("disable iff expression before property call and in its "
                                   "body is not legal");
                    தள்ளு_Deletep(propExprp->disablep()->unlinkFrBack());
                }
                // If disable iff is in outer property, move it to inner
                எனில் (nodep->disablep()) {
                    Ast_முனை_கோவை* மாறிலி disablep = nodep->disablep()->unlinkFrBack();
                    propExprp->disablep(disablep);
                }

                எனில் (nodep->sensesp() && propExprp->sensesp()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Clock event before property call and in its body");
                    தள்ளு_Deletep(propExprp->sensesp()->unlinkFrBack());
                }
                // If clock event is in outer property, move it to inner
                எனில் (nodep->sensesp()) {
                    AstSenItem* மாறிலி sensesp = nodep->sensesp();
                    sensesp->unlinkFrBack();
                    propExprp->sensesp(sensesp);
                }

                // Now substitute property reference with property body
                nodep->replaceWith(propExprp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                திரும்பு propExprp;
            }
        }
        திரும்பு nodep;
    }

    வெறுமை substituteNestedPropertyCall(AstFuncRef* funcrefp, மாறிலி AstProperty* propp) {
        AstPropSpec* மாறிலி specp = clonePropertyBody(funcrefp, propp);
        Ast_முனை* replp = nullptr;
        எனில் (specp->disablep()) {
            funcrefp->v3error("Illegal 'disable iff' in a nested property instance"
                              " (IEEE 1800-2023 16.12.1)");
        } இல்லையெனில் எனில் (specp->sensesp()) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: clock event in a nested property instance");
        } இல்லையெனில் எனில் (specp->propp()->exists(
                       [](மாறிலி Ast_முனை_கோவை* np) { திரும்பு np->isMultiCycleSva(); })) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: multi-cycle body in a nested property instance");
        } இல்லையெனில் {
            replp = specp->propp()->unlinkFrBack();
        }
        எனில் (!replp) replp = புதிய AstConst{funcrefp->fileline(), AstConst::BitFalse{}};
        funcrefp->replaceWith(replp);
        VL_DO_DANGLING(தள்ளு_Deletep(funcrefp), funcrefp);
        VL_DO_DANGLING(தள்ளு_Deletep(specp), specp);
    }

    // VISITORS
    //========== Statements
    வெறுமை visit(AstClocking* மாறிலி nodep) override {
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        UINFO(8, "   CLOCKING" << nodep);
        எனில் (nodep->isDefault()) {
            எனில் (m_defaultClockingp) {
                nodep->v3error("Only one default clocking block allowed per module"
                               " (IEEE 1800-2023 14.12)");
            } இல்லையெனில் {
                m_defaultClockingp = nodep;
                m_defaultClkEvtVarp = nodep->ensureEventp();
            }
        }
        iterateChildren(nodep);
        எனில் (nodep->eventp()) nodep->addNextHere(nodep->eventp()->unlinkFrBack());
        VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
    }
    வெறுமை visit(AstModportClockingRef* மாறிலி nodep) override {
        // It has to be converted to a list of ModportClockingVarRefs,
        // because clocking blocks are removed in this pass
        க்கு (Ast_முனை* itemp = nodep->clockingp()->itemsp(); itemp; itemp = itemp->nextp()) {
            எனில் (AstClockingItem* மாறிலி citemp = VN_CAST(itemp, ClockingItem)) {
                எனில் (AstVar* மாறிலி varp
                    = citemp->varp() ? citemp->varp() : VN_AS(citemp->user1p(), Var)) {
                    AstModportVarRef* மாறிலி modVarp = புதிய AstModportVarRef{
                        nodep->fileline(), varp->name(), citemp->direction()};
                    modVarp->varp(varp);
                    nodep->addNextHere(modVarp);
                    எனில் (citemp->direction().isOutput()) {
                        // Drives through the modport set the drive flag
                        AstVar* மாறிலி drivenVarp = getCreateDrivenVarp(citemp);
                        AstModportVarRef* மாறிலி drivenRefp = புதிய AstModportVarRef{
                            nodep->fileline(), drivenVarp->name(), VDirection::OUTPUT};
                        drivenRefp->varp(drivenVarp);
                        nodep->addNextHere(drivenRefp);
                    }
                }
            }
        }
        VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
    }
    வெறுமை visit(AstClockingItem* மாறிலி nodep) override {
        // Get a ref to the sampled/driven variable
        AstVar* மாறிலி varp = nodep->varp();
        எனில் (!varp) {
            // Unused item
            திரும்பு;
        }
        // Flag set by drives of an output clockvar, possibly visited before this item
        எனில் (nodep->direction().isOutput()) m_modp->addStmtsp(getCreateDrivenVarp(nodep));
        File_வரி* மாறிலி flp = nodep->fileline();
        V3Const::constifyEdit(nodep->skewp());
        எனில் (!VN_IS(nodep->skewp(), Const)) {
            nodep->skewp()->v3error("Skew must be constant (IEEE 1800-2023 14.4)");
            திரும்பு;
        }
        AstConst* மாறிலி skewp = VN_AS(nodep->skewp(), Const);
        எனில் (skewp->இலக்கம்().isNegative()) skewp->v3error("Skew cannot be negative");
        Ast_முனை_கோவை* மாறிலி exprp = nodep->exprp();
        varp->name(m_clockingp->name() + "__DOT__" + varp->name());
        m_clockingp->addNextHere(varp->unlinkFrBack());
        nodep->user1p(varp);
        varp->user1p(nodep);
        எனில் (nodep->direction() == VDirection::OUTPUT) {
            exprp->foreach([](மாறிலி Ast_முனை_Var_Ref* varrefp) {
                // Prevent confusing BLKANDNBLK warnings on clockvars due to generated assignments
                varrefp->fileline()->warnOff(V3ErrorCode::BLKANDNBLK, உண்மை);
                // Mark the driven signal so later passes (e.g. V3Inst port
                // connection) know the drive flows out from inside the cell.
                // Item expressions are plain signal references, so only the
                // driven signal is marked, never index selects.
                varrefp->varp()->isClockingDriven(உண்மை);
            });
            AstVarRef* மாறிலி skewedReadRefp = புதிய AstVarRef{flp, varp, VAccess::READ};
            skewedReadRefp->user1(உண்மை);
            // Initialize the clockvar
            AstVarRef* மாறிலி skewedWriteRefp = புதிய AstVarRef{flp, varp, VAccess::WRITE};
            skewedWriteRefp->user1(உண்மை);
            AstInitialStatic* மாறிலி initClockvarp = புதிய AstInitialStatic{
                flp, புதிய AstAssign{flp, skewedWriteRefp, exprp->clone_மரம்_Pure(பொய்)}};
            m_modp->addStmtsp(initClockvarp);
            // Each drive sets a flag, so the signal is also assigned when driven with the same
            // value again, e.g. after another assignment to the signal (IEEE 1800-2023 14.16.2)
            AstVar* மாறிலி drivenVarp = getCreateDrivenVarp(nodep);
            m_modp->addStmtsp(புதிய AstInitialStatic{
                flp, புதிய AstAssign{flp, புதிய AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                   புதிய AstConst{flp, AstConst::BitFalse{}}}});
            // Assign the clockvar to the actual var if it was driven
            AstAssign* மாறிலி assignp
                = புதிய AstAssign{flp, exprp->clone_மரம்_Pure(பொய்), skewedReadRefp};
            AstIf* மாறிலி ifp
                = புதிய AstIf{flp, புதிய AstVarRef{flp, drivenVarp, VAccess::READ},
                            புதிய AstAssign{flp, புதிய AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                          புதிய AstConst{flp, AstConst::BitFalse{}}}};
            ifp->addThensp(assignp);
            // Only in the time step of the clocking event
            AstCMethodHard* மாறிலி trigp = புதிய AstCMethodHard{
                nodep->fileline(), புதிய AstVarRef{flp, m_clockingp->ensureEventp(), VAccess::READ},
                VCMethod::EVENT_IS_TRIGGERED};
            trigp->dtypeSetBit();
            ifp->condp(புதிய AstLogAnd{flp, ifp->condp()->unlinkFrBack(), trigp});
            // A skew rounding to no time precision ticks is a zero skew
            Ast_முனை_கோவை* மாறிலி ticksp
                = V3Timing::delayToTicks(skewp->clone_மரம்(பொய்), m_modp->timeunit());
            மாறிலி தருக்க zeroSkew = ticksp->isZero();
            VL_DO_DANGLING(ticksp->delete_மரம்(), ticksp);
            எனில் (zeroSkew || !skewp->fileline()->timingOn()) {
                // Drive the var in Re-NBA (IEEE 1800-2023 14.16), ignoring skews where timing is
                // turned off
                Ast_Sen_மரம்* senTreep = m_clockingp->sentreep()->clone_மரம்(பொய்);
                senTreep->addSensesp(புதிய AstSenItem{
                    flp, VEdgeType::ET_CHANGED, புதிய AstVarRef{flp, drivenVarp, VAccess::READ}});
                m_clockingp->addNextHere(புதிய AstAlwaysReactive{flp, senTreep, ifp});
            } இல்லையெனில் {
                // Fork only the assignment, so this AlwaysPostponed can be retriggered
                // before the assignment happens
                AstFork* மாறிலி forkp = புதிய AstFork{flp, VJoinType::JOIN_NONE};
                forkp->immediateStart(உண்மை);
                forkp->addForksp(புதிய AstBegin{flp, "", assignp->unlinkFrBack(), உண்மை});
                ifp->addThensp(forkp);
                // Read the clockvar in Postponed, so a drive late in the time step is applied
                // at this event plus the skew (IEEE 1800-2023 14.16)
                m_clockingp->addNextHere(புதிய AstAlwaysPostponed{flp, ifp});
                எனில் (v3Global.opt.timing().isSetTrue()) {
                    AstDelay* மாறிலி delayp = புதிய AstDelay{flp, skewp->unlinkFrBack(), பொய்};
                    delayp->timeunit(m_modp->timeunit());
                    assignp->timingControlp(delayp);
                } இல்லையெனில் எனில் (v3Global.opt.timing().isSetFalse()) {
                    nodep->v3warn(E_NOTIMING,
                                  "Clocking output skew greater than #0 requires --timing");
                } இல்லையெனில் {
                    nodep->v3warn(E_NEEDTIMINGOPT,
                                  "Use --timing or --no-timing to specify how "
                                  "clocking output skew greater than #0 should be handled");
                }
            }
        } இல்லையெனில் எனில் (nodep->direction() == VDirection::உள்ளீடு) {
            // Ref to the clockvar
            AstVarRef* மாறிலி refp = புதிய AstVarRef{flp, varp, VAccess::WRITE};
            refp->user1(உண்மை);
            எனில் (skewp->இலக்கம்().is1Step()) {
                // #1step means the value that is sampled is always the signal's last value
                // before the clock edge (IEEE 1800-2023 14.4)
                AstSampled* மாறிலி sampledp
                    = புதிய AstSampled{flp, exprp->clone_மரம்_Pure(பொய்), exprp->dtypep(), உண்மை};
                AstAssign* மாறிலி assignp = புதிய AstAssign{flp, refp, sampledp};
                m_clockingp->addNextHere(புதிய AstAlways{
                    flp, VAlwaysKwd::ALWAYS, m_clockingp->sentreep()->clone_மரம்(பொய்), assignp});
            } இல்லையெனில் எனில் (skewp->isZero()) {
                // #0 means the var has to be sampled in Observed (IEEE 1800-2023 14.13)
                AstAssign* மாறிலி assignp = புதிய AstAssign{flp, refp, exprp->clone_மரம்_Pure(பொய்)};
                m_clockingp->addNextHere(புதிய AstAlwaysObserved{
                    flp, m_clockingp->sentreep()->clone_மரம்(பொய்), assignp});
            } இல்லையெனில் {
                // Create a queue where we'll store sampled values with timestamps
                AstSampleQueueDType* மாறிலி queueDtp
                    = புதிய AstSampleQueueDType{flp, exprp->dtypep()};
                m_netlistp->typeTablep()->addTypesp(queueDtp);
                AstVar* மாறிலி queueVarp
                    = புதிய AstVar{flp, VVarType::MODULETEMP, "__Vqueue__" + varp->name(), queueDtp};
                queueVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_clockingp->addNextHere(queueVarp);
                // Create a process like this:
                //     always queue.push(<sampled var>);
                AstCMethodHard* மாறிலி pushp = புதிய AstCMethodHard{
                    flp, புதிய AstVarRef{flp, queueVarp, VAccess::WRITE}, VCMethod::DYN_தள்ளு,
                    புதிய AstTime{nodep->fileline(), m_modp->timeunit()}};
                pushp->addPinsp(exprp->clone_மரம்_Pure(பொய்));
                pushp->dtypeSetVoid();
                m_clockingp->addNextHere(
                    புதிய AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, pushp->makeStmt()});
                // Create a process like this:
                //     always @<clocking event> queue.pop(<skew>, /*out*/<skewed var>);
                AstCMethodHard* மாறிலி popp = புதிய AstCMethodHard{
                    flp, புதிய AstVarRef{flp, queueVarp, VAccess::READWRITE}, VCMethod::DYN_எடு,
                    புதிய AstTime{nodep->fileline(), m_modp->timeunit()}};
                popp->addPinsp(skewp->unlinkFrBack());
                refp->access(VAccess::READWRITE);  // Only conditionally assigned
                popp->addPinsp(refp);
                popp->dtypeSetVoid();
                m_clockingp->addNextHere(புதிய AstAlways{flp, VAlwaysKwd::ALWAYS,
                                                       m_clockingp->sentreep()->clone_மரம்(பொய்),
                                                       popp->makeStmt()});
            }
        } இல்லையெனில் {
            nodep->v3fatalSrc("Invalid direction");
        }
    }
    வெறுமை visit(AstDelay* nodep) override {
        m_hasCycleDelay |= nodep->isCycleDelay();
        // Only cycle delays are relevant in this stage; also only process once
        எனில் (!nodep->isCycleDelay()) {
            எனில் (m_inSynchDrive) {
                nodep->v3error("Only cycle delays can be used in synchronous drives"
                               " (IEEE 1800-2023 14.16)");
            }
            iterateChildren(nodep);
            திரும்பு;
        }
        எனில் (m_inAssign && !m_inSynchDrive) {
            nodep->v3error("Cycle delays not allowed as intra-assignment delays"
                           " (IEEE 1800-2023 14.11)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
            திரும்பு;
        }
        எனில் (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBackWithNext());
        File_வரி* மாறிலி flp = nodep->fileline();
        Ast_முனை_கோவை* valuep = V3Const::constifyEdit(nodep->lhsp()->unlinkFrBack());
        மாறிலி AstConst* மாறிலி constp = VN_CAST(valuep, Const);
        எனில் (!constp && m_pexprp) {
            // V3AssertNfa handles non-const delays in sequences before this pass and
            // replaces the property; this branch should never be reached.
            nodep->v3fatalSrc("Non-constant cycle delay in assertion: "
                              "should have been caught by V3AssertNfa");
        } இல்லையெனில் எனில் (constp && constp->isZero()) {
            VL_DO_DANGLING(தள்ளு_Deletep(valuep), valuep);
            எனில் (m_inSynchDrive) {
                // ##0 has no effect in synchronous drives (IEEE 1800-2023 14.11)
                VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
                திரும்பு;
            }
            எனில் (m_pexprp) {
                // ##0 in sequence context = zero delay = same clock tick
                VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
                திரும்பு;
            }
            // Procedural ##0: synchronize with default clocking event (IEEE 1800-2023 14.11)
            எனில் (!m_defaultClockingp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
                திரும்பு;
            }
            nodep->replaceWith(newCycleDelayZerop(flp));
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு;
        }
        Ast_Sen_மரம்* sentreep = nullptr;
        எனில் (!m_drives.empty()) {
            எனில் (m_inConcatAssign) {
                // Also the clockvars may be of different clocking blocks
                nodep->v3error("Cycle delays cannot be used in synchronous drives to a"
                               " concatenation (IEEE 1800-2023 14.16)");
                VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(தள்ளு_Deletep(valuep), valuep);
                திரும்பு;
            }
            // Count the cycles of the driven clockvar's clocking block (IEEE 1800-2023 14.16)
            sentreep = newDriveSentreep(nodep, m_drives.front());
            எனில் (!sentreep) {
                VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(தள்ளு_Deletep(valuep), valuep);
                திரும்பு;
            }
        } இல்லையெனில் எனில் (!m_defaultClockingp) {
            எனில் (!m_pexprp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(தள்ளு_Deletep(valuep), valuep);
                திரும்பு;
            }
            sentreep = புதிய Ast_Sen_மரம்{flp, m_senip->clone_மரம்(பொய்)};
        } இல்லையெனில் {
            sentreep = m_defaultClockingp->sentreep()->clone_மரம்(பொய்);
        }
        AstEventControl* மாறிலி controlp
            = புதிய AstEventControl{nodep->fileline(), sentreep, nullptr};
        மாறிலி std::string delayName = m_cycleDlyNames.get(nodep);
        Ast_முனை_கோவை* throughoutp
            = nodep->throughoutp() ? nodep->throughoutp()->unlinkFrBack() : nullptr;

        AstVar* மாறிலி cntVarp = புதிய AstVar{flp, VVarType::BLOCKTEMP, delayName + "__counter",
                                           nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstBegin* மாறிலி beginp = புதிய AstBegin{flp, delayName + "__block", cntVarp, உண்மை};
        // A non-constant delay is counted at runtime. The count is integral of 64 bits, as
        // V3Width makes delays, or real, which is rounded (IEEE 1800-2023 6.12.1)
        எனில் (valuep->isDouble()) {
            valuep = புதிய AstRToIRoundS{flp, valuep};
            valuep->dtypeFrom(cntVarp);
        } இல்லையெனில் {
            valuep = புதிய AstSel{flp, valuep, 0, cntVarp->width()};
        }
        beginp->addStmtsp(புதிய AstAssign{flp, புதிய AstVarRef{flp, cntVarp, VAccess::WRITE}, valuep});
        // If the count is zero, a synchronous drive waits for none, like with a constant '##0',
        // and a procedural delay waits like a procedural '##0' (IEEE 1800-2023 14.11)
        எனில் (!constp && !m_inSynchDrive) {
            beginp->addStmtsp(புதிய AstIf{
                flp,
                புதிய AstEq{flp, புதிய AstVarRef{flp, cntVarp, VAccess::READ}, புதிய AstConst{flp, 0}},
                newCycleDelayZerop(flp)});
        }

        // Throughout: create flag tracking whether condition held every tick
        AstVar* throughoutOkp = nullptr;
        எனில் (throughoutp) {
            throughoutOkp = புதிய AstVar{flp, VVarType::BLOCKTEMP, delayName + "__throughoutOk",
                                       nodep->findBasicDType(VBasicDTypeKwd::LOGIC_IMPLICIT)};
            throughoutOkp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->addStmtsp(throughoutOkp);
            beginp->addStmtsp(புதிய AstAssign{flp, புதிய AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            புதிய AstConst{flp, AstConst::BitTrue{}}});
            // Check condition at tick 0 (sequence start, before entering loop)
            beginp->addStmtsp(
                புதிய AstIf{flp, புதிய AstLogNot{flp, throughoutp->clone_மரம்_Pure(பொய்)},
                          புதிய AstAssign{flp, புதிய AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                        புதிய AstConst{flp, AstConst::BitFalse{}}}});
        }

        {
            AstLoop* மாறிலி loopp = புதிய AstLoop{flp};
            // When throughout is present, exit loop early if condition fails
            Ast_முனை_கோவை* loopCondp
                = புதிய AstGt{flp, புதிய AstVarRef{flp, cntVarp, VAccess::READ}, புதிய AstConst{flp, 0}};
            எனில் (throughoutOkp) {
                loopCondp = புதிய AstLogAnd{flp, loopCondp,
                                          புதிய AstVarRef{flp, throughoutOkp, VAccess::READ}};
            }
            loopp->addStmtsp(புதிய AstLoopTest{flp, loopp, loopCondp});
            loopp->addStmtsp(controlp);
            loopp->addStmtsp(
                புதிய AstAssign{flp, புதிய AstVarRef{flp, cntVarp, VAccess::WRITE},
                              புதிய AstSub{flp, புதிய AstVarRef{flp, cntVarp, VAccess::READ},
                                         புதிய AstConst{flp, 1}}});
            // Check throughout condition at each tick during delay (IEEE 1800-2023 16.9.9)
            எனில் (throughoutp) {
                loopp->addStmtsp(
                    புதிய AstIf{flp, புதிய AstLogNot{flp, throughoutp},
                              புதிய AstAssign{flp, புதிய AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            புதிய AstConst{flp, AstConst::BitFalse{}}}});
            }
            beginp->addStmtsp(loopp);
        }
        // Compose wrappers on remaining sequence: throughout gate (inner), disable iff (outer)
        Ast_முனை* remainp = nodep->nextp() ? nodep->nextp()->unlinkFrBackWithNext() : nullptr;
        எனில் (throughoutOkp) {
            // If condition failed during delay, fail assertion
            remainp = புதிய AstIf{flp, புதிய AstVarRef{flp, throughoutOkp, VAccess::READ}, remainp,
                                புதிய AstPExprClause{flp, /*pass=*/பொய்}};
        }
        எனில் (m_disableSeqIfp && remainp) {
            AstIf* மாறிலி disableSeqIfp = m_disableSeqIfp->clone_மரம்(பொய்);
            disableSeqIfp->addThensp(remainp);
            remainp = disableSeqIfp;
        }
        எனில் (remainp) {
            எனில் (throughoutOkp) {
                // throughoutOkp is declared in beginp scope -- check must be inside it
                beginp->addStmtsp(remainp);
            } இல்லையெனில் {
                nodep->addNextHere(remainp);
            }
        }
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
    }
    வெறுமை visit(Ast_Sen_மரம்* nodep) override {
        எனில் (m_inSynchDrive) {
            nodep->v3error("Event controls cannot be used in "
                           "synchronous drives (IEEE 1800-2023 14.16)");
        }

        மாறிலி AstSampled* sampledp;
        எனில் (nodep->exists([&sampledp](மாறிலி AstSampled* மாறிலி sp) {
                sampledp = sp;
                திரும்பு உண்மை;
            })) {
            sampledp->v3warn(E_UNSUPPORTED, "Unsupported: $sampled inside sensitivity list");
        }
    }
    வெறுமை visit(Ast_முனை_Var_Ref* nodep) override {
        UINFO(8, " -varref:  " << nodep);
        UINFO(8, " -varref-var-back:  " << nodep->varp()->backp());
        UINFO(8, " -varref-var-user1:  " << nodep->varp()->user1p());
        எனில் (AstClockingItem* மாறிலி itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->firstAbovep(),
                ClockingItem)) {
            எனில் (nodep->user1()) திரும்பு;

            // ensure linking still works, this has to be done only once
            எனில் (AstVarXRef* xrefp = VN_CAST(nodep, VarXRef)) {
                UINFO(8, " -clockvarxref-in:  " << xrefp);
                string dotted = xrefp->dotted();
                மாறிலி size_t dotPos = dotted.rfind('.');
                dotted.erase(dotPos, string::npos);
                xrefp->dotted(dotted);
                UINFO(8, " -clockvarxref-out: " << xrefp);
                m_xrefsp.emplace_back(xrefp);
            }

            எனில் (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            எனில் (nodep->access().isWriteOrRW()) {
                எனில் (itemp->direction() == VDirection::OUTPUT) {
                    எனில் (!m_inAssignDlyLhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    எனில் (m_inAssign) {
                        m_inSynchDrive = உண்மை;
                        m_drives.தள்ளு_back({itemp, nodep});
                    }
                } இல்லையெனில் எனில் (itemp->direction() == VDirection::உள்ளீடு) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
            nodep->user1(உண்மை);
        }
    }
    வெறுமை visit(AstMemberSel* nodep) override {
        எனில் (AstClockingItem* மாறிலி itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->firstAbovep(),
                ClockingItem)) {
            எனில் (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            எனில் (nodep->access().isWriteOrRW()) {
                எனில் (itemp->direction() == VDirection::OUTPUT) {
                    எனில் (!m_inAssignDlyLhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    எனில் (m_inAssign) {
                        m_inSynchDrive = உண்மை;
                        m_drives.தள்ளு_back({itemp, nodep});
                    }
                } இல்லையெனில் எனில் (itemp->direction() == VDirection::உள்ளீடு) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
        }
    }
    வெறுமை visit(Ast_முனை_Assign* nodep) override {
        எனில் (nodep->user1()) திரும்பு;
        VL_RESTORER(m_inAssign);
        VL_RESTORER(m_inSynchDrive);
        VL_RESTORER(m_inConcatAssign);
        VL_RESTORER_CLEAR(m_drives);
        m_inAssign = உண்மை;
        m_inSynchDrive = பொய்;
        {
            // The braces of a concatenation are a replication, by one
            மாறிலி Ast_முனை_கோவை* lhsp = nodep->lhsp();
            எனில் (மாறிலி AstReplicate* மாறிலி repp = VN_CAST(lhsp, Replicate)) lhsp = repp->srcp();
            m_inConcatAssign = VN_IS(lhsp, Concat);
        }
        {
            VL_RESTORER(m_inAssignDlyLhs);
            m_inAssignDlyLhs = VN_IS(nodep, AssignDly);
            iterate(nodep->lhsp());
        }
        iterate(nodep->rhsp());
        எனில் (nodep->timingControlp()) iterate(nodep->timingControlp());
        // A drive without a cycle delay, also after removing '##0', updates the clockvar now
        எனில் (m_inSynchDrive && !nodep->timingControlp()) {
            AstAssign* மாறிலி assignp = புதிய AstAssign{
                nodep->fileline(), nodep->lhsp()->unlinkFrBack(), nodep->rhsp()->unlinkFrBack()};
            assignp->user1(உண்மை);
            nodep->replaceWith(assignp);
            VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
            nodep = assignp;
        }
        // Note the drives when they take effect, after the cycle delay if any. Each clockvar
        // of the LHS is driven, e.g. in a concatenation which other simulators accept, although
        // IEEE 1800-2023 14.16 does not allow it
        Ast_முனை* setsp = nullptr;
        க்கு (மாறிலி SynchDrive& drive : m_drives) {
            setsp = Ast_முனை::addNext(setsp, newDrivenSetp(nodep->fileline(), drive));
        }
        எனில் (!setsp) திரும்பு;
        எனில் (AstBegin* மாறிலி beginp = VN_CAST(nodep->timingControlp(), Begin)) {
            beginp->addStmtsp(setsp);
        } இல்லையெனில் {
            nodep->addNextHere(setsp);
        }
    }
    வெறுமை visit(AstAlways* nodep) override {
        iterateAndNextNull(nodep->sentreep());
        எனில் (nodep->sentreep()) m_seniAlwaysp = nodep->sentreep()->sensesp();
        iterateAndNextNull(nodep->stmtsp());
        m_seniAlwaysp = nullptr;
    }

    வெறுமை visit(Ast_முனை_Cover_Or_Assert* nodep) override {
        எனில் (nodep->sentreep()) திரும்பு;  // Already processed

        VL_RESTORER(m_senip);
        VL_RESTORER(m_disablep);
        m_senip = nullptr;
        m_disablep = nullptr;

        // Find Clocking's buried under nodep->exprsp
        iterateChildren(nodep);
        எனில் (!nodep->immediate()) nodep->sentreep(new_Sen_மரம்(nodep, nullptr, nodep));
    }
    வெறுமை visit(AstFalling* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_வரி* மாறிலி fl = nodep->fileline();
        Ast_முனை_கோவை* exprp = nodep->exprp()->unlinkFrBack();
        எனில் (exprp->width() > 1) exprp = புதிய AstSel{fl, exprp, 0, 1};
        Ast_முனை_கோவை* மாறிலி futurep = புதிய AstFuture{fl, exprp, new_Sen_மரம்(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = புதிய AstAnd{fl, exprp->clone_மரம்_Pure(பொய்), புதிய AstNot{fl, futurep}};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(AstFell* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_வரி* மாறிலி fl = nodep->fileline();
        Ast_முனை_கோவை* exprp = nodep->exprp()->unlinkFrBack();
        எனில் (exprp->width() > 1) exprp = புதிய AstSel{fl, exprp, 0, 1};
        Ast_Sen_மரம்* sentreep = nodep->sentreep();
        எனில் (sentreep) sentreep->unlinkFrBack();
        AstPast* மாறிலி pastp = புதிய AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_மரம்(nodep, sentreep));
        exprp = புதிய AstAnd{fl, pastp, புதிய AstNot{fl, exprp->clone_மரம்_Pure(பொய்)}};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(AstFuture* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        Ast_Sen_மரம்* மாறிலி sentreep = nodep->sentreep();
        எனில் (sentreep) VL_DO_DANGLING(தள்ளு_Deletep(sentreep->unlinkFrBack()), sentreep);
        nodep->sentreep(new_Sen_மரம்(nodep));
    }
    வெறுமை visit(AstPast* nodep) override {
        checkSamplingFuncDType(nodep, nodep->exprp());
        எனில் (nodep->sentreep()) திரும்பு;  // Already processed
        iterateChildren(nodep);
        nodep->sentreep(new_Sen_மரம்(nodep));
    }
    வெறுமை visit(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- Lower standalone exact [*N] (N >= 2) via saturating counter.
        // Range/unbounded forms and SExpr-contained forms are lowered by V3AssertNfa.
        iterateChildren(nodep);
        // V3AssertNfa handles unbounded/ranged forms upstream, so this fast-path
        // is effectively unreachable when NFA is enabled.
        எனில் (nodep->unbounded() || nodep->maxCountp()) திரும்பு;  // LCOV_EXCL_LINE
        மாறிலி AstConst* மாறிலி constp = VN_CAST(nodep->countp(), Const);
        எனில் (VL_UNLIKELY(!constp || constp->toSInt() < 1)) {
            nodep->v3fatalSrc("Consecutive repetition count must be a positive constant"
                              " (should have been caught by V3Width)");
            திரும்பு;
        }
        மாறிலி முழு n = constp->toSInt();
        File_வரி* மாறிலி flp = nodep->fileline();
        Ast_முனை_கோவை* மாறிலி exprp = nodep->exprp()->unlinkFrBack();
        எனில் (n == 1) {
            nodep->replaceWith(exprp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு;
        }
        // Saturating counter: if (expr) cnt <= min(cnt+1, N); else cnt <= 0;
        AstVar* மாறிலி cntVarp = புதிய AstVar{flp, VVarType::MODULETEMP, m_consRepNames.get(""),
                                           nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(cntVarp);
        Ast_முனை_கோவை* மாறிலி கோவை_Clonep = exprp->clone_மரம்_Pure(பொய்);
        Ast_முனை_கோவை* மாறிலி saturatingIncrp = புதிய AstCond{
            flp,
            புதிய AstLt{flp, புதிய AstVarRef{flp, cntVarp, VAccess::READ},
                      புதிய AstConst{flp, static_cast<uint32_t>(n)}},
            புதிய AstAdd{flp, புதிய AstVarRef{flp, cntVarp, VAccess::READ}, புதிய AstConst{flp, 1u}},
            புதிய AstConst{flp, static_cast<uint32_t>(n)}};
        AstAssignDly* மாறிலி incrAssignp
            = புதிய AstAssignDly{flp, புதிய AstVarRef{flp, cntVarp, VAccess::WRITE}, saturatingIncrp};
        AstAssignDly* மாறிலி resetAssignp = புதிய AstAssignDly{
            flp, புதிய AstVarRef{flp, cntVarp, VAccess::WRITE}, புதிய AstConst{flp, 0u}};
        AstIf* மாறிலி ifp = புதிய AstIf{flp, கோவை_Clonep, incrAssignp, resetAssignp};
        Ast_Sen_மரம்* மாறிலி senTreep = new_Sen_மரம்(nodep);
        AstAlways* மாறிலி alwaysp = புதிய AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, ifp};
        cntVarp->addNextHere(alwaysp);
        // Match: cnt >= N-1 (previous cycles via NBA) && expr (current cycle)
        Ast_முனை_கோவை* மாறிலி cntCheckp = புதிய AstGte{flp, புதிய AstVarRef{flp, cntVarp, VAccess::READ},
                                                  புதிய AstConst{flp, static_cast<uint32_t>(n - 1)}};
        cntCheckp->dtypeSetBit();
        Ast_முனை_கோவை* மாறிலி matchp = புதிய AstAnd{flp, cntCheckp, exprp};
        matchp->dtypeSetBit();
        nodep->replaceWith(matchp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(AstRising* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_வரி* மாறிலி fl = nodep->fileline();
        Ast_முனை_கோவை* exprp = nodep->exprp()->unlinkFrBack();
        எனில் (exprp->width() > 1) exprp = புதிய AstSel{fl, exprp, 0, 1};
        Ast_முனை_கோவை* மாறிலி futurep = புதிய AstFuture{fl, exprp, new_Sen_மரம்(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = புதிய AstAnd{fl, புதிய AstNot{fl, exprp->clone_மரம்_Pure(பொய்)}, futurep};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(AstRose* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_வரி* மாறிலி fl = nodep->fileline();
        Ast_முனை_கோவை* exprp = nodep->exprp()->unlinkFrBack();
        எனில் (exprp->width() > 1) exprp = புதிய AstSel{fl, exprp, 0, 1};
        Ast_Sen_மரம்* sentreep = nodep->sentreep();
        எனில் (sentreep) sentreep->unlinkFrBack();
        AstPast* மாறிலி pastp = புதிய AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_மரம்(nodep, sentreep));
        exprp = புதிய AstAnd{fl, புதிய AstNot{fl, pastp}, exprp->clone_மரம்_Pure(பொய்)};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    நிலையான AstAssocArrayDType* getProcessAssocArrayType(File_வரி* மாறிலி flp) {
        // Type of __VassertsActive___x[std::process]
        Ast_முனை_DType* valp
            = v3Global.rootp()->typeTablep()->findBasicDType(flp, VBasicDTypeKwd::BIT);
        AstClassRefDType* keyp
            = புதிய AstClassRefDType{flp, v3Global.rootp()->stdPackageProcessp(), nullptr};
        keyp->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        v3Global.rootp()->typeTablep()->addTypesp(keyp);
        AstAssocArrayDType* மாறிலி typep = புதிய AstAssocArrayDType{flp, valp, keyp};
        typep->dtypep(typep);
        v3Global.rootp()->typeTablep()->addTypesp(typep);
        திரும்பு typep;
    }
    நிலையான Ast_Stmt_கோவை* getProcessAssocArrayDelete(AstVarRef* மாறிலி refp) {
        // Constructs refp.delete(std::process::self()) statement
        File_வரி* மாறிலி flp = refp->fileline();
        AstCMethodHard* மாறிலி deletep = புதிய AstCMethodHard{
            flp, refp, VCMethod::ASSOC_ERASE, v3Global.rootp()->stdPackageProcessSelfp(flp)};
        deletep->dtypep(refp->findVoidDType());
        திரும்பு புதிய Ast_Stmt_கோவை{flp, deletep};
    }
    நிலையான Ast_முனை_கோவை* getProcessAssocArraySize(AstVarRef* மாறிலி refp) {
        // Constructs refp.size() statement
        AstCMethodHard* மாறிலி sizep
            = புதிய AstCMethodHard{refp->fileline(), refp, VCMethod::ASSOC_SIZE};
        sizep->dtypep(refp->findBasicDType(VBasicDTypeKwd::UINT32));
        திரும்பு sizep;
    }
    வெறுமை visit(AstSEventually* nodep) override {
        UASSERT(v3Global.rootp()->stdPackagep(), "Should be imported");

        Ast_Sen_மரம்* மாறிலி sentreep = new_Sen_மரம்(nodep);
        எனில் (!sentreep->sensesp()) {
            VL_DO_DANGLING(தள்ளு_Deletep(sentreep), sentreep);
            nodep->replaceWith(புதிய AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு;
        }

        File_வரி* மாறிலி flp = nodep->fileline();

        // Track active assertions
        AstVar* மாறிலி activep = புதிய AstVar{flp, VVarType::MODULETEMP, m_activeNames.get(""),
                                           getProcessAssocArrayType(flp)};
        activep->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(activep);

        // Assertion condition check
        AstLoop* மாறிலி loopp = புதிய AstLoop{flp};
        AstSampled* மாறிலி condp
            = புதிய AstSampled{flp, nodep->exprp()->unlinkFrBack(), nullptr, உண்மை};
        loopp->addStmtsp(புதிய AstLoopTest{flp, loopp, புதிய AstLogNot{flp, condp}});
        loopp->addStmtsp(புதிய AstEventControl{flp, sentreep, nullptr});

        // Add assertion to the active set
        AstAssocSel* மாறிலி selp = புதிய AstAssocSel{flp, புதிய AstVarRef{flp, activep, VAccess::WRITE},
                                                  v3Global.rootp()->stdPackageProcessSelfp(flp)};
        AstAssign* மாறிலி incrementp = புதிய AstAssign{flp, selp, புதிய AstConst{flp, 1}};
        AstPExprClause* மாறிலி clausep = புதிய AstPExprClause{flp};
        Ast_Stmt_கோவை* மாறிலி deletep
            = getProcessAssocArrayDelete(புதிய AstVarRef{flp, activep, VAccess::WRITE});

        // Main assertion block
        AstBegin* மாறிலி bodyp = புதிய AstBegin{flp, m_blockNames.get(""), nullptr, உண்மை};
        bodyp->addStmtsp(incrementp);
        bodyp->addStmtsp(loopp);
        bodyp->addStmtsp(clausep);
        bodyp->addStmtsp(deletep);

        // Validate assertion condition for each active assert
        AstVar* மாறிலி activeCountp = புதிய AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        activeCountp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstAssign* மாறிலி initActiveCountp
            = புதிய AstAssign{flp, புதிய AstVarRef{flp, activeCountp, VAccess::WRITE},
                            getProcessAssocArraySize(புதிய AstVarRef{flp, activep, VAccess::READ})};
        AstLoop* மாறிலி finalLoopp = புதிய AstLoop{flp};
        AstIf* மாறிலி finalBodypCondp
            = புதிய AstIf{flp, condp->clone_மரம்_Pure(பொய்), புதிய AstPExprClause{flp},
                        புதிய AstPExprClause{flp, பொய்}};
        finalLoopp->addStmtsp(
            புதிய AstLoopTest{flp, finalLoopp,
                            புதிய AstNeq{flp, புதிய AstVarRef{flp, activeCountp, VAccess::READ},
                                       புதிய AstConst{flp, 0}}});
        finalLoopp->addStmtsp(finalBodypCondp);
        finalLoopp->addStmtsp(
            புதிய AstAssign{flp, புதிய AstVarRef{flp, activeCountp, VAccess::WRITE},
                          புதிய AstSub{flp, புதிய AstVarRef{flp, activeCountp, VAccess::READ},
                                     புதிய AstConst{flp, 1}}});

        // Final assertion block
        AstBegin* மாறிலி finalp = புதிய AstBegin{flp, m_blockNames.get(""), nullptr, உண்மை};
        finalp->addStmtsp(activeCountp);
        finalp->addStmtsp(initActiveCountp);
        finalp->addStmtsp(finalLoopp);

        VL_RESTORER(m_pexprp);
        m_pexprp = புதிய AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
        VL_RESTORER(m_hasCycleDelay);
        m_hasCycleDelay = பொய்;
        iterate(bodyp);
        iterate(finalp);

        எனில் (m_hasCycleDelay) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in s_eventually");
            nodep->replaceWith(புதிய AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            VL_DO_DANGLING(m_pexprp->delete_மரம்(), m_pexprp);
            திரும்பு;
        }

        nodep->replaceWith(m_pexprp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(AstStable* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_வரி* மாறிலி fl = nodep->fileline();
        Ast_முனை_கோவை* exprp = nodep->exprp()->unlinkFrBack();
        Ast_Sen_மரம்* sentreep = nodep->sentreep();
        எனில் (sentreep) sentreep->unlinkFrBack();
        AstPast* மாறிலி pastp = புதிய AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_மரம்(nodep, sentreep));
        exprp = புதிய AstEq{fl, pastp, exprp->clone_மரம்_Pure(பொய்)};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(AstSteady* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_வரி* மாறிலி fl = nodep->fileline();
        Ast_முனை_கோவை* exprp = nodep->exprp()->unlinkFrBack();
        எனில் (exprp->width() > 1) exprp = புதிய AstSel{fl, exprp, 0, 1};
        Ast_முனை_கோவை* மாறிலி futurep = புதிய AstFuture{fl, exprp, new_Sen_மரம்(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = புதிய AstEq{fl, exprp->clone_மரம்_Pure(பொய்), futurep};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(AstSampled* nodep) override {
        iterateChildren(nodep);
        எனில் (!nodep->internal()) checkSamplingFuncDType(nodep, nodep->exprp());
    }

    // Validate repetition count: must be a non-negative elaboration-time constant.
    // Shared by goto [->N] and nonconsecutive [=N] repetition.
    // On error, replaces nodep with BitFalse placeholder and returns nullptr.
    மாறிலி AstConst* validate_Rep_மொத்தம்(Ast_முனை* nodep, Ast_முனை_கோவை*& countp) {
        countp = V3Const::constifyEdit(countp);
        மாறிலி AstConst* மாறிலி constp = VN_CAST(countp, Const);
        எனில் (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(தள்ளு_Deletep(countp), countp);
            nodep->replaceWith(புதிய AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு nullptr;
        }
        எனில் (constp->toSInt() < 0) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(தள்ளு_Deletep(countp), countp);
            nodep->replaceWith(புதிய AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு nullptr;
        }
        எனில் (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(தள்ளு_Deletep(countp), countp);
            nodep->replaceWith(புதிய AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு nullptr;
        }
        திரும்பு constp;
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
    AstPExpr* createRepPExpr(File_வரி* flp, Ast_முனை_கோவை* exprp, Ast_முனை_கோவை* countp,
                             Ast_முனை_கோவை* rhsp, தருக்க isOverlapped, தருக்க isNonConsec) {
        AstSenItem* மாறிலி sensesp = m_senip;
        UASSERT_OBJ(sensesp, exprp, "Repetition requires a clock");

        மாறிலி std::string name
            = isNonConsec ? m_nonConsRepNames.get(exprp) : m_gotoRepNames.get(exprp);
        AstVar* மாறிலி cntVarp = புதிய AstVar{flp, VVarType::BLOCKTEMP, name + "__counter",
                                           exprp->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstBegin* மாறிலி beginp = புதிய AstBegin{flp, name + "__block", cntVarp, உண்மை};
        beginp->addStmtsp(
            புதிய AstAssign{flp, புதிய AstVarRef{flp, cntVarp, VAccess::WRITE}, புதிய AstConst{flp, 0}});

        // Counting loop: find N occurrences of expr (shared by goto and nonconsec)
        AstLoop* மாறிலி loopp = புதிய AstLoop{flp};
        loopp->addStmtsp(புதிய AstLoopTest{flp, loopp,
                                         புதிய AstLt{flp, புதிய AstVarRef{flp, cntVarp, VAccess::READ},
                                                   countp->clone_மரம்_Pure(பொய்)}});
        // if (expr) cnt++ -- sampled is applied to whole property expr by V3Assert
        loopp->addStmtsp(
            புதிய AstIf{flp, exprp,
                      புதிய AstAssign{flp, புதிய AstVarRef{flp, cntVarp, VAccess::WRITE},
                                    புதிய AstAdd{flp, புதிய AstVarRef{flp, cntVarp, VAccess::READ},
                                               புதிய AstConst{flp, 1}}}});
        loopp->addStmtsp(புதிய AstIf{
            flp, புதிய AstLt{flp, புதிய AstVarRef{flp, cntVarp, VAccess::READ}, countp},
            புதிய AstEventControl{flp, புதிய Ast_Sen_மரம்{flp, sensesp->clone_மரம்(பொய்)}, nullptr}});

        beginp->addStmtsp(loopp);

        எனில் (isNonConsec && rhsp) {
            // IEEE 16.9.2: b[=N] = b[->N] ##1 !b[*0:$]
            // Trailing window: ##1 advance, then scan !expr cycles checking rhs.
            // Window closes when expr becomes true again (fail if rhs was never true).
            beginp->addStmtsp(புதிய AstEventControl{
                flp, புதிய Ast_Sen_மரம்{flp, sensesp->clone_மரம்(பொய்)}, nullptr});  // ##1
            எனில் (!isOverlapped) {
                beginp->addStmtsp(புதிய AstEventControl{
                    flp, புதிய Ast_Sen_மரம்{flp, sensesp->clone_மரம்(பொய்)}, nullptr});  // |=>
            }
            // Window loop: check rhs at each !expr cycle (done variable for termination)
            AstVar* மாறிலி doneVarp
                = புதிய AstVar{flp, VVarType::BLOCKTEMP, name + "__done", exprp->findBitDType()};
            doneVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->addStmtsp(doneVarp);
            beginp->addStmtsp(புதிய AstAssign{flp, புதிய AstVarRef{flp, doneVarp, VAccess::WRITE},
                                            புதிய AstConst{flp, AstConst::BitFalse{}}});
            auto setDone = [&]() {
                திரும்பு புதிய AstAssign{flp, புதிய AstVarRef{flp, doneVarp, VAccess::WRITE},
                                     புதிய AstConst{flp, AstConst::BitTrue{}}};
            };
            AstLoop* மாறிலி windowp = புதிய AstLoop{flp};
            // LoopTest: continue while !done
            windowp->addStmtsp(புதிய AstLoopTest{
                flp, windowp, புதிய AstNot{flp, புதிய AstVarRef{flp, doneVarp, VAccess::READ}}});
            // if (expr) { fail; done = 1; } -- window closed, expr true again
            Ast_முனை* மாறிலி failsp = புதிய AstPExprClause{flp, பொய்};
            failsp->addNext(setDone());
            windowp->addStmtsp(புதிய AstIf{flp, exprp->clone_மரம்_Pure(பொய்), failsp});
            // if (rhs) { pass; done = 1; } -- consequent true at this !expr endpoint
            Ast_முனை* மாறிலி passsp = புதிய AstPExprClause{flp, உண்மை};
            passsp->addNext(setDone());
            windowp->addStmtsp(புதிய AstIf{flp, rhsp, passsp});
            // @(clk) -- advance to next cycle in window
            windowp->addStmtsp(
                புதிய AstEventControl{flp, புதிய Ast_Sen_மரம்{flp, sensesp->clone_மரம்(பொய்)}, nullptr});
            beginp->addStmtsp(windowp);
        } இல்லையெனில் எனில் (isNonConsec) {
            // Standalone nonconsec: ##1 into window, then pass
            beginp->addStmtsp(
                புதிய AstEventControl{flp, புதிய Ast_Sen_மரம்{flp, sensesp->clone_மரம்(பொய்)}, nullptr});
            beginp->addStmtsp(புதிய AstPExprClause{flp, உண்மை});
        } இல்லையெனில் எனில் (rhsp) {
            // Goto rep: check consequent once at match endpoint
            எனில் (!isOverlapped) {
                beginp->addStmtsp(புதிய AstEventControl{
                    flp, புதிய Ast_Sen_மரம்{flp, sensesp->clone_மரம்(பொய்)}, nullptr});
            }
            beginp->addStmtsp(புதிய AstIf{flp, rhsp, புதிய AstPExprClause{flp, உண்மை},
                                        புதிய AstPExprClause{flp, பொய்}});
        } இல்லையெனில் {
            // Standalone goto: pass after counting
            beginp->addStmtsp(புதிய AstPExprClause{flp, உண்மை});
        }

        திரும்பு புதிய AstPExpr{flp, beginp, exprp->findBitDType()};
    }

    // Reject [=M:N] nonconsecutive range that reached V3AssertPre.
    // [->M:N] is fully handled upstream (NFA path); only [=M:N] still lands
    // here because AstSNonConsRep lowering is not yet implemented there.
    // Replaces the node with BitFalse so lowering continues after the warning.
    தருக்க rejectNonconsecRange(Ast_முனை* nodep, Ast_முனை_கோவை* maxCountp) {
        எனில் (!maxCountp) திரும்பு பொய்;
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: [=M:N] nonconsecutive range repetition"
                                     " (IEEE 1800-2023 16.9.2)");
        nodep->replaceWith(புதிய AstConst{nodep->fileline(), AstConst::BitFalse{}});
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        திரும்பு உண்மை;
    }

    வெறுமை visit(AstSGotoRep* nodep) override {
        // Standalone goto rep (not inside implication antecedent)
        iterateChildren(nodep);
        File_வரி* மாறிலி flp = nodep->fileline();
        Ast_முனை_கோவை* countp = nodep->countp()->unlinkFrBack();
        எனில் (!validate_Rep_மொத்தம்(nodep, countp)) திரும்பு;
        Ast_முனை_கோவை* மாறிலி exprp = nodep->exprp()->unlinkFrBack();
        AstPExpr* மாறிலி pexprp = createRepPExpr(flp, exprp, countp, nullptr, உண்மை, பொய்);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }

    வெறுமை visit(AstSNonConsRep* nodep) override {
        // Standalone nonconsecutive rep (not inside implication antecedent)
        iterateChildren(nodep);
        எனில் (rejectNonconsecRange(nodep, nodep->maxCountp())) திரும்பு;
        File_வரி* மாறிலி flp = nodep->fileline();
        Ast_முனை_கோவை* countp = nodep->countp()->unlinkFrBack();
        எனில் (!validate_Rep_மொத்தம்(nodep, countp)) திரும்பு;
        Ast_முனை_கோவை* மாறிலி exprp = nodep->exprp()->unlinkFrBack();
        AstPExpr* மாறிலி pexprp = createRepPExpr(flp, exprp, countp, nullptr, உண்மை, உண்மை);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }

    வெறுமை visit(AstFuncRef* nodep) override {
        // IEEE 1800-2023 16.8: Inline sequence instances wherever they appear
        // in the expression tree (inside implications, boolean ops, nested refs, etc.)
        எனில் (AstSequence* மாறிலி seqp = VN_CAST(nodep->taskp(), Sequence)) {
            substituteSequenceCall(nodep, seqp);
            // The FuncRef has been replaced; do not access nodep after this point.
            // The replacement node will be visited by the parent's iterateChildren.
            திரும்பு;
        }
        எனில் (மாறிலி AstProperty* மாறிலி propp = VN_CAST(nodep->taskp(), Property)) {
            substituteNestedPropertyCall(nodep, propp);
            திரும்பு;
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(AstImplication* nodep) override {
        எனில் (nodep->sentreep()) திரும்பு;  // Already processed

        // Top-level followed-by is claimed by V3AssertNfa; anything reaching here
        // is nested inside iff/implies/or. IEEE 1800-2023 16.12.9 permits the
        // composition, but silent lowering would drop non-vacuous-fail semantics.
        எனில் (nodep->isFollowedBy()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: followed-by (#-# / #=#) nested inside property operator"
                          " (iff/implies/or) (IEEE 1800-2023 16.12.9)");
            nodep->replaceWith(புதிய AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு;
        }

        // Handle goto repetition as antecedent before iterateChildren,
        // so the standalone AstSGotoRep visitor doesn't process it
        எனில் (AstSGotoRep* மாறிலி gotop = VN_CAST(nodep->lhsp(), SGotoRep)) {
            iterateChildren(gotop);
            iterateAndNextNull(nodep->rhsp());
            File_வரி* மாறிலி flp = nodep->fileline();
            Ast_முனை_கோவை* countp = gotop->countp()->unlinkFrBack();
            எனில் (!validate_Rep_மொத்தம்(nodep, countp)) திரும்பு;
            Ast_முனை_கோவை* மாறிலி exprp = gotop->exprp()->unlinkFrBack();
            Ast_முனை_கோவை* மாறிலி rhsp = nodep->rhsp()->unlinkFrBack();
            AstPExpr* மாறிலி pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), பொய்);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு;
        }

        // Handle nonconsecutive repetition as antecedent before iterateChildren,
        // so the standalone AstSNonConsRep visitor doesn't process it
        எனில் (AstSNonConsRep* மாறிலி ncrp = VN_CAST(nodep->lhsp(), SNonConsRep)) {
            iterateChildren(ncrp);
            iterateAndNextNull(nodep->rhsp());
            எனில் (rejectNonconsecRange(nodep, ncrp->maxCountp())) திரும்பு;
            File_வரி* மாறிலி flp = nodep->fileline();
            Ast_முனை_கோவை* countp = ncrp->countp()->unlinkFrBack();
            எனில் (!validate_Rep_மொத்தம்(nodep, countp)) திரும்பு;
            Ast_முனை_கோவை* மாறிலி exprp = ncrp->exprp()->unlinkFrBack();
            Ast_முனை_கோவை* மாறிலி rhsp = nodep->rhsp()->unlinkFrBack();
            AstPExpr* மாறிலி pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), உண்மை);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு;
        }

        iterateChildren(nodep);

        File_வரி* மாறிலி flp = nodep->fileline();
        Ast_முனை_கோவை* மாறிலி rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_முனை_கோவை* lhsp = nodep->lhsp()->unlinkFrBack();

        // Lower sequence match items (IEEE 16.11): (expr, var = val, ...) |-> / |=>
        எனில் (Ast_கோவை_Stmt* மாறிலி கோவை_Stmtp = VN_CAST(lhsp, கோவை_Stmt)) {
            Ast_முனை_கோவை* மாறிலி antExprp = கோவை_Stmtp->resultp()->unlinkFrBack();

            எனில் (nodep->isOverlapped()) {
                // |-> : assign to __Vpropvar via always_comb (continuous).
                // The assign evaluates RHS once; V3Sampled snapshots the
                // result so all consequent refs read the same value.
                Ast_முனை* matchAssignsp = nullptr;
                க்கு (Ast_முனை* stmtp = கோவை_Stmtp->stmtsp(); stmtp;) {
                    Ast_முனை* மாறிலி nextp = stmtp->nextp();
                    எனில் (AstAssign* மாறிலி assignp = VN_CAST(stmtp, Assign)) {
                        assignp->unlinkFrBack();
                        matchAssignsp = Ast_முனை::addNextNull(matchAssignsp, assignp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(தள்ளு_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                எனில் (matchAssignsp) {
                    AstAlways* மாறிலி alwaysp
                        = புதிய AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, matchAssignsp};
                    m_modp->addStmtsp(alwaysp);
                }
            } இல்லையெனில் {
                // |=> : assign to __Vpropvar via NBA in a clocked always block.
                // The NBA commits before the next cycle's sampled snapshot,
                // so the consequent (which already references __Vpropvar)
                // sees the captured value.
                Ast_முனை* matchAssignsp = nullptr;
                க்கு (Ast_முனை* stmtp = கோவை_Stmtp->stmtsp(); stmtp;) {
                    Ast_முனை* மாறிலி nextp = stmtp->nextp();
                    எனில் (AstAssign* மாறிலி assignp = VN_CAST(stmtp, Assign)) {
                        assignp->unlinkFrBack();
                        Ast_முனை_கோவை* மாறிலி assignLhsp = assignp->lhsp()->unlinkFrBack();
                        Ast_முனை_கோவை* மாறிலி assignRhsp = assignp->rhsp()->unlinkFrBack();
                        AstAssignDly* மாறிலி dlyp = புதிய AstAssignDly{flp, assignLhsp, assignRhsp};
                        VL_DO_DANGLING(தள்ளு_Deletep(assignp), assignp);
                        matchAssignsp = Ast_முனை::addNextNull(matchAssignsp, dlyp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(தள்ளு_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                எனில் (matchAssignsp) {
                    AstIf* மாறிலி condp
                        = புதிய AstIf{flp, antExprp->clone_மரம்_Pure(பொய்), matchAssignsp};
                    AstAlways* மாறிலி alwaysp
                        = புதிய AstAlways{flp, VAlwaysKwd::ALWAYS, new_Sen_மரம்(nodep), condp};
                    m_modp->addStmtsp(alwaysp);
                }
            }
        }

        எனில் (AstPExpr* மாறிலி pexprp = VN_CAST(rhsp, PExpr)) {
            // Implication with sequence expression on RHS (IEEE 1800-2023 16.11, 16.12.7).
            // The PExpr was lowered from the property expression earlier in this pass.
            // Wrap the PExpr body with the antecedent check so the sequence only
            // starts when the antecedent holds.
            Ast_முனை_கோவை* condp;
            எனில் (nodep->isOverlapped()) {
                // Overlapped implication (|->): check antecedent on same cycle.
                // disable iff is applied at the assertion level, not at the
                // antecedent gate, matching the existing non-PExpr overlapped path.
                condp = lhsp;
            } இல்லையெனில் {
                // Non-overlapped implication (|=>): check antecedent from previous cycle
                எனில் (m_disablep) {
                    lhsp
                        = புதிய AstAnd{flp, புதிய AstNot{flp, m_disablep->clone_மரம்_Pure(பொய்)}, lhsp};
                }
                AstPast* மாறிலி pastp
                    = புதிய AstPast{flp, lhsp, nullptr, nullptr, /* propertyTiming */ உண்மை};
                pastp->dtypeFrom(lhsp);
                pastp->sentreep(new_Sen_மரம்(nodep));
                condp = pastp;
            }
            // Wrap existing PExpr body: if (antecedent) { <original body> } else { /* vacuous pass
            // */ }
            // Only the statements are guarded; declarations stay directly in the block. After
            // V3Fork moves the block's statements into a task, V3Task only handles variables
            // that are direct statements of the task.
            AstBegin* மாறிலி bodyp = pexprp->bodyp();
            AstIf* மாறிலி guardp = புதிய AstIf{flp, condp};
            க்கு (Ast_முனை* stmtp = bodyp->stmtsp(); stmtp;) {
                Ast_முனை* மாறிலி nextp = stmtp->nextp();
                எனில் (!VN_IS(stmtp, Var)) guardp->addThensp(stmtp->unlinkFrBack());
                stmtp = nextp;
            }
            bodyp->addStmtsp(guardp);
            nodep->replaceWith(pexprp);
            // Don't iterate pexprp here -- it was already iterated when created
            // (in visit(AstSExpr*)), so delays and disable iff are already processed.
        } இல்லையெனில் எனில் (nodep->isOverlapped()) {
            Ast_முனை_கோவை* மாறிலி exprp
                = m_underCover ? static_cast<Ast_முனை_கோவை*>(புதிய AstLogAnd{flp, lhsp, rhsp})
                               : புதிய AstLogOr{flp, புதிய AstLogNot{flp, lhsp}, rhsp};
            nodep->replaceWith(exprp);
        } இல்லையெனில் {
            எனில் (m_disablep) {
                lhsp = புதிய AstAnd{flp, புதிய AstNot{flp, m_disablep->clone_மரம்_Pure(பொய்)}, lhsp};
            }

            AstPast* மாறிலி pastp
                = புதிய AstPast{flp, lhsp, nullptr, nullptr, /* propertyTiming */ உண்மை};
            pastp->dtypeFrom(lhsp);
            pastp->sentreep(new_Sen_மரம்(nodep));
            Ast_முனை_கோவை* மாறிலி exprp
                = m_underCover ? static_cast<Ast_முனை_கோவை*>(புதிய AstAnd{flp, pastp, rhsp})
                               : புதிய AstOr{flp, புதிய AstNot{flp, pastp}, rhsp};
            exprp->dtypeSetBit();
            nodep->replaceWith(exprp);
        }
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(AstUntil* nodep) override {
        File_வரி* மாறிலி flp = nodep->fileline();
        UASSERT_OBJ(
            !m_pexprp, nodep,
            "'" << nodep->verilogKwd()
                << "' in complex property expression should have been rejected by V3AssertNfa");
        எனில் (nodep->isStrong()
            && (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue())) {
            nodep->v3warn(E_NOTIMING, nodep->verilogKwd() << " requires --timing");
            nodep->replaceWith(புதிய AstConst{flp, AstConst::BitFalse{}});
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு;
        }
        எனில் (nodep->isStrong()) {
            // p s_until q / p s_until_with q: q must eventually be true. Until then, p must
            // be true on every sampled tick. For s_until, check q first: when q is true on
            // this tick, p is not required. For s_until_with, p must be true on the q tick too.
            Ast_முனை_கோவை* மாறிலி rawLhsp = nodep->lhsp()->unlinkFrBack();
            Ast_முனை_கோவை* மாறிலி rawRhsp = nodep->rhsp()->unlinkFrBack();
            AstSampled* மாறிலி lhsp = புதிய AstSampled{flp, rawLhsp, rawLhsp->dtypep(), உண்மை};
            AstSampled* மாறிலி rhsp = புதிய AstSampled{flp, rawRhsp, rawRhsp->dtypep(), உண்மை};
            Ast_முனை_கோவை* finalCondp = rhsp->clone_மரம்_Pure(பொய்);
            எனில் (nodep->isOverlapping()) {
                finalCondp = புதிய AstLogAnd{flp, lhsp->clone_மரம்_Pure(பொய்), finalCondp};
            }

            // Track active assertion attempts. Only the count is needed to emit final failures.
            AstVar* மாறிலி activep = புதிய AstVar{flp, VVarType::MODULETEMP, m_activeNames.get(""),
                                               nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            activep->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(activep);

            AstVar* மாறிலி donep
                = புதிய AstVar{flp, VVarType::BLOCKTEMP, "__VassertDone", nodep->findBitDType()};
            donep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            auto setDone = [&]() {
                திரும்பு புதிய AstAssign{flp, புதிய AstVarRef{flp, donep, VAccess::WRITE},
                                     புதிய AstConst{flp, AstConst::BitTrue{}}};
            };

            AstLoop* மாறிலி loopp = புதிய AstLoop{flp};
            AstAssign* மாறிலி decrementVar = புதிய AstAssign{
                flp, புதிய AstVarRef{flp, activep, VAccess::WRITE},
                புதிய AstSub{flp, புதிய AstVarRef{flp, activep, VAccess::READ}, புதிய AstConst{flp, 1}}};
            loopp->addStmtsp(புதிய AstLoopTest{
                flp, loopp, புதிய AstLogNot{flp, புதிய AstVarRef{flp, donep, VAccess::READ}}});
            {
                Ast_முனை* மாறிலி passp = புதிய AstPExprClause{flp};
                passp->addNext(decrementVar);
                passp->addNext(setDone());
                Ast_முனை_கோவை* passCondp = rhsp;
                எனில் (nodep->isOverlapping()) {
                    passCondp = புதிய AstLogAnd{flp, lhsp->clone_மரம்_Pure(பொய்), passCondp};
                }
                loopp->addStmtsp(புதிய AstIf{flp, passCondp, passp});
            }
            {
                Ast_முனை* மாறிலி failp = புதிய AstPExprClause{flp, பொய்};
                failp->addNext(decrementVar->clone_மரம்(பொய்));
                failp->addNext(setDone());
                loopp->addStmtsp(புதிய AstIf{
                    flp,
                    புதிய AstLogAnd{flp,
                                  புதிய AstLogNot{flp, புதிய AstVarRef{flp, donep, VAccess::READ}},
                                  புதிய AstLogNot{flp, lhsp}},
                    failp});
            }
            AstDelay* மாறிலி delayp = புதிய AstDelay{flp, புதிய AstConst{flp, 1}, பொய்};
            delayp->timeunit(m_modp->timeunit());
            loopp->addStmtsp(புதிய AstIf{
                flp, புதிய AstLogNot{flp, புதிய AstVarRef{flp, donep, VAccess::READ}}, delayp});

            // Main assertion block
            AstBegin* மாறிலி bodyp = புதிய AstBegin{flp, m_blockNames.get(""), nullptr, உண்மை};
            bodyp->addStmtsp(donep);
            bodyp->addStmtsp(புதிய AstAssign{flp, புதிய AstVarRef{flp, donep, VAccess::WRITE},
                                           புதிய AstConst{flp, AstConst::BitFalse{}}});
            File_வரி* மாறிலி flp = activep->fileline();
            bodyp->addStmtsp(
                புதிய AstAssign{flp, புதிய AstVarRef{flp, activep, VAccess::WRITE},
                              புதிய AstAdd{flp, புதிய AstVarRef{flp, activep, VAccess::READ},
                                         புதிய AstConst{flp, 1}}});
            bodyp->addStmtsp(loopp);

            // Validate assertion condition for each active assert
            AstVar* மாறிலி activeCountp = புதிய AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                    nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            activeCountp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

            AstAssign* மாறிலி initActiveCountp
                = புதிய AstAssign{flp, புதிய AstVarRef{flp, activeCountp, VAccess::WRITE},
                                புதிய AstVarRef{flp, activep, VAccess::READ}};
            AstLoop* மாறிலி finalLoopp = புதிய AstLoop{flp};
            finalLoopp->addStmtsp(
                புதிய AstLoopTest{flp, finalLoopp,
                                புதிய AstNeq{flp, புதிய AstVarRef{flp, activeCountp, VAccess::READ},
                                           புதிய AstConst{flp, 0}}});
            finalLoopp->addStmtsp(புதிய AstIf{flp, finalCondp, புதிய AstPExprClause{flp},
                                            புதிய AstPExprClause{flp, பொய்}});
            finalLoopp->addStmtsp(
                புதிய AstAssign{flp, புதிய AstVarRef{flp, activeCountp, VAccess::WRITE},
                              புதிய AstSub{flp, புதிய AstVarRef{flp, activeCountp, VAccess::READ},
                                         புதிய AstConst{flp, 1}}});

            // Final assertion block
            AstBegin* மாறிலி finalp = புதிய AstBegin{flp, m_blockNames.get(""), nullptr, உண்மை};
            finalp->addStmtsp(activeCountp);
            finalp->addStmtsp(initActiveCountp);
            finalp->addStmtsp(finalLoopp);

            AstPExpr* மாறிலி pexprp = புதிய AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
            VL_RESTORER(m_pexprp);
            VL_RESTORER(m_hasCycleDelay);
            m_pexprp = pexprp;
            m_hasCycleDelay = பொய்;
            iterate(bodyp);
            iterate(finalp);

            UASSERT_OBJ(!m_hasCycleDelay, nodep,
                        "s_until cycle delay should have been handled by V3AssertNfa");

            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு;
        }
        AstLoop* மாறிலி loopp = புதிய AstLoop{flp};
        Ast_முனை_கோவை* மாறிலி rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி lhsp = nodep->lhsp()->unlinkFrBack();
        AstLogAnd* மாறிலி loopCondp = புதிய AstLogAnd{flp, lhsp, புதிய AstLogNot{flp, rhsp}};
        loopp->addStmtsp(புதிய AstLoopTest{flp, loopp, loopCondp});
        loopp->addStmtsp(புதிய AstEventControl{flp, new_Sen_மரம்(nodep), nullptr});

        Ast_முனை_கோவை* மாறிலி rhsCopyp = rhsp->clone_மரம்_Pure(பொய்);
        Ast_முனை_கோவை* மாறிலி passCondp
            = nodep->isOverlapping() ? புதிய AstLogAnd{flp, lhsp->clone_மரம்_Pure(பொய்), rhsCopyp}
                                     : rhsCopyp;
        AstBegin* மாறிலி beginp = புதிய AstBegin{flp, m_blockNames.get(""), loopp, உண்மை};
        beginp->addStmtsp(
            புதிய AstIf{flp, passCondp, புதிய AstPExprClause{flp}, புதிய AstPExprClause{flp, பொய்}});

        AstPExpr* மாறிலி pexprp = புதிய AstPExpr{flp, beginp, nodep->dtypep()};
        pexprp->user1(1);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }

    வெறுமை visit(AstDefaultDisable* nodep) override {
        VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
    }
    வெறுமை visit(AstInferredDisable* nodep) override {
        Ast_முனை* newp;
        எனில் (m_defaultDisablep) {
            newp = m_defaultDisablep->condp()->clone_மரம்_Pure(உண்மை);
        } இல்லையெனில் {
            newp = புதிய AstConst{nodep->fileline(), AstConst::BitFalse{}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }

    வெறுமை visit(AstPropSpec* nodep) override {
        nodep = substitutePropertyCall(nodep);
        iterateAndNextNull(nodep->sensesp());
        எனில் (m_senip && m_senip != nodep->sensesp())
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Only one PSL clock allowed per assertion");
        மாறிலி AstCover* மாறிலி coverp = VN_CAST(nodep->backp(), Cover);
        மாறிலி தருக்க seqEvent = coverp && coverp->isSeqEvent();
        எனில் (!nodep->disablep() && m_defaultDisablep && !seqEvent) {
            nodep->disablep(m_defaultDisablep->condp()->clone_மரம்_Pure(உண்மை));
        }
        m_disablep = nodep->disablep();
        // Unlink and just keep a pointer to it, convert to sentree as needed
        m_senip = nodep->sensesp();
        iterateNull(nodep->disablep());
        எனில் (!VN_AS(nodep->backp(), முனை_Cover_Or_Assert)->immediate()) {
            மாறிலி Ast_முனை_DType* மாறிலி propDtp = nodep->propp()->dtypep();
            nodep->propp(புதிய AstSampled{nodep->fileline(), nodep->propp()->unlinkFrBack(),
                                        propDtp->dtypep(), உண்மை});
        }
        // cover counts non-vacuous matches only (IEEE 1800-2023 16.15.2), so an
        // implication antecedent must hold; assert passes vacuously instead.
        VL_RESTORER(m_underCover);
        m_underCover = VN_IS(nodep->backp(), Cover);
        iterate(nodep->propp());
        iterateNull(nodep->matchCountp());
    }
    வெறுமை visit(AstPExpr* nodep) override {
        // V3AssertNfa handles multi-cycle property expressions before this pass,
        // so the following unsupported paths are defensive and typically unreached.
        எனில் (m_pexprp && m_pexprp->user1()) {  // LCOV_EXCL_START
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Complex property expression inside 'until''");
            nodep->replaceWith(புதிய AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு;
        }  // LCOV_EXCL_STOP
        எனில் (AstLogNot* மாறிலி notp = VN_CAST(nodep->backp(), LogNot)) {
            notp->replaceWith(nodep->unlinkFrBack());
            VL_DO_DANGLING(தள்ளு_Deletep(notp), notp);
            iterate(nodep);
            திரும்பு;
        }
        // Sequence expression as antecedent of implication is not yet supported
        எனில் (AstImplication* மாறிலி implp = VN_CAST(nodep->backp(), Implication)) {
            எனில் (implp->lhsp() == nodep) {  // LCOV_EXCL_START
                implp->v3warn(E_UNSUPPORTED,
                              "Unsupported: Implication with sequence expression as antecedent");
                nodep->replaceWith(புதிய AstConst{nodep->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                திரும்பு;
            }  // LCOV_EXCL_STOP
        }
        VL_RESTORER(m_pexprp);
        VL_RESTORER(m_disableSeqIfp);
        m_pexprp = nodep;

        எனில் (m_disablep) {
            மாறிலி AstSampled* sampledp = nullptr;
            எனில் (m_disablep->exists([&sampledp](மாறிலி AstSampled* மாறிலி sp) {
                    sampledp = sp;
                    திரும்பு உண்மை;
                })) {
                sampledp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: $sampled inside disabled condition of a sequence");
                m_disablep = புதிய AstConst{m_disablep->fileline(), AstConst::BitFalse{}};
                // always a copy is used, so remove it now
                தள்ளு_Deletep(m_disablep);  // m_disablep used later
            }
            File_வரி* மாறிலி flp = nodep->fileline();
            // Add counter which counts times the condition turned true
            AstVar* மாறிலி disableCntp
                = புதிய AstVar{flp, VVarType::MODULETEMP, m_disableCntNames.get(""),
                             nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            disableCntp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(disableCntp);
            AstVarRef* மாறிலி readCntRefp = புதிய AstVarRef{flp, disableCntp, VAccess::READ};
            AstVarRef* மாறிலி writeCntRefp = புதிய AstVarRef{flp, disableCntp, VAccess::WRITE};
            AstAssign* மாறிலி incrStmtp = புதிய AstAssign{
                flp, writeCntRefp, புதிய AstAdd{flp, readCntRefp, புதிய AstConst{flp, 1}}};
            AstAlways* மாறிலி alwaysp
                = புதிய AstAlways{flp, VAlwaysKwd::ALWAYS,
                                புதிய Ast_Sen_மரம்{flp, புதிய AstSenItem{flp, VEdgeType::ET_POSEDGE,
                                                                   m_disablep->clone_மரம்(பொய்)}},
                                incrStmtp};
            disableCntp->addNextHere(alwaysp);

            // Store value of that counter at the beginning of sequence evaluation
            AstBegin* மாறிலி bodyp = nodep->bodyp();
            AstVar* மாறிலி initialCntp = புதிய AstVar{flp, VVarType::BLOCKTEMP, "__VinitialCnt",
                                                   nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            initialCntp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            AstAssign* மாறிலி assignp
                = புதிய AstAssign{flp, புதிய AstVarRef{flp, initialCntp, VAccess::WRITE},
                                readCntRefp->clone_மரம்(பொய்)};
            // Prepend to the sequence body to keep statement list structure valid.
            UASSERT_OBJ(!bodyp->name().empty(), bodyp, "Sequence body should be named");
            Ast_முனை* மாறிலி newStmtsp = Ast_முனை::addNext<Ast_முனை, Ast_முனை>(initialCntp, assignp);
            bodyp->prependStmtsp(newStmtsp);

            m_disableSeqIfp
                = புதிய AstIf{flp, புதிய AstEq{flp, புதிய AstVarRef{flp, initialCntp, VAccess::READ},
                                           readCntRefp->clone_மரம்(பொய்)}};
            // Delete it, because it is always copied before insetion to the AST
            தள்ளு_Deletep(m_disableSeqIfp);  // m_disableSeqIfp used later
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultClkEvtVarp);
        VL_RESTORER(m_defaultDisablep);
        VL_RESTORER(m_modp);
        எனில் (!VN_IS(nodep, Class)) {
            // A class nested in a module is in the scope of the module's default clocking
            m_defaultClockingp = nullptr;
            m_defaultClkEvtVarp = nullptr;
        }
        m_defaultDisablep = nodep->defaultDisablep();
        m_modp = nodep;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    வெறுமை visit(AstProperty* nodep) override {
        // The body will be visited when will be substituted in place of property reference
        // (AstFuncRef)
        VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
    }
    வெறுமை visit(AstSequence* nodep) override {
        // Sequence declarations are not visited directly; their bodies are inlined
        // at call sites by visit(AstFuncRef*). Collect for post-traversal cleanup.
        m_seqsToCleanup.தள்ளு_back(nodep);
    }
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    explicit AssertPreVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        { AssertPreClockingVisitor{nodep}; }
        // Process
        iterate(nodep);
        // Fix up varref names
        க்கு (AstVarXRef* xrefp : m_xrefsp) xrefp->name(xrefp->varp()->name());
        // Clean up sequence declarations after inlining.
        // Referenced sequences that were inlined have isReferenced cleared.
        // Remaining referenced sequences are in unsupported contexts (e.g. @seq event).
        க்கு (AstSequence* seqp : m_seqsToCleanup) {
            எனில் (seqp->isReferenced()) {
                seqp->v3warn(E_UNSUPPORTED,
                             "Unsupported: sequence referenced outside assertion property");
            } இல்லையெனில் {
                VL_DO_DANGLING(seqp->unlinkFrBack()->delete_மரம்(), seqp);
            }
        }
    }
    ~AssertPreVisitor() override = இயல்பு;
};

//######################################################################
// Top Assert class

வெறுமை V3AssertPre::assertPreAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { AssertPreVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("assertpre", 0, dump_மரம்_Either_Level() >= 3);
}
