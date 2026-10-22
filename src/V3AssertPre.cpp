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

শ্রেণী AssertPreClockingVisitor final : সর্বজনীন VNVisitorConst {
    // Captures the clocking block of each clocking item before AssertPreVisitor, which needs it
    // at synchronous drives, often visited before the clocking block, e.g. in a parent module
    // NODE STATE
    // AstClockingItem::user3p()  // AstClocking*. Clocking block of the item, user3 claimed by
    //                            //               AssertPreVisitor

    // VISITORS
    শূন্য visit(AstNetlist* nodep) override { iterate_And_পরবর্তী_Const_Null(nodep->modulesp()); }
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override { iterate_And_পরবর্তী_Const_Null(nodep->stmtsp()); }
    শূন্য visit(AstGenBlock* nodep) override { iterate_And_পরবর্তী_Const_Null(nodep->itemsp()); }
    শূন্য visit(AstClocking* nodep) override {
        জন্য (Ast_গ্রন্থি* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            যদি (AstClockingItem* ধ্রুবক citemp = VN_CAST(itemp, ClockingItem)) {
                citemp->user3p(nodep);
            }
        }
    }
    // Clocking blocks are only in modules and their generate blocks
    শূন্য visit(Ast_গ্রন্থি*) override {}

সর্বজনীন:
    // CONSTRUCTORS
    explicit AssertPreClockingVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~AssertPreClockingVisitor() override = পূর্বনির্ধারিত;
};

//######################################################################
// Assert class functions

