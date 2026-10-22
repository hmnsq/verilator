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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3AssertPre.h"

#شامل "V3Assert.h"
#شامل "V3Const.h"
#شامل "V3Task.h"
#شامل "V3Timing.h"
#شامل "V3UniqueNames.h"

#شامل <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clocking blocks of clocking items

رده AssertPreClockingVisitor final : عمومی VNVisitorConst {
    // Captures the clocking block of each clocking item before AssertPreVisitor, which needs it
    // at synchronous drives, often visited before the clocking block, e.g. in a parent module
    // NODE STATE
    // AstClockingItem::user3p()  // AstClocking*. Clocking block of the item, user3 claimed by
    //                            //               AssertPreVisitor

    // VISITORS
    تهی visit(AstNetlist* nodep) override { iterateAndNextConstNull(nodep->modulesp()); }
    تهی visit(Ast_گره_Module* nodep) override { iterateAndNextConstNull(nodep->stmtsp()); }
    تهی visit(AstGenBlock* nodep) override { iterateAndNextConstNull(nodep->itemsp()); }
    تهی visit(AstClocking* nodep) override {
        برای (Ast_گره* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            اگر (AstClockingItem* ثابت citemp = VN_CAST(itemp, ClockingItem)) {
                citemp->user3p(nodep);
            }
        }
    }
    // Clocking blocks are only in modules and their generate blocks
    تهی visit(Ast_گره*) override {}

عمومی:
    // CONSTRUCTORS
    explicit AssertPreClockingVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~AssertPreClockingVisitor() override = پیشفرض;
};

//######################################################################
// Assert class functions

