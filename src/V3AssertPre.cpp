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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3AssertPre.h"

#ಅನ್ತರ್ಭಾವ "V3Assert.h"
#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3Task.h"
#ಅನ್ತರ್ಭಾವ "V3Timing.h"
#ಅನ್ತರ್ಭಾವ "V3UniqueNames.h"

#ಅನ್ತರ್ಭಾವ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clocking blocks of clocking items

ವರ್ಗ AssertPreClockingVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // Captures the clocking block of each clocking item before AssertPreVisitor, which needs it
    // at synchronous drives, often visited before the clocking block, e.g. in a parent module
    // NODE STATE
    // AstClockingItem::user3p()  // AstClocking*. Clocking block of the item, user3 claimed by
    //                            //               AssertPreVisitor

    // VISITORS
    ಶೂನ್ಯ visit(AstNetlist* nodep) override { iterateAndNextConstNull(nodep->modulesp()); }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override { iterateAndNextConstNull(nodep->stmtsp()); }
    ಶೂನ್ಯ visit(AstGenBlock* nodep) override { iterateAndNextConstNull(nodep->itemsp()); }
    ಶೂನ್ಯ visit(AstClocking* nodep) override {
        ಪ್ರತಿ (Ast_ಗಂಟು* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            ಯದಿ (AstClockingItem* ಧ್ರುವ citemp = VN_CAST(itemp, ClockingItem)) {
                citemp->user3p(nodep);
            }
        }
    }
    // Clocking blocks are only in modules and their generate blocks
    ಶೂನ್ಯ visit(Ast_ಗಂಟು*) override {}

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit AssertPreClockingVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~AssertPreClockingVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// Assert class functions

ವರ್ಗ AssertPreVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // Removes clocks and other pre-optimizations
    // Eventually inlines calls to sequences, properties, etc.
    // We're not parsing the tree, or anything more complicated.