শ্রেণী AssertPreVisitor final : সর্বজনীন VNVisitor {
    // Removes clocks and other pre-optimizations
    // Eventually inlines calls to sequences, properties, etc.
    // We're not parsing the tree, or anything more complicated.
ব্যক্তিগত:
    // TYPES
    কাঠামো SynchDrive final {
        AstClockingItem* itemp;  // Clocking item of the driven clockvar
        Ast_গ্রন্থি_রাশিমালা* refp;  // Reference to the driven clockvar
    };

    // NODE STATE
    // AstClockingItem::user1p()         // AstVar*.      varp() of ClockingItem after unlink
    // AstClockingItem::user2p()         // AstVar*.      Flag set by drives of output clockvar
    // AstClockingItem::user3p()         // AstClocking*. Clocking block of the item, captured
    //                                   //               by AssertPreClockingVisitor
    // AstPExpr::user1()                 // bool.         Created from AstUntil
    ধ্রুবক VNUser1InUse m_inuser1;
    ধ্রুবক VNUser2InUse m_inuser2;
    ধ্রুবক VNUser3InUse m_inuser3;
    // STATE
    // Current context:
    AstNetlist* ধ্রুবক m_netlistp = nullptr;  // Current netlist
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
    যৌক্তিক m_underCover = মিথ্যা;  // True if the enclosing assertion is a cover
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
    যৌক্তিক m_inAssign = মিথ্যা;  // True if in an AssignNode
    যৌক্তিক m_inAssignDlyLhs = মিথ্যা;  // True if in AssignDly's LHS
    যৌক্তিক m_inSynchDrive = মিথ্যা;  // True if in synchronous drive
    যৌক্তিক m_inConcatAssign = মিথ্যা;  // True if in assignment to a concatenation
    std::vector<SynchDrive> m_drives;  // Clockvars written by the synchronous drive
    যৌক্তিক m_hasCycleDelay = মিথ্যা;  // True if node has cycle delay beneath
    std::vector<AstVarXRef*> m_xrefsp;  // list of xrefs that need name fixup
    std::vector<AstSequence*> m_seqsToCleanup;  // Sequences to clean up after traversal

    // METHODS

    // Flag set by each drive of an output clockvar, added to the module with its clocking item
    AstVar* getCreateDrivenVarp(AstClockingItem* itemp) {
        যদি (!itemp->user2p()) {
            AstVar* ধ্রুবক varp = নতুন AstVar{itemp->fileline(), VVarType::MODULETEMP,
                                            m_drivenNames.get(itemp), itemp->findBitDType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            itemp->user2p(varp);
        }
        ফেরত VN_AS(itemp->user2p(), Var);
    }
    // Reference to a variable in the scope of a driven clockvar, made like the clockvar's
    স্থির Ast_গ্রন্থি_রাশিমালা* newDriveRefp(File_সারি* flp, ধ্রুবক SynchDrive& drive, AstVar* varp,
                                     ধ্রুবক VAccess& access) {
        যদি (ধ্রুবক AstVarXRef* ধ্রুবক xrefp = VN_CAST(drive.refp, VarXRef)) {
            ফেরত নতুন AstVarXRef{flp, varp, xrefp->dotted(), access};
        } অন্যথা যদি (ধ্রুবক AstMemberSel* ধ্রুবক selp = VN_CAST(drive.refp, MemberSel)) {
            // The interface expression is evaluated again, so it must not have side effects;
            // cloneTreePure warns if it has any
            AstMemberSel* ধ্রুবক newSelp
                = নতুন AstMemberSel{flp, selp->fromp()->clone_বৃক্ষ_Pure(মিথ্যা), varp};
            newSelp->access(access);
            ফেরত newSelp;
        }
        ফেরত নতুন AstVarRef{flp, varp, access};
    }
    // Assignment setting the drive flag of a driven clockvar
    AstAssign* newDrivenSetp(File_সারি* flp, ধ্রুবক SynchDrive& drive) {
        AstVar* ধ্রুবক varp = getCreateDrivenVarp(drive.itemp);
        AstAssign* ধ্রুবক setp = নতুন AstAssign{flp, newDriveRefp(flp, drive, varp, VAccess::WRITE),
                                              নতুন AstConst{flp, AstConst::BitTrue{}}};
        setp->user1(সত্য);
        ফেরত setp;
    }
    // Clocking block of a clocking item
    স্থির AstClocking* clockingOf(ধ্রুবক AstClockingItem* itemp) {
        UASSERT_OBJ(itemp->user3p(), itemp, "Clocking item not captured in a clocking block");
        ফেরত VN_AS(itemp->user3p(), Clocking);
    }
    // Clocking event of a driven clockvar, referenced from the drive like the clockvar, or
    // nullptr if unsupported, which is reported on the drive's cycle delay
    Ast_Sen_বৃক্ষ* newDriveSentreep(AstDelay* delayp, ধ্রুবক SynchDrive& drive) {
        Ast_Sen_বৃক্ষ* ধ্রুবক origp = clockingOf(drive.itemp)->sentreep();
        যদি (VN_IS(drive.refp, VarRef)) ফেরত origp->clone_বৃক্ষ(মিথ্যা);
        যদি (origp->exists([](ধ্রুবক AstVarXRef*) { ফেরত সত্য; })) {
            // The reference is relative to the clocking block
            delayp->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in synchronous drive to"
                                          " clockvar in another scope, whose clocking event"
                                          " has a hierarchical reference");
            ফেরত nullptr;
        }
        ধ্রুবক AstMemberSel* ধ্রুবক selp = VN_CAST(drive.refp, MemberSel);
        // Processes outside classes evaluate their events also when not waiting on them,
        // possibly before the interface reference is set, so the event is guarded against a
        // null reference, which needs an integral value; scheduling does not guard it
        ধ্রুবক যৌক্তিক guardNull = selp && !VN_IS(m_modp, Class);
        যদি (guardNull) {
            জন্য (AstSenItem* itemp = origp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                যদি (!itemp->sensp()->dtypep()->skipRefp()->isIntegralOrPacked()) {
                    delayp->v3warn(E_UNSUPPORTED,
                                   "Unsupported: cycle delay in synchronous drive through"
                                   " an interface reference outside a class, to clocking"
                                   " block with non-integral clocking event");
                    ফেরত nullptr;
                }
            }
        }
        // Interface declaring the clocking block, if driven through an interface reference
        AstIface* ধ্রুবক ifacep
            = selp ? VN_AS(selp->fromp()->dtypep()->skipRefp(), IfaceRefDType)->ifaceViaCellp()
                   : nullptr;
        Ast_Sen_বৃক্ষ* ধ্রুবক sentreep = origp->clone_বৃক্ষ(মিথ্যা);
        std::vector<AstVarRef*> refps;
        sentreep->foreach([&](AstVarRef* refp) {
            যদি (!refp->classOrPackagep()) refps.ঠেলো_back(refp);
        });
        জন্য (AstVarRef* ধ্রুবক refp : refps) {
            AstVar* ধ্রুবক varp = refp->varp();
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
            জন্য (AstSenItem* itemp = sentreep->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                guardNullIface(itemp, selp->fromp());
            }
        }
        ফেরত sentreep;
    }
    // Make an integral clocking event through an interface reference false while the reference
    // is null
    স্থির শূন্য guardNullIface(AstSenItem* sensesp, Ast_গ্রন্থি_রাশিমালা* fromp) {
        File_সারি* ধ্রুবক flp = sensesp->fileline();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক sensp = sensesp->sensp()->unlinkFrBack();
        sensesp->sensp(নতুন AstCond{flp, newNotNullp(flp, fromp), sensp,
                                   নতুন AstConst{flp, AstConst::DTyped{}, sensp->dtypep()}});
        যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুবক condp = sensesp->condp()) {
            condp->unlinkFrBack();
            sensesp->condp(নতুন AstLogAnd{flp, newNotNullp(flp, fromp), condp});
        }
    }
    স্থির Ast_গ্রন্থি_রাশিমালা* newNotNullp(File_সারি* flp, Ast_গ্রন্থি_রাশিমালা* fromp) {
        ফেরত নতুন AstNeq{flp, fromp->clone_বৃক্ষ_Pure(মিথ্যা), নতুন AstConst{flp, AstConst::Null{}}};
    }
    // Procedural '##0': if the default clocking event has not yet occurred in this time step,
    // wait for it; otherwise continue without suspension (IEEE 1800-2023 14.11)
    AstIf* newCycleDelayZerop(File_সারি* flp) ধ্রুবক {
        AstVar* ধ্রুবক evtVarp = m_defaultClkEvtVarp;
        UASSERT_OBJ(evtVarp, flp, "Default clocking event var not pre-created");
        AstCMethodHard* ধ্রুবক isTriggeredp = নতুন AstCMethodHard{
            flp, নতুন AstVarRef{flp, evtVarp, VAccess::READ}, VCMethod::EVENT_IS_TRIGGERED};
        isTriggeredp->dtypeSetBit();
        AstEventControl* ধ্রুবক waitp = নতুন AstEventControl{
            flp,
            নতুন Ast_Sen_বৃক্ষ{flp, নতুন AstSenItem{flp, VEdgeType::ET_EVENT,
                                               নতুন AstVarRef{flp, evtVarp, VAccess::READ}}},
            nullptr};
        ফেরত নতুন AstIf{flp, নতুন AstNot{flp, isTriggeredp}, waitp};
    }

    স্থির শূন্য checkSamplingFuncDType(Ast_গ্রন্থি_রাশিমালা* nodep, ধ্রুবক Ast_গ্রন্থি* exprp) {
        ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক dtypep = exprp->dtypep()->skipRefp();
        যদি (!dtypep->isIntegralOrPacked()) {
            nodep->v3error("Expected numeric type, but got a " << dtypep->prettyDTypeNameQ()
                                                               << " data type");
        }
    }

    Ast_Sen_বৃক্ষ* new_Sen_বৃক্ষ(Ast_গ্রন্থি* nodep, Ast_Sen_বৃক্ষ* useTreep = nullptr,
                           Ast_গ্রন্থি_Cover_Or_Assert* cassertp = nullptr) {
        // Create sentree based on clocked or default clock
        // Return nullptr for always
        যদি (useTreep) ফেরত useTreep;
        Ast_Sen_বৃক্ষ* newp = nullptr;
        AstSenItem* senip = m_senip;
        যৌক্তিক fromAlways = মিথ্যা;
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
            newp = নতুন Ast_Sen_বৃক্ষ{nodep->fileline(), nullptr};
        } অন্যথা {
            যদি (cassertp && fromAlways) cassertp->senFromAlways(সত্য);
            newp = নতুন Ast_Sen_বৃক্ষ{nodep->fileline(), senip->clone_বৃক্ষ(সত্য)};
        }
        ফেরত newp;
    }
    Ast_গ্রন্থি_রাশিমালা* getSequenceBodyExprp(ধ্রুবক AstSequence* ধ্রুবক seqp) ধ্রুবক {
        // The statements in AstSequence are optional AstVar (ports) followed by body expr.
        Ast_গ্রন্থি* bodyp = seqp->stmtsp();
        যখন (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        ফেরত VN_CAST(bodyp, গ্রন্থি_রাশিমালা);
    }
    AstPropSpec* getPropertyExprp(ধ্রুবক AstProperty* ধ্রুবক propp) {
        // Statements in AstProperty: AstVar (ports/local vars),
        // AstInitialStaticStmt/AstInitialAutomaticStmt (var init), AstPropSpec (body).
        Ast_গ্রন্থি* propExprp = propp->stmtsp();
        যখন (propExprp
               && (VN_IS(propExprp, Var) || VN_IS(propExprp, InitialStaticStmt)
                   || VN_IS(propExprp, InitialAutomaticStmt))) {
            propExprp = propExprp->nextp();
        }
        ফেরত VN_CAST(propExprp, PropSpec);
    }
    শূন্য substituteSequenceCall(AstFuncRef* funcrefp, AstSequence* seqp) {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Inline the sequence body at the call site, replacing the FuncRef
        Ast_গ্রন্থি_রাশিমালা* bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        // Clone the body expression since the sequence may be referenced multiple times
        Ast_গ্রন্থি_রাশিমালা* clonedp = bodyExprp->clone_বৃক্ষ(মিথ্যা);
        // Build substitution map, then do a single traversal to replace all formals
        // (textual substitution per IEEE 16.8.2).
        ধ্রুবক V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<ধ্রুবক AstVar*, Ast_গ্রন্থি_রাশিমালা*> portMap;
        জন্য (ধ্রুবক auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            ধ্রুবক auto it = portMap.find(refp->varp());
            যদি (it != portMap.end()) {
                refp->replaceWith(it->second->clone_বৃক্ষ(মিথ্যা));
                VL_DO_DANGLING(ঠেলো_Deletep(refp), refp);
            }
        });
        জন্য (ধ্রুবক auto& tconnect : tconnects) {
            ঠেলো_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        // Replace the FuncRef with the inlined body
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(ঠেলো_Deletep(funcrefp), funcrefp);
        // Clear referenced flag; sequences with isReferenced==false are deleted in assertPreAll
        seqp->isReferenced(মিথ্যা);
    }
    AstPropSpec* clonePropertyBody(AstFuncRef* funcrefp, ধ্রুবক AstProperty* propp) {
        AstPropSpec* propExprp = getPropertyExprp(propp);
        // Substitute inner property call before copying in order to not doing the same for
        // each call of outer property call.
        propExprp = substitutePropertyCall(propExprp);
        // Clone subtree after substitution. It is needed, because property might be called
        // multiple times with different arguments.
        propExprp = propExprp->clone_বৃক্ষ(মিথ্যা);
        // Build substitution maps for formal arguments and property-local
        // variables, then perform a single foreach to apply all replacements.
        // Map port vars to their actual argument expressions
        ধ্রুবক V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propp->stmtsp());
        std::unordered_map<ধ্রুবক AstVar*, Ast_গ্রন্থি_রাশিমালা*> portMap;
        জন্য (ধ্রুবক auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }

        // Promote property-local variables (non-port vars, IEEE 16.10) to
        // module-level __Vpropvar temps. Cross-cycle persistence is handled
        // by the match item lowering in visit(AstImplication*).
        std::unordered_map<ধ্রুবক AstVar*, AstVar*> localVarMap;
        জন্য (Ast_গ্রন্থি* stmtp = propp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (AstVar* ধ্রুবক varp = VN_CAST(stmtp, Var)) {
                যদি (!varp->isIO()) {
                    ধ্রুবক string newName = m_propVarNames.get(varp);
                    AstVar* ধ্রুবক newVarp = নতুন AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->addStmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        // Single traversal: substitute ports and update local var references
        propExprp->foreach([&](AstVarRef* refp) {
            ধ্রুবক auto portIt = portMap.find(refp->varp());
            যদি (portIt != portMap.end()) {
                refp->replaceWith(portIt->second->clone_বৃক্ষ(মিথ্যা));
                VL_DO_DANGLING(ঠেলো_Deletep(refp), refp);
                ফেরত;
            }
            ধ্রুবক auto localIt = localVarMap.find(refp->varp());
            যদি (localIt != localVarMap.end()) refp->varp(localIt->second);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        // Clean up argument expressions
        জন্য (ধ্রুবক auto& tconnect : tconnects) {
            ঠেলো_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(propExprp, funcrefp);
        ফেরত propExprp;
    }
    AstPropSpec* substitutePropertyCall(AstPropSpec* nodep) {
        যদি (AstFuncRef* ধ্রুবক funcrefp = VN_CAST(nodep->propp(), FuncRef)) {
            যদি (ধ্রুবক AstProperty* ধ্রুবক propp = VN_CAST(funcrefp->taskp(), Property)) {
                AstPropSpec* ধ্রুবক propExprp = clonePropertyBody(funcrefp, propp);

                // Handle case with 2 disable iff statement (IEEE 1800-2023 16.12.1)
                যদি (nodep->disablep() && propExprp->disablep()) {
                    nodep->v3error("disable iff expression before property call and in its "
                                   "body is not legal");
                    ঠেলো_Deletep(propExprp->disablep()->unlinkFrBack());
                }
                // If disable iff is in outer property, move it to inner
                যদি (nodep->disablep()) {
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক disablep = nodep->disablep()->unlinkFrBack();
                    propExprp->disablep(disablep);
                }

                যদি (nodep->sensesp() && propExprp->sensesp()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Clock event before property call and in its body");
                    ঠেলো_Deletep(propExprp->sensesp()->unlinkFrBack());
                }
                // If clock event is in outer property, move it to inner
                যদি (nodep->sensesp()) {
                    AstSenItem* ধ্রুবক sensesp = nodep->sensesp();
                    sensesp->unlinkFrBack();
                    propExprp->sensesp(sensesp);
                }

                // Now substitute property reference with property body
                nodep->replaceWith(propExprp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                ফেরত propExprp;
            }
        }
        ফেরত nodep;
    }

    শূন্য substituteNestedPropertyCall(AstFuncRef* funcrefp, ধ্রুবক AstProperty* propp) {
        AstPropSpec* ধ্রুবক specp = clonePropertyBody(funcrefp, propp);
        Ast_গ্রন্থি* replp = nullptr;
        যদি (specp->disablep()) {
            funcrefp->v3error("Illegal 'disable iff' in a nested property instance"
                              " (IEEE 1800-2023 16.12.1)");
        } অন্যথা যদি (specp->sensesp()) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: clock event in a nested property instance");
        } অন্যথা যদি (specp->propp()->exists(
                       [](ধ্রুবক Ast_গ্রন্থি_রাশিমালা* np) { ফেরত np->isMultiCycleSva(); })) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: multi-cycle body in a nested property instance");
        } অন্যথা {
            replp = specp->propp()->unlinkFrBack();
        }
        যদি (!replp) replp = নতুন AstConst{funcrefp->fileline(), AstConst::BitFalse{}};
        funcrefp->replaceWith(replp);
        VL_DO_DANGLING(ঠেলো_Deletep(funcrefp), funcrefp);
        VL_DO_DANGLING(ঠেলো_Deletep(specp), specp);
    }

    // VISITORS
    //========== Statements
    শূন্য visit(AstClocking* ধ্রুবক nodep) override {
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
        যদি (nodep->eventp()) nodep->add_পরবর্তী_Here(nodep->eventp()->unlinkFrBack());
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
    }
    শূন্য visit(AstModportClockingRef* ধ্রুবক nodep) override {
        // It has to be converted to a list of ModportClockingVarRefs,
        // because clocking blocks are removed in this pass
        জন্য (Ast_গ্রন্থি* itemp = nodep->clockingp()->itemsp(); itemp; itemp = itemp->nextp()) {
            যদি (AstClockingItem* ধ্রুবক citemp = VN_CAST(itemp, ClockingItem)) {
                যদি (AstVar* ধ্রুবক varp
                    = citemp->varp() ? citemp->varp() : VN_AS(citemp->user1p(), Var)) {
                    AstModportVarRef* ধ্রুবক modVarp = নতুন AstModportVarRef{
                        nodep->fileline(), varp->name(), citemp->direction()};
                    modVarp->varp(varp);
                    nodep->add_পরবর্তী_Here(modVarp);
                    যদি (citemp->direction().isOutput()) {
                        // Drives through the modport set the drive flag
                        AstVar* ধ্রুবক drivenVarp = getCreateDrivenVarp(citemp);
                        AstModportVarRef* ধ্রুবক drivenRefp = নতুন AstModportVarRef{
                            nodep->fileline(), drivenVarp->name(), VDirection::OUTPUT};
                        drivenRefp->varp(drivenVarp);
                        nodep->add_পরবর্তী_Here(drivenRefp);
                    }
                }
            }
        }
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
    }
    শূন্য visit(AstClockingItem* ধ্রুবক nodep) override {
        // Get a ref to the sampled/driven variable
        AstVar* ধ্রুবক varp = nodep->varp();
        যদি (!varp) {
            // Unused item
            ফেরত;
        }
        // Flag set by drives of an output clockvar, possibly visited before this item
        যদি (nodep->direction().isOutput()) m_modp->addStmtsp(getCreateDrivenVarp(nodep));
        File_সারি* ধ্রুবক flp = nodep->fileline();
        V3Const::constifyEdit(nodep->skewp());
        যদি (!VN_IS(nodep->skewp(), Const)) {
            nodep->skewp()->v3error("Skew must be constant (IEEE 1800-2023 14.4)");
            ফেরত;
        }
        AstConst* ধ্রুবক skewp = VN_AS(nodep->skewp(), Const);
        যদি (skewp->অঙ্ক().isNegative()) skewp->v3error("Skew cannot be negative");
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক exprp = nodep->exprp();
        varp->name(m_clockingp->name() + "__DOT__" + varp->name());
        m_clockingp->add_পরবর্তী_Here(varp->unlinkFrBack());
        nodep->user1p(varp);
        varp->user1p(nodep);
        যদি (nodep->direction() == VDirection::OUTPUT) {
            exprp->foreach([](ধ্রুবক Ast_গ্রন্থি_Var_Ref* varrefp) {
                // Prevent confusing BLKANDNBLK warnings on clockvars due to generated assignments
                varrefp->fileline()->warnOff(V3ErrorCode::BLKANDNBLK, সত্য);
                // Mark the driven signal so later passes (e.g. V3Inst port
                // connection) know the drive flows out from inside the cell.
                // Item expressions are plain signal references, so only the
                // driven signal is marked, never index selects.
                varrefp->varp()->isClockingDriven(সত্য);
            });
            AstVarRef* ধ্রুবক skewedReadRefp = নতুন AstVarRef{flp, varp, VAccess::READ};
            skewedReadRefp->user1(সত্য);
            // Initialize the clockvar
            AstVarRef* ধ্রুবক skewedWriteRefp = নতুন AstVarRef{flp, varp, VAccess::WRITE};
            skewedWriteRefp->user1(সত্য);
            AstInitialStatic* ধ্রুবক initClockvarp = নতুন AstInitialStatic{
                flp, নতুন AstAssign{flp, skewedWriteRefp, exprp->clone_বৃক্ষ_Pure(মিথ্যা)}};
            m_modp->addStmtsp(initClockvarp);
            // Each drive sets a flag, so the signal is also assigned when driven with the same
            // value again, e.g. after another assignment to the signal (IEEE 1800-2023 14.16.2)
            AstVar* ধ্রুবক drivenVarp = getCreateDrivenVarp(nodep);
            m_modp->addStmtsp(নতুন AstInitialStatic{
                flp, নতুন AstAssign{flp, নতুন AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                   নতুন AstConst{flp, AstConst::BitFalse{}}}});
            // Assign the clockvar to the actual var if it was driven
            AstAssign* ধ্রুবক assignp
                = নতুন AstAssign{flp, exprp->clone_বৃক্ষ_Pure(মিথ্যা), skewedReadRefp};
            AstIf* ধ্রুবক ifp
                = নতুন AstIf{flp, নতুন AstVarRef{flp, drivenVarp, VAccess::READ},
                            নতুন AstAssign{flp, নতুন AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                          নতুন AstConst{flp, AstConst::BitFalse{}}}};
            ifp->addThensp(assignp);
            // Only in the time step of the clocking event
            AstCMethodHard* ধ্রুবক trigp = নতুন AstCMethodHard{
                nodep->fileline(), নতুন AstVarRef{flp, m_clockingp->ensureEventp(), VAccess::READ},
                VCMethod::EVENT_IS_TRIGGERED};
            trigp->dtypeSetBit();
            ifp->condp(নতুন AstLogAnd{flp, ifp->condp()->unlinkFrBack(), trigp});
            // A skew rounding to no time precision ticks is a zero skew
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক ticksp
                = V3Timing::delayToTicks(skewp->clone_বৃক্ষ(মিথ্যা), m_modp->timeunit());
            ধ্রুবক যৌক্তিক zeroSkew = ticksp->isZero();
            VL_DO_DANGLING(ticksp->delete_বৃক্ষ(), ticksp);
            যদি (zeroSkew || !skewp->fileline()->timingOn()) {
                // Drive the var in Re-NBA (IEEE 1800-2023 14.16), ignoring skews where timing is
                // turned off
                Ast_Sen_বৃক্ষ* senTreep = m_clockingp->sentreep()->clone_বৃক্ষ(মিথ্যা);
                senTreep->addSensesp(নতুন AstSenItem{
                    flp, VEdgeType::ET_CHANGED, নতুন AstVarRef{flp, drivenVarp, VAccess::READ}});
                m_clockingp->add_পরবর্তী_Here(নতুন AstAlwaysReactive{flp, senTreep, ifp});
            } অন্যথা {
                // Fork only the assignment, so this AlwaysPostponed can be retriggered
                // before the assignment happens
                AstFork* ধ্রুবক forkp = নতুন AstFork{flp, VJoinType::JOIN_NONE};
                forkp->immediateStart(সত্য);
                forkp->addForksp(নতুন AstBegin{flp, "", assignp->unlinkFrBack(), সত্য});
                ifp->addThensp(forkp);
                // Read the clockvar in Postponed, so a drive late in the time step is applied
                // at this event plus the skew (IEEE 1800-2023 14.16)
                m_clockingp->add_পরবর্তী_Here(নতুন AstAlwaysPostponed{flp, ifp});
                যদি (v3Global.opt.timing().isSetTrue()) {
                    AstDelay* ধ্রুবক delayp = নতুন AstDelay{flp, skewp->unlinkFrBack(), মিথ্যা};
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
            AstVarRef* ধ্রুবক refp = নতুন AstVarRef{flp, varp, VAccess::WRITE};
            refp->user1(সত্য);
            যদি (skewp->অঙ্ক().is1Step()) {
                // #1step means the value that is sampled is always the signal's last value
                // before the clock edge (IEEE 1800-2023 14.4)
                AstSampled* ধ্রুবক sampledp
                    = নতুন AstSampled{flp, exprp->clone_বৃক্ষ_Pure(মিথ্যা), exprp->dtypep(), সত্য};
                AstAssign* ধ্রুবক assignp = নতুন AstAssign{flp, refp, sampledp};
                m_clockingp->add_পরবর্তী_Here(নতুন AstAlways{
                    flp, VAlwaysKwd::ALWAYS, m_clockingp->sentreep()->clone_বৃক্ষ(মিথ্যা), assignp});
            } অন্যথা যদি (skewp->isZero()) {
                // #0 means the var has to be sampled in Observed (IEEE 1800-2023 14.13)
                AstAssign* ধ্রুবক assignp = নতুন AstAssign{flp, refp, exprp->clone_বৃক্ষ_Pure(মিথ্যা)};
                m_clockingp->add_পরবর্তী_Here(নতুন AstAlwaysObserved{
                    flp, m_clockingp->sentreep()->clone_বৃক্ষ(মিথ্যা), assignp});
            } অন্যথা {
                // Create a queue where we'll store sampled values with timestamps
                AstSampleQueueDType* ধ্রুবক queueDtp
                    = নতুন AstSampleQueueDType{flp, exprp->dtypep()};
                m_netlistp->typeTablep()->addTypesp(queueDtp);
                AstVar* ধ্রুবক queueVarp
                    = নতুন AstVar{flp, VVarType::MODULETEMP, "__Vqueue__" + varp->name(), queueDtp};
                queueVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_clockingp->add_পরবর্তী_Here(queueVarp);
                // Create a process like this:
                //     always queue.push(<sampled var>);
                AstCMethodHard* ধ্রুবক pushp = নতুন AstCMethodHard{
                    flp, নতুন AstVarRef{flp, queueVarp, VAccess::WRITE}, VCMethod::DYN_ঠেলো,
                    নতুন AstTime{nodep->fileline(), m_modp->timeunit()}};
                pushp->addPinsp(exprp->clone_বৃক্ষ_Pure(মিথ্যা));
                pushp->dtypeSetVoid();
                m_clockingp->add_পরবর্তী_Here(
                    নতুন AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, pushp->makeStmt()});
                // Create a process like this:
                //     always @<clocking event> queue.pop(<skew>, /*out*/<skewed var>);
                AstCMethodHard* ধ্রুবক popp = নতুন AstCMethodHard{
                    flp, নতুন AstVarRef{flp, queueVarp, VAccess::READWRITE}, VCMethod::DYN_বের_করো,
                    নতুন AstTime{nodep->fileline(), m_modp->timeunit()}};
                popp->addPinsp(skewp->unlinkFrBack());
                refp->access(VAccess::READWRITE);  // Only conditionally assigned
                popp->addPinsp(refp);
                popp->dtypeSetVoid();
                m_clockingp->add_পরবর্তী_Here(নতুন AstAlways{flp, VAlwaysKwd::ALWAYS,
                                                       m_clockingp->sentreep()->clone_বৃক্ষ(মিথ্যা),
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
            ফেরত;
        }
        যদি (m_inAssign && !m_inSynchDrive) {
            nodep->v3error("Cycle delays not allowed as intra-assignment delays"
                           " (IEEE 1800-2023 14.11)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
            ফেরত;
        }
        যদি (nodep->stmtsp()) nodep->add_পরবর্তী_Here(nodep->stmtsp()->unlink_Fr_Back_With_পরবর্তী());
        File_সারি* ধ্রুবক flp = nodep->fileline();
        Ast_গ্রন্থি_রাশিমালা* valuep = V3Const::constifyEdit(nodep->lhsp()->unlinkFrBack());
        ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(valuep, Const);
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
                ফেরত;
            }
            যদি (m_pexprp) {
                // ##0 in sequence context = zero delay = same clock tick
                VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
                ফেরত;
            }
            // Procedural ##0: synchronize with default clocking event (IEEE 1800-2023 14.11)
            যদি (!m_defaultClockingp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
                ফেরত;
            }
            nodep->replaceWith(newCycleDelayZerop(flp));
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            ফেরত;
        }
        Ast_Sen_বৃক্ষ* sentreep = nullptr;
        যদি (!m_drives.empty()) {
            যদি (m_inConcatAssign) {
                // Also the clockvars may be of different clocking blocks
                nodep->v3error("Cycle delays cannot be used in synchronous drives to a"
                               " concatenation (IEEE 1800-2023 14.16)");
                VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(ঠেলো_Deletep(valuep), valuep);
                ফেরত;
            }
            // Count the cycles of the driven clockvar's clocking block (IEEE 1800-2023 14.16)
            sentreep = newDriveSentreep(nodep, m_drives.front());
            যদি (!sentreep) {
                VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(ঠেলো_Deletep(valuep), valuep);
                ফেরত;
            }
        } অন্যথা যদি (!m_defaultClockingp) {
            যদি (!m_pexprp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(ঠেলো_Deletep(valuep), valuep);
                ফেরত;
            }
            sentreep = নতুন Ast_Sen_বৃক্ষ{flp, m_senip->clone_বৃক্ষ(মিথ্যা)};
        } অন্যথা {
            sentreep = m_defaultClockingp->sentreep()->clone_বৃক্ষ(মিথ্যা);
        }
        AstEventControl* ধ্রুবক controlp
            = নতুন AstEventControl{nodep->fileline(), sentreep, nullptr};
        ধ্রুবক std::string delayName = m_cycleDlyNames.get(nodep);
        Ast_গ্রন্থি_রাশিমালা* throughoutp
            = nodep->throughoutp() ? nodep->throughoutp()->unlinkFrBack() : nullptr;

        AstVar* ধ্রুবক cntVarp = নতুন AstVar{flp, VVarType::BLOCKTEMP, delayName + "__counter",
                                           nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstBegin* ধ্রুবক beginp = নতুন AstBegin{flp, delayName + "__block", cntVarp, সত্য};
        // A non-constant delay is counted at runtime. The count is integral of 64 bits, as
        // V3Width makes delays, or real, which is rounded (IEEE 1800-2023 6.12.1)
        যদি (valuep->isDouble()) {
            valuep = নতুন AstRToIRoundS{flp, valuep};
            valuep->dtypeFrom(cntVarp);
        } অন্যথা {
            valuep = নতুন AstSel{flp, valuep, 0, cntVarp->width()};
        }
        beginp->addStmtsp(নতুন AstAssign{flp, নতুন AstVarRef{flp, cntVarp, VAccess::WRITE}, valuep});
        // If the count is zero, a synchronous drive waits for none, like with a constant '##0',
        // and a procedural delay waits like a procedural '##0' (IEEE 1800-2023 14.11)
        যদি (!constp && !m_inSynchDrive) {
            beginp->addStmtsp(নতুন AstIf{
                flp,
                নতুন AstEq{flp, নতুন AstVarRef{flp, cntVarp, VAccess::READ}, নতুন AstConst{flp, 0}},
                newCycleDelayZerop(flp)});
        }

        // Throughout: create flag tracking whether condition held every tick
        AstVar* throughoutOkp = nullptr;
        যদি (throughoutp) {
            throughoutOkp = নতুন AstVar{flp, VVarType::BLOCKTEMP, delayName + "__throughoutOk",
                                       nodep->findBasicDType(VBasicDTypeKwd::LOGIC_IMPLICIT)};
            throughoutOkp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->addStmtsp(throughoutOkp);
            beginp->addStmtsp(নতুন AstAssign{flp, নতুন AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            নতুন AstConst{flp, AstConst::BitTrue{}}});
            // Check condition at tick 0 (sequence start, before entering loop)
            beginp->addStmtsp(
                নতুন AstIf{flp, নতুন AstLogNot{flp, throughoutp->clone_বৃক্ষ_Pure(মিথ্যা)},
                          নতুন AstAssign{flp, নতুন AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                        নতুন AstConst{flp, AstConst::BitFalse{}}}});
        }

        {
            AstLoop* ধ্রুবক loopp = নতুন AstLoop{flp};
            // When throughout is present, exit loop early if condition fails
            Ast_গ্রন্থি_রাশিমালা* loopCondp
                = নতুন AstGt{flp, নতুন AstVarRef{flp, cntVarp, VAccess::READ}, নতুন AstConst{flp, 0}};
            যদি (throughoutOkp) {
                loopCondp = নতুন AstLogAnd{flp, loopCondp,
                                          নতুন AstVarRef{flp, throughoutOkp, VAccess::READ}};
            }
            loopp->addStmtsp(নতুন AstLoopTest{flp, loopp, loopCondp});
            loopp->addStmtsp(controlp);
            loopp->addStmtsp(
                নতুন AstAssign{flp, নতুন AstVarRef{flp, cntVarp, VAccess::WRITE},
                              নতুন AstSub{flp, নতুন AstVarRef{flp, cntVarp, VAccess::READ},
                                         নতুন AstConst{flp, 1}}});
            // Check throughout condition at each tick during delay (IEEE 1800-2023 16.9.9)
            যদি (throughoutp) {
                loopp->addStmtsp(
                    নতুন AstIf{flp, নতুন AstLogNot{flp, throughoutp},
                              নতুন AstAssign{flp, নতুন AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            নতুন AstConst{flp, AstConst::BitFalse{}}}});
            }
            beginp->addStmtsp(loopp);
        }
        // Compose wrappers on remaining sequence: throughout gate (inner), disable iff (outer)
        Ast_গ্রন্থি* remainp = nodep->nextp() ? nodep->nextp()->unlink_Fr_Back_With_পরবর্তী() : nullptr;
        যদি (throughoutOkp) {
            // If condition failed during delay, fail assertion
            remainp = নতুন AstIf{flp, নতুন AstVarRef{flp, throughoutOkp, VAccess::READ}, remainp,
                                নতুন AstPExprClause{flp, /*pass=*/মিথ্যা}};
        }
        যদি (m_disableSeqIfp && remainp) {
            AstIf* ধ্রুবক disableSeqIfp = m_disableSeqIfp->clone_বৃক্ষ(মিথ্যা);
            disableSeqIfp->addThensp(remainp);
            remainp = disableSeqIfp;
        }
        যদি (remainp) {
            যদি (throughoutOkp) {
                // throughoutOkp is declared in beginp scope -- check must be inside it
                beginp->addStmtsp(remainp);
            } অন্যথা {
                nodep->add_পরবর্তী_Here(remainp);
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

        ধ্রুবক AstSampled* sampledp;
        যদি (nodep->exists([&sampledp](ধ্রুবক AstSampled* ধ্রুবক sp) {
                sampledp = sp;
                ফেরত সত্য;
            })) {
            sampledp->v3warn(E_UNSUPPORTED, "Unsupported: $sampled inside sensitivity list");
        }
    }
    শূন্য visit(Ast_গ্রন্থি_Var_Ref* nodep) override {
        UINFO(8, " -varref:  " << nodep);
        UINFO(8, " -varref-var-back:  " << nodep->varp()->backp());
        UINFO(8, " -varref-var-user1:  " << nodep->varp()->user1p());
        যদি (AstClockingItem* ধ্রুবক itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->firstAbovep(),
                ClockingItem)) {
            যদি (nodep->user1()) ফেরত;

            // ensure linking still works, this has to be done only once
            যদি (AstVarXRef* xrefp = VN_CAST(nodep, VarXRef)) {
                UINFO(8, " -clockvarxref-in:  " << xrefp);
                string dotted = xrefp->dotted();
                ধ্রুবক আকার_t dotPos = dotted.rfind('.');
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
        যদি (AstClockingItem* ধ্রুবক itemp = VN_CAST(
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
        যদি (nodep->user1()) ফেরত;
        VL_RESTORER(m_inAssign);
        VL_RESTORER(m_inSynchDrive);
        VL_RESTORER(m_inConcatAssign);
        VL_RESTORER_CLEAR(m_drives);
        m_inAssign = সত্য;
        m_inSynchDrive = মিথ্যা;
        {
            // The braces of a concatenation are a replication, by one
            ধ্রুবক Ast_গ্রন্থি_রাশিমালা* lhsp = nodep->lhsp();
            যদি (ধ্রুবক AstReplicate* ধ্রুবক repp = VN_CAST(lhsp, Replicate)) lhsp = repp->srcp();
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
            AstAssign* ধ্রুবক assignp = নতুন AstAssign{
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
        জন্য (ধ্রুবক SynchDrive& drive : m_drives) {
            setsp = Ast_গ্রন্থি::add_পরবর্তী(setsp, newDrivenSetp(nodep->fileline(), drive));
        }
        যদি (!setsp) ফেরত;
        যদি (AstBegin* ধ্রুবক beginp = VN_CAST(nodep->timingControlp(), Begin)) {
            beginp->addStmtsp(setsp);
        } অন্যথা {
            nodep->add_পরবর্তী_Here(setsp);
        }
    }
    শূন্য visit(AstAlways* nodep) override {
        iterate_And_পরবর্তী_Null(nodep->sentreep());
        যদি (nodep->sentreep()) m_seniAlwaysp = nodep->sentreep()->sensesp();
        iterate_And_পরবর্তী_Null(nodep->stmtsp());
        m_seniAlwaysp = nullptr;
    }

    শূন্য visit(Ast_গ্রন্থি_Cover_Or_Assert* nodep) override {
        যদি (nodep->sentreep()) ফেরত;  // Already processed

        VL_RESTORER(m_senip);
        VL_RESTORER(m_disablep);
        m_senip = nullptr;
        m_disablep = nullptr;

        // Find Clocking's buried under nodep->exprsp
        iterateChildren(nodep);
        যদি (!nodep->immediate()) nodep->sentreep(new_Sen_বৃক্ষ(nodep, nullptr, nodep));
    }
    শূন্য visit(AstFalling* nodep) override {
        যদি (nodep->user1SetOnce()) ফেরত;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_সারি* ধ্রুবক fl = nodep->fileline();
        Ast_গ্রন্থি_রাশিমালা* exprp = nodep->exprp()->unlinkFrBack();
        যদি (exprp->width() > 1) exprp = নতুন AstSel{fl, exprp, 0, 1};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক futurep = নতুন AstFuture{fl, exprp, new_Sen_বৃক্ষ(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = নতুন AstAnd{fl, exprp->clone_বৃক্ষ_Pure(মিথ্যা), নতুন AstNot{fl, futurep}};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstFell* nodep) override {
        যদি (nodep->user1SetOnce()) ফেরত;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_সারি* ধ্রুবক fl = nodep->fileline();
        Ast_গ্রন্থি_রাশিমালা* exprp = nodep->exprp()->unlinkFrBack();
        যদি (exprp->width() > 1) exprp = নতুন AstSel{fl, exprp, 0, 1};
        Ast_Sen_বৃক্ষ* sentreep = nodep->sentreep();
        যদি (sentreep) sentreep->unlinkFrBack();
        AstPast* ধ্রুবক pastp = নতুন AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_বৃক্ষ(nodep, sentreep));
        exprp = নতুন AstAnd{fl, pastp, নতুন AstNot{fl, exprp->clone_বৃক্ষ_Pure(মিথ্যা)}};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstFuture* nodep) override {
        যদি (nodep->user1SetOnce()) ফেরত;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        Ast_Sen_বৃক্ষ* ধ্রুবক sentreep = nodep->sentreep();
        যদি (sentreep) VL_DO_DANGLING(ঠেলো_Deletep(sentreep->unlinkFrBack()), sentreep);
        nodep->sentreep(new_Sen_বৃক্ষ(nodep));
    }
    শূন্য visit(AstPast* nodep) override {
        checkSamplingFuncDType(nodep, nodep->exprp());
        যদি (nodep->sentreep()) ফেরত;  // Already processed
        iterateChildren(nodep);
        nodep->sentreep(new_Sen_বৃক্ষ(nodep));
    }
    শূন্য visit(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- Lower standalone exact [*N] (N >= 2) via saturating counter.
        // Range/unbounded forms and SExpr-contained forms are lowered by V3AssertNfa.
        iterateChildren(nodep);
        // V3AssertNfa handles unbounded/ranged forms upstream, so this fast-path
        // is effectively unreachable when NFA is enabled.
        যদি (nodep->unbounded() || nodep->maxCountp()) ফেরত;  // LCOV_EXCL_LINE
        ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(nodep->countp(), Const);
        যদি (VL_UNLIKELY(!constp || constp->toSInt() < 1)) {
            nodep->v3fatalSrc("Consecutive repetition count must be a positive constant"
                              " (should have been caught by V3Width)");
            ফেরত;
        }
        ধ্রুবক পূর্ণসংখ্যা n = constp->toSInt();
        File_সারি* ধ্রুবক flp = nodep->fileline();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক exprp = nodep->exprp()->unlinkFrBack();
        যদি (n == 1) {
            nodep->replaceWith(exprp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            ফেরত;
        }
        // Saturating counter: if (expr) cnt <= min(cnt+1, N); else cnt <= 0;
        AstVar* ধ্রুবক cntVarp = নতুন AstVar{flp, VVarType::MODULETEMP, m_consRepNames.get(""),
                                           nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(cntVarp);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক রাশিমালা_Clonep = exprp->clone_বৃক্ষ_Pure(মিথ্যা);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক saturatingIncrp = নতুন AstCond{
            flp,
            নতুন AstLt{flp, নতুন AstVarRef{flp, cntVarp, VAccess::READ},
                      নতুন AstConst{flp, static_cast<uint32_t>(n)}},
            নতুন AstAdd{flp, নতুন AstVarRef{flp, cntVarp, VAccess::READ}, নতুন AstConst{flp, 1u}},
            নতুন AstConst{flp, static_cast<uint32_t>(n)}};
        AstAssignDly* ধ্রুবক incrAssignp
            = নতুন AstAssignDly{flp, নতুন AstVarRef{flp, cntVarp, VAccess::WRITE}, saturatingIncrp};
        AstAssignDly* ধ্রুবক resetAssignp = নতুন AstAssignDly{
            flp, নতুন AstVarRef{flp, cntVarp, VAccess::WRITE}, নতুন AstConst{flp, 0u}};
        AstIf* ধ্রুবক ifp = নতুন AstIf{flp, রাশিমালা_Clonep, incrAssignp, resetAssignp};
        Ast_Sen_বৃক্ষ* ধ্রুবক senTreep = new_Sen_বৃক্ষ(nodep);
        AstAlways* ধ্রুবক alwaysp = নতুন AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, ifp};
        cntVarp->add_পরবর্তী_Here(alwaysp);
        // Match: cnt >= N-1 (previous cycles via NBA) && expr (current cycle)
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক cntCheckp = নতুন AstGte{flp, নতুন AstVarRef{flp, cntVarp, VAccess::READ},
                                                  নতুন AstConst{flp, static_cast<uint32_t>(n - 1)}};
        cntCheckp->dtypeSetBit();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক matchp = নতুন AstAnd{flp, cntCheckp, exprp};
        matchp->dtypeSetBit();
        nodep->replaceWith(matchp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstRising* nodep) override {
        যদি (nodep->user1SetOnce()) ফেরত;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_সারি* ধ্রুবক fl = nodep->fileline();
        Ast_গ্রন্থি_রাশিমালা* exprp = nodep->exprp()->unlinkFrBack();
        যদি (exprp->width() > 1) exprp = নতুন AstSel{fl, exprp, 0, 1};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক futurep = নতুন AstFuture{fl, exprp, new_Sen_বৃক্ষ(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = নতুন AstAnd{fl, নতুন AstNot{fl, exprp->clone_বৃক্ষ_Pure(মিথ্যা)}, futurep};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstRose* nodep) override {
        যদি (nodep->user1SetOnce()) ফেরত;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_সারি* ধ্রুবক fl = nodep->fileline();
        Ast_গ্রন্থি_রাশিমালা* exprp = nodep->exprp()->unlinkFrBack();
        যদি (exprp->width() > 1) exprp = নতুন AstSel{fl, exprp, 0, 1};
        Ast_Sen_বৃক্ষ* sentreep = nodep->sentreep();
        যদি (sentreep) sentreep->unlinkFrBack();
        AstPast* ধ্রুবক pastp = নতুন AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_বৃক্ষ(nodep, sentreep));
        exprp = নতুন AstAnd{fl, নতুন AstNot{fl, pastp}, exprp->clone_বৃক্ষ_Pure(মিথ্যা)};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    স্থির AstAssocArrayDType* getProcessAssocArrayType(File_সারি* ধ্রুবক flp) {
        // Type of __VassertsActive___x[std::process]
        Ast_গ্রন্থি_DType* valp
            = v3Global.rootp()->typeTablep()->findBasicDType(flp, VBasicDTypeKwd::BIT);
        AstClassRefDType* keyp
            = নতুন AstClassRefDType{flp, v3Global.rootp()->stdPackageProcessp(), nullptr};
        keyp->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        v3Global.rootp()->typeTablep()->addTypesp(keyp);
        AstAssocArrayDType* ধ্রুবক typep = নতুন AstAssocArrayDType{flp, valp, keyp};
        typep->dtypep(typep);
        v3Global.rootp()->typeTablep()->addTypesp(typep);
        ফেরত typep;
    }
    স্থির Ast_Stmt_রাশিমালা* getProcessAssocArrayDelete(AstVarRef* ধ্রুবক refp) {
        // Constructs refp.delete(std::process::self()) statement
        File_সারি* ধ্রুবক flp = refp->fileline();
        AstCMethodHard* ধ্রুবক deletep = নতুন AstCMethodHard{
            flp, refp, VCMethod::ASSOC_ERASE, v3Global.rootp()->stdPackageProcessSelfp(flp)};
        deletep->dtypep(refp->findVoidDType());
        ফেরত নতুন Ast_Stmt_রাশিমালা{flp, deletep};
    }
    স্থির Ast_গ্রন্থি_রাশিমালা* get_Process_Assoc_Array_আকার(AstVarRef* ধ্রুবক refp) {
        // Constructs refp.size() statement
        AstCMethodHard* ধ্রুবক sizep
            = নতুন AstCMethodHard{refp->fileline(), refp, VCMethod::ASSOC_আকার};
        sizep->dtypep(refp->findBasicDType(VBasicDTypeKwd::UINT32));
        ফেরত sizep;
    }
    শূন্য visit(AstSEventually* nodep) override {
        UASSERT(v3Global.rootp()->stdPackagep(), "Should be imported");

        Ast_Sen_বৃক্ষ* ধ্রুবক sentreep = new_Sen_বৃক্ষ(nodep);
        যদি (!sentreep->sensesp()) {
            VL_DO_DANGLING(ঠেলো_Deletep(sentreep), sentreep);
            nodep->replaceWith(নতুন AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            ফেরত;
        }

        File_সারি* ধ্রুবক flp = nodep->fileline();

        // Track active assertions
        AstVar* ধ্রুবক activep = নতুন AstVar{flp, VVarType::MODULETEMP, m_activeNames.get(""),
                                           getProcessAssocArrayType(flp)};
        activep->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(activep);

        // Assertion condition check
        AstLoop* ধ্রুবক loopp = নতুন AstLoop{flp};
        AstSampled* ধ্রুবক condp
            = নতুন AstSampled{flp, nodep->exprp()->unlinkFrBack(), nullptr, সত্য};
        loopp->addStmtsp(নতুন AstLoopTest{flp, loopp, নতুন AstLogNot{flp, condp}});
        loopp->addStmtsp(নতুন AstEventControl{flp, sentreep, nullptr});

        // Add assertion to the active set
        AstAssocSel* ধ্রুবক selp = নতুন AstAssocSel{flp, নতুন AstVarRef{flp, activep, VAccess::WRITE},
                                                  v3Global.rootp()->stdPackageProcessSelfp(flp)};
        AstAssign* ধ্রুবক incrementp = নতুন AstAssign{flp, selp, নতুন AstConst{flp, 1}};
        AstPExprClause* ধ্রুবক clausep = নতুন AstPExprClause{flp};
        Ast_Stmt_রাশিমালা* ধ্রুবক deletep
            = getProcessAssocArrayDelete(নতুন AstVarRef{flp, activep, VAccess::WRITE});

        // Main assertion block
        AstBegin* ধ্রুবক bodyp = নতুন AstBegin{flp, m_blockNames.get(""), nullptr, সত্য};
        bodyp->addStmtsp(incrementp);
        bodyp->addStmtsp(loopp);
        bodyp->addStmtsp(clausep);
        bodyp->addStmtsp(deletep);

        // Validate assertion condition for each active assert
        AstVar* ধ্রুবক activeCountp = নতুন AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        activeCountp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstAssign* ধ্রুবক initActiveCountp
            = নতুন AstAssign{flp, নতুন AstVarRef{flp, activeCountp, VAccess::WRITE},
                            get_Process_Assoc_Array_আকার(নতুন AstVarRef{flp, activep, VAccess::READ})};
        AstLoop* ধ্রুবক finalLoopp = নতুন AstLoop{flp};
        AstIf* ধ্রুবক finalBodypCondp
            = নতুন AstIf{flp, condp->clone_বৃক্ষ_Pure(মিথ্যা), নতুন AstPExprClause{flp},
                        নতুন AstPExprClause{flp, মিথ্যা}};
        finalLoopp->addStmtsp(
            নতুন AstLoopTest{flp, finalLoopp,
                            নতুন AstNeq{flp, নতুন AstVarRef{flp, activeCountp, VAccess::READ},
                                       নতুন AstConst{flp, 0}}});
        finalLoopp->addStmtsp(finalBodypCondp);
        finalLoopp->addStmtsp(
            নতুন AstAssign{flp, নতুন AstVarRef{flp, activeCountp, VAccess::WRITE},
                          নতুন AstSub{flp, নতুন AstVarRef{flp, activeCountp, VAccess::READ},
                                     নতুন AstConst{flp, 1}}});

        // Final assertion block
        AstBegin* ধ্রুবক finalp = নতুন AstBegin{flp, m_blockNames.get(""), nullptr, সত্য};
        finalp->addStmtsp(activeCountp);
        finalp->addStmtsp(initActiveCountp);
        finalp->addStmtsp(finalLoopp);

        VL_RESTORER(m_pexprp);
        m_pexprp = নতুন AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
        VL_RESTORER(m_hasCycleDelay);
        m_hasCycleDelay = মিথ্যা;
        iterate(bodyp);
        iterate(finalp);

        যদি (m_hasCycleDelay) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in s_eventually");
            nodep->replaceWith(নতুন AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            VL_DO_DANGLING(m_pexprp->delete_বৃক্ষ(), m_pexprp);
            ফেরত;
        }

        nodep->replaceWith(m_pexprp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstStable* nodep) override {
        যদি (nodep->user1SetOnce()) ফেরত;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_সারি* ধ্রুবক fl = nodep->fileline();
        Ast_গ্রন্থি_রাশিমালা* exprp = nodep->exprp()->unlinkFrBack();
        Ast_Sen_বৃক্ষ* sentreep = nodep->sentreep();
        যদি (sentreep) sentreep->unlinkFrBack();
        AstPast* ধ্রুবক pastp = নতুন AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_বৃক্ষ(nodep, sentreep));
        exprp = নতুন AstEq{fl, pastp, exprp->clone_বৃক্ষ_Pure(মিথ্যা)};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstSteady* nodep) override {
        যদি (nodep->user1SetOnce()) ফেরত;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_সারি* ধ্রুবক fl = nodep->fileline();
        Ast_গ্রন্থি_রাশিমালা* exprp = nodep->exprp()->unlinkFrBack();
        যদি (exprp->width() > 1) exprp = নতুন AstSel{fl, exprp, 0, 1};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক futurep = নতুন AstFuture{fl, exprp, new_Sen_বৃক্ষ(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = নতুন AstEq{fl, exprp->clone_বৃক্ষ_Pure(মিথ্যা), futurep};
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
    ধ্রুবক AstConst* validate_Rep_গুনতি(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি_রাশিমালা*& countp) {
        countp = V3Const::constifyEdit(countp);
        ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(countp, Const);
        যদি (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ঠেলো_Deletep(countp), countp);
            nodep->replaceWith(নতুন AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            ফেরত nullptr;
        }
        যদি (constp->toSInt() < 0) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ঠেলো_Deletep(countp), countp);
            nodep->replaceWith(নতুন AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            ফেরত nullptr;
        }
        যদি (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ঠেলো_Deletep(countp), countp);
            nodep->replaceWith(নতুন AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            ফেরত nullptr;
        }
        ফেরত constp;
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
                             Ast_গ্রন্থি_রাশিমালা* rhsp, যৌক্তিক isOverlapped, যৌক্তিক isNonConsec) {
        AstSenItem* ধ্রুবক sensesp = m_senip;
        UASSERT_OBJ(sensesp, exprp, "Repetition requires a clock");

        ধ্রুবক std::string name
            = isNonConsec ? m_nonConsRepNames.get(exprp) : m_gotoRepNames.get(exprp);
        AstVar* ধ্রুবক cntVarp = নতুন AstVar{flp, VVarType::BLOCKTEMP, name + "__counter",
                                           exprp->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstBegin* ধ্রুবক beginp = নতুন AstBegin{flp, name + "__block", cntVarp, সত্য};
        beginp->addStmtsp(
            নতুন AstAssign{flp, নতুন AstVarRef{flp, cntVarp, VAccess::WRITE}, নতুন AstConst{flp, 0}});

        // Counting loop: find N occurrences of expr (shared by goto and nonconsec)
        AstLoop* ধ্রুবক loopp = নতুন AstLoop{flp};
        loopp->addStmtsp(নতুন AstLoopTest{flp, loopp,
                                         নতুন AstLt{flp, নতুন AstVarRef{flp, cntVarp, VAccess::READ},
                                                   countp->clone_বৃক্ষ_Pure(মিথ্যা)}});
        // if (expr) cnt++ -- sampled is applied to whole property expr by V3Assert
        loopp->addStmtsp(
            নতুন AstIf{flp, exprp,
                      নতুন AstAssign{flp, নতুন AstVarRef{flp, cntVarp, VAccess::WRITE},
                                    নতুন AstAdd{flp, নতুন AstVarRef{flp, cntVarp, VAccess::READ},
                                               নতুন AstConst{flp, 1}}}});
        loopp->addStmtsp(নতুন AstIf{
            flp, নতুন AstLt{flp, নতুন AstVarRef{flp, cntVarp, VAccess::READ}, countp},
            নতুন AstEventControl{flp, নতুন Ast_Sen_বৃক্ষ{flp, sensesp->clone_বৃক্ষ(মিথ্যা)}, nullptr}});

        beginp->addStmtsp(loopp);

        যদি (isNonConsec && rhsp) {
            // IEEE 16.9.2: b[=N] = b[->N] ##1 !b[*0:$]
            // Trailing window: ##1 advance, then scan !expr cycles checking rhs.
            // Window closes when expr becomes true again (fail if rhs was never true).
            beginp->addStmtsp(নতুন AstEventControl{
                flp, নতুন Ast_Sen_বৃক্ষ{flp, sensesp->clone_বৃক্ষ(মিথ্যা)}, nullptr});  // ##1
            যদি (!isOverlapped) {
                beginp->addStmtsp(নতুন AstEventControl{
                    flp, নতুন Ast_Sen_বৃক্ষ{flp, sensesp->clone_বৃক্ষ(মিথ্যা)}, nullptr});  // |=>
            }
            // Window loop: check rhs at each !expr cycle (done variable for termination)
            AstVar* ধ্রুবক doneVarp
                = নতুন AstVar{flp, VVarType::BLOCKTEMP, name + "__done", exprp->findBitDType()};
            doneVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->addStmtsp(doneVarp);
            beginp->addStmtsp(নতুন AstAssign{flp, নতুন AstVarRef{flp, doneVarp, VAccess::WRITE},
                                            নতুন AstConst{flp, AstConst::BitFalse{}}});
            auto setDone = [&]() {
                ফেরত নতুন AstAssign{flp, নতুন AstVarRef{flp, doneVarp, VAccess::WRITE},
                                     নতুন AstConst{flp, AstConst::BitTrue{}}};
            };
            AstLoop* ধ্রুবক windowp = নতুন AstLoop{flp};
            // LoopTest: continue while !done
            windowp->addStmtsp(নতুন AstLoopTest{
                flp, windowp, নতুন AstNot{flp, নতুন AstVarRef{flp, doneVarp, VAccess::READ}}});
            // if (expr) { fail; done = 1; } -- window closed, expr true again
            Ast_গ্রন্থি* ধ্রুবক failsp = নতুন AstPExprClause{flp, মিথ্যা};
            failsp->add_পরবর্তী(setDone());
            windowp->addStmtsp(নতুন AstIf{flp, exprp->clone_বৃক্ষ_Pure(মিথ্যা), failsp});
            // if (rhs) { pass; done = 1; } -- consequent true at this !expr endpoint
            Ast_গ্রন্থি* ধ্রুবক passsp = নতুন AstPExprClause{flp, সত্য};
            passsp->add_পরবর্তী(setDone());
            windowp->addStmtsp(নতুন AstIf{flp, rhsp, passsp});
            // @(clk) -- advance to next cycle in window
            windowp->addStmtsp(
                নতুন AstEventControl{flp, নতুন Ast_Sen_বৃক্ষ{flp, sensesp->clone_বৃক্ষ(মিথ্যা)}, nullptr});
            beginp->addStmtsp(windowp);
        } অন্যথা যদি (isNonConsec) {
            // Standalone nonconsec: ##1 into window, then pass
            beginp->addStmtsp(
                নতুন AstEventControl{flp, নতুন Ast_Sen_বৃক্ষ{flp, sensesp->clone_বৃক্ষ(মিথ্যা)}, nullptr});
            beginp->addStmtsp(নতুন AstPExprClause{flp, সত্য});
        } অন্যথা যদি (rhsp) {
            // Goto rep: check consequent once at match endpoint
            যদি (!isOverlapped) {
                beginp->addStmtsp(নতুন AstEventControl{
                    flp, নতুন Ast_Sen_বৃক্ষ{flp, sensesp->clone_বৃক্ষ(মিথ্যা)}, nullptr});
            }
            beginp->addStmtsp(নতুন AstIf{flp, rhsp, নতুন AstPExprClause{flp, সত্য},
                                        নতুন AstPExprClause{flp, মিথ্যা}});
        } অন্যথা {
            // Standalone goto: pass after counting
            beginp->addStmtsp(নতুন AstPExprClause{flp, সত্য});
        }

        ফেরত নতুন AstPExpr{flp, beginp, exprp->findBitDType()};
    }

    // Reject [=M:N] nonconsecutive range that reached V3AssertPre.
    // [->M:N] is fully handled upstream (NFA path); only [=M:N] still lands
    // here because AstSNonConsRep lowering is not yet implemented there.
    // Replaces the node with BitFalse so lowering continues after the warning.
    যৌক্তিক rejectNonconsecRange(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি_রাশিমালা* maxCountp) {
        যদি (!maxCountp) ফেরত মিথ্যা;
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: [=M:N] nonconsecutive range repetition"
                                     " (IEEE 1800-2023 16.9.2)");
        nodep->replaceWith(নতুন AstConst{nodep->fileline(), AstConst::BitFalse{}});
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        ফেরত সত্য;
    }

    শূন্য visit(AstSGotoRep* nodep) override {
        // Standalone goto rep (not inside implication antecedent)
        iterateChildren(nodep);
        File_সারি* ধ্রুবক flp = nodep->fileline();
        Ast_গ্রন্থি_রাশিমালা* countp = nodep->countp()->unlinkFrBack();
        যদি (!validate_Rep_গুনতি(nodep, countp)) ফেরত;
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক exprp = nodep->exprp()->unlinkFrBack();
        AstPExpr* ধ্রুবক pexprp = createRepPExpr(flp, exprp, countp, nullptr, সত্য, মিথ্যা);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }

    শূন্য visit(AstSNonConsRep* nodep) override {
        // Standalone nonconsecutive rep (not inside implication antecedent)
        iterateChildren(nodep);
        যদি (rejectNonconsecRange(nodep, nodep->maxCountp())) ফেরত;
        File_সারি* ধ্রুবক flp = nodep->fileline();
        Ast_গ্রন্থি_রাশিমালা* countp = nodep->countp()->unlinkFrBack();
        যদি (!validate_Rep_গুনতি(nodep, countp)) ফেরত;
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক exprp = nodep->exprp()->unlinkFrBack();
        AstPExpr* ধ্রুবক pexprp = createRepPExpr(flp, exprp, countp, nullptr, সত্য, সত্য);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }

    শূন্য visit(AstFuncRef* nodep) override {
        // IEEE 1800-2023 16.8: Inline sequence instances wherever they appear
        // in the expression tree (inside implications, boolean ops, nested refs, etc.)
        যদি (AstSequence* ধ্রুবক seqp = VN_CAST(nodep->taskp(), Sequence)) {
            substituteSequenceCall(nodep, seqp);
            // The FuncRef has been replaced; do not access nodep after this point.
            // The replacement node will be visited by the parent's iterateChildren.
            ফেরত;
        }
        যদি (ধ্রুবক AstProperty* ধ্রুবক propp = VN_CAST(nodep->taskp(), Property)) {
            substituteNestedPropertyCall(nodep, propp);
            ফেরত;
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstImplication* nodep) override {
        যদি (nodep->sentreep()) ফেরত;  // Already processed

        // Top-level followed-by is claimed by V3AssertNfa; anything reaching here
        // is nested inside iff/implies/or. IEEE 1800-2023 16.12.9 permits the
        // composition, but silent lowering would drop non-vacuous-fail semantics.
        যদি (nodep->isFollowedBy()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: followed-by (#-# / #=#) nested inside property operator"
                          " (iff/implies/or) (IEEE 1800-2023 16.12.9)");
            nodep->replaceWith(নতুন AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            ফেরত;
        }

        // Handle goto repetition as antecedent before iterateChildren,
        // so the standalone AstSGotoRep visitor doesn't process it
        যদি (AstSGotoRep* ধ্রুবক gotop = VN_CAST(nodep->lhsp(), SGotoRep)) {
            iterateChildren(gotop);
            iterate_And_পরবর্তী_Null(nodep->rhsp());
            File_সারি* ধ্রুবক flp = nodep->fileline();
            Ast_গ্রন্থি_রাশিমালা* countp = gotop->countp()->unlinkFrBack();
            যদি (!validate_Rep_গুনতি(nodep, countp)) ফেরত;
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক exprp = gotop->exprp()->unlinkFrBack();
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rhsp = nodep->rhsp()->unlinkFrBack();
            AstPExpr* ধ্রুবক pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), মিথ্যা);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            ফেরত;
        }

        // Handle nonconsecutive repetition as antecedent before iterateChildren,
        // so the standalone AstSNonConsRep visitor doesn't process it
        যদি (AstSNonConsRep* ধ্রুবক ncrp = VN_CAST(nodep->lhsp(), SNonConsRep)) {
            iterateChildren(ncrp);
            iterate_And_পরবর্তী_Null(nodep->rhsp());
            যদি (rejectNonconsecRange(nodep, ncrp->maxCountp())) ফেরত;
            File_সারি* ধ্রুবক flp = nodep->fileline();
            Ast_গ্রন্থি_রাশিমালা* countp = ncrp->countp()->unlinkFrBack();
            যদি (!validate_Rep_গুনতি(nodep, countp)) ফেরত;
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক exprp = ncrp->exprp()->unlinkFrBack();
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rhsp = nodep->rhsp()->unlinkFrBack();
            AstPExpr* ধ্রুবক pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), সত্য);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            ফেরত;
        }

        iterateChildren(nodep);

        File_সারি* ধ্রুবক flp = nodep->fileline();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_গ্রন্থি_রাশিমালা* lhsp = nodep->lhsp()->unlinkFrBack();

        // Lower sequence match items (IEEE 16.11): (expr, var = val, ...) |-> / |=>
        যদি (Ast_রাশিমালা_Stmt* ধ্রুবক রাশিমালা_Stmtp = VN_CAST(lhsp, রাশিমালা_Stmt)) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক antExprp = রাশিমালা_Stmtp->resultp()->unlinkFrBack();

            যদি (nodep->isOverlapped()) {
                // |-> : assign to __Vpropvar via always_comb (continuous).
                // The assign evaluates RHS once; V3Sampled snapshots the
                // result so all consequent refs read the same value.
                Ast_গ্রন্থি* matchAssignsp = nullptr;
                জন্য (Ast_গ্রন্থি* stmtp = রাশিমালা_Stmtp->stmtsp(); stmtp;) {
                    Ast_গ্রন্থি* ধ্রুবক nextp = stmtp->nextp();
                    যদি (AstAssign* ধ্রুবক assignp = VN_CAST(stmtp, Assign)) {
                        assignp->unlinkFrBack();
                        matchAssignsp = Ast_গ্রন্থি::add_পরবর্তী_Null(matchAssignsp, assignp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(ঠেলো_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                যদি (matchAssignsp) {
                    AstAlways* ধ্রুবক alwaysp
                        = নতুন AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, matchAssignsp};
                    m_modp->addStmtsp(alwaysp);
                }
            } অন্যথা {
                // |=> : assign to __Vpropvar via NBA in a clocked always block.
                // The NBA commits before the next cycle's sampled snapshot,
                // so the consequent (which already references __Vpropvar)
                // sees the captured value.
                Ast_গ্রন্থি* matchAssignsp = nullptr;
                জন্য (Ast_গ্রন্থি* stmtp = রাশিমালা_Stmtp->stmtsp(); stmtp;) {
                    Ast_গ্রন্থি* ধ্রুবক nextp = stmtp->nextp();
                    যদি (AstAssign* ধ্রুবক assignp = VN_CAST(stmtp, Assign)) {
                        assignp->unlinkFrBack();
                        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক assignLhsp = assignp->lhsp()->unlinkFrBack();
                        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক assignRhsp = assignp->rhsp()->unlinkFrBack();
                        AstAssignDly* ধ্রুবক dlyp = নতুন AstAssignDly{flp, assignLhsp, assignRhsp};
                        VL_DO_DANGLING(ঠেলো_Deletep(assignp), assignp);
                        matchAssignsp = Ast_গ্রন্থি::add_পরবর্তী_Null(matchAssignsp, dlyp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(ঠেলো_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                যদি (matchAssignsp) {
                    AstIf* ধ্রুবক condp
                        = নতুন AstIf{flp, antExprp->clone_বৃক্ষ_Pure(মিথ্যা), matchAssignsp};
                    AstAlways* ধ্রুবক alwaysp
                        = নতুন AstAlways{flp, VAlwaysKwd::ALWAYS, new_Sen_বৃক্ষ(nodep), condp};
                    m_modp->addStmtsp(alwaysp);
                }
            }
        }

        যদি (AstPExpr* ধ্রুবক pexprp = VN_CAST(rhsp, PExpr)) {
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
                        = নতুন AstAnd{flp, নতুন AstNot{flp, m_disablep->clone_বৃক্ষ_Pure(মিথ্যা)}, lhsp};
                }
                AstPast* ধ্রুবক pastp
                    = নতুন AstPast{flp, lhsp, nullptr, nullptr, /* propertyTiming */ সত্য};
                pastp->dtypeFrom(lhsp);
                pastp->sentreep(new_Sen_বৃক্ষ(nodep));
                condp = pastp;
            }
            // Wrap existing PExpr body: if (antecedent) { <original body> } else { /* vacuous pass
            // */ }
            // Only the statements are guarded; declarations stay directly in the block. After
            // V3Fork moves the block's statements into a task, V3Task only handles variables
            // that are direct statements of the task.
            AstBegin* ধ্রুবক bodyp = pexprp->bodyp();
            AstIf* ধ্রুবক guardp = নতুন AstIf{flp, condp};
            জন্য (Ast_গ্রন্থি* stmtp = bodyp->stmtsp(); stmtp;) {
                Ast_গ্রন্থি* ধ্রুবক nextp = stmtp->nextp();
                যদি (!VN_IS(stmtp, Var)) guardp->addThensp(stmtp->unlinkFrBack());
                stmtp = nextp;
            }
            bodyp->addStmtsp(guardp);
            nodep->replaceWith(pexprp);
            // Don't iterate pexprp here -- it was already iterated when created
            // (in visit(AstSExpr*)), so delays and disable iff are already processed.
        } অন্যথা যদি (nodep->isOverlapped()) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক exprp
                = m_underCover ? static_cast<Ast_গ্রন্থি_রাশিমালা*>(নতুন AstLogAnd{flp, lhsp, rhsp})
                               : নতুন AstLogOr{flp, নতুন AstLogNot{flp, lhsp}, rhsp};
            nodep->replaceWith(exprp);
        } অন্যথা {
            যদি (m_disablep) {
                lhsp = নতুন AstAnd{flp, নতুন AstNot{flp, m_disablep->clone_বৃক্ষ_Pure(মিথ্যা)}, lhsp};
            }

            AstPast* ধ্রুবক pastp
                = নতুন AstPast{flp, lhsp, nullptr, nullptr, /* propertyTiming */ সত্য};
            pastp->dtypeFrom(lhsp);
            pastp->sentreep(new_Sen_বৃক্ষ(nodep));
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক exprp
                = m_underCover ? static_cast<Ast_গ্রন্থি_রাশিমালা*>(নতুন AstAnd{flp, pastp, rhsp})
                               : নতুন AstOr{flp, নতুন AstNot{flp, pastp}, rhsp};
            exprp->dtypeSetBit();
            nodep->replaceWith(exprp);
        }
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstUntil* nodep) override {
        File_সারি* ধ্রুবক flp = nodep->fileline();
        UASSERT_OBJ(
            !m_pexprp, nodep,
            "'" << nodep->verilogKwd()
                << "' in complex property expression should have been rejected by V3AssertNfa");
        যদি (nodep->isStrong()
            && (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue())) {
            nodep->v3warn(E_NOTIMING, nodep->verilogKwd() << " requires --timing");
            nodep->replaceWith(নতুন AstConst{flp, AstConst::BitFalse{}});
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            ফেরত;
        }
        যদি (nodep->isStrong()) {
            // p s_until q / p s_until_with q: q must eventually be true. Until then, p must
            // be true on every sampled tick. For s_until, check q first: when q is true on
            // this tick, p is not required. For s_until_with, p must be true on the q tick too.
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rawLhsp = nodep->lhsp()->unlinkFrBack();
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rawRhsp = nodep->rhsp()->unlinkFrBack();
            AstSampled* ধ্রুবক lhsp = নতুন AstSampled{flp, rawLhsp, rawLhsp->dtypep(), সত্য};
            AstSampled* ধ্রুবক rhsp = নতুন AstSampled{flp, rawRhsp, rawRhsp->dtypep(), সত্য};
            Ast_গ্রন্থি_রাশিমালা* finalCondp = rhsp->clone_বৃক্ষ_Pure(মিথ্যা);
            যদি (nodep->isOverlapping()) {
                finalCondp = নতুন AstLogAnd{flp, lhsp->clone_বৃক্ষ_Pure(মিথ্যা), finalCondp};
            }

            // Track active assertion attempts. Only the count is needed to emit final failures.
            AstVar* ধ্রুবক activep = নতুন AstVar{flp, VVarType::MODULETEMP, m_activeNames.get(""),
                                               nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            activep->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(activep);

            AstVar* ধ্রুবক donep
                = নতুন AstVar{flp, VVarType::BLOCKTEMP, "__VassertDone", nodep->findBitDType()};
            donep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            auto setDone = [&]() {
                ফেরত নতুন AstAssign{flp, নতুন AstVarRef{flp, donep, VAccess::WRITE},
                                     নতুন AstConst{flp, AstConst::BitTrue{}}};
            };

            AstLoop* ধ্রুবক loopp = নতুন AstLoop{flp};
            AstAssign* ধ্রুবক decrementVar = নতুন AstAssign{
                flp, নতুন AstVarRef{flp, activep, VAccess::WRITE},
                নতুন AstSub{flp, নতুন AstVarRef{flp, activep, VAccess::READ}, নতুন AstConst{flp, 1}}};
            loopp->addStmtsp(নতুন AstLoopTest{
                flp, loopp, নতুন AstLogNot{flp, নতুন AstVarRef{flp, donep, VAccess::READ}}});
            {
                Ast_গ্রন্থি* ধ্রুবক passp = নতুন AstPExprClause{flp};
                passp->add_পরবর্তী(decrementVar);
                passp->add_পরবর্তী(setDone());
                Ast_গ্রন্থি_রাশিমালা* passCondp = rhsp;
                যদি (nodep->isOverlapping()) {
                    passCondp = নতুন AstLogAnd{flp, lhsp->clone_বৃক্ষ_Pure(মিথ্যা), passCondp};
                }
                loopp->addStmtsp(নতুন AstIf{flp, passCondp, passp});
            }
            {
                Ast_গ্রন্থি* ধ্রুবক failp = নতুন AstPExprClause{flp, মিথ্যা};
                failp->add_পরবর্তী(decrementVar->clone_বৃক্ষ(মিথ্যা));
                failp->add_পরবর্তী(setDone());
                loopp->addStmtsp(নতুন AstIf{
                    flp,
                    নতুন AstLogAnd{flp,
                                  নতুন AstLogNot{flp, নতুন AstVarRef{flp, donep, VAccess::READ}},
                                  নতুন AstLogNot{flp, lhsp}},
                    failp});
            }
            AstDelay* ধ্রুবক delayp = নতুন AstDelay{flp, নতুন AstConst{flp, 1}, মিথ্যা};
            delayp->timeunit(m_modp->timeunit());
            loopp->addStmtsp(নতুন AstIf{
                flp, নতুন AstLogNot{flp, নতুন AstVarRef{flp, donep, VAccess::READ}}, delayp});

            // Main assertion block
            AstBegin* ধ্রুবক bodyp = নতুন AstBegin{flp, m_blockNames.get(""), nullptr, সত্য};
            bodyp->addStmtsp(donep);
            bodyp->addStmtsp(নতুন AstAssign{flp, নতুন AstVarRef{flp, donep, VAccess::WRITE},
                                           নতুন AstConst{flp, AstConst::BitFalse{}}});
            File_সারি* ধ্রুবক flp = activep->fileline();
            bodyp->addStmtsp(
                নতুন AstAssign{flp, নতুন AstVarRef{flp, activep, VAccess::WRITE},
                              নতুন AstAdd{flp, নতুন AstVarRef{flp, activep, VAccess::READ},
                                         নতুন AstConst{flp, 1}}});
            bodyp->addStmtsp(loopp);

            // Validate assertion condition for each active assert
            AstVar* ধ্রুবক activeCountp = নতুন AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                    nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            activeCountp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

            AstAssign* ধ্রুবক initActiveCountp
                = নতুন AstAssign{flp, নতুন AstVarRef{flp, activeCountp, VAccess::WRITE},
                                নতুন AstVarRef{flp, activep, VAccess::READ}};
            AstLoop* ধ্রুবক finalLoopp = নতুন AstLoop{flp};
            finalLoopp->addStmtsp(
                নতুন AstLoopTest{flp, finalLoopp,
                                নতুন AstNeq{flp, নতুন AstVarRef{flp, activeCountp, VAccess::READ},
                                           নতুন AstConst{flp, 0}}});
            finalLoopp->addStmtsp(নতুন AstIf{flp, finalCondp, নতুন AstPExprClause{flp},
                                            নতুন AstPExprClause{flp, মিথ্যা}});
            finalLoopp->addStmtsp(
                নতুন AstAssign{flp, নতুন AstVarRef{flp, activeCountp, VAccess::WRITE},
                              নতুন AstSub{flp, নতুন AstVarRef{flp, activeCountp, VAccess::READ},
                                         নতুন AstConst{flp, 1}}});

            // Final assertion block
            AstBegin* ধ্রুবক finalp = নতুন AstBegin{flp, m_blockNames.get(""), nullptr, সত্য};
            finalp->addStmtsp(activeCountp);
            finalp->addStmtsp(initActiveCountp);
            finalp->addStmtsp(finalLoopp);

            AstPExpr* ধ্রুবক pexprp = নতুন AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
            VL_RESTORER(m_pexprp);
            VL_RESTORER(m_hasCycleDelay);
            m_pexprp = pexprp;
            m_hasCycleDelay = মিথ্যা;
            iterate(bodyp);
            iterate(finalp);

            UASSERT_OBJ(!m_hasCycleDelay, nodep,
                        "s_until cycle delay should have been handled by V3AssertNfa");

            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            ফেরত;
        }
        AstLoop* ধ্রুবক loopp = নতুন AstLoop{flp};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lhsp = nodep->lhsp()->unlinkFrBack();
        AstLogAnd* ধ্রুবক loopCondp = নতুন AstLogAnd{flp, lhsp, নতুন AstLogNot{flp, rhsp}};
        loopp->addStmtsp(নতুন AstLoopTest{flp, loopp, loopCondp});
        loopp->addStmtsp(নতুন AstEventControl{flp, new_Sen_বৃক্ষ(nodep), nullptr});

        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rhsCopyp = rhsp->clone_বৃক্ষ_Pure(মিথ্যা);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক passCondp
            = nodep->isOverlapping() ? নতুন AstLogAnd{flp, lhsp->clone_বৃক্ষ_Pure(মিথ্যা), rhsCopyp}
                                     : rhsCopyp;
        AstBegin* ধ্রুবক beginp = নতুন AstBegin{flp, m_blockNames.get(""), loopp, সত্য};
        beginp->addStmtsp(
            নতুন AstIf{flp, passCondp, নতুন AstPExprClause{flp}, নতুন AstPExprClause{flp, মিথ্যা}});

        AstPExpr* ধ্রুবক pexprp = নতুন AstPExpr{flp, beginp, nodep->dtypep()};
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
            newp = নতুন AstConst{nodep->fileline(), AstConst::BitFalse{}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }

    শূন্য visit(AstPropSpec* nodep) override {
        nodep = substitutePropertyCall(nodep);
        iterate_And_পরবর্তী_Null(nodep->sensesp());
        যদি (m_senip && m_senip != nodep->sensesp())
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Only one PSL clock allowed per assertion");
        ধ্রুবক AstCover* ধ্রুবক coverp = VN_CAST(nodep->backp(), Cover);
        ধ্রুবক যৌক্তিক seqEvent = coverp && coverp->isSeqEvent();
        যদি (!nodep->disablep() && m_defaultDisablep && !seqEvent) {
            nodep->disablep(m_defaultDisablep->condp()->clone_বৃক্ষ_Pure(সত্য));
        }
        m_disablep = nodep->disablep();
        // Unlink and just keep a pointer to it, convert to sentree as needed
        m_senip = nodep->sensesp();
        iterateNull(nodep->disablep());
        যদি (!VN_AS(nodep->backp(), গ্রন্থি_Cover_Or_Assert)->immediate()) {
            ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক propDtp = nodep->propp()->dtypep();
            nodep->propp(নতুন AstSampled{nodep->fileline(), nodep->propp()->unlinkFrBack(),
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
            nodep->replaceWith(নতুন AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            ফেরত;
        }  // LCOV_EXCL_STOP
        যদি (AstLogNot* ধ্রুবক notp = VN_CAST(nodep->backp(), LogNot)) {
            notp->replaceWith(nodep->unlinkFrBack());
            VL_DO_DANGLING(ঠেলো_Deletep(notp), notp);
            iterate(nodep);
            ফেরত;
        }
        // Sequence expression as antecedent of implication is not yet supported
        যদি (AstImplication* ধ্রুবক implp = VN_CAST(nodep->backp(), Implication)) {
            যদি (implp->lhsp() == nodep) {  // LCOV_EXCL_START
                implp->v3warn(E_UNSUPPORTED,
                              "Unsupported: Implication with sequence expression as antecedent");
                nodep->replaceWith(নতুন AstConst{nodep->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                ফেরত;
            }  // LCOV_EXCL_STOP
        }
        VL_RESTORER(m_pexprp);
        VL_RESTORER(m_disableSeqIfp);
        m_pexprp = nodep;

        যদি (m_disablep) {
            ধ্রুবক AstSampled* sampledp = nullptr;
            যদি (m_disablep->exists([&sampledp](ধ্রুবক AstSampled* ধ্রুবক sp) {
                    sampledp = sp;
                    ফেরত সত্য;
                })) {
                sampledp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: $sampled inside disabled condition of a sequence");
                m_disablep = নতুন AstConst{m_disablep->fileline(), AstConst::BitFalse{}};
                // always a copy is used, so remove it now
                ঠেলো_Deletep(m_disablep);  // m_disablep used later
            }
            File_সারি* ধ্রুবক flp = nodep->fileline();
            // Add counter which counts times the condition turned true
            AstVar* ধ্রুবক disableCntp
                = নতুন AstVar{flp, VVarType::MODULETEMP, m_disableCntNames.get(""),
                             nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            disableCntp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(disableCntp);
            AstVarRef* ধ্রুবক readCntRefp = নতুন AstVarRef{flp, disableCntp, VAccess::READ};
            AstVarRef* ধ্রুবক writeCntRefp = নতুন AstVarRef{flp, disableCntp, VAccess::WRITE};
            AstAssign* ধ্রুবক incrStmtp = নতুন AstAssign{
                flp, writeCntRefp, নতুন AstAdd{flp, readCntRefp, নতুন AstConst{flp, 1}}};
            AstAlways* ধ্রুবক alwaysp
                = নতুন AstAlways{flp, VAlwaysKwd::ALWAYS,
                                নতুন Ast_Sen_বৃক্ষ{flp, নতুন AstSenItem{flp, VEdgeType::ET_POSEDGE,
                                                                   m_disablep->clone_বৃক্ষ(মিথ্যা)}},
                                incrStmtp};
            disableCntp->add_পরবর্তী_Here(alwaysp);

            // Store value of that counter at the beginning of sequence evaluation
            AstBegin* ধ্রুবক bodyp = nodep->bodyp();
            AstVar* ধ্রুবক initialCntp = নতুন AstVar{flp, VVarType::BLOCKTEMP, "__VinitialCnt",
                                                   nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            initialCntp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            AstAssign* ধ্রুবক assignp
                = নতুন AstAssign{flp, নতুন AstVarRef{flp, initialCntp, VAccess::WRITE},
                                readCntRefp->clone_বৃক্ষ(মিথ্যা)};
            // Prepend to the sequence body to keep statement list structure valid.
            UASSERT_OBJ(!bodyp->name().empty(), bodyp, "Sequence body should be named");
            Ast_গ্রন্থি* ধ্রুবক newStmtsp = Ast_গ্রন্থি::add_পরবর্তী<Ast_গ্রন্থি, Ast_গ্রন্থি>(initialCntp, assignp);
            bodyp->prependStmtsp(newStmtsp);

            m_disableSeqIfp
                = নতুন AstIf{flp, নতুন AstEq{flp, নতুন AstVarRef{flp, initialCntp, VAccess::READ},
                                           readCntRefp->clone_বৃক্ষ(মিথ্যা)}};
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

সর্বজনীন:
    // CONSTRUCTORS
    explicit AssertPreVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        { AssertPreClockingVisitor{nodep}; }
        // Process
        iterate(nodep);
        // Fix up varref names
        জন্য (AstVarXRef* xrefp : m_xrefsp) xrefp->name(xrefp->varp()->name());
        // Clean up sequence declarations after inlining.
        // Referenced sequences that were inlined have isReferenced cleared.
        // Remaining referenced sequences are in unsupported contexts (e.g. @seq event).
        জন্য (AstSequence* seqp : m_seqsToCleanup) {
            যদি (seqp->isReferenced()) {
                seqp->v3warn(E_UNSUPPORTED,
                             "Unsupported: sequence referenced outside assertion property");
            } অন্যথা {
                VL_DO_DANGLING(seqp->unlinkFrBack()->delete_বৃক্ষ(), seqp);
            }
        }
    }
    ~AssertPreVisitor() override = পূর্বনির্ধারিত;
};

//######################################################################
// Top Assert class

শূন্য V3AssertPre::assertPreAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { AssertPreVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("assertpre", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