رده AssertPreVisitor final : عمومی VNVisitor {
    // Removes clocks and other pre-optimizations
    // Eventually inlines calls to sequences, properties, etc.
    // We're not parsing the tree, or anything more complicated.
خصوصی:
    // TYPES
    ساختار SynchDrive final {
        AstClockingItem* itemp;  // Clocking item of the driven clockvar
        Ast_گره_عبارت* refp;  // Reference to the driven clockvar
    };

    // NODE STATE
    // AstClockingItem::user1p()         // AstVar*.      varp() of ClockingItem after unlink
    // AstClockingItem::user2p()         // AstVar*.      Flag set by drives of output clockvar
    // AstClockingItem::user3p()         // AstClocking*. Clocking block of the item, captured
    //                                   //               by AssertPreClockingVisitor
    // AstPExpr::user1()                 // bool.         Created from AstUntil
    ثابت VNUser1InUse m_inuser1;
    ثابت VNUser2InUse m_inuser2;
    ثابت VNUser3InUse m_inuser3;
    // STATE
    // Current context:
    AstNetlist* ثابت m_netlistp = nullptr;  // Current netlist
    Ast_گره_Module* m_modp = nullptr;  // Current module
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
    Ast_گره_عبارت* m_disablep = nullptr;  // Last disable
    AstIf* m_disableSeqIfp = nullptr;  // Used for handling disable iff in sequences
    AstPExpr* m_pexprp = nullptr;  // Last AstPExpr
    منطقی m_underCover = نادرست;  // True if the enclosing assertion is a cover
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
    منطقی m_inAssign = نادرست;  // True if in an AssignNode
    منطقی m_inAssignDlyLhs = نادرست;  // True if in AssignDly's LHS
    منطقی m_inSynchDrive = نادرست;  // True if in synchronous drive
    منطقی m_inConcatAssign = نادرست;  // True if in assignment to a concatenation
    std::vector<SynchDrive> m_drives;  // Clockvars written by the synchronous drive
    منطقی m_hasCycleDelay = نادرست;  // True if node has cycle delay beneath
    std::vector<AstVarXRef*> m_xrefsp;  // list of xrefs that need name fixup
    std::vector<AstSequence*> m_seqsToCleanup;  // Sequences to clean up after traversal

    // METHODS

    // Flag set by each drive of an output clockvar, added to the module with its clocking item
    AstVar* getCreateDrivenVarp(AstClockingItem* itemp) {
        اگر (!itemp->user2p()) {
            AstVar* ثابت varp = جدید AstVar{itemp->fileline(), VVarType::MODULETEMP,
                                            m_drivenNames.get(itemp), itemp->findBitDType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            itemp->user2p(varp);
        }
        بازگشت VN_AS(itemp->user2p(), Var);
    }
    // Reference to a variable in the scope of a driven clockvar, made like the clockvar's
    ایستا Ast_گره_عبارت* newDriveRefp(File_سطر* flp, ثابت SynchDrive& drive, AstVar* varp,
                                     ثابت VAccess& access) {
        اگر (ثابت AstVarXRef* ثابت xrefp = VN_CAST(drive.refp, VarXRef)) {
            بازگشت جدید AstVarXRef{flp, varp, xrefp->dotted(), access};
        } وگرنه اگر (ثابت AstMemberSel* ثابت selp = VN_CAST(drive.refp, MemberSel)) {
            // The interface expression is evaluated again, so it must not have side effects;
            // cloneTreePure warns if it has any
            AstMemberSel* ثابت newSelp
                = جدید AstMemberSel{flp, selp->fromp()->clone_درخت_Pure(نادرست), varp};
            newSelp->access(access);
            بازگشت newSelp;
        }
        بازگشت جدید AstVarRef{flp, varp, access};
    }
    // Assignment setting the drive flag of a driven clockvar
    AstAssign* newDrivenSetp(File_سطر* flp, ثابت SynchDrive& drive) {
        AstVar* ثابت varp = getCreateDrivenVarp(drive.itemp);
        AstAssign* ثابت setp = جدید AstAssign{flp, newDriveRefp(flp, drive, varp, VAccess::WRITE),
                                              جدید AstConst{flp, AstConst::BitTrue{}}};
        setp->user1(درست);
        بازگشت setp;
    }
    // Clocking block of a clocking item
    ایستا AstClocking* clockingOf(ثابت AstClockingItem* itemp) {
        UASSERT_OBJ(itemp->user3p(), itemp, "Clocking item not captured in a clocking block");
        بازگشت VN_AS(itemp->user3p(), Clocking);
    }
    // Clocking event of a driven clockvar, referenced from the drive like the clockvar, or
    // nullptr if unsupported, which is reported on the drive's cycle delay
    Ast_Sen_درخت* newDriveSentreep(AstDelay* delayp, ثابت SynchDrive& drive) {
        Ast_Sen_درخت* ثابت origp = clockingOf(drive.itemp)->sentreep();
        اگر (VN_IS(drive.refp, VarRef)) بازگشت origp->clone_درخت(نادرست);
        اگر (origp->exists([](ثابت AstVarXRef*) { بازگشت درست; })) {
            // The reference is relative to the clocking block
            delayp->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in synchronous drive to"
                                          " clockvar in another scope, whose clocking event"
                                          " has a hierarchical reference");
            بازگشت nullptr;
        }
        ثابت AstMemberSel* ثابت selp = VN_CAST(drive.refp, MemberSel);
        // Processes outside classes evaluate their events also when not waiting on them,
        // possibly before the interface reference is set, so the event is guarded against a
        // null reference, which needs an integral value; scheduling does not guard it
        ثابت منطقی guardNull = selp && !VN_IS(m_modp, Class);
        اگر (guardNull) {
            برای (AstSenItem* itemp = origp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                اگر (!itemp->sensp()->dtypep()->skipRefp()->isIntegralOrPacked()) {
                    delayp->v3warn(E_UNSUPPORTED,
                                   "Unsupported: cycle delay in synchronous drive through"
                                   " an interface reference outside a class, to clocking"
                                   " block with non-integral clocking event");
                    بازگشت nullptr;
                }
            }
        }
        // Interface declaring the clocking block, if driven through an interface reference
        AstIface* ثابت ifacep
            = selp ? VN_AS(selp->fromp()->dtypep()->skipRefp(), IfaceRefDType)->ifaceViaCellp()
                   : nullptr;
        Ast_Sen_درخت* ثابت sentreep = origp->clone_درخت(نادرست);
        std::vector<AstVarRef*> refps;
        sentreep->foreach([&](AstVarRef* refp) {
            اگر (!refp->classOrPackagep()) refps.افزودن_back(refp);
        });
        برای (AstVarRef* ثابت refp : refps) {
            AstVar* ثابت varp = refp->varp();
            اگر (ifacep) {
                // Mark the variable as read through an interface reference, as V3Width does for
                // a member select. The mark is the interface declaring the variable, whose
                // instances all share it, so drives through references to any instances agree.
                UASSERT_OBJ(!varp->sensIfacep() || varp->sensIfacep() == ifacep, refp,
                            "Variable read through references to different interfaces");
                varp->sensIfacep(ifacep);
            }
            refp->replaceWith(newDriveRefp(refp->fileline(), drive, varp, refp->access()));
            VL_DO_DANGLING(افزودن_Deletep(refp), refp);
        }
        اگر (guardNull) {
            برای (AstSenItem* itemp = sentreep->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                guardNullIface(itemp, selp->fromp());
            }
        }
        بازگشت sentreep;
    }
    // Make an integral clocking event through an interface reference false while the reference
    // is null
    ایستا تهی guardNullIface(AstSenItem* sensesp, Ast_گره_عبارت* fromp) {
        File_سطر* ثابت flp = sensesp->fileline();
        Ast_گره_عبارت* ثابت sensp = sensesp->sensp()->unlinkFrBack();
        sensesp->sensp(جدید AstCond{flp, newNotNullp(flp, fromp), sensp,
                                   جدید AstConst{flp, AstConst::DTyped{}, sensp->dtypep()}});
        اگر (Ast_گره_عبارت* ثابت condp = sensesp->condp()) {
            condp->unlinkFrBack();
            sensesp->condp(جدید AstLogAnd{flp, newNotNullp(flp, fromp), condp});
        }
    }
    ایستا Ast_گره_عبارت* newNotNullp(File_سطر* flp, Ast_گره_عبارت* fromp) {
        بازگشت جدید AstNeq{flp, fromp->clone_درخت_Pure(نادرست), جدید AstConst{flp, AstConst::Null{}}};
    }
    // Procedural '##0': if the default clocking event has not yet occurred in this time step,
    // wait for it; otherwise continue without suspension (IEEE 1800-2023 14.11)
    AstIf* newCycleDelayZerop(File_سطر* flp) ثابت {
        AstVar* ثابت evtVarp = m_defaultClkEvtVarp;
        UASSERT_OBJ(evtVarp, flp, "Default clocking event var not pre-created");
        AstCMethodHard* ثابت isTriggeredp = جدید AstCMethodHard{
            flp, جدید AstVarRef{flp, evtVarp, VAccess::READ}, VCMethod::EVENT_IS_TRIGGERED};
        isTriggeredp->dtypeSetBit();
        AstEventControl* ثابت waitp = جدید AstEventControl{
            flp,
            جدید Ast_Sen_درخت{flp, جدید AstSenItem{flp, VEdgeType::ET_EVENT,
                                               جدید AstVarRef{flp, evtVarp, VAccess::READ}}},
            nullptr};
        بازگشت جدید AstIf{flp, جدید AstNot{flp, isTriggeredp}, waitp};
    }

    ایستا تهی checkSamplingFuncDType(Ast_گره_عبارت* nodep, ثابت Ast_گره* exprp) {
        ثابت Ast_گره_DType* ثابت dtypep = exprp->dtypep()->skipRefp();
        اگر (!dtypep->isIntegralOrPacked()) {
            nodep->v3error("Expected numeric type, but got a " << dtypep->prettyDTypeNameQ()
                                                               << " data type");
        }
    }

    Ast_Sen_درخت* new_Sen_درخت(Ast_گره* nodep, Ast_Sen_درخت* useTreep = nullptr,
                           Ast_گره_Cover_Or_Assert* cassertp = nullptr) {
        // Create sentree based on clocked or default clock
        // Return nullptr for always
        اگر (useTreep) بازگشت useTreep;
        Ast_Sen_درخت* newp = nullptr;
        AstSenItem* senip = m_senip;
        منطقی fromAlways = نادرست;
        اگر (!senip && m_defaultClockingp) senip = m_defaultClockingp->sentreep()->sensesp();
        اگر (!senip) {
            senip = m_seniAlwaysp;
            fromAlways = درست;
        }
        اگر (!senip) {
            nodep->v3error("Concurrent assertion has no clock (IEEE 1800-2023 16.16)\n"
                           << nodep->warnMore()
                           << "... Suggest provide a clocking event, a default"
                              " clocking, or a clocked procedural context");
            newp = جدید Ast_Sen_درخت{nodep->fileline(), nullptr};
        } وگرنه {
            اگر (cassertp && fromAlways) cassertp->senFromAlways(درست);
            newp = جدید Ast_Sen_درخت{nodep->fileline(), senip->clone_درخت(درست)};
        }
        بازگشت newp;
    }
    Ast_گره_عبارت* getSequenceBodyExprp(ثابت AstSequence* ثابت seqp) ثابت {
        // The statements in AstSequence are optional AstVar (ports) followed by body expr.
        Ast_گره* bodyp = seqp->stmtsp();
        وقتی (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        بازگشت VN_CAST(bodyp, گره_عبارت);
    }
    AstPropSpec* getPropertyExprp(ثابت AstProperty* ثابت propp) {
        // Statements in AstProperty: AstVar (ports/local vars),
        // AstInitialStaticStmt/AstInitialAutomaticStmt (var init), AstPropSpec (body).
        Ast_گره* propExprp = propp->stmtsp();
        وقتی (propExprp
               && (VN_IS(propExprp, Var) || VN_IS(propExprp, InitialStaticStmt)
                   || VN_IS(propExprp, InitialAutomaticStmt))) {
            propExprp = propExprp->nextp();
        }
        بازگشت VN_CAST(propExprp, PropSpec);
    }
    تهی substituteSequenceCall(AstFuncRef* funcrefp, AstSequence* seqp) {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Inline the sequence body at the call site, replacing the FuncRef
        Ast_گره_عبارت* bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        // Clone the body expression since the sequence may be referenced multiple times
        Ast_گره_عبارت* clonedp = bodyExprp->clone_درخت(نادرست);
        // Build substitution map, then do a single traversal to replace all formals
        // (textual substitution per IEEE 16.8.2).
        ثابت V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<ثابت AstVar*, Ast_گره_عبارت*> portMap;
        برای (ثابت auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            ثابت auto it = portMap.find(refp->varp());
            اگر (it != portMap.end()) {
                refp->replaceWith(it->second->clone_درخت(نادرست));
                VL_DO_DANGLING(افزودن_Deletep(refp), refp);
            }
        });
        برای (ثابت auto& tconnect : tconnects) {
            افزودن_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        // Replace the FuncRef with the inlined body
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(افزودن_Deletep(funcrefp), funcrefp);
        // Clear referenced flag; sequences with isReferenced==false are deleted in assertPreAll
        seqp->isReferenced(نادرست);
    }
    AstPropSpec* clonePropertyBody(AstFuncRef* funcrefp, ثابت AstProperty* propp) {
        AstPropSpec* propExprp = getPropertyExprp(propp);
        // Substitute inner property call before copying in order to not doing the same for
        // each call of outer property call.
        propExprp = substitutePropertyCall(propExprp);
        // Clone subtree after substitution. It is needed, because property might be called
        // multiple times with different arguments.
        propExprp = propExprp->clone_درخت(نادرست);
        // Build substitution maps for formal arguments and property-local
        // variables, then perform a single foreach to apply all replacements.
        // Map port vars to their actual argument expressions
        ثابت V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propp->stmtsp());
        std::unordered_map<ثابت AstVar*, Ast_گره_عبارت*> portMap;
        برای (ثابت auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }

        // Promote property-local variables (non-port vars, IEEE 16.10) to
        // module-level __Vpropvar temps. Cross-cycle persistence is handled
        // by the match item lowering in visit(AstImplication*).
        std::unordered_map<ثابت AstVar*, AstVar*> localVarMap;
        برای (Ast_گره* stmtp = propp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                اگر (!varp->isIO()) {
                    ثابت string newName = m_propVarNames.get(varp);
                    AstVar* ثابت newVarp = جدید AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->addStmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        // Single traversal: substitute ports and update local var references
        propExprp->foreach([&](AstVarRef* refp) {
            ثابت auto portIt = portMap.find(refp->varp());
            اگر (portIt != portMap.end()) {
                refp->replaceWith(portIt->second->clone_درخت(نادرست));
                VL_DO_DANGLING(افزودن_Deletep(refp), refp);
                بازگشت;
            }
            ثابت auto localIt = localVarMap.find(refp->varp());
            اگر (localIt != localVarMap.end()) refp->varp(localIt->second);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        // Clean up argument expressions
        برای (ثابت auto& tconnect : tconnects) {
            افزودن_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(propExprp, funcrefp);
        بازگشت propExprp;
    }
    AstPropSpec* substitutePropertyCall(AstPropSpec* nodep) {
        اگر (AstFuncRef* ثابت funcrefp = VN_CAST(nodep->propp(), FuncRef)) {
            اگر (ثابت AstProperty* ثابت propp = VN_CAST(funcrefp->taskp(), Property)) {
                AstPropSpec* ثابت propExprp = clonePropertyBody(funcrefp, propp);

                // Handle case with 2 disable iff statement (IEEE 1800-2023 16.12.1)
                اگر (nodep->disablep() && propExprp->disablep()) {
                    nodep->v3error("disable iff expression before property call and in its "
                                   "body is not legal");
                    افزودن_Deletep(propExprp->disablep()->unlinkFrBack());
                }
                // If disable iff is in outer property, move it to inner
                اگر (nodep->disablep()) {
                    Ast_گره_عبارت* ثابت disablep = nodep->disablep()->unlinkFrBack();
                    propExprp->disablep(disablep);
                }

                اگر (nodep->sensesp() && propExprp->sensesp()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Clock event before property call and in its body");
                    افزودن_Deletep(propExprp->sensesp()->unlinkFrBack());
                }
                // If clock event is in outer property, move it to inner
                اگر (nodep->sensesp()) {
                    AstSenItem* ثابت sensesp = nodep->sensesp();
                    sensesp->unlinkFrBack();
                    propExprp->sensesp(sensesp);
                }

                // Now substitute property reference with property body
                nodep->replaceWith(propExprp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                بازگشت propExprp;
            }
        }
        بازگشت nodep;
    }

    تهی substituteNestedPropertyCall(AstFuncRef* funcrefp, ثابت AstProperty* propp) {
        AstPropSpec* ثابت specp = clonePropertyBody(funcrefp, propp);
        Ast_گره* replp = nullptr;
        اگر (specp->disablep()) {
            funcrefp->v3error("Illegal 'disable iff' in a nested property instance"
                              " (IEEE 1800-2023 16.12.1)");
        } وگرنه اگر (specp->sensesp()) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: clock event in a nested property instance");
        } وگرنه اگر (specp->propp()->exists(
                       [](ثابت Ast_گره_عبارت* np) { بازگشت np->isMultiCycleSva(); })) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: multi-cycle body in a nested property instance");
        } وگرنه {
            replp = specp->propp()->unlinkFrBack();
        }
        اگر (!replp) replp = جدید AstConst{funcrefp->fileline(), AstConst::BitFalse{}};
        funcrefp->replaceWith(replp);
        VL_DO_DANGLING(افزودن_Deletep(funcrefp), funcrefp);
        VL_DO_DANGLING(افزودن_Deletep(specp), specp);
    }

    // VISITORS
    //========== Statements
    تهی visit(AstClocking* ثابت nodep) override {
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        UINFO(8, "   CLOCKING" << nodep);
        اگر (nodep->isDefault()) {
            اگر (m_defaultClockingp) {
                nodep->v3error("Only one default clocking block allowed per module"
                               " (IEEE 1800-2023 14.12)");
            } وگرنه {
                m_defaultClockingp = nodep;
                m_defaultClkEvtVarp = nodep->ensureEventp();
            }
        }
        iterateChildren(nodep);
        اگر (nodep->eventp()) nodep->addNextHere(nodep->eventp()->unlinkFrBack());
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
    }
    تهی visit(AstModportClockingRef* ثابت nodep) override {
        // It has to be converted to a list of ModportClockingVarRefs,
        // because clocking blocks are removed in this pass
        برای (Ast_گره* itemp = nodep->clockingp()->itemsp(); itemp; itemp = itemp->nextp()) {
            اگر (AstClockingItem* ثابت citemp = VN_CAST(itemp, ClockingItem)) {
                اگر (AstVar* ثابت varp
                    = citemp->varp() ? citemp->varp() : VN_AS(citemp->user1p(), Var)) {
                    AstModportVarRef* ثابت modVarp = جدید AstModportVarRef{
                        nodep->fileline(), varp->name(), citemp->direction()};
                    modVarp->varp(varp);
                    nodep->addNextHere(modVarp);
                    اگر (citemp->direction().isOutput()) {
                        // Drives through the modport set the drive flag
                        AstVar* ثابت drivenVarp = getCreateDrivenVarp(citemp);
                        AstModportVarRef* ثابت drivenRefp = جدید AstModportVarRef{
                            nodep->fileline(), drivenVarp->name(), VDirection::OUTPUT};
                        drivenRefp->varp(drivenVarp);
                        nodep->addNextHere(drivenRefp);
                    }
                }
            }
        }
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
    }
    تهی visit(AstClockingItem* ثابت nodep) override {
        // Get a ref to the sampled/driven variable
        AstVar* ثابت varp = nodep->varp();
        اگر (!varp) {
            // Unused item
            بازگشت;
        }
        // Flag set by drives of an output clockvar, possibly visited before this item
        اگر (nodep->direction().isOutput()) m_modp->addStmtsp(getCreateDrivenVarp(nodep));
        File_سطر* ثابت flp = nodep->fileline();
        V3Const::constifyEdit(nodep->skewp());
        اگر (!VN_IS(nodep->skewp(), Const)) {
            nodep->skewp()->v3error("Skew must be constant (IEEE 1800-2023 14.4)");
            بازگشت;
        }
        AstConst* ثابت skewp = VN_AS(nodep->skewp(), Const);
        اگر (skewp->رقم().isNegative()) skewp->v3error("Skew cannot be negative");
        Ast_گره_عبارت* ثابت exprp = nodep->exprp();
        varp->name(m_clockingp->name() + "__DOT__" + varp->name());
        m_clockingp->addNextHere(varp->unlinkFrBack());
        nodep->user1p(varp);
        varp->user1p(nodep);
        اگر (nodep->direction() == VDirection::OUTPUT) {
            exprp->foreach([](ثابت Ast_گره_Var_Ref* varrefp) {
                // Prevent confusing BLKANDNBLK warnings on clockvars due to generated assignments
                varrefp->fileline()->warnOff(V3ErrorCode::BLKANDNBLK, درست);
                // Mark the driven signal so later passes (e.g. V3Inst port
                // connection) know the drive flows out from inside the cell.
                // Item expressions are plain signal references, so only the
                // driven signal is marked, never index selects.
                varrefp->varp()->isClockingDriven(درست);
            });
            AstVarRef* ثابت skewedReadRefp = جدید AstVarRef{flp, varp, VAccess::READ};
            skewedReadRefp->user1(درست);
            // Initialize the clockvar
            AstVarRef* ثابت skewedWriteRefp = جدید AstVarRef{flp, varp, VAccess::WRITE};
            skewedWriteRefp->user1(درست);
            AstInitialStatic* ثابت initClockvarp = جدید AstInitialStatic{
                flp, جدید AstAssign{flp, skewedWriteRefp, exprp->clone_درخت_Pure(نادرست)}};
            m_modp->addStmtsp(initClockvarp);
            // Each drive sets a flag, so the signal is also assigned when driven with the same
            // value again, e.g. after another assignment to the signal (IEEE 1800-2023 14.16.2)
            AstVar* ثابت drivenVarp = getCreateDrivenVarp(nodep);
            m_modp->addStmtsp(جدید AstInitialStatic{
                flp, جدید AstAssign{flp, جدید AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                   جدید AstConst{flp, AstConst::BitFalse{}}}});
            // Assign the clockvar to the actual var if it was driven
            AstAssign* ثابت assignp
                = جدید AstAssign{flp, exprp->clone_درخت_Pure(نادرست), skewedReadRefp};
            AstIf* ثابت ifp
                = جدید AstIf{flp, جدید AstVarRef{flp, drivenVarp, VAccess::READ},
                            جدید AstAssign{flp, جدید AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                          جدید AstConst{flp, AstConst::BitFalse{}}}};
            ifp->addThensp(assignp);
            // Only in the time step of the clocking event
            AstCMethodHard* ثابت trigp = جدید AstCMethodHard{
                nodep->fileline(), جدید AstVarRef{flp, m_clockingp->ensureEventp(), VAccess::READ},
                VCMethod::EVENT_IS_TRIGGERED};
            trigp->dtypeSetBit();
            ifp->condp(جدید AstLogAnd{flp, ifp->condp()->unlinkFrBack(), trigp});
            // A skew rounding to no time precision ticks is a zero skew
            Ast_گره_عبارت* ثابت ticksp
                = V3Timing::delayToTicks(skewp->clone_درخت(نادرست), m_modp->timeunit());
            ثابت منطقی zeroSkew = ticksp->isZero();
            VL_DO_DANGLING(ticksp->delete_درخت(), ticksp);
            اگر (zeroSkew || !skewp->fileline()->timingOn()) {
                // Drive the var in Re-NBA (IEEE 1800-2023 14.16), ignoring skews where timing is
                // turned off
                Ast_Sen_درخت* senTreep = m_clockingp->sentreep()->clone_درخت(نادرست);
                senTreep->addSensesp(جدید AstSenItem{
                    flp, VEdgeType::ET_CHANGED, جدید AstVarRef{flp, drivenVarp, VAccess::READ}});
                m_clockingp->addNextHere(جدید AstAlwaysReactive{flp, senTreep, ifp});
            } وگرنه {
                // Fork only the assignment, so this AlwaysPostponed can be retriggered
                // before the assignment happens
                AstFork* ثابت forkp = جدید AstFork{flp, VJoinType::JOIN_NONE};
                forkp->immediateStart(درست);
                forkp->addForksp(جدید AstBegin{flp, "", assignp->unlinkFrBack(), درست});
                ifp->addThensp(forkp);
                // Read the clockvar in Postponed, so a drive late in the time step is applied
                // at this event plus the skew (IEEE 1800-2023 14.16)
                m_clockingp->addNextHere(جدید AstAlwaysPostponed{flp, ifp});
                اگر (v3Global.opt.timing().isSetTrue()) {
                    AstDelay* ثابت delayp = جدید AstDelay{flp, skewp->unlinkFrBack(), نادرست};
                    delayp->timeunit(m_modp->timeunit());
                    assignp->timingControlp(delayp);
                } وگرنه اگر (v3Global.opt.timing().isSetFalse()) {
                    nodep->v3warn(E_NOTIMING,
                                  "Clocking output skew greater than #0 requires --timing");
                } وگرنه {
                    nodep->v3warn(E_NEEDTIMINGOPT,
                                  "Use --timing or --no-timing to specify how "
                                  "clocking output skew greater than #0 should be handled");
                }
            }
        } وگرنه اگر (nodep->direction() == VDirection::درون_داد) {
            // Ref to the clockvar
            AstVarRef* ثابت refp = جدید AstVarRef{flp, varp, VAccess::WRITE};
            refp->user1(درست);
            اگر (skewp->رقم().is1Step()) {
                // #1step means the value that is sampled is always the signal's last value
                // before the clock edge (IEEE 1800-2023 14.4)
                AstSampled* ثابت sampledp
                    = جدید AstSampled{flp, exprp->clone_درخت_Pure(نادرست), exprp->dtypep(), درست};
                AstAssign* ثابت assignp = جدید AstAssign{flp, refp, sampledp};
                m_clockingp->addNextHere(جدید AstAlways{
                    flp, VAlwaysKwd::ALWAYS, m_clockingp->sentreep()->clone_درخت(نادرست), assignp});
            } وگرنه اگر (skewp->isZero()) {
                // #0 means the var has to be sampled in Observed (IEEE 1800-2023 14.13)
                AstAssign* ثابت assignp = جدید AstAssign{flp, refp, exprp->clone_درخت_Pure(نادرست)};
                m_clockingp->addNextHere(جدید AstAlwaysObserved{
                    flp, m_clockingp->sentreep()->clone_درخت(نادرست), assignp});
            } وگرنه {
                // Create a queue where we'll store sampled values with timestamps
                AstSampleQueueDType* ثابت queueDtp
                    = جدید AstSampleQueueDType{flp, exprp->dtypep()};
                m_netlistp->typeTablep()->addTypesp(queueDtp);
                AstVar* ثابت queueVarp
                    = جدید AstVar{flp, VVarType::MODULETEMP, "__Vqueue__" + varp->name(), queueDtp};
                queueVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_clockingp->addNextHere(queueVarp);
                // Create a process like this:
                //     always queue.push(<sampled var>);
                AstCMethodHard* ثابت pushp = جدید AstCMethodHard{
                    flp, جدید AstVarRef{flp, queueVarp, VAccess::WRITE}, VCMethod::DYN_افزودن,
                    جدید AstTime{nodep->fileline(), m_modp->timeunit()}};
                pushp->addPinsp(exprp->clone_درخت_Pure(نادرست));
                pushp->dtypeSetVoid();
                m_clockingp->addNextHere(
                    جدید AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, pushp->makeStmt()});
                // Create a process like this:
                //     always @<clocking event> queue.pop(<skew>, /*out*/<skewed var>);
                AstCMethodHard* ثابت popp = جدید AstCMethodHard{
                    flp, جدید AstVarRef{flp, queueVarp, VAccess::READWRITE}, VCMethod::DYN_برداشت,
                    جدید AstTime{nodep->fileline(), m_modp->timeunit()}};
                popp->addPinsp(skewp->unlinkFrBack());
                refp->access(VAccess::READWRITE);  // Only conditionally assigned
                popp->addPinsp(refp);
                popp->dtypeSetVoid();
                m_clockingp->addNextHere(جدید AstAlways{flp, VAlwaysKwd::ALWAYS,
                                                       m_clockingp->sentreep()->clone_درخت(نادرست),
                                                       popp->makeStmt()});
            }
        } وگرنه {
            nodep->v3fatalSrc("Invalid direction");
        }
    }
    تهی visit(AstDelay* nodep) override {
        m_hasCycleDelay |= nodep->isCycleDelay();
        // Only cycle delays are relevant in this stage; also only process once
        اگر (!nodep->isCycleDelay()) {
            اگر (m_inSynchDrive) {
                nodep->v3error("Only cycle delays can be used in synchronous drives"
                               " (IEEE 1800-2023 14.16)");
            }
            iterateChildren(nodep);
            بازگشت;
        }
        اگر (m_inAssign && !m_inSynchDrive) {
            nodep->v3error("Cycle delays not allowed as intra-assignment delays"
                           " (IEEE 1800-2023 14.11)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
            بازگشت;
        }
        اگر (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBackWithNext());
        File_سطر* ثابت flp = nodep->fileline();
        Ast_گره_عبارت* valuep = V3Const::constifyEdit(nodep->lhsp()->unlinkFrBack());
        ثابت AstConst* ثابت constp = VN_CAST(valuep, Const);
        اگر (!constp && m_pexprp) {
            // V3AssertNfa handles non-const delays in sequences before this pass and
            // replaces the property; this branch should never be reached.
            nodep->v3fatalSrc("Non-constant cycle delay in assertion: "
                              "should have been caught by V3AssertNfa");
        } وگرنه اگر (constp && constp->isZero()) {
            VL_DO_DANGLING(افزودن_Deletep(valuep), valuep);
            اگر (m_inSynchDrive) {
                // ##0 has no effect in synchronous drives (IEEE 1800-2023 14.11)
                VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
                بازگشت;
            }
            اگر (m_pexprp) {
                // ##0 in sequence context = zero delay = same clock tick
                VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
                بازگشت;
            }
            // Procedural ##0: synchronize with default clocking event (IEEE 1800-2023 14.11)
            اگر (!m_defaultClockingp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
                بازگشت;
            }
            nodep->replaceWith(newCycleDelayZerop(flp));
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت;
        }
        Ast_Sen_درخت* sentreep = nullptr;
        اگر (!m_drives.empty()) {
            اگر (m_inConcatAssign) {
                // Also the clockvars may be of different clocking blocks
                nodep->v3error("Cycle delays cannot be used in synchronous drives to a"
                               " concatenation (IEEE 1800-2023 14.16)");
                VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(افزودن_Deletep(valuep), valuep);
                بازگشت;
            }
            // Count the cycles of the driven clockvar's clocking block (IEEE 1800-2023 14.16)
            sentreep = newDriveSentreep(nodep, m_drives.front());
            اگر (!sentreep) {
                VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(افزودن_Deletep(valuep), valuep);
                بازگشت;
            }
        } وگرنه اگر (!m_defaultClockingp) {
            اگر (!m_pexprp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(افزودن_Deletep(valuep), valuep);
                بازگشت;
            }
            sentreep = جدید Ast_Sen_درخت{flp, m_senip->clone_درخت(نادرست)};
        } وگرنه {
            sentreep = m_defaultClockingp->sentreep()->clone_درخت(نادرست);
        }
        AstEventControl* ثابت controlp
            = جدید AstEventControl{nodep->fileline(), sentreep, nullptr};
        ثابت std::string delayName = m_cycleDlyNames.get(nodep);
        Ast_گره_عبارت* throughoutp
            = nodep->throughoutp() ? nodep->throughoutp()->unlinkFrBack() : nullptr;

        AstVar* ثابت cntVarp = جدید AstVar{flp, VVarType::BLOCKTEMP, delayName + "__counter",
                                           nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstBegin* ثابت beginp = جدید AstBegin{flp, delayName + "__block", cntVarp, درست};
        // A non-constant delay is counted at runtime. The count is integral of 64 bits, as
        // V3Width makes delays, or real, which is rounded (IEEE 1800-2023 6.12.1)
        اگر (valuep->isDouble()) {
            valuep = جدید AstRToIRoundS{flp, valuep};
            valuep->dtypeFrom(cntVarp);
        } وگرنه {
            valuep = جدید AstSel{flp, valuep, 0, cntVarp->width()};
        }
        beginp->addStmtsp(جدید AstAssign{flp, جدید AstVarRef{flp, cntVarp, VAccess::WRITE}, valuep});
        // If the count is zero, a synchronous drive waits for none, like with a constant '##0',
        // and a procedural delay waits like a procedural '##0' (IEEE 1800-2023 14.11)
        اگر (!constp && !m_inSynchDrive) {
            beginp->addStmtsp(جدید AstIf{
                flp,
                جدید AstEq{flp, جدید AstVarRef{flp, cntVarp, VAccess::READ}, جدید AstConst{flp, 0}},
                newCycleDelayZerop(flp)});
        }

        // Throughout: create flag tracking whether condition held every tick
        AstVar* throughoutOkp = nullptr;
        اگر (throughoutp) {
            throughoutOkp = جدید AstVar{flp, VVarType::BLOCKTEMP, delayName + "__throughoutOk",
                                       nodep->findBasicDType(VBasicDTypeKwd::LOGIC_IMPLICIT)};
            throughoutOkp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->addStmtsp(throughoutOkp);
            beginp->addStmtsp(جدید AstAssign{flp, جدید AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            جدید AstConst{flp, AstConst::BitTrue{}}});
            // Check condition at tick 0 (sequence start, before entering loop)
            beginp->addStmtsp(
                جدید AstIf{flp, جدید AstLogNot{flp, throughoutp->clone_درخت_Pure(نادرست)},
                          جدید AstAssign{flp, جدید AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                        جدید AstConst{flp, AstConst::BitFalse{}}}});
        }

        {
            AstLoop* ثابت loopp = جدید AstLoop{flp};
            // When throughout is present, exit loop early if condition fails
            Ast_گره_عبارت* loopCondp
                = جدید AstGt{flp, جدید AstVarRef{flp, cntVarp, VAccess::READ}, جدید AstConst{flp, 0}};
            اگر (throughoutOkp) {
                loopCondp = جدید AstLogAnd{flp, loopCondp,
                                          جدید AstVarRef{flp, throughoutOkp, VAccess::READ}};
            }
            loopp->addStmtsp(جدید AstLoopTest{flp, loopp, loopCondp});
            loopp->addStmtsp(controlp);
            loopp->addStmtsp(
                جدید AstAssign{flp, جدید AstVarRef{flp, cntVarp, VAccess::WRITE},
                              جدید AstSub{flp, جدید AstVarRef{flp, cntVarp, VAccess::READ},
                                         جدید AstConst{flp, 1}}});
            // Check throughout condition at each tick during delay (IEEE 1800-2023 16.9.9)
            اگر (throughoutp) {
                loopp->addStmtsp(
                    جدید AstIf{flp, جدید AstLogNot{flp, throughoutp},
                              جدید AstAssign{flp, جدید AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            جدید AstConst{flp, AstConst::BitFalse{}}}});
            }
            beginp->addStmtsp(loopp);
        }
        // Compose wrappers on remaining sequence: throughout gate (inner), disable iff (outer)
        Ast_گره* remainp = nodep->nextp() ? nodep->nextp()->unlinkFrBackWithNext() : nullptr;
        اگر (throughoutOkp) {
            // If condition failed during delay, fail assertion
            remainp = جدید AstIf{flp, جدید AstVarRef{flp, throughoutOkp, VAccess::READ}, remainp,
                                جدید AstPExprClause{flp, /*pass=*/نادرست}};
        }
        اگر (m_disableSeqIfp && remainp) {
            AstIf* ثابت disableSeqIfp = m_disableSeqIfp->clone_درخت(نادرست);
            disableSeqIfp->addThensp(remainp);
            remainp = disableSeqIfp;
        }
        اگر (remainp) {
            اگر (throughoutOkp) {
                // throughoutOkp is declared in beginp scope -- check must be inside it
                beginp->addStmtsp(remainp);
            } وگرنه {
                nodep->addNextHere(remainp);
            }
        }
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    تهی visit(Ast_Sen_درخت* nodep) override {
        اگر (m_inSynchDrive) {
            nodep->v3error("Event controls cannot be used in "
                           "synchronous drives (IEEE 1800-2023 14.16)");
        }

        ثابت AstSampled* sampledp;
        اگر (nodep->exists([&sampledp](ثابت AstSampled* ثابت sp) {
                sampledp = sp;
                بازگشت درست;
            })) {
            sampledp->v3warn(E_UNSUPPORTED, "Unsupported: $sampled inside sensitivity list");
        }
    }
    تهی visit(Ast_گره_Var_Ref* nodep) override {
        UINFO(8, " -varref:  " << nodep);
        UINFO(8, " -varref-var-back:  " << nodep->varp()->backp());
        UINFO(8, " -varref-var-user1:  " << nodep->varp()->user1p());
        اگر (AstClockingItem* ثابت itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->firstAbovep(),
                ClockingItem)) {
            اگر (nodep->user1()) بازگشت;

            // ensure linking still works, this has to be done only once
            اگر (AstVarXRef* xrefp = VN_CAST(nodep, VarXRef)) {
                UINFO(8, " -clockvarxref-in:  " << xrefp);
                string dotted = xrefp->dotted();
                ثابت size_t dotPos = dotted.rfind('.');
                dotted.erase(dotPos, string::npos);
                xrefp->dotted(dotted);
                UINFO(8, " -clockvarxref-out: " << xrefp);
                m_xrefsp.emplace_back(xrefp);
            }

            اگر (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            اگر (nodep->access().isWriteOrRW()) {
                اگر (itemp->direction() == VDirection::OUTPUT) {
                    اگر (!m_inAssignDlyLhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    اگر (m_inAssign) {
                        m_inSynchDrive = درست;
                        m_drives.افزودن_back({itemp, nodep});
                    }
                } وگرنه اگر (itemp->direction() == VDirection::درون_داد) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
            nodep->user1(درست);
        }
    }
    تهی visit(AstMemberSel* nodep) override {
        اگر (AstClockingItem* ثابت itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->firstAbovep(),
                ClockingItem)) {
            اگر (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            اگر (nodep->access().isWriteOrRW()) {
                اگر (itemp->direction() == VDirection::OUTPUT) {
                    اگر (!m_inAssignDlyLhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    اگر (m_inAssign) {
                        m_inSynchDrive = درست;
                        m_drives.افزودن_back({itemp, nodep});
                    }
                } وگرنه اگر (itemp->direction() == VDirection::درون_داد) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
        }
    }
    تهی visit(Ast_گره_Assign* nodep) override {
        اگر (nodep->user1()) بازگشت;
        VL_RESTORER(m_inAssign);
        VL_RESTORER(m_inSynchDrive);
        VL_RESTORER(m_inConcatAssign);
        VL_RESTORER_CLEAR(m_drives);
        m_inAssign = درست;
        m_inSynchDrive = نادرست;
        {
            // The braces of a concatenation are a replication, by one
            ثابت Ast_گره_عبارت* lhsp = nodep->lhsp();
            اگر (ثابت AstReplicate* ثابت repp = VN_CAST(lhsp, Replicate)) lhsp = repp->srcp();
            m_inConcatAssign = VN_IS(lhsp, Concat);
        }
        {
            VL_RESTORER(m_inAssignDlyLhs);
            m_inAssignDlyLhs = VN_IS(nodep, AssignDly);
            iterate(nodep->lhsp());
        }
        iterate(nodep->rhsp());
        اگر (nodep->timingControlp()) iterate(nodep->timingControlp());
        // A drive without a cycle delay, also after removing '##0', updates the clockvar now
        اگر (m_inSynchDrive && !nodep->timingControlp()) {
            AstAssign* ثابت assignp = جدید AstAssign{
                nodep->fileline(), nodep->lhsp()->unlinkFrBack(), nodep->rhsp()->unlinkFrBack()};
            assignp->user1(درست);
            nodep->replaceWith(assignp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            nodep = assignp;
        }
        // Note the drives when they take effect, after the cycle delay if any. Each clockvar
        // of the LHS is driven, e.g. in a concatenation which other simulators accept, although
        // IEEE 1800-2023 14.16 does not allow it
        Ast_گره* setsp = nullptr;
        برای (ثابت SynchDrive& drive : m_drives) {
            setsp = Ast_گره::addNext(setsp, newDrivenSetp(nodep->fileline(), drive));
        }
        اگر (!setsp) بازگشت;
        اگر (AstBegin* ثابت beginp = VN_CAST(nodep->timingControlp(), Begin)) {
            beginp->addStmtsp(setsp);
        } وگرنه {
            nodep->addNextHere(setsp);
        }
    }
    تهی visit(AstAlways* nodep) override {
        iterateAndNextNull(nodep->sentreep());
        اگر (nodep->sentreep()) m_seniAlwaysp = nodep->sentreep()->sensesp();
        iterateAndNextNull(nodep->stmtsp());
        m_seniAlwaysp = nullptr;
    }

    تهی visit(Ast_گره_Cover_Or_Assert* nodep) override {
        اگر (nodep->sentreep()) بازگشت;  // Already processed

        VL_RESTORER(m_senip);
        VL_RESTORER(m_disablep);
        m_senip = nullptr;
        m_disablep = nullptr;

        // Find Clocking's buried under nodep->exprsp
        iterateChildren(nodep);
        اگر (!nodep->immediate()) nodep->sentreep(new_Sen_درخت(nodep, nullptr, nodep));
    }
    تهی visit(AstFalling* nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* exprp = nodep->exprp()->unlinkFrBack();
        اگر (exprp->width() > 1) exprp = جدید AstSel{fl, exprp, 0, 1};
        Ast_گره_عبارت* ثابت futurep = جدید AstFuture{fl, exprp, new_Sen_درخت(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = جدید AstAnd{fl, exprp->clone_درخت_Pure(نادرست), جدید AstNot{fl, futurep}};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(AstFell* nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* exprp = nodep->exprp()->unlinkFrBack();
        اگر (exprp->width() > 1) exprp = جدید AstSel{fl, exprp, 0, 1};
        Ast_Sen_درخت* sentreep = nodep->sentreep();
        اگر (sentreep) sentreep->unlinkFrBack();
        AstPast* ثابت pastp = جدید AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_درخت(nodep, sentreep));
        exprp = جدید AstAnd{fl, pastp, جدید AstNot{fl, exprp->clone_درخت_Pure(نادرست)}};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(AstFuture* nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        Ast_Sen_درخت* ثابت sentreep = nodep->sentreep();
        اگر (sentreep) VL_DO_DANGLING(افزودن_Deletep(sentreep->unlinkFrBack()), sentreep);
        nodep->sentreep(new_Sen_درخت(nodep));
    }
    تهی visit(AstPast* nodep) override {
        checkSamplingFuncDType(nodep, nodep->exprp());
        اگر (nodep->sentreep()) بازگشت;  // Already processed
        iterateChildren(nodep);
        nodep->sentreep(new_Sen_درخت(nodep));
    }
    تهی visit(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- Lower standalone exact [*N] (N >= 2) via saturating counter.
        // Range/unbounded forms and SExpr-contained forms are lowered by V3AssertNfa.
        iterateChildren(nodep);
        // V3AssertNfa handles unbounded/ranged forms upstream, so this fast-path
        // is effectively unreachable when NFA is enabled.
        اگر (nodep->unbounded() || nodep->maxCountp()) بازگشت;  // LCOV_EXCL_LINE
        ثابت AstConst* ثابت constp = VN_CAST(nodep->countp(), Const);
        اگر (VL_UNLIKELY(!constp || constp->toSInt() < 1)) {
            nodep->v3fatalSrc("Consecutive repetition count must be a positive constant"
                              " (should have been caught by V3Width)");
            بازگشت;
        }
        ثابت صحیح n = constp->toSInt();
        File_سطر* ثابت flp = nodep->fileline();
        Ast_گره_عبارت* ثابت exprp = nodep->exprp()->unlinkFrBack();
        اگر (n == 1) {
            nodep->replaceWith(exprp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت;
        }
        // Saturating counter: if (expr) cnt <= min(cnt+1, N); else cnt <= 0;
        AstVar* ثابت cntVarp = جدید AstVar{flp, VVarType::MODULETEMP, m_consRepNames.get(""),
                                           nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(cntVarp);
        Ast_گره_عبارت* ثابت عبارت_Clonep = exprp->clone_درخت_Pure(نادرست);
        Ast_گره_عبارت* ثابت saturatingIncrp = جدید AstCond{
            flp,
            جدید AstLt{flp, جدید AstVarRef{flp, cntVarp, VAccess::READ},
                      جدید AstConst{flp, static_cast<uint32_t>(n)}},
            جدید AstAdd{flp, جدید AstVarRef{flp, cntVarp, VAccess::READ}, جدید AstConst{flp, 1u}},
            جدید AstConst{flp, static_cast<uint32_t>(n)}};
        AstAssignDly* ثابت incrAssignp
            = جدید AstAssignDly{flp, جدید AstVarRef{flp, cntVarp, VAccess::WRITE}, saturatingIncrp};
        AstAssignDly* ثابت resetAssignp = جدید AstAssignDly{
            flp, جدید AstVarRef{flp, cntVarp, VAccess::WRITE}, جدید AstConst{flp, 0u}};
        AstIf* ثابت ifp = جدید AstIf{flp, عبارت_Clonep, incrAssignp, resetAssignp};
        Ast_Sen_درخت* ثابت senTreep = new_Sen_درخت(nodep);
        AstAlways* ثابت alwaysp = جدید AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, ifp};
        cntVarp->addNextHere(alwaysp);
        // Match: cnt >= N-1 (previous cycles via NBA) && expr (current cycle)
        Ast_گره_عبارت* ثابت cntCheckp = جدید AstGte{flp, جدید AstVarRef{flp, cntVarp, VAccess::READ},
                                                  جدید AstConst{flp, static_cast<uint32_t>(n - 1)}};
        cntCheckp->dtypeSetBit();
        Ast_گره_عبارت* ثابت matchp = جدید AstAnd{flp, cntCheckp, exprp};
        matchp->dtypeSetBit();
        nodep->replaceWith(matchp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(AstRising* nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* exprp = nodep->exprp()->unlinkFrBack();
        اگر (exprp->width() > 1) exprp = جدید AstSel{fl, exprp, 0, 1};
        Ast_گره_عبارت* ثابت futurep = جدید AstFuture{fl, exprp, new_Sen_درخت(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = جدید AstAnd{fl, جدید AstNot{fl, exprp->clone_درخت_Pure(نادرست)}, futurep};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(AstRose* nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* exprp = nodep->exprp()->unlinkFrBack();
        اگر (exprp->width() > 1) exprp = جدید AstSel{fl, exprp, 0, 1};
        Ast_Sen_درخت* sentreep = nodep->sentreep();
        اگر (sentreep) sentreep->unlinkFrBack();
        AstPast* ثابت pastp = جدید AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_درخت(nodep, sentreep));
        exprp = جدید AstAnd{fl, جدید AstNot{fl, pastp}, exprp->clone_درخت_Pure(نادرست)};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    ایستا AstAssocArrayDType* getProcessAssocArrayType(File_سطر* ثابت flp) {
        // Type of __VassertsActive___x[std::process]
        Ast_گره_DType* valp
            = v3Global.rootp()->typeTablep()->findBasicDType(flp, VBasicDTypeKwd::BIT);
        AstClassRefDType* keyp
            = جدید AstClassRefDType{flp, v3Global.rootp()->stdPackageProcessp(), nullptr};
        keyp->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        v3Global.rootp()->typeTablep()->addTypesp(keyp);
        AstAssocArrayDType* ثابت typep = جدید AstAssocArrayDType{flp, valp, keyp};
        typep->dtypep(typep);
        v3Global.rootp()->typeTablep()->addTypesp(typep);
        بازگشت typep;
    }
    ایستا Ast_Stmt_عبارت* getProcessAssocArrayDelete(AstVarRef* ثابت refp) {
        // Constructs refp.delete(std::process::self()) statement
        File_سطر* ثابت flp = refp->fileline();
        AstCMethodHard* ثابت deletep = جدید AstCMethodHard{
            flp, refp, VCMethod::ASSOC_ERASE, v3Global.rootp()->stdPackageProcessSelfp(flp)};
        deletep->dtypep(refp->findVoidDType());
        بازگشت جدید Ast_Stmt_عبارت{flp, deletep};
    }
    ایستا Ast_گره_عبارت* getProcessAssocArraySize(AstVarRef* ثابت refp) {
        // Constructs refp.size() statement
        AstCMethodHard* ثابت sizep
            = جدید AstCMethodHard{refp->fileline(), refp, VCMethod::ASSOC_SIZE};
        sizep->dtypep(refp->findBasicDType(VBasicDTypeKwd::UINT32));
        بازگشت sizep;
    }
    تهی visit(AstSEventually* nodep) override {
        UASSERT(v3Global.rootp()->stdPackagep(), "Should be imported");

        Ast_Sen_درخت* ثابت sentreep = new_Sen_درخت(nodep);
        اگر (!sentreep->sensesp()) {
            VL_DO_DANGLING(افزودن_Deletep(sentreep), sentreep);
            nodep->replaceWith(جدید AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت;
        }

        File_سطر* ثابت flp = nodep->fileline();

        // Track active assertions
        AstVar* ثابت activep = جدید AstVar{flp, VVarType::MODULETEMP, m_activeNames.get(""),
                                           getProcessAssocArrayType(flp)};
        activep->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(activep);

        // Assertion condition check
        AstLoop* ثابت loopp = جدید AstLoop{flp};
        AstSampled* ثابت condp
            = جدید AstSampled{flp, nodep->exprp()->unlinkFrBack(), nullptr, درست};
        loopp->addStmtsp(جدید AstLoopTest{flp, loopp, جدید AstLogNot{flp, condp}});
        loopp->addStmtsp(جدید AstEventControl{flp, sentreep, nullptr});

        // Add assertion to the active set
        AstAssocSel* ثابت selp = جدید AstAssocSel{flp, جدید AstVarRef{flp, activep, VAccess::WRITE},
                                                  v3Global.rootp()->stdPackageProcessSelfp(flp)};
        AstAssign* ثابت incrementp = جدید AstAssign{flp, selp, جدید AstConst{flp, 1}};
        AstPExprClause* ثابت clausep = جدید AstPExprClause{flp};
        Ast_Stmt_عبارت* ثابت deletep
            = getProcessAssocArrayDelete(جدید AstVarRef{flp, activep, VAccess::WRITE});

        // Main assertion block
        AstBegin* ثابت bodyp = جدید AstBegin{flp, m_blockNames.get(""), nullptr, درست};
        bodyp->addStmtsp(incrementp);
        bodyp->addStmtsp(loopp);
        bodyp->addStmtsp(clausep);
        bodyp->addStmtsp(deletep);

        // Validate assertion condition for each active assert
        AstVar* ثابت activeCountp = جدید AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        activeCountp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstAssign* ثابت initActiveCountp
            = جدید AstAssign{flp, جدید AstVarRef{flp, activeCountp, VAccess::WRITE},
                            getProcessAssocArraySize(جدید AstVarRef{flp, activep, VAccess::READ})};
        AstLoop* ثابت finalLoopp = جدید AstLoop{flp};
        AstIf* ثابت finalBodypCondp
            = جدید AstIf{flp, condp->clone_درخت_Pure(نادرست), جدید AstPExprClause{flp},
                        جدید AstPExprClause{flp, نادرست}};
        finalLoopp->addStmtsp(
            جدید AstLoopTest{flp, finalLoopp,
                            جدید AstNeq{flp, جدید AstVarRef{flp, activeCountp, VAccess::READ},
                                       جدید AstConst{flp, 0}}});
        finalLoopp->addStmtsp(finalBodypCondp);
        finalLoopp->addStmtsp(
            جدید AstAssign{flp, جدید AstVarRef{flp, activeCountp, VAccess::WRITE},
                          جدید AstSub{flp, جدید AstVarRef{flp, activeCountp, VAccess::READ},
                                     جدید AstConst{flp, 1}}});

        // Final assertion block
        AstBegin* ثابت finalp = جدید AstBegin{flp, m_blockNames.get(""), nullptr, درست};
        finalp->addStmtsp(activeCountp);
        finalp->addStmtsp(initActiveCountp);
        finalp->addStmtsp(finalLoopp);

        VL_RESTORER(m_pexprp);
        m_pexprp = جدید AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
        VL_RESTORER(m_hasCycleDelay);
        m_hasCycleDelay = نادرست;
        iterate(bodyp);
        iterate(finalp);

        اگر (m_hasCycleDelay) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in s_eventually");
            nodep->replaceWith(جدید AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            VL_DO_DANGLING(m_pexprp->delete_درخت(), m_pexprp);
            بازگشت;
        }

        nodep->replaceWith(m_pexprp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(AstStable* nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* exprp = nodep->exprp()->unlinkFrBack();
        Ast_Sen_درخت* sentreep = nodep->sentreep();
        اگر (sentreep) sentreep->unlinkFrBack();
        AstPast* ثابت pastp = جدید AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_درخت(nodep, sentreep));
        exprp = جدید AstEq{fl, pastp, exprp->clone_درخت_Pure(نادرست)};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(AstSteady* nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* exprp = nodep->exprp()->unlinkFrBack();
        اگر (exprp->width() > 1) exprp = جدید AstSel{fl, exprp, 0, 1};
        Ast_گره_عبارت* ثابت futurep = جدید AstFuture{fl, exprp, new_Sen_درخت(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = جدید AstEq{fl, exprp->clone_درخت_Pure(نادرست), futurep};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(AstSampled* nodep) override {
        iterateChildren(nodep);
        اگر (!nodep->internal()) checkSamplingFuncDType(nodep, nodep->exprp());
    }

    // Validate repetition count: must be a non-negative elaboration-time constant.
    // Shared by goto [->N] and nonconsecutive [=N] repetition.
    // On error, replaces nodep with BitFalse placeholder and returns nullptr.
    ثابت AstConst* validate_Rep_شمار(Ast_گره* nodep, Ast_گره_عبارت*& countp) {
        countp = V3Const::constifyEdit(countp);
        ثابت AstConst* ثابت constp = VN_CAST(countp, Const);
        اگر (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(افزودن_Deletep(countp), countp);
            nodep->replaceWith(جدید AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت nullptr;
        }
        اگر (constp->toSInt() < 0) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(افزودن_Deletep(countp), countp);
            nodep->replaceWith(جدید AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت nullptr;
        }
        اگر (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(افزودن_Deletep(countp), countp);
            nodep->replaceWith(جدید AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت nullptr;
        }
        بازگشت constp;
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
    AstPExpr* createRepPExpr(File_سطر* flp, Ast_گره_عبارت* exprp, Ast_گره_عبارت* countp,
                             Ast_گره_عبارت* rhsp, منطقی isOverlapped, منطقی isNonConsec) {
        AstSenItem* ثابت sensesp = m_senip;
        UASSERT_OBJ(sensesp, exprp, "Repetition requires a clock");

        ثابت std::string name
            = isNonConsec ? m_nonConsRepNames.get(exprp) : m_gotoRepNames.get(exprp);
        AstVar* ثابت cntVarp = جدید AstVar{flp, VVarType::BLOCKTEMP, name + "__counter",
                                           exprp->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstBegin* ثابت beginp = جدید AstBegin{flp, name + "__block", cntVarp, درست};
        beginp->addStmtsp(
            جدید AstAssign{flp, جدید AstVarRef{flp, cntVarp, VAccess::WRITE}, جدید AstConst{flp, 0}});

        // Counting loop: find N occurrences of expr (shared by goto and nonconsec)
        AstLoop* ثابت loopp = جدید AstLoop{flp};
        loopp->addStmtsp(جدید AstLoopTest{flp, loopp,
                                         جدید AstLt{flp, جدید AstVarRef{flp, cntVarp, VAccess::READ},
                                                   countp->clone_درخت_Pure(نادرست)}});
        // if (expr) cnt++ -- sampled is applied to whole property expr by V3Assert
        loopp->addStmtsp(
            جدید AstIf{flp, exprp,
                      جدید AstAssign{flp, جدید AstVarRef{flp, cntVarp, VAccess::WRITE},
                                    جدید AstAdd{flp, جدید AstVarRef{flp, cntVarp, VAccess::READ},
                                               جدید AstConst{flp, 1}}}});
        loopp->addStmtsp(جدید AstIf{
            flp, جدید AstLt{flp, جدید AstVarRef{flp, cntVarp, VAccess::READ}, countp},
            جدید AstEventControl{flp, جدید Ast_Sen_درخت{flp, sensesp->clone_درخت(نادرست)}, nullptr}});

        beginp->addStmtsp(loopp);

        اگر (isNonConsec && rhsp) {
            // IEEE 16.9.2: b[=N] = b[->N] ##1 !b[*0:$]
            // Trailing window: ##1 advance, then scan !expr cycles checking rhs.
            // Window closes when expr becomes true again (fail if rhs was never true).
            beginp->addStmtsp(جدید AstEventControl{
                flp, جدید Ast_Sen_درخت{flp, sensesp->clone_درخت(نادرست)}, nullptr});  // ##1
            اگر (!isOverlapped) {
                beginp->addStmtsp(جدید AstEventControl{
                    flp, جدید Ast_Sen_درخت{flp, sensesp->clone_درخت(نادرست)}, nullptr});  // |=>
            }
            // Window loop: check rhs at each !expr cycle (done variable for termination)
            AstVar* ثابت doneVarp
                = جدید AstVar{flp, VVarType::BLOCKTEMP, name + "__done", exprp->findBitDType()};
            doneVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->addStmtsp(doneVarp);
            beginp->addStmtsp(جدید AstAssign{flp, جدید AstVarRef{flp, doneVarp, VAccess::WRITE},
                                            جدید AstConst{flp, AstConst::BitFalse{}}});
            auto setDone = [&]() {
                بازگشت جدید AstAssign{flp, جدید AstVarRef{flp, doneVarp, VAccess::WRITE},
                                     جدید AstConst{flp, AstConst::BitTrue{}}};
            };
            AstLoop* ثابت windowp = جدید AstLoop{flp};
            // LoopTest: continue while !done
            windowp->addStmtsp(جدید AstLoopTest{
                flp, windowp, جدید AstNot{flp, جدید AstVarRef{flp, doneVarp, VAccess::READ}}});
            // if (expr) { fail; done = 1; } -- window closed, expr true again
            Ast_گره* ثابت failsp = جدید AstPExprClause{flp, نادرست};
            failsp->addNext(setDone());
            windowp->addStmtsp(جدید AstIf{flp, exprp->clone_درخت_Pure(نادرست), failsp});
            // if (rhs) { pass; done = 1; } -- consequent true at this !expr endpoint
            Ast_گره* ثابت passsp = جدید AstPExprClause{flp, درست};
            passsp->addNext(setDone());
            windowp->addStmtsp(جدید AstIf{flp, rhsp, passsp});
            // @(clk) -- advance to next cycle in window
            windowp->addStmtsp(
                جدید AstEventControl{flp, جدید Ast_Sen_درخت{flp, sensesp->clone_درخت(نادرست)}, nullptr});
            beginp->addStmtsp(windowp);
        } وگرنه اگر (isNonConsec) {
            // Standalone nonconsec: ##1 into window, then pass
            beginp->addStmtsp(
                جدید AstEventControl{flp, جدید Ast_Sen_درخت{flp, sensesp->clone_درخت(نادرست)}, nullptr});
            beginp->addStmtsp(جدید AstPExprClause{flp, درست});
        } وگرنه اگر (rhsp) {
            // Goto rep: check consequent once at match endpoint
            اگر (!isOverlapped) {
                beginp->addStmtsp(جدید AstEventControl{
                    flp, جدید Ast_Sen_درخت{flp, sensesp->clone_درخت(نادرست)}, nullptr});
            }
            beginp->addStmtsp(جدید AstIf{flp, rhsp, جدید AstPExprClause{flp, درست},
                                        جدید AstPExprClause{flp, نادرست}});
        } وگرنه {
            // Standalone goto: pass after counting
            beginp->addStmtsp(جدید AstPExprClause{flp, درست});
        }

        بازگشت جدید AstPExpr{flp, beginp, exprp->findBitDType()};
    }

    // Reject [=M:N] nonconsecutive range that reached V3AssertPre.
    // [->M:N] is fully handled upstream (NFA path); only [=M:N] still lands
    // here because AstSNonConsRep lowering is not yet implemented there.
    // Replaces the node with BitFalse so lowering continues after the warning.
    منطقی rejectNonconsecRange(Ast_گره* nodep, Ast_گره_عبارت* maxCountp) {
        اگر (!maxCountp) بازگشت نادرست;
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: [=M:N] nonconsecutive range repetition"
                                     " (IEEE 1800-2023 16.9.2)");
        nodep->replaceWith(جدید AstConst{nodep->fileline(), AstConst::BitFalse{}});
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        بازگشت درست;
    }

    تهی visit(AstSGotoRep* nodep) override {
        // Standalone goto rep (not inside implication antecedent)
        iterateChildren(nodep);
        File_سطر* ثابت flp = nodep->fileline();
        Ast_گره_عبارت* countp = nodep->countp()->unlinkFrBack();
        اگر (!validate_Rep_شمار(nodep, countp)) بازگشت;
        Ast_گره_عبارت* ثابت exprp = nodep->exprp()->unlinkFrBack();
        AstPExpr* ثابت pexprp = createRepPExpr(flp, exprp, countp, nullptr, درست, نادرست);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    تهی visit(AstSNonConsRep* nodep) override {
        // Standalone nonconsecutive rep (not inside implication antecedent)
        iterateChildren(nodep);
        اگر (rejectNonconsecRange(nodep, nodep->maxCountp())) بازگشت;
        File_سطر* ثابت flp = nodep->fileline();
        Ast_گره_عبارت* countp = nodep->countp()->unlinkFrBack();
        اگر (!validate_Rep_شمار(nodep, countp)) بازگشت;
        Ast_گره_عبارت* ثابت exprp = nodep->exprp()->unlinkFrBack();
        AstPExpr* ثابت pexprp = createRepPExpr(flp, exprp, countp, nullptr, درست, درست);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    تهی visit(AstFuncRef* nodep) override {
        // IEEE 1800-2023 16.8: Inline sequence instances wherever they appear
        // in the expression tree (inside implications, boolean ops, nested refs, etc.)
        اگر (AstSequence* ثابت seqp = VN_CAST(nodep->taskp(), Sequence)) {
            substituteSequenceCall(nodep, seqp);
            // The FuncRef has been replaced; do not access nodep after this point.
            // The replacement node will be visited by the parent's iterateChildren.
            بازگشت;
        }
        اگر (ثابت AstProperty* ثابت propp = VN_CAST(nodep->taskp(), Property)) {
            substituteNestedPropertyCall(nodep, propp);
            بازگشت;
        }
        iterateChildren(nodep);
    }
    تهی visit(AstImplication* nodep) override {
        اگر (nodep->sentreep()) بازگشت;  // Already processed

        // Top-level followed-by is claimed by V3AssertNfa; anything reaching here
        // is nested inside iff/implies/or. IEEE 1800-2023 16.12.9 permits the
        // composition, but silent lowering would drop non-vacuous-fail semantics.
        اگر (nodep->isFollowedBy()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: followed-by (#-# / #=#) nested inside property operator"
                          " (iff/implies/or) (IEEE 1800-2023 16.12.9)");
            nodep->replaceWith(جدید AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت;
        }

        // Handle goto repetition as antecedent before iterateChildren,
        // so the standalone AstSGotoRep visitor doesn't process it
        اگر (AstSGotoRep* ثابت gotop = VN_CAST(nodep->lhsp(), SGotoRep)) {
            iterateChildren(gotop);
            iterateAndNextNull(nodep->rhsp());
            File_سطر* ثابت flp = nodep->fileline();
            Ast_گره_عبارت* countp = gotop->countp()->unlinkFrBack();
            اگر (!validate_Rep_شمار(nodep, countp)) بازگشت;
            Ast_گره_عبارت* ثابت exprp = gotop->exprp()->unlinkFrBack();
            Ast_گره_عبارت* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
            AstPExpr* ثابت pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), نادرست);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت;
        }

        // Handle nonconsecutive repetition as antecedent before iterateChildren,
        // so the standalone AstSNonConsRep visitor doesn't process it
        اگر (AstSNonConsRep* ثابت ncrp = VN_CAST(nodep->lhsp(), SNonConsRep)) {
            iterateChildren(ncrp);
            iterateAndNextNull(nodep->rhsp());
            اگر (rejectNonconsecRange(nodep, ncrp->maxCountp())) بازگشت;
            File_سطر* ثابت flp = nodep->fileline();
            Ast_گره_عبارت* countp = ncrp->countp()->unlinkFrBack();
            اگر (!validate_Rep_شمار(nodep, countp)) بازگشت;
            Ast_گره_عبارت* ثابت exprp = ncrp->exprp()->unlinkFrBack();
            Ast_گره_عبارت* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
            AstPExpr* ثابت pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), درست);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت;
        }

        iterateChildren(nodep);

        File_سطر* ثابت flp = nodep->fileline();
        Ast_گره_عبارت* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_گره_عبارت* lhsp = nodep->lhsp()->unlinkFrBack();

        // Lower sequence match items (IEEE 16.11): (expr, var = val, ...) |-> / |=>
        اگر (Ast_عبارت_Stmt* ثابت عبارت_Stmtp = VN_CAST(lhsp, عبارت_Stmt)) {
            Ast_گره_عبارت* ثابت antExprp = عبارت_Stmtp->resultp()->unlinkFrBack();

            اگر (nodep->isOverlapped()) {
                // |-> : assign to __Vpropvar via always_comb (continuous).
                // The assign evaluates RHS once; V3Sampled snapshots the
                // result so all consequent refs read the same value.
                Ast_گره* matchAssignsp = nullptr;
                برای (Ast_گره* stmtp = عبارت_Stmtp->stmtsp(); stmtp;) {
                    Ast_گره* ثابت nextp = stmtp->nextp();
                    اگر (AstAssign* ثابت assignp = VN_CAST(stmtp, Assign)) {
                        assignp->unlinkFrBack();
                        matchAssignsp = Ast_گره::addNextNull(matchAssignsp, assignp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(افزودن_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                اگر (matchAssignsp) {
                    AstAlways* ثابت alwaysp
                        = جدید AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, matchAssignsp};
                    m_modp->addStmtsp(alwaysp);
                }
            } وگرنه {
                // |=> : assign to __Vpropvar via NBA in a clocked always block.
                // The NBA commits before the next cycle's sampled snapshot,
                // so the consequent (which already references __Vpropvar)
                // sees the captured value.
                Ast_گره* matchAssignsp = nullptr;
                برای (Ast_گره* stmtp = عبارت_Stmtp->stmtsp(); stmtp;) {
                    Ast_گره* ثابت nextp = stmtp->nextp();
                    اگر (AstAssign* ثابت assignp = VN_CAST(stmtp, Assign)) {
                        assignp->unlinkFrBack();
                        Ast_گره_عبارت* ثابت assignLhsp = assignp->lhsp()->unlinkFrBack();
                        Ast_گره_عبارت* ثابت assignRhsp = assignp->rhsp()->unlinkFrBack();
                        AstAssignDly* ثابت dlyp = جدید AstAssignDly{flp, assignLhsp, assignRhsp};
                        VL_DO_DANGLING(افزودن_Deletep(assignp), assignp);
                        matchAssignsp = Ast_گره::addNextNull(matchAssignsp, dlyp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(افزودن_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                اگر (matchAssignsp) {
                    AstIf* ثابت condp
                        = جدید AstIf{flp, antExprp->clone_درخت_Pure(نادرست), matchAssignsp};
                    AstAlways* ثابت alwaysp
                        = جدید AstAlways{flp, VAlwaysKwd::ALWAYS, new_Sen_درخت(nodep), condp};
                    m_modp->addStmtsp(alwaysp);
                }
            }
        }

        اگر (AstPExpr* ثابت pexprp = VN_CAST(rhsp, PExpr)) {
            // Implication with sequence expression on RHS (IEEE 1800-2023 16.11, 16.12.7).
            // The PExpr was lowered from the property expression earlier in this pass.
            // Wrap the PExpr body with the antecedent check so the sequence only
            // starts when the antecedent holds.
            Ast_گره_عبارت* condp;
            اگر (nodep->isOverlapped()) {
                // Overlapped implication (|->): check antecedent on same cycle.
                // disable iff is applied at the assertion level, not at the
                // antecedent gate, matching the existing non-PExpr overlapped path.
                condp = lhsp;
            } وگرنه {
                // Non-overlapped implication (|=>): check antecedent from previous cycle
                اگر (m_disablep) {
                    lhsp
                        = جدید AstAnd{flp, جدید AstNot{flp, m_disablep->clone_درخت_Pure(نادرست)}, lhsp};
                }
                AstPast* ثابت pastp
                    = جدید AstPast{flp, lhsp, nullptr, nullptr, /* propertyTiming */ درست};
                pastp->dtypeFrom(lhsp);
                pastp->sentreep(new_Sen_درخت(nodep));
                condp = pastp;
            }
            // Wrap existing PExpr body: if (antecedent) { <original body> } else { /* vacuous pass
            // */ }
            // Only the statements are guarded; declarations stay directly in the block. After
            // V3Fork moves the block's statements into a task, V3Task only handles variables
            // that are direct statements of the task.
            AstBegin* ثابت bodyp = pexprp->bodyp();
            AstIf* ثابت guardp = جدید AstIf{flp, condp};
            برای (Ast_گره* stmtp = bodyp->stmtsp(); stmtp;) {
                Ast_گره* ثابت nextp = stmtp->nextp();
                اگر (!VN_IS(stmtp, Var)) guardp->addThensp(stmtp->unlinkFrBack());
                stmtp = nextp;
            }
            bodyp->addStmtsp(guardp);
            nodep->replaceWith(pexprp);
            // Don't iterate pexprp here -- it was already iterated when created
            // (in visit(AstSExpr*)), so delays and disable iff are already processed.
        } وگرنه اگر (nodep->isOverlapped()) {
            Ast_گره_عبارت* ثابت exprp
                = m_underCover ? static_cast<Ast_گره_عبارت*>(جدید AstLogAnd{flp, lhsp, rhsp})
                               : جدید AstLogOr{flp, جدید AstLogNot{flp, lhsp}, rhsp};
            nodep->replaceWith(exprp);
        } وگرنه {
            اگر (m_disablep) {
                lhsp = جدید AstAnd{flp, جدید AstNot{flp, m_disablep->clone_درخت_Pure(نادرست)}, lhsp};
            }

            AstPast* ثابت pastp
                = جدید AstPast{flp, lhsp, nullptr, nullptr, /* propertyTiming */ درست};
            pastp->dtypeFrom(lhsp);
            pastp->sentreep(new_Sen_درخت(nodep));
            Ast_گره_عبارت* ثابت exprp
                = m_underCover ? static_cast<Ast_گره_عبارت*>(جدید AstAnd{flp, pastp, rhsp})
                               : جدید AstOr{flp, جدید AstNot{flp, pastp}, rhsp};
            exprp->dtypeSetBit();
            nodep->replaceWith(exprp);
        }
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(AstUntil* nodep) override {
        File_سطر* ثابت flp = nodep->fileline();
        UASSERT_OBJ(
            !m_pexprp, nodep,
            "'" << nodep->verilogKwd()
                << "' in complex property expression should have been rejected by V3AssertNfa");
        اگر (nodep->isStrong()
            && (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue())) {
            nodep->v3warn(E_NOTIMING, nodep->verilogKwd() << " requires --timing");
            nodep->replaceWith(جدید AstConst{flp, AstConst::BitFalse{}});
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت;
        }
        اگر (nodep->isStrong()) {
            // p s_until q / p s_until_with q: q must eventually be true. Until then, p must
            // be true on every sampled tick. For s_until, check q first: when q is true on
            // this tick, p is not required. For s_until_with, p must be true on the q tick too.
            Ast_گره_عبارت* ثابت rawLhsp = nodep->lhsp()->unlinkFrBack();
            Ast_گره_عبارت* ثابت rawRhsp = nodep->rhsp()->unlinkFrBack();
            AstSampled* ثابت lhsp = جدید AstSampled{flp, rawLhsp, rawLhsp->dtypep(), درست};
            AstSampled* ثابت rhsp = جدید AstSampled{flp, rawRhsp, rawRhsp->dtypep(), درست};
            Ast_گره_عبارت* finalCondp = rhsp->clone_درخت_Pure(نادرست);
            اگر (nodep->isOverlapping()) {
                finalCondp = جدید AstLogAnd{flp, lhsp->clone_درخت_Pure(نادرست), finalCondp};
            }

            // Track active assertion attempts. Only the count is needed to emit final failures.
            AstVar* ثابت activep = جدید AstVar{flp, VVarType::MODULETEMP, m_activeNames.get(""),
                                               nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            activep->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(activep);

            AstVar* ثابت donep
                = جدید AstVar{flp, VVarType::BLOCKTEMP, "__VassertDone", nodep->findBitDType()};
            donep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            auto setDone = [&]() {
                بازگشت جدید AstAssign{flp, جدید AstVarRef{flp, donep, VAccess::WRITE},
                                     جدید AstConst{flp, AstConst::BitTrue{}}};
            };

            AstLoop* ثابت loopp = جدید AstLoop{flp};
            AstAssign* ثابت decrementVar = جدید AstAssign{
                flp, جدید AstVarRef{flp, activep, VAccess::WRITE},
                جدید AstSub{flp, جدید AstVarRef{flp, activep, VAccess::READ}, جدید AstConst{flp, 1}}};
            loopp->addStmtsp(جدید AstLoopTest{
                flp, loopp, جدید AstLogNot{flp, جدید AstVarRef{flp, donep, VAccess::READ}}});
            {
                Ast_گره* ثابت passp = جدید AstPExprClause{flp};
                passp->addNext(decrementVar);
                passp->addNext(setDone());
                Ast_گره_عبارت* passCondp = rhsp;
                اگر (nodep->isOverlapping()) {
                    passCondp = جدید AstLogAnd{flp, lhsp->clone_درخت_Pure(نادرست), passCondp};
                }
                loopp->addStmtsp(جدید AstIf{flp, passCondp, passp});
            }
            {
                Ast_گره* ثابت failp = جدید AstPExprClause{flp, نادرست};
                failp->addNext(decrementVar->clone_درخت(نادرست));
                failp->addNext(setDone());
                loopp->addStmtsp(جدید AstIf{
                    flp,
                    جدید AstLogAnd{flp,
                                  جدید AstLogNot{flp, جدید AstVarRef{flp, donep, VAccess::READ}},
                                  جدید AstLogNot{flp, lhsp}},
                    failp});
            }
            AstDelay* ثابت delayp = جدید AstDelay{flp, جدید AstConst{flp, 1}, نادرست};
            delayp->timeunit(m_modp->timeunit());
            loopp->addStmtsp(جدید AstIf{
                flp, جدید AstLogNot{flp, جدید AstVarRef{flp, donep, VAccess::READ}}, delayp});

            // Main assertion block
            AstBegin* ثابت bodyp = جدید AstBegin{flp, m_blockNames.get(""), nullptr, درست};
            bodyp->addStmtsp(donep);
            bodyp->addStmtsp(جدید AstAssign{flp, جدید AstVarRef{flp, donep, VAccess::WRITE},
                                           جدید AstConst{flp, AstConst::BitFalse{}}});
            File_سطر* ثابت flp = activep->fileline();
            bodyp->addStmtsp(
                جدید AstAssign{flp, جدید AstVarRef{flp, activep, VAccess::WRITE},
                              جدید AstAdd{flp, جدید AstVarRef{flp, activep, VAccess::READ},
                                         جدید AstConst{flp, 1}}});
            bodyp->addStmtsp(loopp);

            // Validate assertion condition for each active assert
            AstVar* ثابت activeCountp = جدید AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                    nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            activeCountp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

            AstAssign* ثابت initActiveCountp
                = جدید AstAssign{flp, جدید AstVarRef{flp, activeCountp, VAccess::WRITE},
                                جدید AstVarRef{flp, activep, VAccess::READ}};
            AstLoop* ثابت finalLoopp = جدید AstLoop{flp};
            finalLoopp->addStmtsp(
                جدید AstLoopTest{flp, finalLoopp,
                                جدید AstNeq{flp, جدید AstVarRef{flp, activeCountp, VAccess::READ},
                                           جدید AstConst{flp, 0}}});
            finalLoopp->addStmtsp(جدید AstIf{flp, finalCondp, جدید AstPExprClause{flp},
                                            جدید AstPExprClause{flp, نادرست}});
            finalLoopp->addStmtsp(
                جدید AstAssign{flp, جدید AstVarRef{flp, activeCountp, VAccess::WRITE},
                              جدید AstSub{flp, جدید AstVarRef{flp, activeCountp, VAccess::READ},
                                         جدید AstConst{flp, 1}}});

            // Final assertion block
            AstBegin* ثابت finalp = جدید AstBegin{flp, m_blockNames.get(""), nullptr, درست};
            finalp->addStmtsp(activeCountp);
            finalp->addStmtsp(initActiveCountp);
            finalp->addStmtsp(finalLoopp);

            AstPExpr* ثابت pexprp = جدید AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
            VL_RESTORER(m_pexprp);
            VL_RESTORER(m_hasCycleDelay);
            m_pexprp = pexprp;
            m_hasCycleDelay = نادرست;
            iterate(bodyp);
            iterate(finalp);

            UASSERT_OBJ(!m_hasCycleDelay, nodep,
                        "s_until cycle delay should have been handled by V3AssertNfa");

            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت;
        }
        AstLoop* ثابت loopp = جدید AstLoop{flp};
        Ast_گره_عبارت* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_گره_عبارت* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
        AstLogAnd* ثابت loopCondp = جدید AstLogAnd{flp, lhsp, جدید AstLogNot{flp, rhsp}};
        loopp->addStmtsp(جدید AstLoopTest{flp, loopp, loopCondp});
        loopp->addStmtsp(جدید AstEventControl{flp, new_Sen_درخت(nodep), nullptr});

        Ast_گره_عبارت* ثابت rhsCopyp = rhsp->clone_درخت_Pure(نادرست);
        Ast_گره_عبارت* ثابت passCondp
            = nodep->isOverlapping() ? جدید AstLogAnd{flp, lhsp->clone_درخت_Pure(نادرست), rhsCopyp}
                                     : rhsCopyp;
        AstBegin* ثابت beginp = جدید AstBegin{flp, m_blockNames.get(""), loopp, درست};
        beginp->addStmtsp(
            جدید AstIf{flp, passCondp, جدید AstPExprClause{flp}, جدید AstPExprClause{flp, نادرست}});

        AstPExpr* ثابت pexprp = جدید AstPExpr{flp, beginp, nodep->dtypep()};
        pexprp->user1(1);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    تهی visit(AstDefaultDisable* nodep) override {
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
    }
    تهی visit(AstInferredDisable* nodep) override {
        Ast_گره* newp;
        اگر (m_defaultDisablep) {
            newp = m_defaultDisablep->condp()->clone_درخت_Pure(درست);
        } وگرنه {
            newp = جدید AstConst{nodep->fileline(), AstConst::BitFalse{}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    تهی visit(AstPropSpec* nodep) override {
        nodep = substitutePropertyCall(nodep);
        iterateAndNextNull(nodep->sensesp());
        اگر (m_senip && m_senip != nodep->sensesp())
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Only one PSL clock allowed per assertion");
        ثابت AstCover* ثابت coverp = VN_CAST(nodep->backp(), Cover);
        ثابت منطقی seqEvent = coverp && coverp->isSeqEvent();
        اگر (!nodep->disablep() && m_defaultDisablep && !seqEvent) {
            nodep->disablep(m_defaultDisablep->condp()->clone_درخت_Pure(درست));
        }
        m_disablep = nodep->disablep();
        // Unlink and just keep a pointer to it, convert to sentree as needed
        m_senip = nodep->sensesp();
        iterateNull(nodep->disablep());
        اگر (!VN_AS(nodep->backp(), گره_Cover_Or_Assert)->immediate()) {
            ثابت Ast_گره_DType* ثابت propDtp = nodep->propp()->dtypep();
            nodep->propp(جدید AstSampled{nodep->fileline(), nodep->propp()->unlinkFrBack(),
                                        propDtp->dtypep(), درست});
        }
        // cover counts non-vacuous matches only (IEEE 1800-2023 16.15.2), so an
        // implication antecedent must hold; assert passes vacuously instead.
        VL_RESTORER(m_underCover);
        m_underCover = VN_IS(nodep->backp(), Cover);
        iterate(nodep->propp());
        iterateNull(nodep->matchCountp());
    }
    تهی visit(AstPExpr* nodep) override {
        // V3AssertNfa handles multi-cycle property expressions before this pass,
        // so the following unsupported paths are defensive and typically unreached.
        اگر (m_pexprp && m_pexprp->user1()) {  // LCOV_EXCL_START
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Complex property expression inside 'until''");
            nodep->replaceWith(جدید AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت;
        }  // LCOV_EXCL_STOP
        اگر (AstLogNot* ثابت notp = VN_CAST(nodep->backp(), LogNot)) {
            notp->replaceWith(nodep->unlinkFrBack());
            VL_DO_DANGLING(افزودن_Deletep(notp), notp);
            iterate(nodep);
            بازگشت;
        }
        // Sequence expression as antecedent of implication is not yet supported
        اگر (AstImplication* ثابت implp = VN_CAST(nodep->backp(), Implication)) {
            اگر (implp->lhsp() == nodep) {  // LCOV_EXCL_START
                implp->v3warn(E_UNSUPPORTED,
                              "Unsupported: Implication with sequence expression as antecedent");
                nodep->replaceWith(جدید AstConst{nodep->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                بازگشت;
            }  // LCOV_EXCL_STOP
        }
        VL_RESTORER(m_pexprp);
        VL_RESTORER(m_disableSeqIfp);
        m_pexprp = nodep;

        اگر (m_disablep) {
            ثابت AstSampled* sampledp = nullptr;
            اگر (m_disablep->exists([&sampledp](ثابت AstSampled* ثابت sp) {
                    sampledp = sp;
                    بازگشت درست;
                })) {
                sampledp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: $sampled inside disabled condition of a sequence");
                m_disablep = جدید AstConst{m_disablep->fileline(), AstConst::BitFalse{}};
                // always a copy is used, so remove it now
                افزودن_Deletep(m_disablep);  // m_disablep used later
            }
            File_سطر* ثابت flp = nodep->fileline();
            // Add counter which counts times the condition turned true
            AstVar* ثابت disableCntp
                = جدید AstVar{flp, VVarType::MODULETEMP, m_disableCntNames.get(""),
                             nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            disableCntp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(disableCntp);
            AstVarRef* ثابت readCntRefp = جدید AstVarRef{flp, disableCntp, VAccess::READ};
            AstVarRef* ثابت writeCntRefp = جدید AstVarRef{flp, disableCntp, VAccess::WRITE};
            AstAssign* ثابت incrStmtp = جدید AstAssign{
                flp, writeCntRefp, جدید AstAdd{flp, readCntRefp, جدید AstConst{flp, 1}}};
            AstAlways* ثابت alwaysp
                = جدید AstAlways{flp, VAlwaysKwd::ALWAYS,
                                جدید Ast_Sen_درخت{flp, جدید AstSenItem{flp, VEdgeType::ET_POSEDGE,
                                                                   m_disablep->clone_درخت(نادرست)}},
                                incrStmtp};
            disableCntp->addNextHere(alwaysp);

            // Store value of that counter at the beginning of sequence evaluation
            AstBegin* ثابت bodyp = nodep->bodyp();
            AstVar* ثابت initialCntp = جدید AstVar{flp, VVarType::BLOCKTEMP, "__VinitialCnt",
                                                   nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            initialCntp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            AstAssign* ثابت assignp
                = جدید AstAssign{flp, جدید AstVarRef{flp, initialCntp, VAccess::WRITE},
                                readCntRefp->clone_درخت(نادرست)};
            // Prepend to the sequence body to keep statement list structure valid.
            UASSERT_OBJ(!bodyp->name().empty(), bodyp, "Sequence body should be named");
            Ast_گره* ثابت newStmtsp = Ast_گره::addNext<Ast_گره, Ast_گره>(initialCntp, assignp);
            bodyp->prependStmtsp(newStmtsp);

            m_disableSeqIfp
                = جدید AstIf{flp, جدید AstEq{flp, جدید AstVarRef{flp, initialCntp, VAccess::READ},
                                           readCntRefp->clone_درخت(نادرست)}};
            // Delete it, because it is always copied before insetion to the AST
            افزودن_Deletep(m_disableSeqIfp);  // m_disableSeqIfp used later
        }
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultClkEvtVarp);
        VL_RESTORER(m_defaultDisablep);
        VL_RESTORER(m_modp);
        اگر (!VN_IS(nodep, Class)) {
            // A class nested in a module is in the scope of the module's default clocking
            m_defaultClockingp = nullptr;
            m_defaultClkEvtVarp = nullptr;
        }
        m_defaultDisablep = nodep->defaultDisablep();
        m_modp = nodep;
        iterateChildren(nodep);
    }
    تهی visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    تهی visit(AstProperty* nodep) override {
        // The body will be visited when will be substituted in place of property reference
        // (AstFuncRef)
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
    }
    تهی visit(AstSequence* nodep) override {
        // Sequence declarations are not visited directly; their bodies are inlined
        // at call sites by visit(AstFuncRef*). Collect for post-traversal cleanup.
        m_seqsToCleanup.افزودن_back(nodep);
    }
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit AssertPreVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        { AssertPreClockingVisitor{nodep}; }
        // Process
        iterate(nodep);
        // Fix up varref names
        برای (AstVarXRef* xrefp : m_xrefsp) xrefp->name(xrefp->varp()->name());
        // Clean up sequence declarations after inlining.
        // Referenced sequences that were inlined have isReferenced cleared.
        // Remaining referenced sequences are in unsupported contexts (e.g. @seq event).
        برای (AstSequence* seqp : m_seqsToCleanup) {
            اگر (seqp->isReferenced()) {
                seqp->v3warn(E_UNSUPPORTED,
                             "Unsupported: sequence referenced outside assertion property");
            } وگرنه {
                VL_DO_DANGLING(seqp->unlinkFrBack()->delete_درخت(), seqp);
            }
        }
    }
    ~AssertPreVisitor() override = پیشفرض;
};

//######################################################################
// Top Assert class

تهی V3AssertPre::assertPreAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { AssertPreVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("assertpre", 0, dump_درخت_Either_Level() >= 3);
}