ಗುಪ್ತ:
    // TYPES
    ಸಂರಚನಾ SynchDrive final {
        AstClockingItem* itemp;  // Clocking item of the driven clockvar
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* refp;  // Reference to the driven clockvar
    };

    // NODE STATE
    // AstClockingItem::user1p()         // AstVar*.      varp() of ClockingItem after unlink
    // AstClockingItem::user2p()         // AstVar*.      Flag set by drives of output clockvar
    // AstClockingItem::user3p()         // AstClocking*. Clocking block of the item, captured
    //                                   //               by AssertPreClockingVisitor
    // AstPExpr::user1()                 // bool.         Created from AstUntil
    ಧ್ರುವ VNUser1InUse m_inuser1;
    ಧ್ರುವ VNUser2InUse m_inuser2;
    ಧ್ರುವ VNUser3InUse m_inuser3;
    // STATE
    // Current context:
    AstNetlist* ಧ್ರುವ m_netlistp = nullptr;  // Current netlist
    Ast_ಗಂಟು_Module* m_modp = nullptr;  // Current module
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
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_disablep = nullptr;  // Last disable
    AstIf* m_disableSeqIfp = nullptr;  // Used for handling disable iff in sequences
    AstPExpr* m_pexprp = nullptr;  // Last AstPExpr
    ತಾರ್ಕಿಕ m_underCover = ಅಸತ್ಯ;  // True if the enclosing assertion is a cover
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
    ತಾರ್ಕಿಕ m_inAssign = ಅಸತ್ಯ;  // True if in an AssignNode
    ತಾರ್ಕಿಕ m_inAssignDlyLhs = ಅಸತ್ಯ;  // True if in AssignDly's LHS
    ತಾರ್ಕಿಕ m_inSynchDrive = ಅಸತ್ಯ;  // True if in synchronous drive
    ತಾರ್ಕಿಕ m_inConcatAssign = ಅಸತ್ಯ;  // True if in assignment to a concatenation
    std::vector<SynchDrive> m_drives;  // Clockvars written by the synchronous drive
    ತಾರ್ಕಿಕ m_hasCycleDelay = ಅಸತ್ಯ;  // True if node has cycle delay beneath
    std::vector<AstVarXRef*> m_xrefsp;  // list of xrefs that need name fixup
    std::vector<AstSequence*> m_seqsToCleanup;  // Sequences to clean up after traversal

    // METHODS

    // Flag set by each drive of an output clockvar, added to the module with its clocking item
    AstVar* getCreateDrivenVarp(AstClockingItem* itemp) {
        ಯದಿ (!itemp->user2p()) {
            AstVar* ಧ್ರುವ varp = ನೂತನ AstVar{itemp->fileline(), VVarType::MODULETEMP,
                                            m_drivenNames.get(itemp), itemp->findBitDType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            itemp->user2p(varp);
        }
        ಪ್ರತ್ಯಾಗಮ VN_AS(itemp->user2p(), Var);
    }
    // Reference to a variable in the scope of a driven clockvar, made like the clockvar's
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newDriveRefp(File_ಸಾಲು* flp, ಧ್ರುವ SynchDrive& drive, AstVar* varp,
                                     ಧ್ರುವ VAccess& access) {
        ಯದಿ (ಧ್ರುವ AstVarXRef* ಧ್ರುವ xrefp = VN_CAST(drive.refp, VarXRef)) {
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarXRef{flp, varp, xrefp->dotted(), access};
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstMemberSel* ಧ್ರುವ selp = VN_CAST(drive.refp, MemberSel)) {
            // The interface expression is evaluated again, so it must not have side effects;
            // cloneTreePure warns if it has any
            AstMemberSel* ಧ್ರುವ newSelp
                = ನೂತನ AstMemberSel{flp, selp->fromp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), varp};
            newSelp->access(access);
            ಪ್ರತ್ಯಾಗಮ newSelp;
        }
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{flp, varp, access};
    }
    // Assignment setting the drive flag of a driven clockvar
    AstAssign* newDrivenSetp(File_ಸಾಲು* flp, ಧ್ರುವ SynchDrive& drive) {
        AstVar* ಧ್ರುವ varp = getCreateDrivenVarp(drive.itemp);
        AstAssign* ಧ್ರುವ setp = ನೂತನ AstAssign{flp, newDriveRefp(flp, drive, varp, VAccess::WRITE),
                                              ನೂತನ AstConst{flp, AstConst::BitTrue{}}};
        setp->user1(ಸತ್ಯ);
        ಪ್ರತ್ಯಾಗಮ setp;
    }
    // Clocking block of a clocking item
    ಸ್ಥಿರ AstClocking* clockingOf(ಧ್ರುವ AstClockingItem* itemp) {
        UASSERT_OBJ(itemp->user3p(), itemp, "Clocking item not captured in a clocking block");
        ಪ್ರತ್ಯಾಗಮ VN_AS(itemp->user3p(), Clocking);
    }
    // Clocking event of a driven clockvar, referenced from the drive like the clockvar, or
    // nullptr if unsupported, which is reported on the drive's cycle delay
    Ast_Sen_ವೃಕ್ಷ* newDriveSentreep(AstDelay* delayp, ಧ್ರುವ SynchDrive& drive) {
        Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ origp = clockingOf(drive.itemp)->sentreep();
        ಯದಿ (VN_IS(drive.refp, VarRef)) ಪ್ರತ್ಯಾಗಮ origp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
        ಯದಿ (origp->exists([](ಧ್ರುವ AstVarXRef*) { ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ; })) {
            // The reference is relative to the clocking block
            delayp->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in synchronous drive to"
                                          " clockvar in another scope, whose clocking event"
                                          " has a hierarchical reference");
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }
        ಧ್ರುವ AstMemberSel* ಧ್ರುವ selp = VN_CAST(drive.refp, MemberSel);
        // Processes outside classes evaluate their events also when not waiting on them,
        // possibly before the interface reference is set, so the event is guarded against a
        // null reference, which needs an integral value; scheduling does not guard it
        ಧ್ರುವ ತಾರ್ಕಿಕ guardNull = selp && !VN_IS(m_modp, Class);
        ಯದಿ (guardNull) {
            ಪ್ರತಿ (AstSenItem* itemp = origp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                ಯದಿ (!itemp->sensp()->dtypep()->skipRefp()->isIntegralOrPacked()) {
                    delayp->v3warn(E_UNSUPPORTED,
                                   "Unsupported: cycle delay in synchronous drive through"
                                   " an interface reference outside a class, to clocking"
                                   " block with non-integral clocking event");
                    ಪ್ರತ್ಯಾಗಮ nullptr;
                }
            }
        }
        // Interface declaring the clocking block, if driven through an interface reference
        AstIface* ಧ್ರುವ ifacep
            = selp ? VN_AS(selp->fromp()->dtypep()->skipRefp(), IfaceRefDType)->ifaceViaCellp()
                   : nullptr;
        Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ sentreep = origp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
        std::vector<AstVarRef*> refps;
        sentreep->foreach([&](AstVarRef* refp) {
            ಯದಿ (!refp->classOrPackagep()) refps.ತಳ್ಳು_back(refp);
        });
        ಪ್ರತಿ (AstVarRef* ಧ್ರುವ refp : refps) {
            AstVar* ಧ್ರುವ varp = refp->varp();
            ಯದಿ (ifacep) {
                // Mark the variable as read through an interface reference, as V3Width does for
                // a member select. The mark is the interface declaring the variable, whose
                // instances all share it, so drives through references to any instances agree.
                UASSERT_OBJ(!varp->sensIfacep() || varp->sensIfacep() == ifacep, refp,
                            "Variable read through references to different interfaces");
                varp->sensIfacep(ifacep);
            }
            refp->replaceWith(newDriveRefp(refp->fileline(), drive, varp, refp->access()));
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp), refp);
        }
        ಯದಿ (guardNull) {
            ಪ್ರತಿ (AstSenItem* itemp = sentreep->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                guardNullIface(itemp, selp->fromp());
            }
        }
        ಪ್ರತ್ಯಾಗಮ sentreep;
    }
    // Make an integral clocking event through an interface reference false while the reference
    // is null
    ಸ್ಥಿರ ಶೂನ್ಯ guardNullIface(AstSenItem* sensesp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp) {
        File_ಸಾಲು* ಧ್ರುವ flp = sensesp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ sensp = sensesp->sensp()->unlinkFrBack();
        sensesp->sensp(ನೂತನ AstCond{flp, newNotNullp(flp, fromp), sensp,
                                   ನೂತನ AstConst{flp, AstConst::DTyped{}, sensp->dtypep()}});
        ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp = sensesp->condp()) {
            condp->unlinkFrBack();
            sensesp->condp(ನೂತನ AstLogAnd{flp, newNotNullp(flp, fromp), condp});
        }
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newNotNullp(File_ಸಾಲು* flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp) {
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstNeq{flp, fromp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), ನೂತನ AstConst{flp, AstConst::Null{}}};
    }
    // Procedural '##0': if the default clocking event has not yet occurred in this time step,
    // wait for it; otherwise continue without suspension (IEEE 1800-2023 14.11)
    AstIf* newCycleDelayZerop(File_ಸಾಲು* flp) ಧ್ರುವ {
        AstVar* ಧ್ರುವ evtVarp = m_defaultClkEvtVarp;
        UASSERT_OBJ(evtVarp, flp, "Default clocking event var not pre-created");
        AstCMethodHard* ಧ್ರುವ isTriggeredp = ನೂತನ AstCMethodHard{
            flp, ನೂತನ AstVarRef{flp, evtVarp, VAccess::READ}, VCMethod::EVENT_IS_TRIGGERED};
        isTriggeredp->dtypeSetBit();
        AstEventControl* ಧ್ರುವ waitp = ನೂತನ AstEventControl{
            flp,
            ನೂತನ Ast_Sen_ವೃಕ್ಷ{flp, ನೂತನ AstSenItem{flp, VEdgeType::ET_EVENT,
                                               ನೂತನ AstVarRef{flp, evtVarp, VAccess::READ}}},
            nullptr};
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstIf{flp, ನೂತನ AstNot{flp, isTriggeredp}, waitp};
    }

    ಸ್ಥಿರ ಶೂನ್ಯ checkSamplingFuncDType(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, ಧ್ರುವ Ast_ಗಂಟು* exprp) {
        ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = exprp->dtypep()->skipRefp();
        ಯದಿ (!dtypep->isIntegralOrPacked()) {
            nodep->v3error("Expected numeric type, but got a " << dtypep->prettyDTypeNameQ()
                                                               << " data type");
        }
    }

    Ast_Sen_ವೃಕ್ಷ* new_Sen_ವೃಕ್ಷ(Ast_ಗಂಟು* nodep, Ast_Sen_ವೃಕ್ಷ* useTreep = nullptr,
                           Ast_ಗಂಟು_Cover_Or_Assert* cassertp = nullptr) {
        // Create sentree based on clocked or default clock
        // Return nullptr for always
        ಯದಿ (useTreep) ಪ್ರತ್ಯಾಗಮ useTreep;
        Ast_Sen_ವೃಕ್ಷ* newp = nullptr;
        AstSenItem* senip = m_senip;
        ತಾರ್ಕಿಕ fromAlways = ಅಸತ್ಯ;
        ಯದಿ (!senip && m_defaultClockingp) senip = m_defaultClockingp->sentreep()->sensesp();
        ಯದಿ (!senip) {
            senip = m_seniAlwaysp;
            fromAlways = ಸತ್ಯ;
        }
        ಯದಿ (!senip) {
            nodep->v3error("Concurrent assertion has no clock (IEEE 1800-2023 16.16)\n"
                           << nodep->warnMore()
                           << "... Suggest provide a clocking event, a default"
                              " clocking, or a clocked procedural context");
            newp = ನೂತನ Ast_Sen_ವೃಕ್ಷ{nodep->fileline(), nullptr};
        } ಅನ್ಯಥಾ {
            ಯದಿ (cassertp && fromAlways) cassertp->senFromAlways(ಸತ್ಯ);
            newp = ನೂತನ Ast_Sen_ವೃಕ್ಷ{nodep->fileline(), senip->clone_ವೃಕ್ಷ(ಸತ್ಯ)};
        }
        ಪ್ರತ್ಯಾಗಮ newp;
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* getSequenceBodyExprp(ಧ್ರುವ AstSequence* ಧ್ರುವ seqp) ಧ್ರುವ {
        // The statements in AstSequence are optional AstVar (ports) followed by body expr.
        Ast_ಗಂಟು* bodyp = seqp->stmtsp();
        ಯಾವತ್ (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        ಪ್ರತ್ಯಾಗಮ VN_CAST(bodyp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
    }
    AstPropSpec* getPropertyExprp(ಧ್ರುವ AstProperty* ಧ್ರುವ propp) {
        // Statements in AstProperty: AstVar (ports/local vars),
        // AstInitialStaticStmt/AstInitialAutomaticStmt (var init), AstPropSpec (body).
        Ast_ಗಂಟು* propExprp = propp->stmtsp();
        ಯಾವತ್ (propExprp
               && (VN_IS(propExprp, Var) || VN_IS(propExprp, InitialStaticStmt)
                   || VN_IS(propExprp, InitialAutomaticStmt))) {
            propExprp = propExprp->nextp();
        }
        ಪ್ರತ್ಯಾಗಮ VN_CAST(propExprp, PropSpec);
    }
    ಶೂನ್ಯ substituteSequenceCall(AstFuncRef* funcrefp, AstSequence* seqp) {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Inline the sequence body at the call site, replacing the FuncRef
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        // Clone the body expression since the sequence may be referenced multiple times
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* clonedp = bodyExprp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
        // Build substitution map, then do a single traversal to replace all formals
        // (textual substitution per IEEE 16.8.2).
        ಧ್ರುವ V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<ಧ್ರುವ AstVar*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> portMap;
        ಪ್ರತಿ (ಧ್ರುವ auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            ಧ್ರುವ auto it = portMap.find(refp->varp());
            ಯದಿ (it != portMap.end()) {
                refp->replaceWith(it->second->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp), refp);
            }
        });
        ಪ್ರತಿ (ಧ್ರುವ auto& tconnect : tconnects) {
            ತಳ್ಳು_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        // Replace the FuncRef with the inlined body
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(funcrefp), funcrefp);
        // Clear referenced flag; sequences with isReferenced==false are deleted in assertPreAll
        seqp->isReferenced(ಅಸತ್ಯ);
    }
    AstPropSpec* clonePropertyBody(AstFuncRef* funcrefp, ಧ್ರುವ AstProperty* propp) {
        AstPropSpec* propExprp = getPropertyExprp(propp);
        // Substitute inner property call before copying in order to not doing the same for
        // each call of outer property call.
        propExprp = substitutePropertyCall(propExprp);
        // Clone subtree after substitution. It is needed, because property might be called
        // multiple times with different arguments.
        propExprp = propExprp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
        // Build substitution maps for formal arguments and property-local
        // variables, then perform a single foreach to apply all replacements.
        // Map port vars to their actual argument expressions
        ಧ್ರುವ V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propp->stmtsp());
        std::unordered_map<ಧ್ರುವ AstVar*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> portMap;
        ಪ್ರತಿ (ಧ್ರುವ auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }

        // Promote property-local variables (non-port vars, IEEE 16.10) to
        // module-level __Vpropvar temps. Cross-cycle persistence is handled
        // by the match item lowering in visit(AstImplication*).
        std::unordered_map<ಧ್ರುವ AstVar*, AstVar*> localVarMap;
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = propp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(stmtp, Var)) {
                ಯದಿ (!varp->isIO()) {
                    ಧ್ರುವ string newName = m_propVarNames.get(varp);
                    AstVar* ಧ್ರುವ newVarp = ನೂತನ AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->addStmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        // Single traversal: substitute ports and update local var references
        propExprp->foreach([&](AstVarRef* refp) {
            ಧ್ರುವ auto portIt = portMap.find(refp->varp());
            ಯದಿ (portIt != portMap.end()) {
                refp->replaceWith(portIt->second->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp), refp);
                ಪ್ರತ್ಯಾಗಮ;
            }
            ಧ್ರುವ auto localIt = localVarMap.find(refp->varp());
            ಯದಿ (localIt != localVarMap.end()) refp->varp(localIt->second);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        // Clean up argument expressions
        ಪ್ರತಿ (ಧ್ರುವ auto& tconnect : tconnects) {
            ತಳ್ಳು_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(propExprp, funcrefp);
        ಪ್ರತ್ಯಾಗಮ propExprp;
    }
    AstPropSpec* substitutePropertyCall(AstPropSpec* nodep) {
        ಯದಿ (AstFuncRef* ಧ್ರುವ funcrefp = VN_CAST(nodep->propp(), FuncRef)) {
            ಯದಿ (ಧ್ರುವ AstProperty* ಧ್ರುವ propp = VN_CAST(funcrefp->taskp(), Property)) {
                AstPropSpec* ಧ್ರುವ propExprp = clonePropertyBody(funcrefp, propp);

                // Handle case with 2 disable iff statement (IEEE 1800-2023 16.12.1)
                ಯದಿ (nodep->disablep() && propExprp->disablep()) {
                    nodep->v3error("disable iff expression before property call and in its "
                                   "body is not legal");
                    ತಳ್ಳು_Deletep(propExprp->disablep()->unlinkFrBack());
                }
                // If disable iff is in outer property, move it to inner
                ಯದಿ (nodep->disablep()) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ disablep = nodep->disablep()->unlinkFrBack();
                    propExprp->disablep(disablep);
                }

                ಯದಿ (nodep->sensesp() && propExprp->sensesp()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Clock event before property call and in its body");
                    ತಳ್ಳು_Deletep(propExprp->sensesp()->unlinkFrBack());
                }
                // If clock event is in outer property, move it to inner
                ಯದಿ (nodep->sensesp()) {
                    AstSenItem* ಧ್ರುವ sensesp = nodep->sensesp();
                    sensesp->unlinkFrBack();
                    propExprp->sensesp(sensesp);
                }

                // Now substitute property reference with property body
                nodep->replaceWith(propExprp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಪ್ರತ್ಯಾಗಮ propExprp;
            }
        }
        ಪ್ರತ್ಯಾಗಮ nodep;
    }

    ಶೂನ್ಯ substituteNestedPropertyCall(AstFuncRef* funcrefp, ಧ್ರುವ AstProperty* propp) {
        AstPropSpec* ಧ್ರುವ specp = clonePropertyBody(funcrefp, propp);
        Ast_ಗಂಟು* replp = nullptr;
        ಯದಿ (specp->disablep()) {
            funcrefp->v3error("Illegal 'disable iff' in a nested property instance"
                              " (IEEE 1800-2023 16.12.1)");
        } ಅನ್ಯಥಾ ಯದಿ (specp->sensesp()) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: clock event in a nested property instance");
        } ಅನ್ಯಥಾ ಯದಿ (specp->propp()->exists(
                       [](ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* np) { ಪ್ರತ್ಯಾಗಮ np->isMultiCycleSva(); })) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: multi-cycle body in a nested property instance");
        } ಅನ್ಯಥಾ {
            replp = specp->propp()->unlinkFrBack();
        }
        ಯದಿ (!replp) replp = ನೂತನ AstConst{funcrefp->fileline(), AstConst::BitFalse{}};
        funcrefp->replaceWith(replp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(funcrefp), funcrefp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(specp), specp);
    }

    // VISITORS
    //========== Statements
    ಶೂನ್ಯ visit(AstClocking* ಧ್ರುವ nodep) override {
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        UINFO(8, "   CLOCKING" << nodep);
        ಯದಿ (nodep->isDefault()) {
            ಯದಿ (m_defaultClockingp) {
                nodep->v3error("Only one default clocking block allowed per module"
                               " (IEEE 1800-2023 14.12)");
            } ಅನ್ಯಥಾ {
                m_defaultClockingp = nodep;
                m_defaultClkEvtVarp = nodep->ensureEventp();
            }
        }
        iterateChildren(nodep);
        ಯದಿ (nodep->eventp()) nodep->addNextHere(nodep->eventp()->unlinkFrBack());
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ಶೂನ್ಯ visit(AstModportClockingRef* ಧ್ರುವ nodep) override {
        // It has to be converted to a list of ModportClockingVarRefs,
        // because clocking blocks are removed in this pass
        ಪ್ರತಿ (Ast_ಗಂಟು* itemp = nodep->clockingp()->itemsp(); itemp; itemp = itemp->nextp()) {
            ಯದಿ (AstClockingItem* ಧ್ರುವ citemp = VN_CAST(itemp, ClockingItem)) {
                ಯದಿ (AstVar* ಧ್ರುವ varp
                    = citemp->varp() ? citemp->varp() : VN_AS(citemp->user1p(), Var)) {
                    AstModportVarRef* ಧ್ರುವ modVarp = ನೂತನ AstModportVarRef{
                        nodep->fileline(), varp->name(), citemp->direction()};
                    modVarp->varp(varp);
                    nodep->addNextHere(modVarp);
                    ಯದಿ (citemp->direction().isOutput()) {
                        // Drives through the modport set the drive flag
                        AstVar* ಧ್ರುವ drivenVarp = getCreateDrivenVarp(citemp);
                        AstModportVarRef* ಧ್ರುವ drivenRefp = ನೂತನ AstModportVarRef{
                            nodep->fileline(), drivenVarp->name(), VDirection::OUTPUT};
                        drivenRefp->varp(drivenVarp);
                        nodep->addNextHere(drivenRefp);
                    }
                }
            }
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ಶೂನ್ಯ visit(AstClockingItem* ಧ್ರುವ nodep) override {
        // Get a ref to the sampled/driven variable
        AstVar* ಧ್ರುವ varp = nodep->varp();
        ಯದಿ (!varp) {
            // Unused item
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Flag set by drives of an output clockvar, possibly visited before this item
        ಯದಿ (nodep->direction().isOutput()) m_modp->addStmtsp(getCreateDrivenVarp(nodep));
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        V3Const::constifyEdit(nodep->skewp());
        ಯದಿ (!VN_IS(nodep->skewp(), Const)) {
            nodep->skewp()->v3error("Skew must be constant (IEEE 1800-2023 14.4)");
            ಪ್ರತ್ಯಾಗಮ;
        }
        AstConst* ಧ್ರುವ skewp = VN_AS(nodep->skewp(), Const);
        ಯದಿ (skewp->ಅಂಕೆ().isNegative()) skewp->v3error("Skew cannot be negative");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = nodep->exprp();
        varp->name(m_clockingp->name() + "__DOT__" + varp->name());
        m_clockingp->addNextHere(varp->unlinkFrBack());
        nodep->user1p(varp);
        varp->user1p(nodep);
        ಯದಿ (nodep->direction() == VDirection::OUTPUT) {
            exprp->foreach([](ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* varrefp) {
                // Prevent confusing BLKANDNBLK warnings on clockvars due to generated assignments
                varrefp->fileline()->warnOff(V3ErrorCode::BLKANDNBLK, ಸತ್ಯ);
                // Mark the driven signal so later passes (e.g. V3Inst port
                // connection) know the drive flows out from inside the cell.
                // Item expressions are plain signal references, so only the
                // driven signal is marked, never index selects.
                varrefp->varp()->isClockingDriven(ಸತ್ಯ);
            });
            AstVarRef* ಧ್ರುವ skewedReadRefp = ನೂತನ AstVarRef{flp, varp, VAccess::READ};
            skewedReadRefp->user1(ಸತ್ಯ);
            // Initialize the clockvar
            AstVarRef* ಧ್ರುವ skewedWriteRefp = ನೂತನ AstVarRef{flp, varp, VAccess::WRITE};
            skewedWriteRefp->user1(ಸತ್ಯ);
            AstInitialStatic* ಧ್ರುವ initClockvarp = ನೂತನ AstInitialStatic{
                flp, ನೂತನ AstAssign{flp, skewedWriteRefp, exprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)}};
            m_modp->addStmtsp(initClockvarp);
            // Each drive sets a flag, so the signal is also assigned when driven with the same
            // value again, e.g. after another assignment to the signal (IEEE 1800-2023 14.16.2)
            AstVar* ಧ್ರುವ drivenVarp = getCreateDrivenVarp(nodep);
            m_modp->addStmtsp(ನೂತನ AstInitialStatic{
                flp, ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                   ನೂತನ AstConst{flp, AstConst::BitFalse{}}}});
            // Assign the clockvar to the actual var if it was driven
            AstAssign* ಧ್ರುವ assignp
                = ನೂತನ AstAssign{flp, exprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), skewedReadRefp};
            AstIf* ಧ್ರುವ ifp
                = ನೂತನ AstIf{flp, ನೂತನ AstVarRef{flp, drivenVarp, VAccess::READ},
                            ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                          ನೂತನ AstConst{flp, AstConst::BitFalse{}}}};
            ifp->addThensp(assignp);
            // Only in the time step of the clocking event
            AstCMethodHard* ಧ್ರುವ trigp = ನೂತನ AstCMethodHard{
                nodep->fileline(), ನೂತನ AstVarRef{flp, m_clockingp->ensureEventp(), VAccess::READ},
                VCMethod::EVENT_IS_TRIGGERED};
            trigp->dtypeSetBit();
            ifp->condp(ನೂತನ AstLogAnd{flp, ifp->condp()->unlinkFrBack(), trigp});
            // A skew rounding to no time precision ticks is a zero skew
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ ticksp
                = V3Timing::delayToTicks(skewp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), m_modp->timeunit());
            ಧ್ರುವ ತಾರ್ಕಿಕ zeroSkew = ticksp->isZero();
            VL_DO_DANGLING(ticksp->delete_ವೃಕ್ಷ(), ticksp);
            ಯದಿ (zeroSkew || !skewp->fileline()->timingOn()) {
                // Drive the var in Re-NBA (IEEE 1800-2023 14.16), ignoring skews where timing is
                // turned off
                Ast_Sen_ವೃಕ್ಷ* senTreep = m_clockingp->sentreep()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                senTreep->addSensesp(ನೂತನ AstSenItem{
                    flp, VEdgeType::ET_CHANGED, ನೂತನ AstVarRef{flp, drivenVarp, VAccess::READ}});
                m_clockingp->addNextHere(ನೂತನ AstAlwaysReactive{flp, senTreep, ifp});
            } ಅನ್ಯಥಾ {
                // Fork only the assignment, so this AlwaysPostponed can be retriggered
                // before the assignment happens
                AstFork* ಧ್ರುವ forkp = ನೂತನ AstFork{flp, VJoinType::JOIN_NONE};
                forkp->immediateStart(ಸತ್ಯ);
                forkp->addForksp(ನೂತನ AstBegin{flp, "", assignp->unlinkFrBack(), ಸತ್ಯ});
                ifp->addThensp(forkp);
                // Read the clockvar in Postponed, so a drive late in the time step is applied
                // at this event plus the skew (IEEE 1800-2023 14.16)
                m_clockingp->addNextHere(ನೂತನ AstAlwaysPostponed{flp, ifp});
                ಯದಿ (v3Global.opt.timing().isSetTrue()) {
                    AstDelay* ಧ್ರುವ delayp = ನೂತನ AstDelay{flp, skewp->unlinkFrBack(), ಅಸತ್ಯ};
                    delayp->timeunit(m_modp->timeunit());
                    assignp->timingControlp(delayp);
                } ಅನ್ಯಥಾ ಯದಿ (v3Global.opt.timing().isSetFalse()) {
                    nodep->v3warn(E_NOTIMING,
                                  "Clocking output skew greater than #0 requires --timing");
                } ಅನ್ಯಥಾ {
                    nodep->v3warn(E_NEEDTIMINGOPT,
                                  "Use --timing or --no-timing to specify how "
                                  "clocking output skew greater than #0 should be handled");
                }
            }
        } ಅನ್ಯಥಾ ಯದಿ (nodep->direction() == VDirection::ಒಳಹರಿವು) {
            // Ref to the clockvar
            AstVarRef* ಧ್ರುವ refp = ನೂತನ AstVarRef{flp, varp, VAccess::WRITE};
            refp->user1(ಸತ್ಯ);
            ಯದಿ (skewp->ಅಂಕೆ().is1Step()) {
                // #1step means the value that is sampled is always the signal's last value
                // before the clock edge (IEEE 1800-2023 14.4)
                AstSampled* ಧ್ರುವ sampledp
                    = ನೂತನ AstSampled{flp, exprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), exprp->dtypep(), ಸತ್ಯ};
                AstAssign* ಧ್ರುವ assignp = ನೂತನ AstAssign{flp, refp, sampledp};
                m_clockingp->addNextHere(ನೂತನ AstAlways{
                    flp, VAlwaysKwd::ALWAYS, m_clockingp->sentreep()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), assignp});
            } ಅನ್ಯಥಾ ಯದಿ (skewp->isZero()) {
                // #0 means the var has to be sampled in Observed (IEEE 1800-2023 14.13)
                AstAssign* ಧ್ರುವ assignp = ನೂತನ AstAssign{flp, refp, exprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
                m_clockingp->addNextHere(ನೂತನ AstAlwaysObserved{
                    flp, m_clockingp->sentreep()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), assignp});
            } ಅನ್ಯಥಾ {
                // Create a queue where we'll store sampled values with timestamps
                AstSampleQueueDType* ಧ್ರುವ queueDtp
                    = ನೂತನ AstSampleQueueDType{flp, exprp->dtypep()};
                m_netlistp->typeTablep()->addTypesp(queueDtp);
                AstVar* ಧ್ರುವ queueVarp
                    = ನೂತನ AstVar{flp, VVarType::MODULETEMP, "__Vqueue__" + varp->name(), queueDtp};
                queueVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_clockingp->addNextHere(queueVarp);
                // Create a process like this:
                //     always queue.push(<sampled var>);
                AstCMethodHard* ಧ್ರುವ pushp = ನೂತನ AstCMethodHard{
                    flp, ನೂತನ AstVarRef{flp, queueVarp, VAccess::WRITE}, VCMethod::DYN_ತಳ್ಳು,
                    ನೂತನ AstTime{nodep->fileline(), m_modp->timeunit()}};
                pushp->addPinsp(exprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ));
                pushp->dtypeSetVoid();
                m_clockingp->addNextHere(
                    ನೂತನ AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, pushp->makeStmt()});
                // Create a process like this:
                //     always @<clocking event> queue.pop(<skew>, /*out*/<skewed var>);
                AstCMethodHard* ಧ್ರುವ popp = ನೂತನ AstCMethodHard{
                    flp, ನೂತನ AstVarRef{flp, queueVarp, VAccess::READWRITE}, VCMethod::DYN_ತೆಗೆ,
                    ನೂತನ AstTime{nodep->fileline(), m_modp->timeunit()}};
                popp->addPinsp(skewp->unlinkFrBack());
                refp->access(VAccess::READWRITE);  // Only conditionally assigned
                popp->addPinsp(refp);
                popp->dtypeSetVoid();
                m_clockingp->addNextHere(ನೂತನ AstAlways{flp, VAlwaysKwd::ALWAYS,
                                                       m_clockingp->sentreep()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ),
                                                       popp->makeStmt()});
            }
        } ಅನ್ಯಥಾ {
            nodep->v3fatalSrc("Invalid direction");
        }
    }
    ಶೂನ್ಯ visit(AstDelay* nodep) override {
        m_hasCycleDelay |= nodep->isCycleDelay();
        // Only cycle delays are relevant in this stage; also only process once
        ಯದಿ (!nodep->isCycleDelay()) {
            ಯದಿ (m_inSynchDrive) {
                nodep->v3error("Only cycle delays can be used in synchronous drives"
                               " (IEEE 1800-2023 14.16)");
            }
            iterateChildren(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (m_inAssign && !m_inSynchDrive) {
            nodep->v3error("Cycle delays not allowed as intra-assignment delays"
                           " (IEEE 1800-2023 14.11)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBackWithNext());
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = V3Const::constifyEdit(nodep->lhsp()->unlinkFrBack());
        ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(valuep, Const);
        ಯದಿ (!constp && m_pexprp) {
            // V3AssertNfa handles non-const delays in sequences before this pass and
            // replaces the property; this branch should never be reached.
            nodep->v3fatalSrc("Non-constant cycle delay in assertion: "
                              "should have been caught by V3AssertNfa");
        } ಅನ್ಯಥಾ ಯದಿ (constp && constp->isZero()) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(valuep), valuep);
            ಯದಿ (m_inSynchDrive) {
                // ##0 has no effect in synchronous drives (IEEE 1800-2023 14.11)
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }
            ಯದಿ (m_pexprp) {
                // ##0 in sequence context = zero delay = same clock tick
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }
            // Procedural ##0: synchronize with default clocking event (IEEE 1800-2023 14.11)
            ಯದಿ (!m_defaultClockingp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }
            nodep->replaceWith(newCycleDelayZerop(flp));
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        Ast_Sen_ವೃಕ್ಷ* sentreep = nullptr;
        ಯದಿ (!m_drives.empty()) {
            ಯದಿ (m_inConcatAssign) {
                // Also the clockvars may be of different clocking blocks
                nodep->v3error("Cycle delays cannot be used in synchronous drives to a"
                               " concatenation (IEEE 1800-2023 14.16)");
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(valuep), valuep);
                ಪ್ರತ್ಯಾಗಮ;
            }
            // Count the cycles of the driven clockvar's clocking block (IEEE 1800-2023 14.16)
            sentreep = newDriveSentreep(nodep, m_drives.front());
            ಯದಿ (!sentreep) {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(valuep), valuep);
                ಪ್ರತ್ಯಾಗಮ;
            }
        } ಅನ್ಯಥಾ ಯದಿ (!m_defaultClockingp) {
            ಯದಿ (!m_pexprp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(valuep), valuep);
                ಪ್ರತ್ಯಾಗಮ;
            }
            sentreep = ನೂತನ Ast_Sen_ವೃಕ್ಷ{flp, m_senip->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)};
        } ಅನ್ಯಥಾ {
            sentreep = m_defaultClockingp->sentreep()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
        }
        AstEventControl* ಧ್ರುವ controlp
            = ನೂತನ AstEventControl{nodep->fileline(), sentreep, nullptr};
        ಧ್ರುವ std::string delayName = m_cycleDlyNames.get(nodep);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* throughoutp
            = nodep->throughoutp() ? nodep->throughoutp()->unlinkFrBack() : nullptr;

        AstVar* ಧ್ರುವ cntVarp = ನೂತನ AstVar{flp, VVarType::BLOCKTEMP, delayName + "__counter",
                                           nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstBegin* ಧ್ರುವ beginp = ನೂತನ AstBegin{flp, delayName + "__block", cntVarp, ಸತ್ಯ};
        // A non-constant delay is counted at runtime. The count is integral of 64 bits, as
        // V3Width makes delays, or real, which is rounded (IEEE 1800-2023 6.12.1)
        ಯದಿ (valuep->isDouble()) {
            valuep = ನೂತನ AstRToIRoundS{flp, valuep};
            valuep->dtypeFrom(cntVarp);
        } ಅನ್ಯಥಾ {
            valuep = ನೂತನ AstSel{flp, valuep, 0, cntVarp->width()};
        }
        beginp->addStmtsp(ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, cntVarp, VAccess::WRITE}, valuep});
        // If the count is zero, a synchronous drive waits for none, like with a constant '##0',
        // and a procedural delay waits like a procedural '##0' (IEEE 1800-2023 14.11)
        ಯದಿ (!constp && !m_inSynchDrive) {
            beginp->addStmtsp(ನೂತನ AstIf{
                flp,
                ನೂತನ AstEq{flp, ನೂತನ AstVarRef{flp, cntVarp, VAccess::READ}, ನೂತನ AstConst{flp, 0}},
                newCycleDelayZerop(flp)});
        }

        // Throughout: create flag tracking whether condition held every tick
        AstVar* throughoutOkp = nullptr;
        ಯದಿ (throughoutp) {
            throughoutOkp = ನೂತನ AstVar{flp, VVarType::BLOCKTEMP, delayName + "__throughoutOk",
                                       nodep->findBasicDType(VBasicDTypeKwd::LOGIC_IMPLICIT)};
            throughoutOkp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->addStmtsp(throughoutOkp);
            beginp->addStmtsp(ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            ನೂತನ AstConst{flp, AstConst::BitTrue{}}});
            // Check condition at tick 0 (sequence start, before entering loop)
            beginp->addStmtsp(
                ನೂತನ AstIf{flp, ನೂತನ AstLogNot{flp, throughoutp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)},
                          ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                        ನೂತನ AstConst{flp, AstConst::BitFalse{}}}});
        }

        {
            AstLoop* ಧ್ರುವ loopp = ನೂತನ AstLoop{flp};
            // When throughout is present, exit loop early if condition fails
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* loopCondp
                = ನೂತನ AstGt{flp, ನೂತನ AstVarRef{flp, cntVarp, VAccess::READ}, ನೂತನ AstConst{flp, 0}};
            ಯದಿ (throughoutOkp) {
                loopCondp = ನೂತನ AstLogAnd{flp, loopCondp,
                                          ನೂತನ AstVarRef{flp, throughoutOkp, VAccess::READ}};
            }
            loopp->addStmtsp(ನೂತನ AstLoopTest{flp, loopp, loopCondp});
            loopp->addStmtsp(controlp);
            loopp->addStmtsp(
                ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, cntVarp, VAccess::WRITE},
                              ನೂತನ AstSub{flp, ನೂತನ AstVarRef{flp, cntVarp, VAccess::READ},
                                         ನೂತನ AstConst{flp, 1}}});
            // Check throughout condition at each tick during delay (IEEE 1800-2023 16.9.9)
            ಯದಿ (throughoutp) {
                loopp->addStmtsp(
                    ನೂತನ AstIf{flp, ನೂತನ AstLogNot{flp, throughoutp},
                              ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            ನೂತನ AstConst{flp, AstConst::BitFalse{}}}});
            }
            beginp->addStmtsp(loopp);
        }
        // Compose wrappers on remaining sequence: throughout gate (inner), disable iff (outer)
        Ast_ಗಂಟು* remainp = nodep->nextp() ? nodep->nextp()->unlinkFrBackWithNext() : nullptr;
        ಯದಿ (throughoutOkp) {
            // If condition failed during delay, fail assertion
            remainp = ನೂತನ AstIf{flp, ನೂತನ AstVarRef{flp, throughoutOkp, VAccess::READ}, remainp,
                                ನೂತನ AstPExprClause{flp, /*pass=*/ಅಸತ್ಯ}};
        }
        ಯದಿ (m_disableSeqIfp && remainp) {
            AstIf* ಧ್ರುವ disableSeqIfp = m_disableSeqIfp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
            disableSeqIfp->addThensp(remainp);
            remainp = disableSeqIfp;
        }
        ಯದಿ (remainp) {
            ಯದಿ (throughoutOkp) {
                // throughoutOkp is declared in beginp scope -- check must be inside it
                beginp->addStmtsp(remainp);
            } ಅನ್ಯಥಾ {
                nodep->addNextHere(remainp);
            }
        }
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ visit(Ast_Sen_ವೃಕ್ಷ* nodep) override {
        ಯದಿ (m_inSynchDrive) {
            nodep->v3error("Event controls cannot be used in "
                           "synchronous drives (IEEE 1800-2023 14.16)");
        }

        ಧ್ರುವ AstSampled* sampledp;
        ಯದಿ (nodep->exists([&sampledp](ಧ್ರುವ AstSampled* ಧ್ರುವ sp) {
                sampledp = sp;
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            })) {
            sampledp->v3warn(E_UNSUPPORTED, "Unsupported: $sampled inside sensitivity list");
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Var_Ref* nodep) override {
        UINFO(8, " -varref:  " << nodep);
        UINFO(8, " -varref-var-back:  " << nodep->varp()->backp());
        UINFO(8, " -varref-var-user1:  " << nodep->varp()->user1p());
        ಯದಿ (AstClockingItem* ಧ್ರುವ itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->firstAbovep(),
                ClockingItem)) {
            ಯದಿ (nodep->user1()) ಪ್ರತ್ಯಾಗಮ;

            // ensure linking still works, this has to be done only once
            ಯದಿ (AstVarXRef* xrefp = VN_CAST(nodep, VarXRef)) {
                UINFO(8, " -clockvarxref-in:  " << xrefp);
                string dotted = xrefp->dotted();
                ಧ್ರುವ size_t dotPos = dotted.rfind('.');
                dotted.erase(dotPos, string::npos);
                xrefp->dotted(dotted);
                UINFO(8, " -clockvarxref-out: " << xrefp);
                m_xrefsp.emplace_back(xrefp);
            }

            ಯದಿ (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            ಯದಿ (nodep->access().isWriteOrRW()) {
                ಯದಿ (itemp->direction() == VDirection::OUTPUT) {
                    ಯದಿ (!m_inAssignDlyLhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    ಯದಿ (m_inAssign) {
                        m_inSynchDrive = ಸತ್ಯ;
                        m_drives.ತಳ್ಳು_back({itemp, nodep});
                    }
                } ಅನ್ಯಥಾ ಯದಿ (itemp->direction() == VDirection::ಒಳಹರಿವು) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
            nodep->user1(ಸತ್ಯ);
        }
    }
    ಶೂನ್ಯ visit(AstMemberSel* nodep) override {
        ಯದಿ (AstClockingItem* ಧ್ರುವ itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->firstAbovep(),
                ClockingItem)) {
            ಯದಿ (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            ಯದಿ (nodep->access().isWriteOrRW()) {
                ಯದಿ (itemp->direction() == VDirection::OUTPUT) {
                    ಯದಿ (!m_inAssignDlyLhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    ಯದಿ (m_inAssign) {
                        m_inSynchDrive = ಸತ್ಯ;
                        m_drives.ತಳ್ಳು_back({itemp, nodep});
                    }
                } ಅನ್ಯಥಾ ಯದಿ (itemp->direction() == VDirection::ಒಳಹರಿವು) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Assign* nodep) override {
        ಯದಿ (nodep->user1()) ಪ್ರತ್ಯಾಗಮ;
        VL_RESTORER(m_inAssign);
        VL_RESTORER(m_inSynchDrive);
        VL_RESTORER(m_inConcatAssign);
        VL_RESTORER_CLEAR(m_drives);
        m_inAssign = ಸತ್ಯ;
        m_inSynchDrive = ಅಸತ್ಯ;
        {
            // The braces of a concatenation are a replication, by one
            ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp = nodep->lhsp();
            ಯದಿ (ಧ್ರುವ AstReplicate* ಧ್ರುವ repp = VN_CAST(lhsp, Replicate)) lhsp = repp->srcp();
            m_inConcatAssign = VN_IS(lhsp, Concat);
        }
        {
            VL_RESTORER(m_inAssignDlyLhs);
            m_inAssignDlyLhs = VN_IS(nodep, AssignDly);
            iterate(nodep->lhsp());
        }
        iterate(nodep->rhsp());
        ಯದಿ (nodep->timingControlp()) iterate(nodep->timingControlp());
        // A drive without a cycle delay, also after removing '##0', updates the clockvar now
        ಯದಿ (m_inSynchDrive && !nodep->timingControlp()) {
            AstAssign* ಧ್ರುವ assignp = ನೂತನ AstAssign{
                nodep->fileline(), nodep->lhsp()->unlinkFrBack(), nodep->rhsp()->unlinkFrBack()};
            assignp->user1(ಸತ್ಯ);
            nodep->replaceWith(assignp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            nodep = assignp;
        }
        // Note the drives when they take effect, after the cycle delay if any. Each clockvar
        // of the LHS is driven, e.g. in a concatenation which other simulators accept, although
        // IEEE 1800-2023 14.16 does not allow it
        Ast_ಗಂಟು* setsp = nullptr;
        ಪ್ರತಿ (ಧ್ರುವ SynchDrive& drive : m_drives) {
            setsp = Ast_ಗಂಟು::addNext(setsp, newDrivenSetp(nodep->fileline(), drive));
        }
        ಯದಿ (!setsp) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (AstBegin* ಧ್ರುವ beginp = VN_CAST(nodep->timingControlp(), Begin)) {
            beginp->addStmtsp(setsp);
        } ಅನ್ಯಥಾ {
            nodep->addNextHere(setsp);
        }
    }
    ಶೂನ್ಯ visit(AstAlways* nodep) override {
        iterateAndNextNull(nodep->sentreep());
        ಯದಿ (nodep->sentreep()) m_seniAlwaysp = nodep->sentreep()->sensesp();
        iterateAndNextNull(nodep->stmtsp());
        m_seniAlwaysp = nullptr;
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Cover_Or_Assert* nodep) override {
        ಯದಿ (nodep->sentreep()) ಪ್ರತ್ಯಾಗಮ;  // Already processed

        VL_RESTORER(m_senip);
        VL_RESTORER(m_disablep);
        m_senip = nullptr;
        m_disablep = nullptr;

        // Find Clocking's buried under nodep->exprsp
        iterateChildren(nodep);
        ಯದಿ (!nodep->immediate()) nodep->sentreep(new_Sen_ವೃಕ್ಷ(nodep, nullptr, nodep));
    }
    ಶೂನ್ಯ visit(AstFalling* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = nodep->exprp()->unlinkFrBack();
        ಯದಿ (exprp->width() > 1) exprp = ನೂತನ AstSel{fl, exprp, 0, 1};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ futurep = ನೂತನ AstFuture{fl, exprp, new_Sen_ವೃಕ್ಷ(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = ನೂತನ AstAnd{fl, exprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), ನೂತನ AstNot{fl, futurep}};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(AstFell* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = nodep->exprp()->unlinkFrBack();
        ಯದಿ (exprp->width() > 1) exprp = ನೂತನ AstSel{fl, exprp, 0, 1};
        Ast_Sen_ವೃಕ್ಷ* sentreep = nodep->sentreep();
        ಯದಿ (sentreep) sentreep->unlinkFrBack();
        AstPast* ಧ್ರುವ pastp = ನೂತನ AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_ವೃಕ್ಷ(nodep, sentreep));
        exprp = ನೂತನ AstAnd{fl, pastp, ನೂತನ AstNot{fl, exprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)}};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(AstFuture* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ sentreep = nodep->sentreep();
        ಯದಿ (sentreep) VL_DO_DANGLING(ತಳ್ಳು_Deletep(sentreep->unlinkFrBack()), sentreep);
        nodep->sentreep(new_Sen_ವೃಕ್ಷ(nodep));
    }
    ಶೂನ್ಯ visit(AstPast* nodep) override {
        checkSamplingFuncDType(nodep, nodep->exprp());
        ಯದಿ (nodep->sentreep()) ಪ್ರತ್ಯಾಗಮ;  // Already processed
        iterateChildren(nodep);
        nodep->sentreep(new_Sen_ವೃಕ್ಷ(nodep));
    }
    ಶೂನ್ಯ visit(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- Lower standalone exact [*N] (N >= 2) via saturating counter.
        // Range/unbounded forms and SExpr-contained forms are lowered by V3AssertNfa.
        iterateChildren(nodep);
        // V3AssertNfa handles unbounded/ranged forms upstream, so this fast-path
        // is effectively unreachable when NFA is enabled.
        ಯದಿ (nodep->unbounded() || nodep->maxCountp()) ಪ್ರತ್ಯಾಗಮ;  // LCOV_EXCL_LINE
        ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(nodep->countp(), Const);
        ಯದಿ (VL_UNLIKELY(!constp || constp->toSInt() < 1)) {
            nodep->v3fatalSrc("Consecutive repetition count must be a positive constant"
                              " (should have been caught by V3Width)");
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ n = constp->toSInt();
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = nodep->exprp()->unlinkFrBack();
        ಯದಿ (n == 1) {
            nodep->replaceWith(exprp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Saturating counter: if (expr) cnt <= min(cnt+1, N); else cnt <= 0;
        AstVar* ಧ್ರುವ cntVarp = ನೂತನ AstVar{flp, VVarType::MODULETEMP, m_consRepNames.get(""),
                                           nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(cntVarp);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ ಅಭಿವ್ಯಕ್ತಿ_Clonep = exprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ saturatingIncrp = ನೂತನ AstCond{
            flp,
            ನೂತನ AstLt{flp, ನೂತನ AstVarRef{flp, cntVarp, VAccess::READ},
                      ನೂತನ AstConst{flp, static_cast<uint32_t>(n)}},
            ನೂತನ AstAdd{flp, ನೂತನ AstVarRef{flp, cntVarp, VAccess::READ}, ನೂತನ AstConst{flp, 1u}},
            ನೂತನ AstConst{flp, static_cast<uint32_t>(n)}};
        AstAssignDly* ಧ್ರುವ incrAssignp
            = ನೂತನ AstAssignDly{flp, ನೂತನ AstVarRef{flp, cntVarp, VAccess::WRITE}, saturatingIncrp};
        AstAssignDly* ಧ್ರುವ resetAssignp = ನೂತನ AstAssignDly{
            flp, ನೂತನ AstVarRef{flp, cntVarp, VAccess::WRITE}, ನೂತನ AstConst{flp, 0u}};
        AstIf* ಧ್ರುವ ifp = ನೂತನ AstIf{flp, ಅಭಿವ್ಯಕ್ತಿ_Clonep, incrAssignp, resetAssignp};
        Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ senTreep = new_Sen_ವೃಕ್ಷ(nodep);
        AstAlways* ಧ್ರುವ alwaysp = ನೂತನ AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, ifp};
        cntVarp->addNextHere(alwaysp);
        // Match: cnt >= N-1 (previous cycles via NBA) && expr (current cycle)
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ cntCheckp = ನೂತನ AstGte{flp, ನೂತನ AstVarRef{flp, cntVarp, VAccess::READ},
                                                  ನೂತನ AstConst{flp, static_cast<uint32_t>(n - 1)}};
        cntCheckp->dtypeSetBit();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ matchp = ನೂತನ AstAnd{flp, cntCheckp, exprp};
        matchp->dtypeSetBit();
        nodep->replaceWith(matchp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(AstRising* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = nodep->exprp()->unlinkFrBack();
        ಯದಿ (exprp->width() > 1) exprp = ನೂತನ AstSel{fl, exprp, 0, 1};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ futurep = ನೂತನ AstFuture{fl, exprp, new_Sen_ವೃಕ್ಷ(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = ನೂತನ AstAnd{fl, ನೂತನ AstNot{fl, exprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)}, futurep};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(AstRose* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = nodep->exprp()->unlinkFrBack();
        ಯದಿ (exprp->width() > 1) exprp = ನೂತನ AstSel{fl, exprp, 0, 1};
        Ast_Sen_ವೃಕ್ಷ* sentreep = nodep->sentreep();
        ಯದಿ (sentreep) sentreep->unlinkFrBack();
        AstPast* ಧ್ರುವ pastp = ನೂತನ AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_ವೃಕ್ಷ(nodep, sentreep));
        exprp = ನೂತನ AstAnd{fl, ನೂತನ AstNot{fl, pastp}, exprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಸ್ಥಿರ AstAssocArrayDType* getProcessAssocArrayType(File_ಸಾಲು* ಧ್ರುವ flp) {
        // Type of __VassertsActive___x[std::process]
        Ast_ಗಂಟು_DType* valp
            = v3Global.rootp()->typeTablep()->findBasicDType(flp, VBasicDTypeKwd::BIT);
        AstClassRefDType* keyp
            = ನೂತನ AstClassRefDType{flp, v3Global.rootp()->stdPackageProcessp(), nullptr};
        keyp->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        v3Global.rootp()->typeTablep()->addTypesp(keyp);
        AstAssocArrayDType* ಧ್ರುವ typep = ನೂತನ AstAssocArrayDType{flp, valp, keyp};
        typep->dtypep(typep);
        v3Global.rootp()->typeTablep()->addTypesp(typep);
        ಪ್ರತ್ಯಾಗಮ typep;
    }
    ಸ್ಥಿರ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* getProcessAssocArrayDelete(AstVarRef* ಧ್ರುವ refp) {
        // Constructs refp.delete(std::process::self()) statement
        File_ಸಾಲು* ಧ್ರುವ flp = refp->fileline();
        AstCMethodHard* ಧ್ರುವ deletep = ನೂತನ AstCMethodHard{
            flp, refp, VCMethod::ASSOC_ERASE, v3Global.rootp()->stdPackageProcessSelfp(flp)};
        deletep->dtypep(refp->findVoidDType());
        ಪ್ರತ್ಯಾಗಮ ನೂತನ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ{flp, deletep};
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* getProcessAssocArraySize(AstVarRef* ಧ್ರುವ refp) {
        // Constructs refp.size() statement
        AstCMethodHard* ಧ್ರುವ sizep
            = ನೂತನ AstCMethodHard{refp->fileline(), refp, VCMethod::ASSOC_SIZE};
        sizep->dtypep(refp->findBasicDType(VBasicDTypeKwd::UINT32));
        ಪ್ರತ್ಯಾಗಮ sizep;
    }
    ಶೂನ್ಯ visit(AstSEventually* nodep) override {
        UASSERT(v3Global.rootp()->stdPackagep(), "Should be imported");

        Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ sentreep = new_Sen_ವೃಕ್ಷ(nodep);
        ಯದಿ (!sentreep->sensesp()) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(sentreep), sentreep);
            nodep->replaceWith(ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }

        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();

        // Track active assertions
        AstVar* ಧ್ರುವ activep = ನೂತನ AstVar{flp, VVarType::MODULETEMP, m_activeNames.get(""),
                                           getProcessAssocArrayType(flp)};
        activep->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(activep);

        // Assertion condition check
        AstLoop* ಧ್ರುವ loopp = ನೂತನ AstLoop{flp};
        AstSampled* ಧ್ರುವ condp
            = ನೂತನ AstSampled{flp, nodep->exprp()->unlinkFrBack(), nullptr, ಸತ್ಯ};
        loopp->addStmtsp(ನೂತನ AstLoopTest{flp, loopp, ನೂತನ AstLogNot{flp, condp}});
        loopp->addStmtsp(ನೂತನ AstEventControl{flp, sentreep, nullptr});

        // Add assertion to the active set
        AstAssocSel* ಧ್ರುವ selp = ನೂತನ AstAssocSel{flp, ನೂತನ AstVarRef{flp, activep, VAccess::WRITE},
                                                  v3Global.rootp()->stdPackageProcessSelfp(flp)};
        AstAssign* ಧ್ರುವ incrementp = ನೂತನ AstAssign{flp, selp, ನೂತನ AstConst{flp, 1}};
        AstPExprClause* ಧ್ರುವ clausep = ನೂತನ AstPExprClause{flp};
        Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ deletep
            = getProcessAssocArrayDelete(ನೂತನ AstVarRef{flp, activep, VAccess::WRITE});

        // Main assertion block
        AstBegin* ಧ್ರುವ bodyp = ನೂತನ AstBegin{flp, m_blockNames.get(""), nullptr, ಸತ್ಯ};
        bodyp->addStmtsp(incrementp);
        bodyp->addStmtsp(loopp);
        bodyp->addStmtsp(clausep);
        bodyp->addStmtsp(deletep);

        // Validate assertion condition for each active assert
        AstVar* ಧ್ರುವ activeCountp = ನೂತನ AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        activeCountp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstAssign* ಧ್ರುವ initActiveCountp
            = ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, activeCountp, VAccess::WRITE},
                            getProcessAssocArraySize(ನೂತನ AstVarRef{flp, activep, VAccess::READ})};
        AstLoop* ಧ್ರುವ finalLoopp = ನೂತನ AstLoop{flp};
        AstIf* ಧ್ರುವ finalBodypCondp
            = ನೂತನ AstIf{flp, condp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), ನೂತನ AstPExprClause{flp},
                        ನೂತನ AstPExprClause{flp, ಅಸತ್ಯ}};
        finalLoopp->addStmtsp(
            ನೂತನ AstLoopTest{flp, finalLoopp,
                            ನೂತನ AstNeq{flp, ನೂತನ AstVarRef{flp, activeCountp, VAccess::READ},
                                       ನೂತನ AstConst{flp, 0}}});
        finalLoopp->addStmtsp(finalBodypCondp);
        finalLoopp->addStmtsp(
            ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, activeCountp, VAccess::WRITE},
                          ನೂತನ AstSub{flp, ನೂತನ AstVarRef{flp, activeCountp, VAccess::READ},
                                     ನೂತನ AstConst{flp, 1}}});

        // Final assertion block
        AstBegin* ಧ್ರುವ finalp = ನೂತನ AstBegin{flp, m_blockNames.get(""), nullptr, ಸತ್ಯ};
        finalp->addStmtsp(activeCountp);
        finalp->addStmtsp(initActiveCountp);
        finalp->addStmtsp(finalLoopp);

        VL_RESTORER(m_pexprp);
        m_pexprp = ನೂತನ AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
        VL_RESTORER(m_hasCycleDelay);
        m_hasCycleDelay = ಅಸತ್ಯ;
        iterate(bodyp);
        iterate(finalp);

        ಯದಿ (m_hasCycleDelay) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in s_eventually");
            nodep->replaceWith(ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            VL_DO_DANGLING(m_pexprp->delete_ವೃಕ್ಷ(), m_pexprp);
            ಪ್ರತ್ಯಾಗಮ;
        }

        nodep->replaceWith(m_pexprp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(AstStable* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = nodep->exprp()->unlinkFrBack();
        Ast_Sen_ವೃಕ್ಷ* sentreep = nodep->sentreep();
        ಯದಿ (sentreep) sentreep->unlinkFrBack();
        AstPast* ಧ್ರುವ pastp = ನೂತನ AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_ವೃಕ್ಷ(nodep, sentreep));
        exprp = ನೂತನ AstEq{fl, pastp, exprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(AstSteady* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = nodep->exprp()->unlinkFrBack();
        ಯದಿ (exprp->width() > 1) exprp = ನೂತನ AstSel{fl, exprp, 0, 1};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ futurep = ನೂತನ AstFuture{fl, exprp, new_Sen_ವೃಕ್ಷ(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = ನೂತನ AstEq{fl, exprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), futurep};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(AstSampled* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (!nodep->internal()) checkSamplingFuncDType(nodep, nodep->exprp());
    }

    // Validate repetition count: must be a non-negative elaboration-time constant.
    // Shared by goto [->N] and nonconsecutive [=N] repetition.
    // On error, replaces nodep with BitFalse placeholder and returns nullptr.
    ಧ್ರುವ AstConst* validate_Rep_ಎಣಿಕೆ(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*& countp) {
        countp = V3Const::constifyEdit(countp);
        ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(countp, Const);
        ಯದಿ (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(countp), countp);
            nodep->replaceWith(ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }
        ಯದಿ (constp->toSInt() < 0) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(countp), countp);
            nodep->replaceWith(ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }
        ಯದಿ (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(countp), countp);
            nodep->replaceWith(ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }
        ಪ್ರತ್ಯಾಗಮ constp;
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
    AstPExpr* createRepPExpr(File_ಸಾಲು* flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* countp,
                             Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, ತಾರ್ಕಿಕ isOverlapped, ತಾರ್ಕಿಕ isNonConsec) {
        AstSenItem* ಧ್ರುವ sensesp = m_senip;
        UASSERT_OBJ(sensesp, exprp, "Repetition requires a clock");

        ಧ್ರುವ std::string name
            = isNonConsec ? m_nonConsRepNames.get(exprp) : m_gotoRepNames.get(exprp);
        AstVar* ಧ್ರುವ cntVarp = ನೂತನ AstVar{flp, VVarType::BLOCKTEMP, name + "__counter",
                                           exprp->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstBegin* ಧ್ರುವ beginp = ನೂತನ AstBegin{flp, name + "__block", cntVarp, ಸತ್ಯ};
        beginp->addStmtsp(
            ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, cntVarp, VAccess::WRITE}, ನೂತನ AstConst{flp, 0}});

        // Counting loop: find N occurrences of expr (shared by goto and nonconsec)
        AstLoop* ಧ್ರುವ loopp = ನೂತನ AstLoop{flp};
        loopp->addStmtsp(ನೂತನ AstLoopTest{flp, loopp,
                                         ನೂತನ AstLt{flp, ನೂತನ AstVarRef{flp, cntVarp, VAccess::READ},
                                                   countp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)}});
        // if (expr) cnt++ -- sampled is applied to whole property expr by V3Assert
        loopp->addStmtsp(
            ನೂತನ AstIf{flp, exprp,
                      ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, cntVarp, VAccess::WRITE},
                                    ನೂತನ AstAdd{flp, ನೂತನ AstVarRef{flp, cntVarp, VAccess::READ},
                                               ನೂತನ AstConst{flp, 1}}}});
        loopp->addStmtsp(ನೂತನ AstIf{
            flp, ನೂತನ AstLt{flp, ನೂತನ AstVarRef{flp, cntVarp, VAccess::READ}, countp},
            ನೂತನ AstEventControl{flp, ನೂತನ Ast_Sen_ವೃಕ್ಷ{flp, sensesp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)}, nullptr}});

        beginp->addStmtsp(loopp);

        ಯದಿ (isNonConsec && rhsp) {
            // IEEE 16.9.2: b[=N] = b[->N] ##1 !b[*0:$]
            // Trailing window: ##1 advance, then scan !expr cycles checking rhs.
            // Window closes when expr becomes true again (fail if rhs was never true).
            beginp->addStmtsp(ನೂತನ AstEventControl{
                flp, ನೂತನ Ast_Sen_ವೃಕ್ಷ{flp, sensesp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)}, nullptr});  // ##1
            ಯದಿ (!isOverlapped) {
                beginp->addStmtsp(ನೂತನ AstEventControl{
                    flp, ನೂತನ Ast_Sen_ವೃಕ್ಷ{flp, sensesp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)}, nullptr});  // |=>
            }
            // Window loop: check rhs at each !expr cycle (done variable for termination)
            AstVar* ಧ್ರುವ doneVarp
                = ನೂತನ AstVar{flp, VVarType::BLOCKTEMP, name + "__done", exprp->findBitDType()};
            doneVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->addStmtsp(doneVarp);
            beginp->addStmtsp(ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, doneVarp, VAccess::WRITE},
                                            ನೂತನ AstConst{flp, AstConst::BitFalse{}}});
            auto setDone = [&]() {
                ಪ್ರತ್ಯಾಗಮ ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, doneVarp, VAccess::WRITE},
                                     ನೂತನ AstConst{flp, AstConst::BitTrue{}}};
            };
            AstLoop* ಧ್ರುವ windowp = ನೂತನ AstLoop{flp};
            // LoopTest: continue while !done
            windowp->addStmtsp(ನೂತನ AstLoopTest{
                flp, windowp, ನೂತನ AstNot{flp, ನೂತನ AstVarRef{flp, doneVarp, VAccess::READ}}});
            // if (expr) { fail; done = 1; } -- window closed, expr true again
            Ast_ಗಂಟು* ಧ್ರುವ failsp = ನೂತನ AstPExprClause{flp, ಅಸತ್ಯ};
            failsp->addNext(setDone());
            windowp->addStmtsp(ನೂತನ AstIf{flp, exprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), failsp});
            // if (rhs) { pass; done = 1; } -- consequent true at this !expr endpoint
            Ast_ಗಂಟು* ಧ್ರುವ passsp = ನೂತನ AstPExprClause{flp, ಸತ್ಯ};
            passsp->addNext(setDone());
            windowp->addStmtsp(ನೂತನ AstIf{flp, rhsp, passsp});
            // @(clk) -- advance to next cycle in window
            windowp->addStmtsp(
                ನೂತನ AstEventControl{flp, ನೂತನ Ast_Sen_ವೃಕ್ಷ{flp, sensesp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)}, nullptr});
            beginp->addStmtsp(windowp);
        } ಅನ್ಯಥಾ ಯದಿ (isNonConsec) {
            // Standalone nonconsec: ##1 into window, then pass
            beginp->addStmtsp(
                ನೂತನ AstEventControl{flp, ನೂತನ Ast_Sen_ವೃಕ್ಷ{flp, sensesp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)}, nullptr});
            beginp->addStmtsp(ನೂತನ AstPExprClause{flp, ಸತ್ಯ});
        } ಅನ್ಯಥಾ ಯದಿ (rhsp) {
            // Goto rep: check consequent once at match endpoint
            ಯದಿ (!isOverlapped) {
                beginp->addStmtsp(ನೂತನ AstEventControl{
                    flp, ನೂತನ Ast_Sen_ವೃಕ್ಷ{flp, sensesp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)}, nullptr});
            }
            beginp->addStmtsp(ನೂತನ AstIf{flp, rhsp, ನೂತನ AstPExprClause{flp, ಸತ್ಯ},
                                        ನೂತನ AstPExprClause{flp, ಅಸತ್ಯ}});
        } ಅನ್ಯಥಾ {
            // Standalone goto: pass after counting
            beginp->addStmtsp(ನೂತನ AstPExprClause{flp, ಸತ್ಯ});
        }

        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstPExpr{flp, beginp, exprp->findBitDType()};
    }

    // Reject [=M:N] nonconsecutive range that reached V3AssertPre.
    // [->M:N] is fully handled upstream (NFA path); only [=M:N] still lands
    // here because AstSNonConsRep lowering is not yet implemented there.
    // Replaces the node with BitFalse so lowering continues after the warning.
    ತಾರ್ಕಿಕ rejectNonconsecRange(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* maxCountp) {
        ಯದಿ (!maxCountp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: [=M:N] nonconsecutive range repetition"
                                     " (IEEE 1800-2023 16.9.2)");
        nodep->replaceWith(ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalse{}});
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    ಶೂನ್ಯ visit(AstSGotoRep* nodep) override {
        // Standalone goto rep (not inside implication antecedent)
        iterateChildren(nodep);
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* countp = nodep->countp()->unlinkFrBack();
        ಯದಿ (!validate_Rep_ಎಣಿಕೆ(nodep, countp)) ಪ್ರತ್ಯಾಗಮ;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = nodep->exprp()->unlinkFrBack();
        AstPExpr* ಧ್ರುವ pexprp = createRepPExpr(flp, exprp, countp, nullptr, ಸತ್ಯ, ಅಸತ್ಯ);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ visit(AstSNonConsRep* nodep) override {
        // Standalone nonconsecutive rep (not inside implication antecedent)
        iterateChildren(nodep);
        ಯದಿ (rejectNonconsecRange(nodep, nodep->maxCountp())) ಪ್ರತ್ಯಾಗಮ;
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* countp = nodep->countp()->unlinkFrBack();
        ಯದಿ (!validate_Rep_ಎಣಿಕೆ(nodep, countp)) ಪ್ರತ್ಯಾಗಮ;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = nodep->exprp()->unlinkFrBack();
        AstPExpr* ಧ್ರುವ pexprp = createRepPExpr(flp, exprp, countp, nullptr, ಸತ್ಯ, ಸತ್ಯ);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ visit(AstFuncRef* nodep) override {
        // IEEE 1800-2023 16.8: Inline sequence instances wherever they appear
        // in the expression tree (inside implications, boolean ops, nested refs, etc.)
        ಯದಿ (AstSequence* ಧ್ರುವ seqp = VN_CAST(nodep->taskp(), Sequence)) {
            substituteSequenceCall(nodep, seqp);
            // The FuncRef has been replaced; do not access nodep after this point.
            // The replacement node will be visited by the parent's iterateChildren.
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (ಧ್ರುವ AstProperty* ಧ್ರುವ propp = VN_CAST(nodep->taskp(), Property)) {
            substituteNestedPropertyCall(nodep, propp);
            ಪ್ರತ್ಯಾಗಮ;
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstImplication* nodep) override {
        ಯದಿ (nodep->sentreep()) ಪ್ರತ್ಯಾಗಮ;  // Already processed

        // Top-level followed-by is claimed by V3AssertNfa; anything reaching here
        // is nested inside iff/implies/or. IEEE 1800-2023 16.12.9 permits the
        // composition, but silent lowering would drop non-vacuous-fail semantics.
        ಯದಿ (nodep->isFollowedBy()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: followed-by (#-# / #=#) nested inside property operator"
                          " (iff/implies/or) (IEEE 1800-2023 16.12.9)");
            nodep->replaceWith(ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Handle goto repetition as antecedent before iterateChildren,
        // so the standalone AstSGotoRep visitor doesn't process it
        ಯದಿ (AstSGotoRep* ಧ್ರುವ gotop = VN_CAST(nodep->lhsp(), SGotoRep)) {
            iterateChildren(gotop);
            iterateAndNextNull(nodep->rhsp());
            File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* countp = gotop->countp()->unlinkFrBack();
            ಯದಿ (!validate_Rep_ಎಣಿಕೆ(nodep, countp)) ಪ್ರತ್ಯಾಗಮ;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = gotop->exprp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = nodep->rhsp()->unlinkFrBack();
            AstPExpr* ಧ್ರುವ pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), ಅಸತ್ಯ);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Handle nonconsecutive repetition as antecedent before iterateChildren,
        // so the standalone AstSNonConsRep visitor doesn't process it
        ಯದಿ (AstSNonConsRep* ಧ್ರುವ ncrp = VN_CAST(nodep->lhsp(), SNonConsRep)) {
            iterateChildren(ncrp);
            iterateAndNextNull(nodep->rhsp());
            ಯದಿ (rejectNonconsecRange(nodep, ncrp->maxCountp())) ಪ್ರತ್ಯಾಗಮ;
            File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* countp = ncrp->countp()->unlinkFrBack();
            ಯದಿ (!validate_Rep_ಎಣಿಕೆ(nodep, countp)) ಪ್ರತ್ಯಾಗಮ;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = ncrp->exprp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = nodep->rhsp()->unlinkFrBack();
            AstPExpr* ಧ್ರುವ pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), ಸತ್ಯ);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }

        iterateChildren(nodep);

        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp = nodep->lhsp()->unlinkFrBack();

        // Lower sequence match items (IEEE 16.11): (expr, var = val, ...) |-> / |=>
        ಯದಿ (Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* ಧ್ರುವ ಅಭಿವ್ಯಕ್ತಿ_Stmtp = VN_CAST(lhsp, ಅಭಿವ್ಯಕ್ತಿ_Stmt)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ antExprp = ಅಭಿವ್ಯಕ್ತಿ_Stmtp->resultp()->unlinkFrBack();

            ಯದಿ (nodep->isOverlapped()) {
                // |-> : assign to __Vpropvar via always_comb (continuous).
                // The assign evaluates RHS once; V3Sampled snapshots the
                // result so all consequent refs read the same value.
                Ast_ಗಂಟು* matchAssignsp = nullptr;
                ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = ಅಭಿವ್ಯಕ್ತಿ_Stmtp->stmtsp(); stmtp;) {
                    Ast_ಗಂಟು* ಧ್ರುವ nextp = stmtp->nextp();
                    ಯದಿ (AstAssign* ಧ್ರುವ assignp = VN_CAST(stmtp, Assign)) {
                        assignp->unlinkFrBack();
                        matchAssignsp = Ast_ಗಂಟು::addNextNull(matchAssignsp, assignp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                ಯದಿ (matchAssignsp) {
                    AstAlways* ಧ್ರುವ alwaysp
                        = ನೂತನ AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, matchAssignsp};
                    m_modp->addStmtsp(alwaysp);
                }
            } ಅನ್ಯಥಾ {
                // |=> : assign to __Vpropvar via NBA in a clocked always block.
                // The NBA commits before the next cycle's sampled snapshot,
                // so the consequent (which already references __Vpropvar)
                // sees the captured value.
                Ast_ಗಂಟು* matchAssignsp = nullptr;
                ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = ಅಭಿವ್ಯಕ್ತಿ_Stmtp->stmtsp(); stmtp;) {
                    Ast_ಗಂಟು* ಧ್ರುವ nextp = stmtp->nextp();
                    ಯದಿ (AstAssign* ಧ್ರುವ assignp = VN_CAST(stmtp, Assign)) {
                        assignp->unlinkFrBack();
                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ assignLhsp = assignp->lhsp()->unlinkFrBack();
                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ assignRhsp = assignp->rhsp()->unlinkFrBack();
                        AstAssignDly* ಧ್ರುವ dlyp = ನೂತನ AstAssignDly{flp, assignLhsp, assignRhsp};
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(assignp), assignp);
                        matchAssignsp = Ast_ಗಂಟು::addNextNull(matchAssignsp, dlyp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                ಯದಿ (matchAssignsp) {
                    AstIf* ಧ್ರುವ condp
                        = ನೂತನ AstIf{flp, antExprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), matchAssignsp};
                    AstAlways* ಧ್ರುವ alwaysp
                        = ನೂತನ AstAlways{flp, VAlwaysKwd::ALWAYS, new_Sen_ವೃಕ್ಷ(nodep), condp};
                    m_modp->addStmtsp(alwaysp);
                }
            }
        }

        ಯದಿ (AstPExpr* ಧ್ರುವ pexprp = VN_CAST(rhsp, PExpr)) {
            // Implication with sequence expression on RHS (IEEE 1800-2023 16.11, 16.12.7).
            // The PExpr was lowered from the property expression earlier in this pass.
            // Wrap the PExpr body with the antecedent check so the sequence only
            // starts when the antecedent holds.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp;
            ಯದಿ (nodep->isOverlapped()) {
                // Overlapped implication (|->): check antecedent on same cycle.
                // disable iff is applied at the assertion level, not at the
                // antecedent gate, matching the existing non-PExpr overlapped path.
                condp = lhsp;
            } ಅನ್ಯಥಾ {
                // Non-overlapped implication (|=>): check antecedent from previous cycle
                ಯದಿ (m_disablep) {
                    lhsp
                        = ನೂತನ AstAnd{flp, ನೂತನ AstNot{flp, m_disablep->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)}, lhsp};
                }
                AstPast* ಧ್ರುವ pastp
                    = ನೂತನ AstPast{flp, lhsp, nullptr, nullptr, /* propertyTiming */ ಸತ್ಯ};
                pastp->dtypeFrom(lhsp);
                pastp->sentreep(new_Sen_ವೃಕ್ಷ(nodep));
                condp = pastp;
            }
            // Wrap existing PExpr body: if (antecedent) { <original body> } else { /* vacuous pass
            // */ }
            // Only the statements are guarded; declarations stay directly in the block. After
            // V3Fork moves the block's statements into a task, V3Task only handles variables
            // that are direct statements of the task.
            AstBegin* ಧ್ರುವ bodyp = pexprp->bodyp();
            AstIf* ಧ್ರುವ guardp = ನೂತನ AstIf{flp, condp};
            ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = bodyp->stmtsp(); stmtp;) {
                Ast_ಗಂಟು* ಧ್ರುವ nextp = stmtp->nextp();
                ಯದಿ (!VN_IS(stmtp, Var)) guardp->addThensp(stmtp->unlinkFrBack());
                stmtp = nextp;
            }
            bodyp->addStmtsp(guardp);
            nodep->replaceWith(pexprp);
            // Don't iterate pexprp here -- it was already iterated when created
            // (in visit(AstSExpr*)), so delays and disable iff are already processed.
        } ಅನ್ಯಥಾ ಯದಿ (nodep->isOverlapped()) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp
                = m_underCover ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ನೂತನ AstLogAnd{flp, lhsp, rhsp})
                               : ನೂತನ AstLogOr{flp, ನೂತನ AstLogNot{flp, lhsp}, rhsp};
            nodep->replaceWith(exprp);
        } ಅನ್ಯಥಾ {
            ಯದಿ (m_disablep) {
                lhsp = ನೂತನ AstAnd{flp, ನೂತನ AstNot{flp, m_disablep->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)}, lhsp};
            }

            AstPast* ಧ್ರುವ pastp
                = ನೂತನ AstPast{flp, lhsp, nullptr, nullptr, /* propertyTiming */ ಸತ್ಯ};
            pastp->dtypeFrom(lhsp);
            pastp->sentreep(new_Sen_ವೃಕ್ಷ(nodep));
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp
                = m_underCover ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ನೂತನ AstAnd{flp, pastp, rhsp})
                               : ನೂತನ AstOr{flp, ನೂತನ AstNot{flp, pastp}, rhsp};
            exprp->dtypeSetBit();
            nodep->replaceWith(exprp);
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(AstUntil* nodep) override {
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        UASSERT_OBJ(
            !m_pexprp, nodep,
            "'" << nodep->verilogKwd()
                << "' in complex property expression should have been rejected by V3AssertNfa");
        ಯದಿ (nodep->isStrong()
            && (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue())) {
            nodep->v3warn(E_NOTIMING, nodep->verilogKwd() << " requires --timing");
            nodep->replaceWith(ನೂತನ AstConst{flp, AstConst::BitFalse{}});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (nodep->isStrong()) {
            // p s_until q / p s_until_with q: q must eventually be true. Until then, p must
            // be true on every sampled tick. For s_until, check q first: when q is true on
            // this tick, p is not required. For s_until_with, p must be true on the q tick too.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rawLhsp = nodep->lhsp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rawRhsp = nodep->rhsp()->unlinkFrBack();
            AstSampled* ಧ್ರುವ lhsp = ನೂತನ AstSampled{flp, rawLhsp, rawLhsp->dtypep(), ಸತ್ಯ};
            AstSampled* ಧ್ರುವ rhsp = ನೂತನ AstSampled{flp, rawRhsp, rawRhsp->dtypep(), ಸತ್ಯ};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* finalCondp = rhsp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
            ಯದಿ (nodep->isOverlapping()) {
                finalCondp = ನೂತನ AstLogAnd{flp, lhsp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), finalCondp};
            }

            // Track active assertion attempts. Only the count is needed to emit final failures.
            AstVar* ಧ್ರುವ activep = ನೂತನ AstVar{flp, VVarType::MODULETEMP, m_activeNames.get(""),
                                               nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            activep->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(activep);

            AstVar* ಧ್ರುವ donep
                = ನೂತನ AstVar{flp, VVarType::BLOCKTEMP, "__VassertDone", nodep->findBitDType()};
            donep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            auto setDone = [&]() {
                ಪ್ರತ್ಯಾಗಮ ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, donep, VAccess::WRITE},
                                     ನೂತನ AstConst{flp, AstConst::BitTrue{}}};
            };

            AstLoop* ಧ್ರುವ loopp = ನೂತನ AstLoop{flp};
            AstAssign* ಧ್ರುವ decrementVar = ನೂತನ AstAssign{
                flp, ನೂತನ AstVarRef{flp, activep, VAccess::WRITE},
                ನೂತನ AstSub{flp, ನೂತನ AstVarRef{flp, activep, VAccess::READ}, ನೂತನ AstConst{flp, 1}}};
            loopp->addStmtsp(ನೂತನ AstLoopTest{
                flp, loopp, ನೂತನ AstLogNot{flp, ನೂತನ AstVarRef{flp, donep, VAccess::READ}}});
            {
                Ast_ಗಂಟು* ಧ್ರುವ passp = ನೂತನ AstPExprClause{flp};
                passp->addNext(decrementVar);
                passp->addNext(setDone());
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* passCondp = rhsp;
                ಯದಿ (nodep->isOverlapping()) {
                    passCondp = ನೂತನ AstLogAnd{flp, lhsp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), passCondp};
                }
                loopp->addStmtsp(ನೂತನ AstIf{flp, passCondp, passp});
            }
            {
                Ast_ಗಂಟು* ಧ್ರುವ failp = ನೂತನ AstPExprClause{flp, ಅಸತ್ಯ};
                failp->addNext(decrementVar->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
                failp->addNext(setDone());
                loopp->addStmtsp(ನೂತನ AstIf{
                    flp,
                    ನೂತನ AstLogAnd{flp,
                                  ನೂತನ AstLogNot{flp, ನೂತನ AstVarRef{flp, donep, VAccess::READ}},
                                  ನೂತನ AstLogNot{flp, lhsp}},
                    failp});
            }
            AstDelay* ಧ್ರುವ delayp = ನೂತನ AstDelay{flp, ನೂತನ AstConst{flp, 1}, ಅಸತ್ಯ};
            delayp->timeunit(m_modp->timeunit());
            loopp->addStmtsp(ನೂತನ AstIf{
                flp, ನೂತನ AstLogNot{flp, ನೂತನ AstVarRef{flp, donep, VAccess::READ}}, delayp});

            // Main assertion block
            AstBegin* ಧ್ರುವ bodyp = ನೂತನ AstBegin{flp, m_blockNames.get(""), nullptr, ಸತ್ಯ};
            bodyp->addStmtsp(donep);
            bodyp->addStmtsp(ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, donep, VAccess::WRITE},
                                           ನೂತನ AstConst{flp, AstConst::BitFalse{}}});
            File_ಸಾಲು* ಧ್ರುವ flp = activep->fileline();
            bodyp->addStmtsp(
                ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, activep, VAccess::WRITE},
                              ನೂತನ AstAdd{flp, ನೂತನ AstVarRef{flp, activep, VAccess::READ},
                                         ನೂತನ AstConst{flp, 1}}});
            bodyp->addStmtsp(loopp);

            // Validate assertion condition for each active assert
            AstVar* ಧ್ರುವ activeCountp = ನೂತನ AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                    nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            activeCountp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

            AstAssign* ಧ್ರುವ initActiveCountp
                = ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, activeCountp, VAccess::WRITE},
                                ನೂತನ AstVarRef{flp, activep, VAccess::READ}};
            AstLoop* ಧ್ರುವ finalLoopp = ನೂತನ AstLoop{flp};
            finalLoopp->addStmtsp(
                ನೂತನ AstLoopTest{flp, finalLoopp,
                                ನೂತನ AstNeq{flp, ನೂತನ AstVarRef{flp, activeCountp, VAccess::READ},
                                           ನೂತನ AstConst{flp, 0}}});
            finalLoopp->addStmtsp(ನೂತನ AstIf{flp, finalCondp, ನೂತನ AstPExprClause{flp},
                                            ನೂತನ AstPExprClause{flp, ಅಸತ್ಯ}});
            finalLoopp->addStmtsp(
                ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, activeCountp, VAccess::WRITE},
                              ನೂತನ AstSub{flp, ನೂತನ AstVarRef{flp, activeCountp, VAccess::READ},
                                         ನೂತನ AstConst{flp, 1}}});

            // Final assertion block
            AstBegin* ಧ್ರುವ finalp = ನೂತನ AstBegin{flp, m_blockNames.get(""), nullptr, ಸತ್ಯ};
            finalp->addStmtsp(activeCountp);
            finalp->addStmtsp(initActiveCountp);
            finalp->addStmtsp(finalLoopp);

            AstPExpr* ಧ್ರುವ pexprp = ನೂತನ AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
            VL_RESTORER(m_pexprp);
            VL_RESTORER(m_hasCycleDelay);
            m_pexprp = pexprp;
            m_hasCycleDelay = ಅಸತ್ಯ;
            iterate(bodyp);
            iterate(finalp);

            UASSERT_OBJ(!m_hasCycleDelay, nodep,
                        "s_until cycle delay should have been handled by V3AssertNfa");

            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        AstLoop* ಧ್ರುವ loopp = ನೂತನ AstLoop{flp};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = nodep->lhsp()->unlinkFrBack();
        AstLogAnd* ಧ್ರುವ loopCondp = ನೂತನ AstLogAnd{flp, lhsp, ನೂತನ AstLogNot{flp, rhsp}};
        loopp->addStmtsp(ನೂತನ AstLoopTest{flp, loopp, loopCondp});
        loopp->addStmtsp(ನೂತನ AstEventControl{flp, new_Sen_ವೃಕ್ಷ(nodep), nullptr});

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsCopyp = rhsp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ passCondp
            = nodep->isOverlapping() ? ನೂತನ AstLogAnd{flp, lhsp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), rhsCopyp}
                                     : rhsCopyp;
        AstBegin* ಧ್ರುವ beginp = ನೂತನ AstBegin{flp, m_blockNames.get(""), loopp, ಸತ್ಯ};
        beginp->addStmtsp(
            ನೂತನ AstIf{flp, passCondp, ನೂತನ AstPExprClause{flp}, ನೂತನ AstPExprClause{flp, ಅಸತ್ಯ}});

        AstPExpr* ಧ್ರುವ pexprp = ನೂತನ AstPExpr{flp, beginp, nodep->dtypep()};
        pexprp->user1(1);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ visit(AstDefaultDisable* nodep) override {
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ಶೂನ್ಯ visit(AstInferredDisable* nodep) override {
        Ast_ಗಂಟು* newp;
        ಯದಿ (m_defaultDisablep) {
            newp = m_defaultDisablep->condp()->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ);
        } ಅನ್ಯಥಾ {
            newp = ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalse{}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ visit(AstPropSpec* nodep) override {
        nodep = substitutePropertyCall(nodep);
        iterateAndNextNull(nodep->sensesp());
        ಯದಿ (m_senip && m_senip != nodep->sensesp())
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Only one PSL clock allowed per assertion");
        ಧ್ರುವ AstCover* ಧ್ರುವ coverp = VN_CAST(nodep->backp(), Cover);
        ಧ್ರುವ ತಾರ್ಕಿಕ seqEvent = coverp && coverp->isSeqEvent();
        ಯದಿ (!nodep->disablep() && m_defaultDisablep && !seqEvent) {
            nodep->disablep(m_defaultDisablep->condp()->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ));
        }
        m_disablep = nodep->disablep();
        // Unlink and just keep a pointer to it, convert to sentree as needed
        m_senip = nodep->sensesp();
        iterateNull(nodep->disablep());
        ಯದಿ (!VN_AS(nodep->backp(), ಗಂಟು_Cover_Or_Assert)->immediate()) {
            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ propDtp = nodep->propp()->dtypep();
            nodep->propp(ನೂತನ AstSampled{nodep->fileline(), nodep->propp()->unlinkFrBack(),
                                        propDtp->dtypep(), ಸತ್ಯ});
        }
        // cover counts non-vacuous matches only (IEEE 1800-2023 16.15.2), so an
        // implication antecedent must hold; assert passes vacuously instead.
        VL_RESTORER(m_underCover);
        m_underCover = VN_IS(nodep->backp(), Cover);
        iterate(nodep->propp());
        iterateNull(nodep->matchCountp());
    }
    ಶೂನ್ಯ visit(AstPExpr* nodep) override {
        // V3AssertNfa handles multi-cycle property expressions before this pass,
        // so the following unsupported paths are defensive and typically unreached.
        ಯದಿ (m_pexprp && m_pexprp->user1()) {  // LCOV_EXCL_START
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Complex property expression inside 'until''");
            nodep->replaceWith(ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }  // LCOV_EXCL_STOP
        ಯದಿ (AstLogNot* ಧ್ರುವ notp = VN_CAST(nodep->backp(), LogNot)) {
            notp->replaceWith(nodep->unlinkFrBack());
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(notp), notp);
            iterate(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Sequence expression as antecedent of implication is not yet supported
        ಯದಿ (AstImplication* ಧ್ರುವ implp = VN_CAST(nodep->backp(), Implication)) {
            ಯದಿ (implp->lhsp() == nodep) {  // LCOV_EXCL_START
                implp->v3warn(E_UNSUPPORTED,
                              "Unsupported: Implication with sequence expression as antecedent");
                nodep->replaceWith(ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }  // LCOV_EXCL_STOP
        }
        VL_RESTORER(m_pexprp);
        VL_RESTORER(m_disableSeqIfp);
        m_pexprp = nodep;

        ಯದಿ (m_disablep) {
            ಧ್ರುವ AstSampled* sampledp = nullptr;
            ಯದಿ (m_disablep->exists([&sampledp](ಧ್ರುವ AstSampled* ಧ್ರುವ sp) {
                    sampledp = sp;
                    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                })) {
                sampledp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: $sampled inside disabled condition of a sequence");
                m_disablep = ನೂತನ AstConst{m_disablep->fileline(), AstConst::BitFalse{}};
                // always a copy is used, so remove it now
                ತಳ್ಳು_Deletep(m_disablep);  // m_disablep used later
            }
            File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
            // Add counter which counts times the condition turned true
            AstVar* ಧ್ರುವ disableCntp
                = ನೂತನ AstVar{flp, VVarType::MODULETEMP, m_disableCntNames.get(""),
                             nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            disableCntp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(disableCntp);
            AstVarRef* ಧ್ರುವ readCntRefp = ನೂತನ AstVarRef{flp, disableCntp, VAccess::READ};
            AstVarRef* ಧ್ರುವ writeCntRefp = ನೂತನ AstVarRef{flp, disableCntp, VAccess::WRITE};
            AstAssign* ಧ್ರುವ incrStmtp = ನೂತನ AstAssign{
                flp, writeCntRefp, ನೂತನ AstAdd{flp, readCntRefp, ನೂತನ AstConst{flp, 1}}};
            AstAlways* ಧ್ರುವ alwaysp
                = ನೂತನ AstAlways{flp, VAlwaysKwd::ALWAYS,
                                ನೂತನ Ast_Sen_ವೃಕ್ಷ{flp, ನೂತನ AstSenItem{flp, VEdgeType::ET_POSEDGE,
                                                                   m_disablep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)}},
                                incrStmtp};
            disableCntp->addNextHere(alwaysp);

            // Store value of that counter at the beginning of sequence evaluation
            AstBegin* ಧ್ರುವ bodyp = nodep->bodyp();
            AstVar* ಧ್ರುವ initialCntp = ನೂತನ AstVar{flp, VVarType::BLOCKTEMP, "__VinitialCnt",
                                                   nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            initialCntp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            AstAssign* ಧ್ರುವ assignp
                = ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, initialCntp, VAccess::WRITE},
                                readCntRefp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)};
            // Prepend to the sequence body to keep statement list structure valid.
            UASSERT_OBJ(!bodyp->name().empty(), bodyp, "Sequence body should be named");
            Ast_ಗಂಟು* ಧ್ರುವ newStmtsp = Ast_ಗಂಟು::addNext<Ast_ಗಂಟು, Ast_ಗಂಟು>(initialCntp, assignp);
            bodyp->prependStmtsp(newStmtsp);

            m_disableSeqIfp
                = ನೂತನ AstIf{flp, ನೂತನ AstEq{flp, ನೂತನ AstVarRef{flp, initialCntp, VAccess::READ},
                                           readCntRefp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)}};
            // Delete it, because it is always copied before insetion to the AST
            ತಳ್ಳು_Deletep(m_disableSeqIfp);  // m_disableSeqIfp used later
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultClkEvtVarp);
        VL_RESTORER(m_defaultDisablep);
        VL_RESTORER(m_modp);
        ಯದಿ (!VN_IS(nodep, Class)) {
            // A class nested in a module is in the scope of the module's default clocking
            m_defaultClockingp = nullptr;
            m_defaultClkEvtVarp = nullptr;
        }
        m_defaultDisablep = nodep->defaultDisablep();
        m_modp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstProperty* nodep) override {
        // The body will be visited when will be substituted in place of property reference
        // (AstFuncRef)
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ಶೂನ್ಯ visit(AstSequence* nodep) override {
        // Sequence declarations are not visited directly; their bodies are inlined
        // at call sites by visit(AstFuncRef*). Collect for post-traversal cleanup.
        m_seqsToCleanup.ತಳ್ಳು_back(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit AssertPreVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        { AssertPreClockingVisitor{nodep}; }
        // Process
        iterate(nodep);
        // Fix up varref names
        ಪ್ರತಿ (AstVarXRef* xrefp : m_xrefsp) xrefp->name(xrefp->varp()->name());
        // Clean up sequence declarations after inlining.
        // Referenced sequences that were inlined have isReferenced cleared.
        // Remaining referenced sequences are in unsupported contexts (e.g. @seq event).
        ಪ್ರತಿ (AstSequence* seqp : m_seqsToCleanup) {
            ಯದಿ (seqp->isReferenced()) {
                seqp->v3warn(E_UNSUPPORTED,
                             "Unsupported: sequence referenced outside assertion property");
            } ಅನ್ಯಥಾ {
                VL_DO_DANGLING(seqp->unlinkFrBack()->delete_ವೃಕ್ಷ(), seqp);
            }
        }
    }
    ~AssertPreVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// Top Assert class

ಶೂನ್ಯ V3AssertPre::assertPreAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { AssertPreVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("assertpre", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
