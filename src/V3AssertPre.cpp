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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3AssertPre.h"

#অন্তর্ভাব "V3Assert.h"
#অন্তর্ভাব "V3Const.h"
#অন্তর্ভাব "V3Task.h"
#অন্তর্ভাব "V3Timing.h"
#অন্তর্ভাব "V3UniqueNames.h"

#অন্তর্ভাব <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clocking blocks of clocking items

বর্গ AssertPreClockingVisitor final : সার্বজনিক VNVisitorConst {
    // Captures the clocking block of each clocking item before AssertPreVisitor, which needs it
    // at synchronous drives, often visited before the clocking block, e.g. in a parent module
    // NODE STATE
    // AstClockingItem::user3p()  // AstClocking*. Clocking block of the item, user3 claimed by
    //                            //               AssertPreVisitor

    // VISITORS
    শূন্য visit(AstNetlist* nodep) override { iterateAndNextConstNull(nodep->modulesp()); }
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override { iterateAndNextConstNull(nodep->stmtsp()); }
    শূন্য visit(AstGenBlock* nodep) override { iterateAndNextConstNull(nodep->itemsp()); }
    শূন্য visit(AstClocking* nodep) override {
        প্রতি (Ast_গ্রন্থি* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            যদি (AstClockingItem* ধ্রুব citemp = VN_CAST(itemp, ClockingItem)) {
                citemp->user3p(nodep);
            }
        }
    }
    // Clocking blocks are only in modules and their generate blocks
    শূন্য visit(Ast_গ্রন্থি*) override {}

সার্বজনিক:
    // CONSTRUCTORS
    explicit AssertPreClockingVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~AssertPreClockingVisitor() override = স্বভাব;
};

//######################################################################
// Assert class functions

বর্গ AssertPreVisitor final : সার্বজনিক VNVisitor {
    // Removes clocks and other pre-optimizations
    // Eventually inlines calls to sequences, properties, etc.
    // We're not parsing the tree, or anything more complicated.
গুপ্ত:
    // TYPES
    সংরচনা SynchDrive final {
        AstClockingItem* itemp;  // Clocking item of the driven clockvar
        Ast_গ্রন্থি_রাশিমালা* refp;  // Reference to the driven clockvar
    };

    // NODE STATE
    // AstClockingItem::user1p()         // AstVar*.      varp() of ClockingItem after unlink
    // AstClockingItem::user2p()         // AstVar*.      Flag set by drives of output clockvar
    // AstClockingItem::user3p()         // AstClocking*. Clocking block of the item, captured
    //                                   //               by AssertPreClockingVisitor
    // AstPExpr::user1()                 // bool.         Created from AstUntil
    ধ্রুব VNUser1InUse m_inuser1;
    ধ্রুব VNUser2InUse m_inuser2;
    ধ্রুব VNUser3InUse m_inuser3;
    // STATE
    // Current context:
    AstNetlist* ধ্রুব m_netlistp = nullptr;  // Current netlist
    Ast_গ্রন্থি_Module* m_modp = nullptr;  // Current module
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
    Ast_গ্রন্থি_রাশিমালা* m_disablep = nullptr;  // Last disable
    AstIf* m_disableSeqIfp = nullptr;  // Used for handling disable iff in sequences
    AstPExpr* m_pexprp = nullptr;  // Last AstPExpr
    তার্কিক m_underCover = অসত্য;  // True if the enclosing assertion is a cover
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
    তার্কিক m_inAssign = অসত্য;  // True if in an AssignNode
    তার্কিক m_inAssignDlyLhs = অসত্য;  // True if in AssignDly's LHS
    তার্কিক m_inSynchDrive = অসত্য;  // True if in synchronous drive
    তার্কিক m_inConcatAssign = অসত্য;  // True if in assignment to a concatenation
    std::vector<SynchDrive> m_drives;  // Clockvars written by the synchronous drive
    তার্কিক m_hasCycleDelay = অসত্য;  // True if node has cycle delay beneath
    std::vector<AstVarXRef*> m_xrefsp;  // list of xrefs that need name fixup
    std::vector<AstSequence*> m_seqsToCleanup;  // Sequences to clean up after traversal

    // METHODS

    // Flag set by each drive of an output clockvar, added to the module with its clocking item
    AstVar* getCreateDrivenVarp(AstClockingItem* itemp) {
        যদি (!itemp->user2p()) {
            AstVar* ধ্রুব varp = নূতন AstVar{itemp->fileline(), VVarType::MODULETEMP,
                                            m_drivenNames.get(itemp), itemp->findBitDType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            itemp->user2p(varp);
        }
        প্রত্যাগম VN_AS(itemp->user2p(), Var);
    }
    // Reference to a variable in the scope of a driven clockvar, made like the clockvar's
    স্থির Ast_গ্রন্থি_রাশিমালা* newDriveRefp(File_সারি* flp, ধ্রুব SynchDrive& drive, AstVar* varp,
                                     ধ্রুব VAccess& access) {
        যদি (ধ্রুব AstVarXRef* ধ্রুব xrefp = VN_CAST(drive.refp, VarXRef)) {
            প্রত্যাগম নূতন AstVarXRef{flp, varp, xrefp->dotted(), access};
        } অন্যথা যদি (ধ্রুব AstMemberSel* ধ্রুব selp = VN_CAST(drive.refp, MemberSel)) {
            // The interface expression is evaluated again, so it must not have side effects;
            // cloneTreePure warns if it has any
            AstMemberSel* ধ্রুব newSelp
                = নূতন AstMemberSel{flp, selp->fromp()->clone_বৃক্ষ_Pure(অসত্য), varp};
            newSelp->access(access);
            প্রত্যাগম newSelp;
        }
        প্রত্যাগম নূতন AstVarRef{flp, varp, access};
    }
    // Assignment setting the drive flag of a driven clockvar
    AstAssign* newDrivenSetp(File_সারি* flp, ধ্রুব SynchDrive& drive) {
        AstVar* ধ্রুব varp = getCreateDrivenVarp(drive.itemp);
        AstAssign* ধ্রুব setp = নূতন AstAssign{flp, newDriveRefp(flp, drive, varp, VAccess::WRITE),
                                              নূতন AstConst{flp, AstConst::BitTrue{}}};
        setp->user1(সত্য);
        প্রত্যাগম setp;
    }
    // Clocking block of a clocking item
    স্থির AstClocking* clockingOf(ধ্রুব AstClockingItem* itemp) {
        UASSERT_OBJ(itemp->user3p(), itemp, "Clocking item not captured in a clocking block");
        প্রত্যাগম VN_AS(itemp->user3p(), Clocking);
    }
    // Clocking event of a driven clockvar, referenced from the drive like the clockvar, or
    // nullptr if unsupported, which is reported on the drive's cycle delay
    Ast_Sen_বৃক্ষ* newDriveSentreep(AstDelay* delayp, ধ্রুব SynchDrive& drive) {
        Ast_Sen_বৃক্ষ* ধ্রুব origp = clockingOf(drive.itemp)->sentreep();
        যদি (VN_IS(drive.refp, VarRef)) প্রত্যাগম origp->clone_বৃক্ষ(অসত্য);
        যদি (origp->exists([](ধ্রুব AstVarXRef*) { প্রত্যাগম সত্য; })) {
            // The reference is relative to the clocking block
            delayp->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in synchronous drive to"
                                          " clockvar in another scope, whose clocking event"
                                          " has a hierarchical reference");
            প্রত্যাগম nullptr;
        }
        ধ্রুব AstMemberSel* ধ্রুব selp = VN_CAST(drive.refp, MemberSel);
        // Processes outside classes evaluate their events also when not waiting on them,
        // possibly before the interface reference is set, so the event is guarded against a
        // null reference, which needs an integral value; scheduling does not guard it
        ধ্রুব তার্কিক guardNull = selp && !VN_IS(m_modp, Class);
        যদি (guardNull) {
            প্রতি (AstSenItem* itemp = origp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                যদি (!itemp->sensp()->dtypep()->skipRefp()->isIntegralOrPacked()) {
                    delayp->v3warn(E_UNSUPPORTED,
                                   "Unsupported: cycle delay in synchronous drive through"
                                   " an interface reference outside a class, to clocking"
                                   " block with non-integral clocking event");
                    প্রত্যাগম nullptr;
                }
            }
        }
        // Interface declaring the clocking block, if driven through an interface reference
        AstIface* ধ্রুব ifacep
            = selp ? VN_AS(selp->fromp()->dtypep()->skipRefp(), IfaceRefDType)->ifaceViaCellp()
                   : nullptr;
        Ast_Sen_বৃক্ষ* ধ্রুব sentreep = origp->clone_বৃক্ষ(অসত্য);
        std::vector<AstVarRef*> refps;
        sentreep->foreach([&](AstVarRef* refp) {
            যদি (!refp->classOrPackagep()) refps.ঠেলো_back(refp);
        });
        প্রতি (AstVarRef* ধ্রুব refp : refps) {
            AstVar* ধ্রুব varp = refp->varp();
            যদি (ifacep) {
                // Mark the variable as read through an interface reference, as V3Width does for
                // a member select. The mark is the interface declaring the variable, whose
                // instances all share it, so drives through references to any instances agree.
                UASSERT_OBJ(!varp->sensIfacep() || varp->sensIfacep() == ifacep, refp,
                            "Variable read through references to different interfaces");
                varp->sensIfacep(ifacep);
            }
            refp->replaceWith(newDriveRefp(refp->fileline(), drive, varp, refp->access()));
            VL_DO_DANGLING(ঠেলো_Deletep(refp), refp);
        }
        যদি (guardNull) {
            প্রতি (AstSenItem* itemp = sentreep->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                guardNullIface(itemp, selp->fromp());
            }
        }
        প্রত্যাগম sentreep;
    }
    // Make an integral clocking event through an interface reference false while the reference
    // is null
    স্থির শূন্য guardNullIface(AstSenItem* sensesp, Ast_গ্রন্থি_রাশিমালা* fromp) {
        File_সারি* ধ্রুব flp = sensesp->fileline();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব sensp = sensesp->sensp()->unlinkFrBack();
        sensesp->sensp(নূতন AstCond{flp, newNotNullp(flp, fromp), sensp,
                                   নূতন AstConst{flp, AstConst::DTyped{}, sensp->dtypep()}});
        যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুব condp = sensesp->condp()) {
            condp->unlinkFrBack();
            sensesp->condp(নূতন AstLogAnd{flp, newNotNullp(flp, fromp), condp});
        }
    }
    স্থির Ast_গ্রন্থি_রাশিমালা* newNotNullp(File_সারি* flp, Ast_গ্রন্থি_রাশিমালা* fromp) {
        প্রত্যাগম নূতন AstNeq{flp, fromp->clone_বৃক্ষ_Pure(অসত্য), নূতন AstConst{flp, AstConst::Null{}}};
    }
    // Procedural '##0': if the default clocking event has not yet occurred in this time step,
    // wait for it; otherwise continue without suspension (IEEE 1800-2023 14.11)
    AstIf* newCycleDelayZerop(File_সারি* flp) ধ্রুব {
        AstVar* ধ্রুব evtVarp = m_defaultClkEvtVarp;
        UASSERT_OBJ(evtVarp, flp, "Default clocking event var not pre-created");
        AstCMethodHard* ধ্রুব isTriggeredp = নূতন AstCMethodHard{
            flp, নূতন AstVarRef{flp, evtVarp, VAccess::READ}, VCMethod::EVENT_IS_TRIGGERED};
        isTriggeredp->dtypeSetBit();
        AstEventControl* ধ্রুব waitp = নূতন AstEventControl{
            flp,
            নূতন Ast_Sen_বৃক্ষ{flp, নূতন AstSenItem{flp, VEdgeType::ET_EVENT,
                                               নূতন AstVarRef{flp, evtVarp, VAccess::READ}}},
            nullptr};
        প্রত্যাগম নূতন AstIf{flp, নূতন AstNot{flp, isTriggeredp}, waitp};
    }

    স্থির শূন্য checkSamplingFuncDType(Ast_গ্রন্থি_রাশিমালা* nodep, ধ্রুব Ast_গ্রন্থি* exprp) {
        ধ্রুব Ast_গ্রন্থি_DType* ধ্রুব dtypep = exprp->dtypep()->skipRefp();
        যদি (!dtypep->isIntegralOrPacked()) {
            nodep->v3error("Expected numeric type, but got a " << dtypep->prettyDTypeNameQ()
                                                               << " data type");
        }
    }

    Ast_Sen_বৃক্ষ* new_Sen_বৃক্ষ(Ast_গ্রন্থি* nodep, Ast_Sen_বৃক্ষ* useTreep = nullptr,
                           Ast_গ্রন্থি_Cover_Or_Assert* cassertp = nullptr) {
        // Create sentree based on clocked or default clock
        // Return nullptr for always
        যদি (useTreep) প্রত্যাগম useTreep;
        Ast_Sen_বৃক্ষ* newp = nullptr;
        AstSenItem* senip = m_senip;
        তার্কিক fromAlways = অসত্য;
        যদি (!senip && m_defaultClockingp) senip = m_defaultClockingp->sentreep()->sensesp();
        যদি (!senip) {
            senip = m_seniAlwaysp;
            fromAlways = সত্য;
        }
        যদি (!senip) {
            nodep->v3error("Concurrent assertion has no clock (IEEE 1800-2023 16.16)\n"
                           << nodep->warnMore()
                           << "... Suggest provide a clocking event, a default"
                              " clocking, or a clocked procedural context");
            newp = নূতন Ast_Sen_বৃক্ষ{nodep->fileline(), nullptr};
        } অন্যথা {
            যদি (cassertp && fromAlways) cassertp->senFromAlways(সত্য);
            newp = নূতন Ast_Sen_বৃক্ষ{nodep->fileline(), senip->clone_বৃক্ষ(সত্য)};
        }
        প্রত্যাগম newp;
    }
    Ast_গ্রন্থি_রাশিমালা* getSequenceBodyExprp(ধ্রুব AstSequence* ধ্রুব seqp) ধ্রুব {
        // The statements in AstSequence are optional AstVar (ports) followed by body expr.
        Ast_গ্রন্থি* bodyp = seqp->stmtsp();
        যাবত্ (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        প্রত্যাগম VN_CAST(bodyp, গ্রন্থি_রাশিমালা);
    }
    AstPropSpec* getPropertyExprp(ধ্রুব AstProperty* ধ্রুব propp) {
        // Statements in AstProperty: AstVar (ports/local vars),
        // AstInitialStaticStmt/AstInitialAutomaticStmt (var init), AstPropSpec (body).
        Ast_গ্রন্থি* propExprp = propp->stmtsp();
        যাবত্ (propExprp
               && (VN_IS(propExprp, Var) || VN_IS(propExprp, InitialStaticStmt)
                   || VN_IS(propExprp, InitialAutomaticStmt))) {
            propExprp = propExprp->nextp();
        }
        প্রত্যাগম VN_CAST(propExprp, PropSpec);
    }
    শূন্য substituteSequenceCall(AstFuncRef* funcrefp, AstSequence* seqp) {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Inline the sequence body at the call site, replacing the FuncRef
        Ast_গ্রন্থি_রাশিমালা* bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        // Clone the body expression since the sequence may be referenced multiple times
        Ast_গ্রন্থি_রাশিমালা* clonedp = bodyExprp->clone_বৃক্ষ(অসত্য);
        // Build substitution map, then do a single traversal to replace all formals
        // (textual substitution per IEEE 16.8.2).
        ধ্রুব V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<ধ্রুব AstVar*, Ast_গ্রন্থি_রাশিমালা*> portMap;
        প্রতি (ধ্রুব auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            ধ্রুব auto it = portMap.find(refp->varp());
            যদি (it != portMap.end()) {
                refp->replaceWith(it->second->clone_বৃক্ষ(অসত্য));
                VL_DO_DANGLING(ঠেলো_Deletep(refp), refp);
            }
        });
        প্রতি (ধ্রুব auto& tconnect : tconnects) {
            ঠেলো_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        // Replace the FuncRef with the inlined body
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(ঠেলো_Deletep(funcrefp), funcrefp);
        // Clear referenced flag; sequences with isReferenced==false are deleted in assertPreAll
        seqp->isReferenced(অসত্য);
    }
    AstPropSpec* clonePropertyBody(AstFuncRef* funcrefp, ধ্রুব AstProperty* propp) {
        AstPropSpec* propExprp = getPropertyExprp(propp);
        // Substitute inner property call before copying in order to not doing the same for
        // each call of outer property call.
        propExprp = substitutePropertyCall(propExprp);
        // Clone subtree after substitution. It is needed, because property might be called
        // multiple times with different arguments.
        propExprp = propExprp->clone_বৃক্ষ(অসত্য);
        // Build substitution maps for formal arguments and property-local
        // variables, then perform a single foreach to apply all replacements.
        // Map port vars to their actual argument expressions
        ধ্রুব V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propp->stmtsp());
        std::unordered_map<ধ্রুব AstVar*, Ast_গ্রন্থি_রাশিমালা*> portMap;
        প্রতি (ধ্রুব auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }

        // Promote property-local variables (non-port vars, IEEE 16.10) to
        // module-level __Vpropvar temps. Cross-cycle persistence is handled
        // by the match item lowering in visit(AstImplication*).
        std::unordered_map<ধ্রুব AstVar*, AstVar*> localVarMap;
        প্রতি (Ast_গ্রন্থি* stmtp = propp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (AstVar* ধ্রুব varp = VN_CAST(stmtp, Var)) {
                যদি (!varp->isIO()) {
                    ধ্রুব string newName = m_propVarNames.get(varp);
                    AstVar* ধ্রুব newVarp = নূতন AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->addStmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        // Single traversal: substitute ports and update local var references
        propExprp->foreach([&](AstVarRef* refp) {
            ধ্রুব auto portIt = portMap.find(refp->varp());
            যদি (portIt != portMap.end()) {
                refp->replaceWith(portIt->second->clone_বৃক্ষ(অসত্য));
                VL_DO_DANGLING(ঠেলো_Deletep(refp), refp);
                প্রত্যাগম;
            }
            ধ্রুব auto localIt = localVarMap.find(refp->varp());
            যদি (localIt != localVarMap.end()) refp->varp(localIt->second);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        // Clean up argument expressions
        প্রতি (ধ্রুব auto& tconnect : tconnects) {
            ঠেলো_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(propExprp, funcrefp);
        প্রত্যাগম propExprp;
    }
    AstPropSpec* substitutePropertyCall(AstPropSpec* nodep) {
        যদি (AstFuncRef* ধ্রুব funcrefp = VN_CAST(nodep->propp(), FuncRef)) {
            যদি (ধ্রুব AstProperty* ধ্রুব propp = VN_CAST(funcrefp->taskp(), Property)) {
                AstPropSpec* ধ্রুব propExprp = clonePropertyBody(funcrefp, propp);

                // Handle case with 2 disable iff statement (IEEE 1800-2023 16.12.1)
                যদি (nodep->disablep() && propExprp->disablep()) {
                    nodep->v3error("disable iff expression before property call and in its "
                                   "body is not legal");
                    ঠেলো_Deletep(propExprp->disablep()->unlinkFrBack());
                }
                // If disable iff is in outer property, move it to inner
                যদি (nodep->disablep()) {
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব disablep = nodep->disablep()->unlinkFrBack();
                    propExprp->disablep(disablep);
                }

                যদি (nodep->sensesp() && propExprp->sensesp()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Clock event before property call and in its body");
                    ঠেলো_Deletep(propExprp->sensesp()->unlinkFrBack());
                }
                // If clock event is in outer property, move it to inner
                যদি (nodep->sensesp()) {
                    AstSenItem* ধ্রুব sensesp = nodep->sensesp();
                    sensesp->unlinkFrBack();
                    propExprp->sensesp(sensesp);
                }

                // Now substitute property reference with property body
                nodep->replaceWith(propExprp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                প্রত্যাগম propExprp;
            }
        }
        প্রত্যাগম nodep;
    }

    শূন্য substituteNestedPropertyCall(AstFuncRef* funcrefp, ধ্রুব AstProperty* propp) {
        AstPropSpec* ধ্রুব specp = clonePropertyBody(funcrefp, propp);
        Ast_গ্রন্থি* replp = nullptr;
        যদি (specp->disablep()) {
            funcrefp->v3error("Illegal 'disable iff' in a nested property instance"
                              " (IEEE 1800-2023 16.12.1)");
        } অন্যথা যদি (specp->sensesp()) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: clock event in a nested property instance");
        } অন্যথা যদি (specp->propp()->exists(
                       [](ধ্রুব Ast_গ্রন্থি_রাশিমালা* np) { প্রত্যাগম np->isMultiCycleSva(); })) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: multi-cycle body in a nested property instance");
        } অন্যথা {
            replp = specp->propp()->unlinkFrBack();
        }
        যদি (!replp) replp = নূতন AstConst{funcrefp->fileline(), AstConst::BitFalse{}};
        funcrefp->replaceWith(replp);
        VL_DO_DANGLING(ঠেলো_Deletep(funcrefp), funcrefp);
        VL_DO_DANGLING(ঠেলো_Deletep(specp), specp);
    }

    // VISITORS
    //========== Statements
    শূন্য visit(AstClocking* ধ্রুব nodep) override {
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        UINFO(8, "   CLOCKING" << nodep);
        যদি (nodep->isDefault()) {
            যদি (m_defaultClockingp) {
                nodep->v3error("Only one default clocking block allowed per module"
                               " (IEEE 1800-2023 14.12)");
            } অন্যথা {
                m_defaultClockingp = nodep;
                m_defaultClkEvtVarp = nodep->ensureEventp();
            }
        }
        iterateChildren(nodep);
        যদি (nodep->eventp()) nodep->addNextHere(nodep->eventp()->unlinkFrBack());
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
    }
    শূন্য visit(AstModportClockingRef* ধ্রুব nodep) override {
        // It has to be converted to a list of ModportClockingVarRefs,
        // because clocking blocks are removed in this pass
        প্রতি (Ast_গ্রন্থি* itemp = nodep->clockingp()->itemsp(); itemp; itemp = itemp->nextp()) {
            যদি (AstClockingItem* ধ্রুব citemp = VN_CAST(itemp, ClockingItem)) {
                যদি (AstVar* ধ্রুব varp
                    = citemp->varp() ? citemp->varp() : VN_AS(citemp->user1p(), Var)) {
                    AstModportVarRef* ধ্রুব modVarp = নূতন AstModportVarRef{
                        nodep->fileline(), varp->name(), citemp->direction()};
                    modVarp->varp(varp);
                    nodep->addNextHere(modVarp);
                    যদি (citemp->direction().isOutput()) {
                        // Drives through the modport set the drive flag
                        AstVar* ধ্রুব drivenVarp = getCreateDrivenVarp(citemp);
                        AstModportVarRef* ধ্রুব drivenRefp = নূতন AstModportVarRef{
                            nodep->fileline(), drivenVarp->name(), VDirection::OUTPUT};
                        drivenRefp->varp(drivenVarp);
                        nodep->addNextHere(drivenRefp);
                    }
                }
            }
        }
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
    }
    শূন্য visit(AstClockingItem* ধ্রুব nodep) override {
        // Get a ref to the sampled/driven variable
        AstVar* ধ্রুব varp = nodep->varp();
        যদি (!varp) {
            // Unused item
            প্রত্যাগম;
        }
        // Flag set by drives of an output clockvar, possibly visited before this item
        যদি (nodep->direction().isOutput()) m_modp->addStmtsp(getCreateDrivenVarp(nodep));
        File_সারি* ধ্রুব flp = nodep->fileline();
        V3Const::constifyEdit(nodep->skewp());
        যদি (!VN_IS(nodep->skewp(), Const)) {
            nodep->skewp()->v3error("Skew must be constant (IEEE 1800-2023 14.4)");
            প্রত্যাগম;
        }
        AstConst* ধ্রুব skewp = VN_AS(nodep->skewp(), Const);
        যদি (skewp->অঙ্ক().isNegative()) skewp->v3error("Skew cannot be negative");
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp = nodep->exprp();
        varp->name(m_clockingp->name() + "__DOT__" + varp->name());
        m_clockingp->addNextHere(varp->unlinkFrBack());
        nodep->user1p(varp);
        varp->user1p(nodep);
        যদি (nodep->direction() == VDirection::OUTPUT) {
            exprp->foreach([](ধ্রুব Ast_গ্রন্থি_Var_Ref* varrefp) {
                // Prevent confusing BLKANDNBLK warnings on clockvars due to generated assignments
                varrefp->fileline()->warnOff(V3ErrorCode::BLKANDNBLK, সত্য);
                // Mark the driven signal so later passes (e.g. V3Inst port
                // connection) know the drive flows out from inside the cell.
                // Item expressions are plain signal references, so only the
                // driven signal is marked, never index selects.
                varrefp->varp()->isClockingDriven(সত্য);
            });
            AstVarRef* ধ্রুব skewedReadRefp = নূতন AstVarRef{flp, varp, VAccess::READ};
            skewedReadRefp->user1(সত্য);
            // Initialize the clockvar
            AstVarRef* ধ্রুব skewedWriteRefp = নূতন AstVarRef{flp, varp, VAccess::WRITE};
            skewedWriteRefp->user1(সত্য);
            AstInitialStatic* ধ্রুব initClockvarp = নূতন AstInitialStatic{
                flp, নূতন AstAssign{flp, skewedWriteRefp, exprp->clone_বৃক্ষ_Pure(অসত্য)}};
            m_modp->addStmtsp(initClockvarp);
            // Each drive sets a flag, so the signal is also assigned when driven with the same
            // value again, e.g. after another assignment to the signal (IEEE 1800-2023 14.16.2)
            AstVar* ধ্রুব drivenVarp = getCreateDrivenVarp(nodep);
            m_modp->addStmtsp(নূতন AstInitialStatic{
                flp, নূতন AstAssign{flp, নূতন AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                   নূতন AstConst{flp, AstConst::BitFalse{}}}});
            // Assign the clockvar to the actual var if it was driven
            AstAssign* ধ্রুব assignp
                = নূতন AstAssign{flp, exprp->clone_বৃক্ষ_Pure(অসত্য), skewedReadRefp};
            AstIf* ধ্রুব ifp
                = নূতন AstIf{flp, নূতন AstVarRef{flp, drivenVarp, VAccess::READ},
                            নূতন AstAssign{flp, নূতন AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                          নূতন AstConst{flp, AstConst::BitFalse{}}}};
            ifp->addThensp(assignp);
            // Only in the time step of the clocking event
            AstCMethodHard* ধ্রুব trigp = নূতন AstCMethodHard{
                nodep->fileline(), নূতন AstVarRef{flp, m_clockingp->ensureEventp(), VAccess::READ},
                VCMethod::EVENT_IS_TRIGGERED};
            trigp->dtypeSetBit();
            ifp->condp(নূতন AstLogAnd{flp, ifp->condp()->unlinkFrBack(), trigp});
            // A skew rounding to no time precision ticks is a zero skew
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব ticksp
                = V3Timing::delayToTicks(skewp->clone_বৃক্ষ(অসত্য), m_modp->timeunit());
            ধ্রুব তার্কিক zeroSkew = ticksp->isZero();
            VL_DO_DANGLING(ticksp->delete_বৃক্ষ(), ticksp);
            যদি (zeroSkew || !skewp->fileline()->timingOn()) {
                // Drive the var in Re-NBA (IEEE 1800-2023 14.16), ignoring skews where timing is
                // turned off
                Ast_Sen_বৃক্ষ* senTreep = m_clockingp->sentreep()->clone_বৃক্ষ(অসত্য);
                senTreep->addSensesp(নূতন AstSenItem{
                    flp, VEdgeType::ET_CHANGED, নূতন AstVarRef{flp, drivenVarp, VAccess::READ}});
                m_clockingp->addNextHere(নূতন AstAlwaysReactive{flp, senTreep, ifp});
            } অন্যথা {
                // Fork only the assignment, so this AlwaysPostponed can be retriggered
                // before the assignment happens
                AstFork* ধ্রুব forkp = নূতন AstFork{flp, VJoinType::JOIN_NONE};
                forkp->immediateStart(সত্য);
                forkp->addForksp(নূতন AstBegin{flp, "", assignp->unlinkFrBack(), সত্য});
                ifp->addThensp(forkp);
                // Read the clockvar in Postponed, so a drive late in the time step is applied
                // at this event plus the skew (IEEE 1800-2023 14.16)
                m_clockingp->addNextHere(নূতন AstAlwaysPostponed{flp, ifp});
                যদি (v3Global.opt.timing().isSetTrue()) {
                    AstDelay* ধ্রুব delayp = নূতন AstDelay{flp, skewp->unlinkFrBack(), অসত্য};
                    delayp->timeunit(m_modp->timeunit());
                    assignp->timingControlp(delayp);
                } অন্যথা যদি (v3Global.opt.timing().isSetFalse()) {
                    nodep->v3warn(E_NOTIMING,
                                  "Clocking output skew greater than #0 requires --timing");
                } অন্যথা {
                    nodep->v3warn(E_NEEDTIMINGOPT,
                                  "Use --timing or --no-timing to specify how "
                                  "clocking output skew greater than #0 should be handled");
                }
            }
        } অন্যথা যদি (nodep->direction() == VDirection::প্রবেশ) {
            // Ref to the clockvar
            AstVarRef* ধ্রুব refp = নূতন AstVarRef{flp, varp, VAccess::WRITE};
            refp->user1(সত্য);
            যদি (skewp->অঙ্ক().is1Step()) {
                // #1step means the value that is sampled is always the signal's last value
                // before the clock edge (IEEE 1800-2023 14.4)
                AstSampled* ধ্রুব sampledp
                    = নূতন AstSampled{flp, exprp->clone_বৃক্ষ_Pure(অসত্য), exprp->dtypep(), সত্য};
                AstAssign* ধ্রুব assignp = নূতন AstAssign{flp, refp, sampledp};
                m_clockingp->addNextHere(নূতন AstAlways{
                    flp, VAlwaysKwd::ALWAYS, m_clockingp->sentreep()->clone_বৃক্ষ(অসত্য), assignp});
            } অন্যথা যদি (skewp->isZero()) {
                // #0 means the var has to be sampled in Observed (IEEE 1800-2023 14.13)
                AstAssign* ধ্রুব assignp = নূতন AstAssign{flp, refp, exprp->clone_বৃক্ষ_Pure(অসত্য)};
                m_clockingp->addNextHere(নূতন AstAlwaysObserved{
                    flp, m_clockingp->sentreep()->clone_বৃক্ষ(অসত্য), assignp});
            } অন্যথা {
                // Create a queue where we'll store sampled values with timestamps
                AstSampleQueueDType* ধ্রুব queueDtp
                    = নূতন AstSampleQueueDType{flp, exprp->dtypep()};
                m_netlistp->typeTablep()->addTypesp(queueDtp);
                AstVar* ধ্রুব queueVarp
                    = নূতন AstVar{flp, VVarType::MODULETEMP, "__Vqueue__" + varp->name(), queueDtp};
                queueVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_clockingp->addNextHere(queueVarp);
                // Create a process like this:
                //     always queue.push(<sampled var>);
                AstCMethodHard* ধ্রুব pushp = নূতন AstCMethodHard{
                    flp, নূতন AstVarRef{flp, queueVarp, VAccess::WRITE}, VCMethod::DYN_ঠেলো,
                    নূতন AstTime{nodep->fileline(), m_modp->timeunit()}};
                pushp->addPinsp(exprp->clone_বৃক্ষ_Pure(অসত্য));
                pushp->dtypeSetVoid();
                m_clockingp->addNextHere(
                    নূতন AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, pushp->makeStmt()});
                // Create a process like this:
                //     always @<clocking event> queue.pop(<skew>, /*out*/<skewed var>);
                AstCMethodHard* ধ্রুব popp = নূতন AstCMethodHard{
                    flp, নূতন AstVarRef{flp, queueVarp, VAccess::READWRITE}, VCMethod::DYN_বের_করো,
                    নূতন AstTime{nodep->fileline(), m_modp->timeunit()}};
                popp->addPinsp(skewp->unlinkFrBack());
                refp->access(VAccess::READWRITE);  // Only conditionally assigned
                popp->addPinsp(refp);
                popp->dtypeSetVoid();
                m_clockingp->addNextHere(নূতন AstAlways{flp, VAlwaysKwd::ALWAYS,
                                                       m_clockingp->sentreep()->clone_বৃক্ষ(অসত্য),
                                                       popp->makeStmt()});
            }
        } অন্যথা {
            nodep->v3fatalSrc("Invalid direction");
        }
    }
    শূন্য visit(AstDelay* nodep) override {
        m_hasCycleDelay |= nodep->isCycleDelay();
        // Only cycle delays are relevant in this stage; also only process once
        যদি (!nodep->isCycleDelay()) {
            যদি (m_inSynchDrive) {
                nodep->v3error("Only cycle delays can be used in synchronous drives"
                               " (IEEE 1800-2023 14.16)");
            }
            iterateChildren(nodep);
            প্রত্যাগম;
        }
        যদি (m_inAssign && !m_inSynchDrive) {
            nodep->v3error("Cycle delays not allowed as intra-assignment delays"
                           " (IEEE 1800-2023 14.11)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
            প্রত্যাগম;
        }
        যদি (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBackWithNext());
        File_সারি* ধ্রুব flp = nodep->fileline();
        Ast_গ্রন্থি_রাশিমালা* valuep = V3Const::constifyEdit(nodep->lhsp()->unlinkFrBack());
        ধ্রুব AstConst* ধ্রুব constp = VN_CAST(valuep, Const);
        যদি (!constp && m_pexprp) {
            // V3AssertNfa handles non-const delays in sequences before this pass and
            // replaces the property; this branch should never be reached.
            nodep->v3fatalSrc("Non-constant cycle delay in assertion: "
                              "should have been caught by V3AssertNfa");
        } অন্যথা যদি (constp && constp->isZero()) {
            VL_DO_DANGLING(ঠেলো_Deletep(valuep), valuep);
            যদি (m_inSynchDrive) {
                // ##0 has no effect in synchronous drives (IEEE 1800-2023 14.11)
                VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
                প্রত্যাগম;
            }
            যদি (m_pexprp) {
                // ##0 in sequence context = zero delay = same clock tick
                VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
                প্রত্যাগম;
            }
            // Procedural ##0: synchronize with default clocking event (IEEE 1800-2023 14.11)
            যদি (!m_defaultClockingp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
                প্রত্যাগম;
            }
            nodep->replaceWith(newCycleDelayZerop(flp));
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            প্রত্যাগম;
        }
        Ast_Sen_বৃক্ষ* sentreep = nullptr;
        যদি (!m_drives.empty()) {
            যদি (m_inConcatAssign) {
                // Also the clockvars may be of different clocking blocks
                nodep->v3error("Cycle delays cannot be used in synchronous drives to a"
                               " concatenation (IEEE 1800-2023 14.16)");
                VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(ঠেলো_Deletep(valuep), valuep);
                প্রত্যাগম;
            }
            // Count the cycles of the driven clockvar's clocking block (IEEE 1800-2023 14.16)
            sentreep = newDriveSentreep(nodep, m_drives.front());
            যদি (!sentreep) {
                VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(ঠেলো_Deletep(valuep), valuep);
                প্রত্যাগম;
            }
        } অন্যথা যদি (!m_defaultClockingp) {
            যদি (!m_pexprp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(ঠেলো_Deletep(valuep), valuep);
                প্রত্যাগম;
            }
            sentreep = নূতন Ast_Sen_বৃক্ষ{flp, m_senip->clone_বৃক্ষ(অসত্য)};
        } অন্যথা {
            sentreep = m_defaultClockingp->sentreep()->clone_বৃক্ষ(অসত্য);
        }
        AstEventControl* ধ্রুব controlp
            = নূতন AstEventControl{nodep->fileline(), sentreep, nullptr};
        ধ্রুব std::string delayName = m_cycleDlyNames.get(nodep);
        Ast_গ্রন্থি_রাশিমালা* throughoutp
            = nodep->throughoutp() ? nodep->throughoutp()->unlinkFrBack() : nullptr;

        AstVar* ধ্রুব cntVarp = নূতন AstVar{flp, VVarType::BLOCKTEMP, delayName + "__counter",
                                           nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstBegin* ধ্রুব beginp = নূতন AstBegin{flp, delayName + "__block", cntVarp, সত্য};
        // A non-constant delay is counted at runtime. The count is integral of 64 bits, as
        // V3Width makes delays, or real, which is rounded (IEEE 1800-2023 6.12.1)
        যদি (valuep->isDouble()) {
            valuep = নূতন AstRToIRoundS{flp, valuep};
            valuep->dtypeFrom(cntVarp);
        } অন্যথা {
            valuep = নূতন AstSel{flp, valuep, 0, cntVarp->width()};
        }
        beginp->addStmtsp(নূতন AstAssign{flp, নূতন AstVarRef{flp, cntVarp, VAccess::WRITE}, valuep});
        // If the count is zero, a synchronous drive waits for none, like with a constant '##0',
        // and a procedural delay waits like a procedural '##0' (IEEE 1800-2023 14.11)
        যদি (!constp && !m_inSynchDrive) {
            beginp->addStmtsp(নূতন AstIf{
                flp,
                নূতন AstEq{flp, নূতন AstVarRef{flp, cntVarp, VAccess::READ}, নূতন AstConst{flp, 0}},
                newCycleDelayZerop(flp)});
        }

        // Throughout: create flag tracking whether condition held every tick
        AstVar* throughoutOkp = nullptr;
        যদি (throughoutp) {
            throughoutOkp = নূতন AstVar{flp, VVarType::BLOCKTEMP, delayName + "__throughoutOk",
                                       nodep->findBasicDType(VBasicDTypeKwd::LOGIC_IMPLICIT)};
            throughoutOkp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->addStmtsp(throughoutOkp);
            beginp->addStmtsp(নূতন AstAssign{flp, নূতন AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            নূতন AstConst{flp, AstConst::BitTrue{}}});
            // Check condition at tick 0 (sequence start, before entering loop)
            beginp->addStmtsp(
                নূতন AstIf{flp, নূতন AstLogNot{flp, throughoutp->clone_বৃক্ষ_Pure(অসত্য)},
                          নূতন AstAssign{flp, নূতন AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                        নূতন AstConst{flp, AstConst::BitFalse{}}}});
        }

        {
            AstLoop* ধ্রুব loopp = নূতন AstLoop{flp};
            // When throughout is present, exit loop early if condition fails
            Ast_গ্রন্থি_রাশিমালা* loopCondp
                = নূতন AstGt{flp, নূতন AstVarRef{flp, cntVarp, VAccess::READ}, নূতন AstConst{flp, 0}};
            যদি (throughoutOkp) {
                loopCondp = নূতন AstLogAnd{flp, loopCondp,
                                          নূতন AstVarRef{flp, throughoutOkp, VAccess::READ}};
            }
            loopp->addStmtsp(নূতন AstLoopTest{flp, loopp, loopCondp});
            loopp->addStmtsp(controlp);
            loopp->addStmtsp(
                নূতন AstAssign{flp, নূতন AstVarRef{flp, cntVarp, VAccess::WRITE},
                              নূতন AstSub{flp, নূতন AstVarRef{flp, cntVarp, VAccess::READ},
                                         নূতন AstConst{flp, 1}}});
            // Check throughout condition at each tick during delay (IEEE 1800-2023 16.9.9)
            যদি (throughoutp) {
                loopp->addStmtsp(
                    নূতন AstIf{flp, নূতন AstLogNot{flp, throughoutp},
                              নূতন AstAssign{flp, নূতন AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            নূতন AstConst{flp, AstConst::BitFalse{}}}});
            }
            beginp->addStmtsp(loopp);
        }
        // Compose wrappers on remaining sequence: throughout gate (inner), disable iff (outer)
        Ast_গ্রন্থি* remainp = nodep->nextp() ? nodep->nextp()->unlinkFrBackWithNext() : nullptr;
        যদি (throughoutOkp) {
            // If condition failed during delay, fail assertion
            remainp = নূতন AstIf{flp, নূতন AstVarRef{flp, throughoutOkp, VAccess::READ}, remainp,
                                নূতন AstPExprClause{flp, /*pass=*/অসত্য}};
        }
        যদি (m_disableSeqIfp && remainp) {
            AstIf* ধ্রুব disableSeqIfp = m_disableSeqIfp->clone_বৃক্ষ(অসত্য);
            disableSeqIfp->addThensp(remainp);
            remainp = disableSeqIfp;
        }
        যদি (remainp) {
            যদি (throughoutOkp) {
                // throughoutOkp is declared in beginp scope -- check must be inside it
                beginp->addStmtsp(remainp);
            } অন্যথা {
                nodep->addNextHere(remainp);
            }
        }
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }
    শূন্য visit(Ast_Sen_বৃক্ষ* nodep) override {
        যদি (m_inSynchDrive) {
            nodep->v3error("Event controls cannot be used in "
                           "synchronous drives (IEEE 1800-2023 14.16)");
        }

        ধ্রুব AstSampled* sampledp;
        যদি (nodep->exists([&sampledp](ধ্রুব AstSampled* ধ্রুব sp) {
                sampledp = sp;
                প্রত্যাগম সত্য;
            })) {
            sampledp->v3warn(E_UNSUPPORTED, "Unsupported: $sampled inside sensitivity list");
        }
    }
    শূন্য visit(Ast_গ্রন্থি_Var_Ref* nodep) override {
        UINFO(8, " -varref:  " << nodep);
        UINFO(8, " -varref-var-back:  " << nodep->varp()->backp());
        UINFO(8, " -varref-var-user1:  " << nodep->varp()->user1p());
        যদি (AstClockingItem* ধ্রুব itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->firstAbovep(),
                ClockingItem)) {
            যদি (nodep->user1()) প্রত্যাগম;

            // ensure linking still works, this has to be done only once
            যদি (AstVarXRef* xrefp = VN_CAST(nodep, VarXRef)) {
                UINFO(8, " -clockvarxref-in:  " << xrefp);
                string dotted = xrefp->dotted();
                ধ্রুব size_t dotPos = dotted.rfind('.');
                dotted.erase(dotPos, string::npos);
                xrefp->dotted(dotted);
                UINFO(8, " -clockvarxref-out: " << xrefp);
                m_xrefsp.emplace_back(xrefp);
            }

            যদি (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            যদি (nodep->access().isWriteOrRW()) {
                যদি (itemp->direction() == VDirection::OUTPUT) {
                    যদি (!m_inAssignDlyLhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    যদি (m_inAssign) {
                        m_inSynchDrive = সত্য;
                        m_drives.ঠেলো_back({itemp, nodep});
                    }
                } অন্যথা যদি (itemp->direction() == VDirection::প্রবেশ) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
            nodep->user1(সত্য);
        }
    }
    শূন্য visit(AstMemberSel* nodep) override {
        যদি (AstClockingItem* ধ্রুব itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->firstAbovep(),
                ClockingItem)) {
            যদি (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            যদি (nodep->access().isWriteOrRW()) {
                যদি (itemp->direction() == VDirection::OUTPUT) {
                    যদি (!m_inAssignDlyLhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    যদি (m_inAssign) {
                        m_inSynchDrive = সত্য;
                        m_drives.ঠেলো_back({itemp, nodep});
                    }
                } অন্যথা যদি (itemp->direction() == VDirection::প্রবেশ) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
        }
    }
    শূন্য visit(Ast_গ্রন্থি_Assign* nodep) override {
        যদি (nodep->user1()) প্রত্যাগম;
        VL_RESTORER(m_inAssign);
        VL_RESTORER(m_inSynchDrive);
        VL_RESTORER(m_inConcatAssign);
        VL_RESTORER_CLEAR(m_drives);
        m_inAssign = সত্য;
        m_inSynchDrive = অসত্য;
        {
            // The braces of a concatenation are a replication, by one
            ধ্রুব Ast_গ্রন্থি_রাশিমালা* lhsp = nodep->lhsp();
            যদি (ধ্রুব AstReplicate* ধ্রুব repp = VN_CAST(lhsp, Replicate)) lhsp = repp->srcp();
            m_inConcatAssign = VN_IS(lhsp, Concat);
        }
        {
            VL_RESTORER(m_inAssignDlyLhs);
            m_inAssignDlyLhs = VN_IS(nodep, AssignDly);
            iterate(nodep->lhsp());
        }
        iterate(nodep->rhsp());
        যদি (nodep->timingControlp()) iterate(nodep->timingControlp());
        // A drive without a cycle delay, also after removing '##0', updates the clockvar now
        যদি (m_inSynchDrive && !nodep->timingControlp()) {
            AstAssign* ধ্রুব assignp = নূতন AstAssign{
                nodep->fileline(), nodep->lhsp()->unlinkFrBack(), nodep->rhsp()->unlinkFrBack()};
            assignp->user1(সত্য);
            nodep->replaceWith(assignp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
            nodep = assignp;
        }
        // Note the drives when they take effect, after the cycle delay if any. Each clockvar
        // of the LHS is driven, e.g. in a concatenation which other simulators accept, although
        // IEEE 1800-2023 14.16 does not allow it
        Ast_গ্রন্থি* setsp = nullptr;
        প্রতি (ধ্রুব SynchDrive& drive : m_drives) {
            setsp = Ast_গ্রন্থি::addNext(setsp, newDrivenSetp(nodep->fileline(), drive));
        }
        যদি (!setsp) প্রত্যাগম;
        যদি (AstBegin* ধ্রুব beginp = VN_CAST(nodep->timingControlp(), Begin)) {
            beginp->addStmtsp(setsp);
        } অন্যথা {
            nodep->addNextHere(setsp);
        }
    }
    শূন্য visit(AstAlways* nodep) override {
        iterateAndNextNull(nodep->sentreep());
        যদি (nodep->sentreep()) m_seniAlwaysp = nodep->sentreep()->sensesp();
        iterateAndNextNull(nodep->stmtsp());
        m_seniAlwaysp = nullptr;
    }

    শূন্য visit(Ast_গ্রন্থি_Cover_Or_Assert* nodep) override {
        যদি (nodep->sentreep()) প্রত্যাগম;  // Already processed

        VL_RESTORER(m_senip);
        VL_RESTORER(m_disablep);
        m_senip = nullptr;
        m_disablep = nullptr;

        // Find Clocking's buried under nodep->exprsp
        iterateChildren(nodep);
        যদি (!nodep->immediate()) nodep->sentreep(new_Sen_বৃক্ষ(nodep, nullptr, nodep));
    }
    শূন্য visit(AstFalling* nodep) override {
        যদি (nodep->user1SetOnce()) প্রত্যাগম;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_সারি* ধ্রুব fl = nodep->fileline();
        Ast_গ্রন্থি_রাশিমালা* exprp = nodep->exprp()->unlinkFrBack();
        যদি (exprp->width() > 1) exprp = নূতন AstSel{fl, exprp, 0, 1};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব futurep = নূতন AstFuture{fl, exprp, new_Sen_বৃক্ষ(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = নূতন AstAnd{fl, exprp->clone_বৃক্ষ_Pure(অসত্য), নূতন AstNot{fl, futurep}};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstFell* nodep) override {
        যদি (nodep->user1SetOnce()) প্রত্যাগম;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_সারি* ধ্রুব fl = nodep->fileline();
        Ast_গ্রন্থি_রাশিমালা* exprp = nodep->exprp()->unlinkFrBack();
        যদি (exprp->width() > 1) exprp = নূতন AstSel{fl, exprp, 0, 1};
        Ast_Sen_বৃক্ষ* sentreep = nodep->sentreep();
        যদি (sentreep) sentreep->unlinkFrBack();
        AstPast* ধ্রুব pastp = নূতন AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_বৃক্ষ(nodep, sentreep));
        exprp = নূতন AstAnd{fl, pastp, নূতন AstNot{fl, exprp->clone_বৃক্ষ_Pure(অসত্য)}};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstFuture* nodep) override {
        যদি (nodep->user1SetOnce()) প্রত্যাগম;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        Ast_Sen_বৃক্ষ* ধ্রুব sentreep = nodep->sentreep();
        যদি (sentreep) VL_DO_DANGLING(ঠেলো_Deletep(sentreep->unlinkFrBack()), sentreep);
        nodep->sentreep(new_Sen_বৃক্ষ(nodep));
    }
    শূন্য visit(AstPast* nodep) override {
        checkSamplingFuncDType(nodep, nodep->exprp());
        যদি (nodep->sentreep()) প্রত্যাগম;  // Already processed
        iterateChildren(nodep);
        nodep->sentreep(new_Sen_বৃক্ষ(nodep));
    }
    শূন্য visit(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- Lower standalone exact [*N] (N >= 2) via saturating counter.
        // Range/unbounded forms and SExpr-contained forms are lowered by V3AssertNfa.
        iterateChildren(nodep);
        // V3AssertNfa handles unbounded/ranged forms upstream, so this fast-path
        // is effectively unreachable when NFA is enabled.
        যদি (nodep->unbounded() || nodep->maxCountp()) প্রত্যাগম;  // LCOV_EXCL_LINE
        ধ্রুব AstConst* ধ্রুব constp = VN_CAST(nodep->countp(), Const);
        যদি (VL_UNLIKELY(!constp || constp->toSInt() < 1)) {
            nodep->v3fatalSrc("Consecutive repetition count must be a positive constant"
                              " (should have been caught by V3Width)");
            প্রত্যাগম;
        }
        ধ্রুব পূর্ণাঙ্ক n = constp->toSInt();
        File_সারি* ধ্রুব flp = nodep->fileline();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp = nodep->exprp()->unlinkFrBack();
        যদি (n == 1) {
            nodep->replaceWith(exprp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            প্রত্যাগম;
        }
        // Saturating counter: if (expr) cnt <= min(cnt+1, N); else cnt <= 0;
        AstVar* ধ্রুব cntVarp = নূতন AstVar{flp, VVarType::MODULETEMP, m_consRepNames.get(""),
                                           nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(cntVarp);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব রাশিমালা_Clonep = exprp->clone_বৃক্ষ_Pure(অসত্য);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব saturatingIncrp = নূতন AstCond{
            flp,
            নূতন AstLt{flp, নূতন AstVarRef{flp, cntVarp, VAccess::READ},
                      নূতন AstConst{flp, static_cast<uint32_t>(n)}},
            নূতন AstAdd{flp, নূতন AstVarRef{flp, cntVarp, VAccess::READ}, নূতন AstConst{flp, 1u}},
            নূতন AstConst{flp, static_cast<uint32_t>(n)}};
        AstAssignDly* ধ্রুব incrAssignp
            = নূতন AstAssignDly{flp, নূতন AstVarRef{flp, cntVarp, VAccess::WRITE}, saturatingIncrp};
        AstAssignDly* ধ্রুব resetAssignp = নূতন AstAssignDly{
            flp, নূতন AstVarRef{flp, cntVarp, VAccess::WRITE}, নূতন AstConst{flp, 0u}};
        AstIf* ধ্রুব ifp = নূতন AstIf{flp, রাশিমালা_Clonep, incrAssignp, resetAssignp};
        Ast_Sen_বৃক্ষ* ধ্রুব senTreep = new_Sen_বৃক্ষ(nodep);
        AstAlways* ধ্রুব alwaysp = নূতন AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, ifp};
        cntVarp->addNextHere(alwaysp);
        // Match: cnt >= N-1 (previous cycles via NBA) && expr (current cycle)
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব cntCheckp = নূতন AstGte{flp, নূতন AstVarRef{flp, cntVarp, VAccess::READ},
                                                  নূতন AstConst{flp, static_cast<uint32_t>(n - 1)}};
        cntCheckp->dtypeSetBit();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব matchp = নূতন AstAnd{flp, cntCheckp, exprp};
        matchp->dtypeSetBit();
        nodep->replaceWith(matchp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstRising* nodep) override {
        যদি (nodep->user1SetOnce()) প্রত্যাগম;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_সারি* ধ্রুব fl = nodep->fileline();
        Ast_গ্রন্থি_রাশিমালা* exprp = nodep->exprp()->unlinkFrBack();
        যদি (exprp->width() > 1) exprp = নূতন AstSel{fl, exprp, 0, 1};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব futurep = নূতন AstFuture{fl, exprp, new_Sen_বৃক্ষ(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = নূতন AstAnd{fl, নূতন AstNot{fl, exprp->clone_বৃক্ষ_Pure(অসত্য)}, futurep};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstRose* nodep) override {
        যদি (nodep->user1SetOnce()) প্রত্যাগম;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_সারি* ধ্রুব fl = nodep->fileline();
        Ast_গ্রন্থি_রাশিমালা* exprp = nodep->exprp()->unlinkFrBack();
        যদি (exprp->width() > 1) exprp = নূতন AstSel{fl, exprp, 0, 1};
        Ast_Sen_বৃক্ষ* sentreep = nodep->sentreep();
        যদি (sentreep) sentreep->unlinkFrBack();
        AstPast* ধ্রুব pastp = নূতন AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_বৃক্ষ(nodep, sentreep));
        exprp = নূতন AstAnd{fl, নূতন AstNot{fl, pastp}, exprp->clone_বৃক্ষ_Pure(অসত্য)};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    স্থির AstAssocArrayDType* getProcessAssocArrayType(File_সারি* ধ্রুব flp) {
        // Type of __VassertsActive___x[std::process]
        Ast_গ্রন্থি_DType* valp
            = v3Global.rootp()->typeTablep()->findBasicDType(flp, VBasicDTypeKwd::BIT);
        AstClassRefDType* keyp
            = নূতন AstClassRefDType{flp, v3Global.rootp()->stdPackageProcessp(), nullptr};
        keyp->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        v3Global.rootp()->typeTablep()->addTypesp(keyp);
        AstAssocArrayDType* ধ্রুব typep = নূতন AstAssocArrayDType{flp, valp, keyp};
        typep->dtypep(typep);
        v3Global.rootp()->typeTablep()->addTypesp(typep);
        প্রত্যাগম typep;
    }
    স্থির Ast_Stmt_রাশিমালা* getProcessAssocArrayDelete(AstVarRef* ধ্রুব refp) {
        // Constructs refp.delete(std::process::self()) statement
        File_সারি* ধ্রুব flp = refp->fileline();
        AstCMethodHard* ধ্রুব deletep = নূতন AstCMethodHard{
            flp, refp, VCMethod::ASSOC_ERASE, v3Global.rootp()->stdPackageProcessSelfp(flp)};
        deletep->dtypep(refp->findVoidDType());
        প্রত্যাগম নূতন Ast_Stmt_রাশিমালা{flp, deletep};
    }
    স্থির Ast_গ্রন্থি_রাশিমালা* getProcessAssocArraySize(AstVarRef* ধ্রুব refp) {
        // Constructs refp.size() statement
        AstCMethodHard* ধ্রুব sizep
            = নূতন AstCMethodHard{refp->fileline(), refp, VCMethod::ASSOC_SIZE};
        sizep->dtypep(refp->findBasicDType(VBasicDTypeKwd::UINT32));
        প্রত্যাগম sizep;
    }
    শূন্য visit(AstSEventually* nodep) override {
        UASSERT(v3Global.rootp()->stdPackagep(), "Should be imported");

        Ast_Sen_বৃক্ষ* ধ্রুব sentreep = new_Sen_বৃক্ষ(nodep);
        যদি (!sentreep->sensesp()) {
            VL_DO_DANGLING(ঠেলো_Deletep(sentreep), sentreep);
            nodep->replaceWith(নূতন AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            প্রত্যাগম;
        }

        File_সারি* ধ্রুব flp = nodep->fileline();

        // Track active assertions
        AstVar* ধ্রুব activep = নূতন AstVar{flp, VVarType::MODULETEMP, m_activeNames.get(""),
                                           getProcessAssocArrayType(flp)};
        activep->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(activep);

        // Assertion condition check
        AstLoop* ধ্রুব loopp = নূতন AstLoop{flp};
        AstSampled* ধ্রুব condp
            = নূতন AstSampled{flp, nodep->exprp()->unlinkFrBack(), nullptr, সত্য};
        loopp->addStmtsp(নূতন AstLoopTest{flp, loopp, নূতন AstLogNot{flp, condp}});
        loopp->addStmtsp(নূতন AstEventControl{flp, sentreep, nullptr});

        // Add assertion to the active set
        AstAssocSel* ধ্রুব selp = নূতন AstAssocSel{flp, নূতন AstVarRef{flp, activep, VAccess::WRITE},
                                                  v3Global.rootp()->stdPackageProcessSelfp(flp)};
        AstAssign* ধ্রুব incrementp = নূতন AstAssign{flp, selp, নূতন AstConst{flp, 1}};
        AstPExprClause* ধ্রুব clausep = নূতন AstPExprClause{flp};
        Ast_Stmt_রাশিমালা* ধ্রুব deletep
            = getProcessAssocArrayDelete(নূতন AstVarRef{flp, activep, VAccess::WRITE});

        // Main assertion block
        AstBegin* ধ্রুব bodyp = নূতন AstBegin{flp, m_blockNames.get(""), nullptr, সত্য};
        bodyp->addStmtsp(incrementp);
        bodyp->addStmtsp(loopp);
        bodyp->addStmtsp(clausep);
        bodyp->addStmtsp(deletep);

        // Validate assertion condition for each active assert
        AstVar* ধ্রুব activeCountp = নূতন AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        activeCountp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstAssign* ধ্রুব initActiveCountp
            = নূতন AstAssign{flp, নূতন AstVarRef{flp, activeCountp, VAccess::WRITE},
                            getProcessAssocArraySize(নূতন AstVarRef{flp, activep, VAccess::READ})};
        AstLoop* ধ্রুব finalLoopp = নূতন AstLoop{flp};
        AstIf* ধ্রুব finalBodypCondp
            = নূতন AstIf{flp, condp->clone_বৃক্ষ_Pure(অসত্য), নূতন AstPExprClause{flp},
                        নূতন AstPExprClause{flp, অসত্য}};
        finalLoopp->addStmtsp(
            নূতন AstLoopTest{flp, finalLoopp,
                            নূতন AstNeq{flp, নূতন AstVarRef{flp, activeCountp, VAccess::READ},
                                       নূতন AstConst{flp, 0}}});
        finalLoopp->addStmtsp(finalBodypCondp);
        finalLoopp->addStmtsp(
            নূতন AstAssign{flp, নূতন AstVarRef{flp, activeCountp, VAccess::WRITE},
                          নূতন AstSub{flp, নূতন AstVarRef{flp, activeCountp, VAccess::READ},
                                     নূতন AstConst{flp, 1}}});

        // Final assertion block
        AstBegin* ধ্রুব finalp = নূতন AstBegin{flp, m_blockNames.get(""), nullptr, সত্য};
        finalp->addStmtsp(activeCountp);
        finalp->addStmtsp(initActiveCountp);
        finalp->addStmtsp(finalLoopp);

        VL_RESTORER(m_pexprp);
        m_pexprp = নূতন AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
        VL_RESTORER(m_hasCycleDelay);
        m_hasCycleDelay = অসত্য;
        iterate(bodyp);
        iterate(finalp);

        যদি (m_hasCycleDelay) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in s_eventually");
            nodep->replaceWith(নূতন AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            VL_DO_DANGLING(m_pexprp->delete_বৃক্ষ(), m_pexprp);
            প্রত্যাগম;
        }

        nodep->replaceWith(m_pexprp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstStable* nodep) override {
        যদি (nodep->user1SetOnce()) প্রত্যাগম;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_সারি* ধ্রুব fl = nodep->fileline();
        Ast_গ্রন্থি_রাশিমালা* exprp = nodep->exprp()->unlinkFrBack();
        Ast_Sen_বৃক্ষ* sentreep = nodep->sentreep();
        যদি (sentreep) sentreep->unlinkFrBack();
        AstPast* ধ্রুব pastp = নূতন AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_বৃক্ষ(nodep, sentreep));
        exprp = নূতন AstEq{fl, pastp, exprp->clone_বৃক্ষ_Pure(অসত্য)};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstSteady* nodep) override {
        যদি (nodep->user1SetOnce()) প্রত্যাগম;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_সারি* ধ্রুব fl = nodep->fileline();
        Ast_গ্রন্থি_রাশিমালা* exprp = nodep->exprp()->unlinkFrBack();
        যদি (exprp->width() > 1) exprp = নূতন AstSel{fl, exprp, 0, 1};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব futurep = নূতন AstFuture{fl, exprp, new_Sen_বৃক্ষ(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = নূতন AstEq{fl, exprp->clone_বৃক্ষ_Pure(অসত্য), futurep};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstSampled* nodep) override {
        iterateChildren(nodep);
        যদি (!nodep->internal()) checkSamplingFuncDType(nodep, nodep->exprp());
    }

    // Validate repetition count: must be a non-negative elaboration-time constant.
    // Shared by goto [->N] and nonconsecutive [=N] repetition.
    // On error, replaces nodep with BitFalse placeholder and returns nullptr.
    ধ্রুব AstConst* validate_Rep_গুনতি(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি_রাশিমালা*& countp) {
        countp = V3Const::constifyEdit(countp);
        ধ্রুব AstConst* ধ্রুব constp = VN_CAST(countp, Const);
        যদি (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ঠেলো_Deletep(countp), countp);
            nodep->replaceWith(নূতন AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            প্রত্যাগম nullptr;
        }
        যদি (constp->toSInt() < 0) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ঠেলো_Deletep(countp), countp);
            nodep->replaceWith(নূতন AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            প্রত্যাগম nullptr;
        }
        যদি (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ঠেলো_Deletep(countp), countp);
            nodep->replaceWith(নূতন AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            প্রত্যাগম nullptr;
        }
        প্রত্যাগম constp;
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
    AstPExpr* createRepPExpr(File_সারি* flp, Ast_গ্রন্থি_রাশিমালা* exprp, Ast_গ্রন্থি_রাশিমালা* countp,
                             Ast_গ্রন্থি_রাশিমালা* rhsp, তার্কিক isOverlapped, তার্কিক isNonConsec) {
        AstSenItem* ধ্রুব sensesp = m_senip;
        UASSERT_OBJ(sensesp, exprp, "Repetition requires a clock");

        ধ্রুব std::string name
            = isNonConsec ? m_nonConsRepNames.get(exprp) : m_gotoRepNames.get(exprp);
        AstVar* ধ্রুব cntVarp = নূতন AstVar{flp, VVarType::BLOCKTEMP, name + "__counter",
                                           exprp->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstBegin* ধ্রুব beginp = নূতন AstBegin{flp, name + "__block", cntVarp, সত্য};
        beginp->addStmtsp(
            নূতন AstAssign{flp, নূতন AstVarRef{flp, cntVarp, VAccess::WRITE}, নূতন AstConst{flp, 0}});

        // Counting loop: find N occurrences of expr (shared by goto and nonconsec)
        AstLoop* ধ্রুব loopp = নূতন AstLoop{flp};
        loopp->addStmtsp(নূতন AstLoopTest{flp, loopp,
                                         নূতন AstLt{flp, নূতন AstVarRef{flp, cntVarp, VAccess::READ},
                                                   countp->clone_বৃক্ষ_Pure(অসত্য)}});
        // if (expr) cnt++ -- sampled is applied to whole property expr by V3Assert
        loopp->addStmtsp(
            নূতন AstIf{flp, exprp,
                      নূতন AstAssign{flp, নূতন AstVarRef{flp, cntVarp, VAccess::WRITE},
                                    নূতন AstAdd{flp, নূতন AstVarRef{flp, cntVarp, VAccess::READ},
                                               নূতন AstConst{flp, 1}}}});
        loopp->addStmtsp(নূতন AstIf{
            flp, নূতন AstLt{flp, নূতন AstVarRef{flp, cntVarp, VAccess::READ}, countp},
            নূতন AstEventControl{flp, নূতন Ast_Sen_বৃক্ষ{flp, sensesp->clone_বৃক্ষ(অসত্য)}, nullptr}});

        beginp->addStmtsp(loopp);

        যদি (isNonConsec && rhsp) {
            // IEEE 16.9.2: b[=N] = b[->N] ##1 !b[*0:$]
            // Trailing window: ##1 advance, then scan !expr cycles checking rhs.
            // Window closes when expr becomes true again (fail if rhs was never true).
            beginp->addStmtsp(নূতন AstEventControl{
                flp, নূতন Ast_Sen_বৃক্ষ{flp, sensesp->clone_বৃক্ষ(অসত্য)}, nullptr});  // ##1
            যদি (!isOverlapped) {
                beginp->addStmtsp(নূতন AstEventControl{
                    flp, নূতন Ast_Sen_বৃক্ষ{flp, sensesp->clone_বৃক্ষ(অসত্য)}, nullptr});  // |=>
            }
            // Window loop: check rhs at each !expr cycle (done variable for termination)
            AstVar* ধ্রুব doneVarp
                = নূতন AstVar{flp, VVarType::BLOCKTEMP, name + "__done", exprp->findBitDType()};
            doneVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->addStmtsp(doneVarp);
            beginp->addStmtsp(নূতন AstAssign{flp, নূতন AstVarRef{flp, doneVarp, VAccess::WRITE},
                                            নূতন AstConst{flp, AstConst::BitFalse{}}});
            auto setDone = [&]() {
                প্রত্যাগম নূতন AstAssign{flp, নূতন AstVarRef{flp, doneVarp, VAccess::WRITE},
                                     নূতন AstConst{flp, AstConst::BitTrue{}}};
            };
            AstLoop* ধ্রুব windowp = নূতন AstLoop{flp};
            // LoopTest: continue while !done
            windowp->addStmtsp(নূতন AstLoopTest{
                flp, windowp, নূতন AstNot{flp, নূতন AstVarRef{flp, doneVarp, VAccess::READ}}});
            // if (expr) { fail; done = 1; } -- window closed, expr true again
            Ast_গ্রন্থি* ধ্রুব failsp = নূতন AstPExprClause{flp, অসত্য};
            failsp->addNext(setDone());
            windowp->addStmtsp(নূতন AstIf{flp, exprp->clone_বৃক্ষ_Pure(অসত্য), failsp});
            // if (rhs) { pass; done = 1; } -- consequent true at this !expr endpoint
            Ast_গ্রন্থি* ধ্রুব passsp = নূতন AstPExprClause{flp, সত্য};
            passsp->addNext(setDone());
            windowp->addStmtsp(নূতন AstIf{flp, rhsp, passsp});
            // @(clk) -- advance to next cycle in window
            windowp->addStmtsp(
                নূতন AstEventControl{flp, নূতন Ast_Sen_বৃক্ষ{flp, sensesp->clone_বৃক্ষ(অসত্য)}, nullptr});
            beginp->addStmtsp(windowp);
        } অন্যথা যদি (isNonConsec) {
            // Standalone nonconsec: ##1 into window, then pass
            beginp->addStmtsp(
                নূতন AstEventControl{flp, নূতন Ast_Sen_বৃক্ষ{flp, sensesp->clone_বৃক্ষ(অসত্য)}, nullptr});
            beginp->addStmtsp(নূতন AstPExprClause{flp, সত্য});
        } অন্যথা যদি (rhsp) {
            // Goto rep: check consequent once at match endpoint
            যদি (!isOverlapped) {
                beginp->addStmtsp(নূতন AstEventControl{
                    flp, নূতন Ast_Sen_বৃক্ষ{flp, sensesp->clone_বৃক্ষ(অসত্য)}, nullptr});
            }
            beginp->addStmtsp(নূতন AstIf{flp, rhsp, নূতন AstPExprClause{flp, সত্য},
                                        নূতন AstPExprClause{flp, অসত্য}});
        } অন্যথা {
            // Standalone goto: pass after counting
            beginp->addStmtsp(নূতন AstPExprClause{flp, সত্য});
        }

        প্রত্যাগম নূতন AstPExpr{flp, beginp, exprp->findBitDType()};
    }

    // Reject [=M:N] nonconsecutive range that reached V3AssertPre.
    // [->M:N] is fully handled upstream (NFA path); only [=M:N] still lands
    // here because AstSNonConsRep lowering is not yet implemented there.
    // Replaces the node with BitFalse so lowering continues after the warning.
    তার্কিক rejectNonconsecRange(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি_রাশিমালা* maxCountp) {
        যদি (!maxCountp) প্রত্যাগম অসত্য;
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: [=M:N] nonconsecutive range repetition"
                                     " (IEEE 1800-2023 16.9.2)");
        nodep->replaceWith(নূতন AstConst{nodep->fileline(), AstConst::BitFalse{}});
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        প্রত্যাগম সত্য;
    }

    শূন্য visit(AstSGotoRep* nodep) override {
        // Standalone goto rep (not inside implication antecedent)
        iterateChildren(nodep);
        File_সারি* ধ্রুব flp = nodep->fileline();
        Ast_গ্রন্থি_রাশিমালা* countp = nodep->countp()->unlinkFrBack();
        যদি (!validate_Rep_গুনতি(nodep, countp)) প্রত্যাগম;
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp = nodep->exprp()->unlinkFrBack();
        AstPExpr* ধ্রুব pexprp = createRepPExpr(flp, exprp, countp, nullptr, সত্য, অসত্য);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }

    শূন্য visit(AstSNonConsRep* nodep) override {
        // Standalone nonconsecutive rep (not inside implication antecedent)
        iterateChildren(nodep);
        যদি (rejectNonconsecRange(nodep, nodep->maxCountp())) প্রত্যাগম;
        File_সারি* ধ্রুব flp = nodep->fileline();
        Ast_গ্রন্থি_রাশিমালা* countp = nodep->countp()->unlinkFrBack();
        যদি (!validate_Rep_গুনতি(nodep, countp)) প্রত্যাগম;
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp = nodep->exprp()->unlinkFrBack();
        AstPExpr* ধ্রুব pexprp = createRepPExpr(flp, exprp, countp, nullptr, সত্য, সত্য);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }

    শূন্য visit(AstFuncRef* nodep) override {
        // IEEE 1800-2023 16.8: Inline sequence instances wherever they appear
        // in the expression tree (inside implications, boolean ops, nested refs, etc.)
        যদি (AstSequence* ধ্রুব seqp = VN_CAST(nodep->taskp(), Sequence)) {
            substituteSequenceCall(nodep, seqp);
            // The FuncRef has been replaced; do not access nodep after this point.
            // The replacement node will be visited by the parent's iterateChildren.
            প্রত্যাগম;
        }
        যদি (ধ্রুব AstProperty* ধ্রুব propp = VN_CAST(nodep->taskp(), Property)) {
            substituteNestedPropertyCall(nodep, propp);
            প্রত্যাগম;
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstImplication* nodep) override {
        যদি (nodep->sentreep()) প্রত্যাগম;  // Already processed

        // Top-level followed-by is claimed by V3AssertNfa; anything reaching here
        // is nested inside iff/implies/or. IEEE 1800-2023 16.12.9 permits the
        // composition, but silent lowering would drop non-vacuous-fail semantics.
        যদি (nodep->isFollowedBy()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: followed-by (#-# / #=#) nested inside property operator"
                          " (iff/implies/or) (IEEE 1800-2023 16.12.9)");
            nodep->replaceWith(নূতন AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            প্রত্যাগম;
        }

        // Handle goto repetition as antecedent before iterateChildren,
        // so the standalone AstSGotoRep visitor doesn't process it
        যদি (AstSGotoRep* ধ্রুব gotop = VN_CAST(nodep->lhsp(), SGotoRep)) {
            iterateChildren(gotop);
            iterateAndNextNull(nodep->rhsp());
            File_সারি* ধ্রুব flp = nodep->fileline();
            Ast_গ্রন্থি_রাশিমালা* countp = gotop->countp()->unlinkFrBack();
            যদি (!validate_Rep_গুনতি(nodep, countp)) প্রত্যাগম;
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp = gotop->exprp()->unlinkFrBack();
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব rhsp = nodep->rhsp()->unlinkFrBack();
            AstPExpr* ধ্রুব pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), অসত্য);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            প্রত্যাগম;
        }

        // Handle nonconsecutive repetition as antecedent before iterateChildren,
        // so the standalone AstSNonConsRep visitor doesn't process it
        যদি (AstSNonConsRep* ধ্রুব ncrp = VN_CAST(nodep->lhsp(), SNonConsRep)) {
            iterateChildren(ncrp);
            iterateAndNextNull(nodep->rhsp());
            যদি (rejectNonconsecRange(nodep, ncrp->maxCountp())) প্রত্যাগম;
            File_সারি* ধ্রুব flp = nodep->fileline();
            Ast_গ্রন্থি_রাশিমালা* countp = ncrp->countp()->unlinkFrBack();
            যদি (!validate_Rep_গুনতি(nodep, countp)) প্রত্যাগম;
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp = ncrp->exprp()->unlinkFrBack();
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব rhsp = nodep->rhsp()->unlinkFrBack();
            AstPExpr* ধ্রুব pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), সত্য);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            প্রত্যাগম;
        }

        iterateChildren(nodep);

        File_সারি* ধ্রুব flp = nodep->fileline();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_গ্রন্থি_রাশিমালা* lhsp = nodep->lhsp()->unlinkFrBack();

        // Lower sequence match items (IEEE 16.11): (expr, var = val, ...) |-> / |=>
        যদি (Ast_রাশিমালা_Stmt* ধ্রুব রাশিমালা_Stmtp = VN_CAST(lhsp, রাশিমালা_Stmt)) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব antExprp = রাশিমালা_Stmtp->resultp()->unlinkFrBack();

            যদি (nodep->isOverlapped()) {
                // |-> : assign to __Vpropvar via always_comb (continuous).
                // The assign evaluates RHS once; V3Sampled snapshots the
                // result so all consequent refs read the same value.
                Ast_গ্রন্থি* matchAssignsp = nullptr;
                প্রতি (Ast_গ্রন্থি* stmtp = রাশিমালা_Stmtp->stmtsp(); stmtp;) {
                    Ast_গ্রন্থি* ধ্রুব nextp = stmtp->nextp();
                    যদি (AstAssign* ধ্রুব assignp = VN_CAST(stmtp, Assign)) {
                        assignp->unlinkFrBack();
                        matchAssignsp = Ast_গ্রন্থি::addNextNull(matchAssignsp, assignp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(ঠেলো_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                যদি (matchAssignsp) {
                    AstAlways* ধ্রুব alwaysp
                        = নূতন AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, matchAssignsp};
                    m_modp->addStmtsp(alwaysp);
                }
            } অন্যথা {
                // |=> : assign to __Vpropvar via NBA in a clocked always block.
                // The NBA commits before the next cycle's sampled snapshot,
                // so the consequent (which already references __Vpropvar)
                // sees the captured value.
                Ast_গ্রন্থি* matchAssignsp = nullptr;
                প্রতি (Ast_গ্রন্থি* stmtp = রাশিমালা_Stmtp->stmtsp(); stmtp;) {
                    Ast_গ্রন্থি* ধ্রুব nextp = stmtp->nextp();
                    যদি (AstAssign* ধ্রুব assignp = VN_CAST(stmtp, Assign)) {
                        assignp->unlinkFrBack();
                        Ast_গ্রন্থি_রাশিমালা* ধ্রুব assignLhsp = assignp->lhsp()->unlinkFrBack();
                        Ast_গ্রন্থি_রাশিমালা* ধ্রুব assignRhsp = assignp->rhsp()->unlinkFrBack();
                        AstAssignDly* ধ্রুব dlyp = নূতন AstAssignDly{flp, assignLhsp, assignRhsp};
                        VL_DO_DANGLING(ঠেলো_Deletep(assignp), assignp);
                        matchAssignsp = Ast_গ্রন্থি::addNextNull(matchAssignsp, dlyp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(ঠেলো_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                যদি (matchAssignsp) {
                    AstIf* ধ্রুব condp
                        = নূতন AstIf{flp, antExprp->clone_বৃক্ষ_Pure(অসত্য), matchAssignsp};
                    AstAlways* ধ্রুব alwaysp
                        = নূতন AstAlways{flp, VAlwaysKwd::ALWAYS, new_Sen_বৃক্ষ(nodep), condp};
                    m_modp->addStmtsp(alwaysp);
                }
            }
        }

        যদি (AstPExpr* ধ্রুব pexprp = VN_CAST(rhsp, PExpr)) {
            // Implication with sequence expression on RHS (IEEE 1800-2023 16.11, 16.12.7).
            // The PExpr was lowered from the property expression earlier in this pass.
            // Wrap the PExpr body with the antecedent check so the sequence only
            // starts when the antecedent holds.
            Ast_গ্রন্থি_রাশিমালা* condp;
            যদি (nodep->isOverlapped()) {
                // Overlapped implication (|->): check antecedent on same cycle.
                // disable iff is applied at the assertion level, not at the
                // antecedent gate, matching the existing non-PExpr overlapped path.
                condp = lhsp;
            } অন্যথা {
                // Non-overlapped implication (|=>): check antecedent from previous cycle
                যদি (m_disablep) {
                    lhsp
                        = নূতন AstAnd{flp, নূতন AstNot{flp, m_disablep->clone_বৃক্ষ_Pure(অসত্য)}, lhsp};
                }
                AstPast* ধ্রুব pastp
                    = নূতন AstPast{flp, lhsp, nullptr, nullptr, /* propertyTiming */ সত্য};
                pastp->dtypeFrom(lhsp);
                pastp->sentreep(new_Sen_বৃক্ষ(nodep));
                condp = pastp;
            }
            // Wrap existing PExpr body: if (antecedent) { <original body> } else { /* vacuous pass
            // */ }
            // Only the statements are guarded; declarations stay directly in the block. After
            // V3Fork moves the block's statements into a task, V3Task only handles variables
            // that are direct statements of the task.
            AstBegin* ধ্রুব bodyp = pexprp->bodyp();
            AstIf* ধ্রুব guardp = নূতন AstIf{flp, condp};
            প্রতি (Ast_গ্রন্থি* stmtp = bodyp->stmtsp(); stmtp;) {
                Ast_গ্রন্থি* ধ্রুব nextp = stmtp->nextp();
                যদি (!VN_IS(stmtp, Var)) guardp->addThensp(stmtp->unlinkFrBack());
                stmtp = nextp;
            }
            bodyp->addStmtsp(guardp);
            nodep->replaceWith(pexprp);
            // Don't iterate pexprp here -- it was already iterated when created
            // (in visit(AstSExpr*)), so delays and disable iff are already processed.
        } অন্যথা যদি (nodep->isOverlapped()) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp
                = m_underCover ? static_cast<Ast_গ্রন্থি_রাশিমালা*>(নূতন AstLogAnd{flp, lhsp, rhsp})
                               : নূতন AstLogOr{flp, নূতন AstLogNot{flp, lhsp}, rhsp};
            nodep->replaceWith(exprp);
        } অন্যথা {
            যদি (m_disablep) {
                lhsp = নূতন AstAnd{flp, নূতন AstNot{flp, m_disablep->clone_বৃক্ষ_Pure(অসত্য)}, lhsp};
            }

            AstPast* ধ্রুব pastp
                = নূতন AstPast{flp, lhsp, nullptr, nullptr, /* propertyTiming */ সত্য};
            pastp->dtypeFrom(lhsp);
            pastp->sentreep(new_Sen_বৃক্ষ(nodep));
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp
                = m_underCover ? static_cast<Ast_গ্রন্থি_রাশিমালা*>(নূতন AstAnd{flp, pastp, rhsp})
                               : নূতন AstOr{flp, নূতন AstNot{flp, pastp}, rhsp};
            exprp->dtypeSetBit();
            nodep->replaceWith(exprp);
        }
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstUntil* nodep) override {
        File_সারি* ধ্রুব flp = nodep->fileline();
        UASSERT_OBJ(
            !m_pexprp, nodep,
            "'" << nodep->verilogKwd()
                << "' in complex property expression should have been rejected by V3AssertNfa");
        যদি (nodep->isStrong()
            && (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue())) {
            nodep->v3warn(E_NOTIMING, nodep->verilogKwd() << " requires --timing");
            nodep->replaceWith(নূতন AstConst{flp, AstConst::BitFalse{}});
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            প্রত্যাগম;
        }
        যদি (nodep->isStrong()) {
            // p s_until q / p s_until_with q: q must eventually be true. Until then, p must
            // be true on every sampled tick. For s_until, check q first: when q is true on
            // this tick, p is not required. For s_until_with, p must be true on the q tick too.
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব rawLhsp = nodep->lhsp()->unlinkFrBack();
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব rawRhsp = nodep->rhsp()->unlinkFrBack();
            AstSampled* ধ্রুব lhsp = নূতন AstSampled{flp, rawLhsp, rawLhsp->dtypep(), সত্য};
            AstSampled* ধ্রুব rhsp = নূতন AstSampled{flp, rawRhsp, rawRhsp->dtypep(), সত্য};
            Ast_গ্রন্থি_রাশিমালা* finalCondp = rhsp->clone_বৃক্ষ_Pure(অসত্য);
            যদি (nodep->isOverlapping()) {
                finalCondp = নূতন AstLogAnd{flp, lhsp->clone_বৃক্ষ_Pure(অসত্য), finalCondp};
            }

            // Track active assertion attempts. Only the count is needed to emit final failures.
            AstVar* ধ্রুব activep = নূতন AstVar{flp, VVarType::MODULETEMP, m_activeNames.get(""),
                                               nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            activep->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(activep);

            AstVar* ধ্রুব donep
                = নূতন AstVar{flp, VVarType::BLOCKTEMP, "__VassertDone", nodep->findBitDType()};
            donep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            auto setDone = [&]() {
                প্রত্যাগম নূতন AstAssign{flp, নূতন AstVarRef{flp, donep, VAccess::WRITE},
                                     নূতন AstConst{flp, AstConst::BitTrue{}}};
            };

            AstLoop* ধ্রুব loopp = নূতন AstLoop{flp};
            AstAssign* ধ্রুব decrementVar = নূতন AstAssign{
                flp, নূতন AstVarRef{flp, activep, VAccess::WRITE},
                নূতন AstSub{flp, নূতন AstVarRef{flp, activep, VAccess::READ}, নূতন AstConst{flp, 1}}};
            loopp->addStmtsp(নূতন AstLoopTest{
                flp, loopp, নূতন AstLogNot{flp, নূতন AstVarRef{flp, donep, VAccess::READ}}});
            {
                Ast_গ্রন্থি* ধ্রুব passp = নূতন AstPExprClause{flp};
                passp->addNext(decrementVar);
                passp->addNext(setDone());
                Ast_গ্রন্থি_রাশিমালা* passCondp = rhsp;
                যদি (nodep->isOverlapping()) {
                    passCondp = নূতন AstLogAnd{flp, lhsp->clone_বৃক্ষ_Pure(অসত্য), passCondp};
                }
                loopp->addStmtsp(নূতন AstIf{flp, passCondp, passp});
            }
            {
                Ast_গ্রন্থি* ধ্রুব failp = নূতন AstPExprClause{flp, অসত্য};
                failp->addNext(decrementVar->clone_বৃক্ষ(অসত্য));
                failp->addNext(setDone());
                loopp->addStmtsp(নূতন AstIf{
                    flp,
                    নূতন AstLogAnd{flp,
                                  নূতন AstLogNot{flp, নূতন AstVarRef{flp, donep, VAccess::READ}},
                                  নূতন AstLogNot{flp, lhsp}},
                    failp});
            }
            AstDelay* ধ্রুব delayp = নূতন AstDelay{flp, নূতন AstConst{flp, 1}, অসত্য};
            delayp->timeunit(m_modp->timeunit());
            loopp->addStmtsp(নূতন AstIf{
                flp, নূতন AstLogNot{flp, নূতন AstVarRef{flp, donep, VAccess::READ}}, delayp});

            // Main assertion block
            AstBegin* ধ্রুব bodyp = নূতন AstBegin{flp, m_blockNames.get(""), nullptr, সত্য};
            bodyp->addStmtsp(donep);
            bodyp->addStmtsp(নূতন AstAssign{flp, নূতন AstVarRef{flp, donep, VAccess::WRITE},
                                           নূতন AstConst{flp, AstConst::BitFalse{}}});
            File_সারি* ধ্রুব flp = activep->fileline();
            bodyp->addStmtsp(
                নূতন AstAssign{flp, নূতন AstVarRef{flp, activep, VAccess::WRITE},
                              নূতন AstAdd{flp, নূতন AstVarRef{flp, activep, VAccess::READ},
                                         নূতন AstConst{flp, 1}}});
            bodyp->addStmtsp(loopp);

            // Validate assertion condition for each active assert
            AstVar* ধ্রুব activeCountp = নূতন AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                    nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            activeCountp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

            AstAssign* ধ্রুব initActiveCountp
                = নূতন AstAssign{flp, নূতন AstVarRef{flp, activeCountp, VAccess::WRITE},
                                নূতন AstVarRef{flp, activep, VAccess::READ}};
            AstLoop* ধ্রুব finalLoopp = নূতন AstLoop{flp};
            finalLoopp->addStmtsp(
                নূতন AstLoopTest{flp, finalLoopp,
                                নূতন AstNeq{flp, নূতন AstVarRef{flp, activeCountp, VAccess::READ},
                                           নূতন AstConst{flp, 0}}});
            finalLoopp->addStmtsp(নূতন AstIf{flp, finalCondp, নূতন AstPExprClause{flp},
                                            নূতন AstPExprClause{flp, অসত্য}});
            finalLoopp->addStmtsp(
                নূতন AstAssign{flp, নূতন AstVarRef{flp, activeCountp, VAccess::WRITE},
                              নূতন AstSub{flp, নূতন AstVarRef{flp, activeCountp, VAccess::READ},
                                         নূতন AstConst{flp, 1}}});

            // Final assertion block
            AstBegin* ধ্রুব finalp = নূতন AstBegin{flp, m_blockNames.get(""), nullptr, সত্য};
            finalp->addStmtsp(activeCountp);
            finalp->addStmtsp(initActiveCountp);
            finalp->addStmtsp(finalLoopp);

            AstPExpr* ধ্রুব pexprp = নূতন AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
            VL_RESTORER(m_pexprp);
            VL_RESTORER(m_hasCycleDelay);
            m_pexprp = pexprp;
            m_hasCycleDelay = অসত্য;
            iterate(bodyp);
            iterate(finalp);

            UASSERT_OBJ(!m_hasCycleDelay, nodep,
                        "s_until cycle delay should have been handled by V3AssertNfa");

            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            প্রত্যাগম;
        }
        AstLoop* ধ্রুব loopp = নূতন AstLoop{flp};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব lhsp = nodep->lhsp()->unlinkFrBack();
        AstLogAnd* ধ্রুব loopCondp = নূতন AstLogAnd{flp, lhsp, নূতন AstLogNot{flp, rhsp}};
        loopp->addStmtsp(নূতন AstLoopTest{flp, loopp, loopCondp});
        loopp->addStmtsp(নূতন AstEventControl{flp, new_Sen_বৃক্ষ(nodep), nullptr});

        Ast_গ্রন্থি_রাশিমালা* ধ্রুব rhsCopyp = rhsp->clone_বৃক্ষ_Pure(অসত্য);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব passCondp
            = nodep->isOverlapping() ? নূতন AstLogAnd{flp, lhsp->clone_বৃক্ষ_Pure(অসত্য), rhsCopyp}
                                     : rhsCopyp;
        AstBegin* ধ্রুব beginp = নূতন AstBegin{flp, m_blockNames.get(""), loopp, সত্য};
        beginp->addStmtsp(
            নূতন AstIf{flp, passCondp, নূতন AstPExprClause{flp}, নূতন AstPExprClause{flp, অসত্য}});

        AstPExpr* ধ্রুব pexprp = নূতন AstPExpr{flp, beginp, nodep->dtypep()};
        pexprp->user1(1);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }

    শূন্য visit(AstDefaultDisable* nodep) override {
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
    }
    শূন্য visit(AstInferredDisable* nodep) override {
        Ast_গ্রন্থি* newp;
        যদি (m_defaultDisablep) {
            newp = m_defaultDisablep->condp()->clone_বৃক্ষ_Pure(সত্য);
        } অন্যথা {
            newp = নূতন AstConst{nodep->fileline(), AstConst::BitFalse{}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }

    শূন্য visit(AstPropSpec* nodep) override {
        nodep = substitutePropertyCall(nodep);
        iterateAndNextNull(nodep->sensesp());
        যদি (m_senip && m_senip != nodep->sensesp())
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Only one PSL clock allowed per assertion");
        ধ্রুব AstCover* ধ্রুব coverp = VN_CAST(nodep->backp(), Cover);
        ধ্রুব তার্কিক seqEvent = coverp && coverp->isSeqEvent();
        যদি (!nodep->disablep() && m_defaultDisablep && !seqEvent) {
            nodep->disablep(m_defaultDisablep->condp()->clone_বৃক্ষ_Pure(সত্য));
        }
        m_disablep = nodep->disablep();
        // Unlink and just keep a pointer to it, convert to sentree as needed
        m_senip = nodep->sensesp();
        iterateNull(nodep->disablep());
        যদি (!VN_AS(nodep->backp(), গ্রন্থি_Cover_Or_Assert)->immediate()) {
            ধ্রুব Ast_গ্রন্থি_DType* ধ্রুব propDtp = nodep->propp()->dtypep();
            nodep->propp(নূতন AstSampled{nodep->fileline(), nodep->propp()->unlinkFrBack(),
                                        propDtp->dtypep(), সত্য});
        }
        // cover counts non-vacuous matches only (IEEE 1800-2023 16.15.2), so an
        // implication antecedent must hold; assert passes vacuously instead.
        VL_RESTORER(m_underCover);
        m_underCover = VN_IS(nodep->backp(), Cover);
        iterate(nodep->propp());
        iterateNull(nodep->matchCountp());
    }
    শূন্য visit(AstPExpr* nodep) override {
        // V3AssertNfa handles multi-cycle property expressions before this pass,
        // so the following unsupported paths are defensive and typically unreached.
        যদি (m_pexprp && m_pexprp->user1()) {  // LCOV_EXCL_START
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Complex property expression inside 'until''");
            nodep->replaceWith(নূতন AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            প্রত্যাগম;
        }  // LCOV_EXCL_STOP
        যদি (AstLogNot* ধ্রুব notp = VN_CAST(nodep->backp(), LogNot)) {
            notp->replaceWith(nodep->unlinkFrBack());
            VL_DO_DANGLING(ঠেলো_Deletep(notp), notp);
            iterate(nodep);
            প্রত্যাগম;
        }
        // Sequence expression as antecedent of implication is not yet supported
        যদি (AstImplication* ধ্রুব implp = VN_CAST(nodep->backp(), Implication)) {
            যদি (implp->lhsp() == nodep) {  // LCOV_EXCL_START
                implp->v3warn(E_UNSUPPORTED,
                              "Unsupported: Implication with sequence expression as antecedent");
                nodep->replaceWith(নূতন AstConst{nodep->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                প্রত্যাগম;
            }  // LCOV_EXCL_STOP
        }
        VL_RESTORER(m_pexprp);
        VL_RESTORER(m_disableSeqIfp);
        m_pexprp = nodep;

        যদি (m_disablep) {
            ধ্রুব AstSampled* sampledp = nullptr;
            যদি (m_disablep->exists([&sampledp](ধ্রুব AstSampled* ধ্রুব sp) {
                    sampledp = sp;
                    প্রত্যাগম সত্য;
                })) {
                sampledp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: $sampled inside disabled condition of a sequence");
                m_disablep = নূতন AstConst{m_disablep->fileline(), AstConst::BitFalse{}};
                // always a copy is used, so remove it now
                ঠেলো_Deletep(m_disablep);  // m_disablep used later
            }
            File_সারি* ধ্রুব flp = nodep->fileline();
            // Add counter which counts times the condition turned true
            AstVar* ধ্রুব disableCntp
                = নূতন AstVar{flp, VVarType::MODULETEMP, m_disableCntNames.get(""),
                             nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            disableCntp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(disableCntp);
            AstVarRef* ধ্রুব readCntRefp = নূতন AstVarRef{flp, disableCntp, VAccess::READ};
            AstVarRef* ধ্রুব writeCntRefp = নূতন AstVarRef{flp, disableCntp, VAccess::WRITE};
            AstAssign* ধ্রুব incrStmtp = নূতন AstAssign{
                flp, writeCntRefp, নূতন AstAdd{flp, readCntRefp, নূতন AstConst{flp, 1}}};
            AstAlways* ধ্রুব alwaysp
                = নূতন AstAlways{flp, VAlwaysKwd::ALWAYS,
                                নূতন Ast_Sen_বৃক্ষ{flp, নূতন AstSenItem{flp, VEdgeType::ET_POSEDGE,
                                                                   m_disablep->clone_বৃক্ষ(অসত্য)}},
                                incrStmtp};
            disableCntp->addNextHere(alwaysp);

            // Store value of that counter at the beginning of sequence evaluation
            AstBegin* ধ্রুব bodyp = nodep->bodyp();
            AstVar* ধ্রুব initialCntp = নূতন AstVar{flp, VVarType::BLOCKTEMP, "__VinitialCnt",
                                                   nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            initialCntp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            AstAssign* ধ্রুব assignp
                = নূতন AstAssign{flp, নূতন AstVarRef{flp, initialCntp, VAccess::WRITE},
                                readCntRefp->clone_বৃক্ষ(অসত্য)};
            // Prepend to the sequence body to keep statement list structure valid.
            UASSERT_OBJ(!bodyp->name().empty(), bodyp, "Sequence body should be named");
            Ast_গ্রন্থি* ধ্রুব newStmtsp = Ast_গ্রন্থি::addNext<Ast_গ্রন্থি, Ast_গ্রন্থি>(initialCntp, assignp);
            bodyp->prependStmtsp(newStmtsp);

            m_disableSeqIfp
                = নূতন AstIf{flp, নূতন AstEq{flp, নূতন AstVarRef{flp, initialCntp, VAccess::READ},
                                           readCntRefp->clone_বৃক্ষ(অসত্য)}};
            // Delete it, because it is always copied before insetion to the AST
            ঠেলো_Deletep(m_disableSeqIfp);  // m_disableSeqIfp used later
        }
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultClkEvtVarp);
        VL_RESTORER(m_defaultDisablep);
        VL_RESTORER(m_modp);
        যদি (!VN_IS(nodep, Class)) {
            // A class nested in a module is in the scope of the module's default clocking
            m_defaultClockingp = nullptr;
            m_defaultClkEvtVarp = nullptr;
        }
        m_defaultDisablep = nodep->defaultDisablep();
        m_modp = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    শূন্য visit(AstProperty* nodep) override {
        // The body will be visited when will be substituted in place of property reference
        // (AstFuncRef)
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
    }
    শূন্য visit(AstSequence* nodep) override {
        // Sequence declarations are not visited directly; their bodies are inlined
        // at call sites by visit(AstFuncRef*). Collect for post-traversal cleanup.
        m_seqsToCleanup.ঠেলো_back(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সার্বজনিক:
    // CONSTRUCTORS
    explicit AssertPreVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        { AssertPreClockingVisitor{nodep}; }
        // Process
        iterate(nodep);
        // Fix up varref names
        প্রতি (AstVarXRef* xrefp : m_xrefsp) xrefp->name(xrefp->varp()->name());
        // Clean up sequence declarations after inlining.
        // Referenced sequences that were inlined have isReferenced cleared.
        // Remaining referenced sequences are in unsupported contexts (e.g. @seq event).
        প্রতি (AstSequence* seqp : m_seqsToCleanup) {
            যদি (seqp->isReferenced()) {
                seqp->v3warn(E_UNSUPPORTED,
                             "Unsupported: sequence referenced outside assertion property");
            } অন্যথা {
                VL_DO_DANGLING(seqp->unlinkFrBack()->delete_বৃক্ষ(), seqp);
            }
        }
    }
    ~AssertPreVisitor() override = স্বভাব;
};

//######################################################################
// Top Assert class

শূন্য V3AssertPre::assertPreAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { AssertPreVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("assertpre", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
