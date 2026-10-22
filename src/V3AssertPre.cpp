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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3AssertPre.h"

#ଅନ୍ତର୍ଭାଵ "V3Assert.h"
#ଅନ୍ତର୍ଭାଵ "V3Const.h"
#ଅନ୍ତର୍ଭାଵ "V3Task.h"
#ଅନ୍ତର୍ଭାଵ "V3Timing.h"
#ଅନ୍ତର୍ଭାଵ "V3UniqueNames.h"

#ଅନ୍ତର୍ଭାଵ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clocking blocks of clocking items

ଶ୍ରେଣୀ AssertPreClockingVisitor final : ସର୍ବସାଧାରଣ VNVisitorConst {
    // Captures the clocking block of each clocking item before AssertPreVisitor, which needs it
    // at synchronous drives, often visited before the clocking block, e.g. in a parent module
    // NODE STATE
    // AstClockingItem::user3p()  // AstClocking*. Clocking block of the item, user3 claimed by
    //                            //               AssertPreVisitor

    // VISITORS
    ଶୂନ୍ୟ visit(AstNetlist* nodep) override { iterateAndNextConstNull(nodep->modulesp()); }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override { iterateAndNextConstNull(nodep->stmtsp()); }
    ଶୂନ୍ୟ visit(AstGenBlock* nodep) override { iterateAndNextConstNull(nodep->itemsp()); }
    ଶୂନ୍ୟ visit(AstClocking* nodep) override {
        ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            ଯଦି (AstClockingItem* ଅଚଳ citemp = VN_CAST(itemp, ClockingItem)) {
                citemp->user3p(nodep);
            }
        }
    }
    // Clocking blocks are only in modules and their generate blocks
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି*) override {}

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit AssertPreClockingVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~AssertPreClockingVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Assert class functions

ଶ୍ରେଣୀ AssertPreVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // Removes clocks and other pre-optimizations
    // Eventually inlines calls to sequences, properties, etc.
    // We're not parsing the tree, or anything more complicated.
ବ୍ୟକ୍ତିଗତ:
    // TYPES
    ସଂରଚନା SynchDrive final {
        AstClockingItem* itemp;  // Clocking item of the driven clockvar
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* refp;  // Reference to the driven clockvar
    };

    // NODE STATE
    // AstClockingItem::user1p()         // AstVar*.      varp() of ClockingItem after unlink
    // AstClockingItem::user2p()         // AstVar*.      Flag set by drives of output clockvar
    // AstClockingItem::user3p()         // AstClocking*. Clocking block of the item, captured
    //                                   //               by AssertPreClockingVisitor
    // AstPExpr::user1()                 // bool.         Created from AstUntil
    ଅଚଳ VNUser1InUse m_inuser1;
    ଅଚଳ VNUser2InUse m_inuser2;
    ଅଚଳ VNUser3InUse m_inuser3;
    // STATE
    // Current context:
    AstNetlist* ଅଚଳ m_netlistp = nullptr;  // Current netlist
    Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Current module
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
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_disablep = nullptr;  // Last disable
    AstIf* m_disableSeqIfp = nullptr;  // Used for handling disable iff in sequences
    AstPExpr* m_pexprp = nullptr;  // Last AstPExpr
    ତାର୍କିକ m_underCover = ମିଥ୍ୟା;  // True if the enclosing assertion is a cover
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
    ତାର୍କିକ m_inAssign = ମିଥ୍ୟା;  // True if in an AssignNode
    ତାର୍କିକ m_inAssignDlyLhs = ମିଥ୍ୟା;  // True if in AssignDly's LHS
    ତାର୍କିକ m_inSynchDrive = ମିଥ୍ୟା;  // True if in synchronous drive
    ତାର୍କିକ m_inConcatAssign = ମିଥ୍ୟା;  // True if in assignment to a concatenation
    std::vector<SynchDrive> m_drives;  // Clockvars written by the synchronous drive
    ତାର୍କିକ m_hasCycleDelay = ମିଥ୍ୟା;  // True if node has cycle delay beneath
    std::vector<AstVarXRef*> m_xrefsp;  // list of xrefs that need name fixup
    std::vector<AstSequence*> m_seqsToCleanup;  // Sequences to clean up after traversal

    // METHODS

    // Flag set by each drive of an output clockvar, added to the module with its clocking item
    AstVar* getCreateDrivenVarp(AstClockingItem* itemp) {
        ଯଦି (!itemp->user2p()) {
            AstVar* ଅଚଳ varp = ନୂତନ AstVar{itemp->fileline(), VVarType::MODULETEMP,
                                            m_drivenNames.get(itemp), itemp->findBitDType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            itemp->user2p(varp);
        }
        ଫେରନ୍ତୁ VN_AS(itemp->user2p(), Var);
    }
    // Reference to a variable in the scope of a driven clockvar, made like the clockvar's
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newDriveRefp(File_ଧାଡ଼ି* flp, ଅଚଳ SynchDrive& drive, AstVar* varp,
                                     ଅଚଳ VAccess& access) {
        ଯଦି (ଅଚଳ AstVarXRef* ଅଚଳ xrefp = VN_CAST(drive.refp, VarXRef)) {
            ଫେରନ୍ତୁ ନୂତନ AstVarXRef{flp, varp, xrefp->dotted(), access};
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstMemberSel* ଅଚଳ selp = VN_CAST(drive.refp, MemberSel)) {
            // The interface expression is evaluated again, so it must not have side effects;
            // cloneTreePure warns if it has any
            AstMemberSel* ଅଚଳ newSelp
                = ନୂତନ AstMemberSel{flp, selp->fromp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), varp};
            newSelp->access(access);
            ଫେରନ୍ତୁ newSelp;
        }
        ଫେରନ୍ତୁ ନୂତନ AstVarRef{flp, varp, access};
    }
    // Assignment setting the drive flag of a driven clockvar
    AstAssign* newDrivenSetp(File_ଧାଡ଼ି* flp, ଅଚଳ SynchDrive& drive) {
        AstVar* ଅଚଳ varp = getCreateDrivenVarp(drive.itemp);
        AstAssign* ଅଚଳ setp = ନୂତନ AstAssign{flp, newDriveRefp(flp, drive, varp, VAccess::WRITE),
                                              ନୂତନ AstConst{flp, AstConst::BitTrue{}}};
        setp->user1(ସତ୍ୟ);
        ଫେରନ୍ତୁ setp;
    }
    // Clocking block of a clocking item
    ସ୍ଥିର AstClocking* clockingOf(ଅଚଳ AstClockingItem* itemp) {
        UASSERT_OBJ(itemp->user3p(), itemp, "Clocking item not captured in a clocking block");
        ଫେରନ୍ତୁ VN_AS(itemp->user3p(), Clocking);
    }
    // Clocking event of a driven clockvar, referenced from the drive like the clockvar, or
    // nullptr if unsupported, which is reported on the drive's cycle delay
    Ast_Sen_ବୃକ୍ଷ* newDriveSentreep(AstDelay* delayp, ଅଚଳ SynchDrive& drive) {
        Ast_Sen_ବୃକ୍ଷ* ଅଚଳ origp = clockingOf(drive.itemp)->sentreep();
        ଯଦି (VN_IS(drive.refp, VarRef)) ଫେରନ୍ତୁ origp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
        ଯଦି (origp->exists([](ଅଚଳ AstVarXRef*) { ଫେରନ୍ତୁ ସତ୍ୟ; })) {
            // The reference is relative to the clocking block
            delayp->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in synchronous drive to"
                                          " clockvar in another scope, whose clocking event"
                                          " has a hierarchical reference");
            ଫେରନ୍ତୁ nullptr;
        }
        ଅଚଳ AstMemberSel* ଅଚଳ selp = VN_CAST(drive.refp, MemberSel);
        // Processes outside classes evaluate their events also when not waiting on them,
        // possibly before the interface reference is set, so the event is guarded against a
        // null reference, which needs an integral value; scheduling does not guard it
        ଅଚଳ ତାର୍କିକ guardNull = selp && !VN_IS(m_modp, Class);
        ଯଦି (guardNull) {
            ପାଇଁ (AstSenItem* itemp = origp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                ଯଦି (!itemp->sensp()->dtypep()->skipRefp()->isIntegralOrPacked()) {
                    delayp->v3warn(E_UNSUPPORTED,
                                   "Unsupported: cycle delay in synchronous drive through"
                                   " an interface reference outside a class, to clocking"
                                   " block with non-integral clocking event");
                    ଫେରନ୍ତୁ nullptr;
                }
            }
        }
        // Interface declaring the clocking block, if driven through an interface reference
        AstIface* ଅଚଳ ifacep
            = selp ? VN_AS(selp->fromp()->dtypep()->skipRefp(), IfaceRefDType)->ifaceViaCellp()
                   : nullptr;
        Ast_Sen_ବୃକ୍ଷ* ଅଚଳ sentreep = origp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
        std::vector<AstVarRef*> refps;
        sentreep->foreach([&](AstVarRef* refp) {
            ଯଦି (!refp->classOrPackagep()) refps.ଠେଲ_back(refp);
        });
        ପାଇଁ (AstVarRef* ଅଚଳ refp : refps) {
            AstVar* ଅଚଳ varp = refp->varp();
            ଯଦି (ifacep) {
                // Mark the variable as read through an interface reference, as V3Width does for
                // a member select. The mark is the interface declaring the variable, whose
                // instances all share it, so drives through references to any instances agree.
                UASSERT_OBJ(!varp->sensIfacep() || varp->sensIfacep() == ifacep, refp,
                            "Variable read through references to different interfaces");
                varp->sensIfacep(ifacep);
            }
            refp->replaceWith(newDriveRefp(refp->fileline(), drive, varp, refp->access()));
            VL_DO_DANGLING(ଠେଲ_Deletep(refp), refp);
        }
        ଯଦି (guardNull) {
            ପାଇଁ (AstSenItem* itemp = sentreep->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                guardNullIface(itemp, selp->fromp());
            }
        }
        ଫେରନ୍ତୁ sentreep;
    }
    // Make an integral clocking event through an interface reference false while the reference
    // is null
    ସ୍ଥିର ଶୂନ୍ୟ guardNullIface(AstSenItem* sensesp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp) {
        File_ଧାଡ଼ି* ଅଚଳ flp = sensesp->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ sensp = sensesp->sensp()->unlinkFrBack();
        sensesp->sensp(ନୂତନ AstCond{flp, newNotNullp(flp, fromp), sensp,
                                   ନୂତନ AstConst{flp, AstConst::DTyped{}, sensp->dtypep()}});
        ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp = sensesp->condp()) {
            condp->unlinkFrBack();
            sensesp->condp(ନୂତନ AstLogAnd{flp, newNotNullp(flp, fromp), condp});
        }
    }
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newNotNullp(File_ଧାଡ଼ି* flp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp) {
        ଫେରନ୍ତୁ ନୂତନ AstNeq{flp, fromp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), ନୂତନ AstConst{flp, AstConst::Null{}}};
    }
    // Procedural '##0': if the default clocking event has not yet occurred in this time step,
    // wait for it; otherwise continue without suspension (IEEE 1800-2023 14.11)
    AstIf* newCycleDelayZerop(File_ଧାଡ଼ି* flp) ଅଚଳ {
        AstVar* ଅଚଳ evtVarp = m_defaultClkEvtVarp;
        UASSERT_OBJ(evtVarp, flp, "Default clocking event var not pre-created");
        AstCMethodHard* ଅଚଳ isTriggeredp = ନୂତନ AstCMethodHard{
            flp, ନୂତନ AstVarRef{flp, evtVarp, VAccess::READ}, VCMethod::EVENT_IS_TRIGGERED};
        isTriggeredp->dtypeSetBit();
        AstEventControl* ଅଚଳ waitp = ନୂତନ AstEventControl{
            flp,
            ନୂତନ Ast_Sen_ବୃକ୍ଷ{flp, ନୂତନ AstSenItem{flp, VEdgeType::ET_EVENT,
                                               ନୂତନ AstVarRef{flp, evtVarp, VAccess::READ}}},
            nullptr};
        ଫେରନ୍ତୁ ନୂତନ AstIf{flp, ନୂତନ AstNot{flp, isTriggeredp}, waitp};
    }

    ସ୍ଥିର ଶୂନ୍ୟ checkSamplingFuncDType(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep, ଅଚଳ Ast_ଗଣ୍ଠି* exprp) {
        ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = exprp->dtypep()->skipRefp();
        ଯଦି (!dtypep->isIntegralOrPacked()) {
            nodep->v3error("Expected numeric type, but got a " << dtypep->prettyDTypeNameQ()
                                                               << " data type");
        }
    }

    Ast_Sen_ବୃକ୍ଷ* new_Sen_ବୃକ୍ଷ(Ast_ଗଣ୍ଠି* nodep, Ast_Sen_ବୃକ୍ଷ* useTreep = nullptr,
                           Ast_ଗଣ୍ଠି_Cover_Or_Assert* cassertp = nullptr) {
        // Create sentree based on clocked or default clock
        // Return nullptr for always
        ଯଦି (useTreep) ଫେରନ୍ତୁ useTreep;
        Ast_Sen_ବୃକ୍ଷ* newp = nullptr;
        AstSenItem* senip = m_senip;
        ତାର୍କିକ fromAlways = ମିଥ୍ୟା;
        ଯଦି (!senip && m_defaultClockingp) senip = m_defaultClockingp->sentreep()->sensesp();
        ଯଦି (!senip) {
            senip = m_seniAlwaysp;
            fromAlways = ସତ୍ୟ;
        }
        ଯଦି (!senip) {
            nodep->v3error("Concurrent assertion has no clock (IEEE 1800-2023 16.16)\n"
                           << nodep->warnMore()
                           << "... Suggest provide a clocking event, a default"
                              " clocking, or a clocked procedural context");
            newp = ନୂତନ Ast_Sen_ବୃକ୍ଷ{nodep->fileline(), nullptr};
        } ଅନ୍ୟଥା {
            ଯଦି (cassertp && fromAlways) cassertp->senFromAlways(ସତ୍ୟ);
            newp = ନୂତନ Ast_Sen_ବୃକ୍ଷ{nodep->fileline(), senip->clone_ବୃକ୍ଷ(ସତ୍ୟ)};
        }
        ଫେରନ୍ତୁ newp;
    }
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* getSequenceBodyExprp(ଅଚଳ AstSequence* ଅଚଳ seqp) ଅଚଳ {
        // The statements in AstSequence are optional AstVar (ports) followed by body expr.
        Ast_ଗଣ୍ଠି* bodyp = seqp->stmtsp();
        ଯେତେବେଳେ (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        ଫେରନ୍ତୁ VN_CAST(bodyp, ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
    }
    AstPropSpec* getPropertyExprp(ଅଚଳ AstProperty* ଅଚଳ propp) {
        // Statements in AstProperty: AstVar (ports/local vars),
        // AstInitialStaticStmt/AstInitialAutomaticStmt (var init), AstPropSpec (body).
        Ast_ଗଣ୍ଠି* propExprp = propp->stmtsp();
        ଯେତେବେଳେ (propExprp
               && (VN_IS(propExprp, Var) || VN_IS(propExprp, InitialStaticStmt)
                   || VN_IS(propExprp, InitialAutomaticStmt))) {
            propExprp = propExprp->nextp();
        }
        ଫେରନ୍ତୁ VN_CAST(propExprp, PropSpec);
    }
    ଶୂନ୍ୟ substituteSequenceCall(AstFuncRef* funcrefp, AstSequence* seqp) {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Inline the sequence body at the call site, replacing the FuncRef
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        // Clone the body expression since the sequence may be referenced multiple times
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* clonedp = bodyExprp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
        // Build substitution map, then do a single traversal to replace all formals
        // (textual substitution per IEEE 16.8.2).
        ଅଚଳ V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<ଅଚଳ AstVar*, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> portMap;
        ପାଇଁ (ଅଚଳ auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            ଅଚଳ auto it = portMap.find(refp->varp());
            ଯଦି (it != portMap.end()) {
                refp->replaceWith(it->second->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
                VL_DO_DANGLING(ଠେଲ_Deletep(refp), refp);
            }
        });
        ପାଇଁ (ଅଚଳ auto& tconnect : tconnects) {
            ଠେଲ_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        // Replace the FuncRef with the inlined body
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(ଠେଲ_Deletep(funcrefp), funcrefp);
        // Clear referenced flag; sequences with isReferenced==false are deleted in assertPreAll
        seqp->isReferenced(ମିଥ୍ୟା);
    }
    AstPropSpec* clonePropertyBody(AstFuncRef* funcrefp, ଅଚଳ AstProperty* propp) {
        AstPropSpec* propExprp = getPropertyExprp(propp);
        // Substitute inner property call before copying in order to not doing the same for
        // each call of outer property call.
        propExprp = substitutePropertyCall(propExprp);
        // Clone subtree after substitution. It is needed, because property might be called
        // multiple times with different arguments.
        propExprp = propExprp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
        // Build substitution maps for formal arguments and property-local
        // variables, then perform a single foreach to apply all replacements.
        // Map port vars to their actual argument expressions
        ଅଚଳ V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propp->stmtsp());
        std::unordered_map<ଅଚଳ AstVar*, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> portMap;
        ପାଇଁ (ଅଚଳ auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }

        // Promote property-local variables (non-port vars, IEEE 16.10) to
        // module-level __Vpropvar temps. Cross-cycle persistence is handled
        // by the match item lowering in visit(AstImplication*).
        std::unordered_map<ଅଚଳ AstVar*, AstVar*> localVarMap;
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = propp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(stmtp, Var)) {
                ଯଦି (!varp->isIO()) {
                    ଅଚଳ string newName = m_propVarNames.get(varp);
                    AstVar* ଅଚଳ newVarp = ନୂତନ AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->addStmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        // Single traversal: substitute ports and update local var references
        propExprp->foreach([&](AstVarRef* refp) {
            ଅଚଳ auto portIt = portMap.find(refp->varp());
            ଯଦି (portIt != portMap.end()) {
                refp->replaceWith(portIt->second->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
                VL_DO_DANGLING(ଠେଲ_Deletep(refp), refp);
                ଫେରନ୍ତୁ;
            }
            ଅଚଳ auto localIt = localVarMap.find(refp->varp());
            ଯଦି (localIt != localVarMap.end()) refp->varp(localIt->second);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        // Clean up argument expressions
        ପାଇଁ (ଅଚଳ auto& tconnect : tconnects) {
            ଠେଲ_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(propExprp, funcrefp);
        ଫେରନ୍ତୁ propExprp;
    }
    AstPropSpec* substitutePropertyCall(AstPropSpec* nodep) {
        ଯଦି (AstFuncRef* ଅଚଳ funcrefp = VN_CAST(nodep->propp(), FuncRef)) {
            ଯଦି (ଅଚଳ AstProperty* ଅଚଳ propp = VN_CAST(funcrefp->taskp(), Property)) {
                AstPropSpec* ଅଚଳ propExprp = clonePropertyBody(funcrefp, propp);

                // Handle case with 2 disable iff statement (IEEE 1800-2023 16.12.1)
                ଯଦି (nodep->disablep() && propExprp->disablep()) {
                    nodep->v3error("disable iff expression before property call and in its "
                                   "body is not legal");
                    ଠେଲ_Deletep(propExprp->disablep()->unlinkFrBack());
                }
                // If disable iff is in outer property, move it to inner
                ଯଦି (nodep->disablep()) {
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ disablep = nodep->disablep()->unlinkFrBack();
                    propExprp->disablep(disablep);
                }

                ଯଦି (nodep->sensesp() && propExprp->sensesp()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Clock event before property call and in its body");
                    ଠେଲ_Deletep(propExprp->sensesp()->unlinkFrBack());
                }
                // If clock event is in outer property, move it to inner
                ଯଦି (nodep->sensesp()) {
                    AstSenItem* ଅଚଳ sensesp = nodep->sensesp();
                    sensesp->unlinkFrBack();
                    propExprp->sensesp(sensesp);
                }

                // Now substitute property reference with property body
                nodep->replaceWith(propExprp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                ଫେରନ୍ତୁ propExprp;
            }
        }
        ଫେରନ୍ତୁ nodep;
    }

    ଶୂନ୍ୟ substituteNestedPropertyCall(AstFuncRef* funcrefp, ଅଚଳ AstProperty* propp) {
        AstPropSpec* ଅଚଳ specp = clonePropertyBody(funcrefp, propp);
        Ast_ଗଣ୍ଠି* replp = nullptr;
        ଯଦି (specp->disablep()) {
            funcrefp->v3error("Illegal 'disable iff' in a nested property instance"
                              " (IEEE 1800-2023 16.12.1)");
        } ଅନ୍ୟଥା ଯଦି (specp->sensesp()) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: clock event in a nested property instance");
        } ଅନ୍ୟଥା ଯଦି (specp->propp()->exists(
                       [](ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* np) { ଫେରନ୍ତୁ np->isMultiCycleSva(); })) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: multi-cycle body in a nested property instance");
        } ଅନ୍ୟଥା {
            replp = specp->propp()->unlinkFrBack();
        }
        ଯଦି (!replp) replp = ନୂତନ AstConst{funcrefp->fileline(), AstConst::BitFalse{}};
        funcrefp->replaceWith(replp);
        VL_DO_DANGLING(ଠେଲ_Deletep(funcrefp), funcrefp);
        VL_DO_DANGLING(ଠେଲ_Deletep(specp), specp);
    }

    // VISITORS
    //========== Statements
    ଶୂନ୍ୟ visit(AstClocking* ଅଚଳ nodep) override {
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        UINFO(8, "   CLOCKING" << nodep);
        ଯଦି (nodep->isDefault()) {
            ଯଦି (m_defaultClockingp) {
                nodep->v3error("Only one default clocking block allowed per module"
                               " (IEEE 1800-2023 14.12)");
            } ଅନ୍ୟଥା {
                m_defaultClockingp = nodep;
                m_defaultClkEvtVarp = nodep->ensureEventp();
            }
        }
        iterateChildren(nodep);
        ଯଦି (nodep->eventp()) nodep->addNextHere(nodep->eventp()->unlinkFrBack());
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ଶୂନ୍ୟ visit(AstModportClockingRef* ଅଚଳ nodep) override {
        // It has to be converted to a list of ModportClockingVarRefs,
        // because clocking blocks are removed in this pass
        ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = nodep->clockingp()->itemsp(); itemp; itemp = itemp->nextp()) {
            ଯଦି (AstClockingItem* ଅଚଳ citemp = VN_CAST(itemp, ClockingItem)) {
                ଯଦି (AstVar* ଅଚଳ varp
                    = citemp->varp() ? citemp->varp() : VN_AS(citemp->user1p(), Var)) {
                    AstModportVarRef* ଅଚଳ modVarp = ନୂତନ AstModportVarRef{
                        nodep->fileline(), varp->name(), citemp->direction()};
                    modVarp->varp(varp);
                    nodep->addNextHere(modVarp);
                    ଯଦି (citemp->direction().isOutput()) {
                        // Drives through the modport set the drive flag
                        AstVar* ଅଚଳ drivenVarp = getCreateDrivenVarp(citemp);
                        AstModportVarRef* ଅଚଳ drivenRefp = ନୂତନ AstModportVarRef{
                            nodep->fileline(), drivenVarp->name(), VDirection::OUTPUT};
                        drivenRefp->varp(drivenVarp);
                        nodep->addNextHere(drivenRefp);
                    }
                }
            }
        }
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ଶୂନ୍ୟ visit(AstClockingItem* ଅଚଳ nodep) override {
        // Get a ref to the sampled/driven variable
        AstVar* ଅଚଳ varp = nodep->varp();
        ଯଦି (!varp) {
            // Unused item
            ଫେରନ୍ତୁ;
        }
        // Flag set by drives of an output clockvar, possibly visited before this item
        ଯଦି (nodep->direction().isOutput()) m_modp->addStmtsp(getCreateDrivenVarp(nodep));
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        V3Const::constifyEdit(nodep->skewp());
        ଯଦି (!VN_IS(nodep->skewp(), Const)) {
            nodep->skewp()->v3error("Skew must be constant (IEEE 1800-2023 14.4)");
            ଫେରନ୍ତୁ;
        }
        AstConst* ଅଚଳ skewp = VN_AS(nodep->skewp(), Const);
        ଯଦି (skewp->ଅଙ୍କ().isNegative()) skewp->v3error("Skew cannot be negative");
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = nodep->exprp();
        varp->name(m_clockingp->name() + "__DOT__" + varp->name());
        m_clockingp->addNextHere(varp->unlinkFrBack());
        nodep->user1p(varp);
        varp->user1p(nodep);
        ଯଦି (nodep->direction() == VDirection::OUTPUT) {
            exprp->foreach([](ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* varrefp) {
                // Prevent confusing BLKANDNBLK warnings on clockvars due to generated assignments
                varrefp->fileline()->warnOff(V3ErrorCode::BLKANDNBLK, ସତ୍ୟ);
                // Mark the driven signal so later passes (e.g. V3Inst port
                // connection) know the drive flows out from inside the cell.
                // Item expressions are plain signal references, so only the
                // driven signal is marked, never index selects.
                varrefp->varp()->isClockingDriven(ସତ୍ୟ);
            });
            AstVarRef* ଅଚଳ skewedReadRefp = ନୂତନ AstVarRef{flp, varp, VAccess::READ};
            skewedReadRefp->user1(ସତ୍ୟ);
            // Initialize the clockvar
            AstVarRef* ଅଚଳ skewedWriteRefp = ନୂତନ AstVarRef{flp, varp, VAccess::WRITE};
            skewedWriteRefp->user1(ସତ୍ୟ);
            AstInitialStatic* ଅଚଳ initClockvarp = ନୂତନ AstInitialStatic{
                flp, ନୂତନ AstAssign{flp, skewedWriteRefp, exprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)}};
            m_modp->addStmtsp(initClockvarp);
            // Each drive sets a flag, so the signal is also assigned when driven with the same
            // value again, e.g. after another assignment to the signal (IEEE 1800-2023 14.16.2)
            AstVar* ଅଚଳ drivenVarp = getCreateDrivenVarp(nodep);
            m_modp->addStmtsp(ନୂତନ AstInitialStatic{
                flp, ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                   ନୂତନ AstConst{flp, AstConst::BitFalse{}}}});
            // Assign the clockvar to the actual var if it was driven
            AstAssign* ଅଚଳ assignp
                = ନୂତନ AstAssign{flp, exprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), skewedReadRefp};
            AstIf* ଅଚଳ ifp
                = ନୂତନ AstIf{flp, ନୂତନ AstVarRef{flp, drivenVarp, VAccess::READ},
                            ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                          ନୂତନ AstConst{flp, AstConst::BitFalse{}}}};
            ifp->addThensp(assignp);
            // Only in the time step of the clocking event
            AstCMethodHard* ଅଚଳ trigp = ନୂତନ AstCMethodHard{
                nodep->fileline(), ନୂତନ AstVarRef{flp, m_clockingp->ensureEventp(), VAccess::READ},
                VCMethod::EVENT_IS_TRIGGERED};
            trigp->dtypeSetBit();
            ifp->condp(ନୂତନ AstLogAnd{flp, ifp->condp()->unlinkFrBack(), trigp});
            // A skew rounding to no time precision ticks is a zero skew
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ ticksp
                = V3Timing::delayToTicks(skewp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), m_modp->timeunit());
            ଅଚଳ ତାର୍କିକ zeroSkew = ticksp->isZero();
            VL_DO_DANGLING(ticksp->delete_ବୃକ୍ଷ(), ticksp);
            ଯଦି (zeroSkew || !skewp->fileline()->timingOn()) {
                // Drive the var in Re-NBA (IEEE 1800-2023 14.16), ignoring skews where timing is
                // turned off
                Ast_Sen_ବୃକ୍ଷ* senTreep = m_clockingp->sentreep()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                senTreep->addSensesp(ନୂତନ AstSenItem{
                    flp, VEdgeType::ET_CHANGED, ନୂତନ AstVarRef{flp, drivenVarp, VAccess::READ}});
                m_clockingp->addNextHere(ନୂତନ AstAlwaysReactive{flp, senTreep, ifp});
            } ଅନ୍ୟଥା {
                // Fork only the assignment, so this AlwaysPostponed can be retriggered
                // before the assignment happens
                AstFork* ଅଚଳ forkp = ନୂତନ AstFork{flp, VJoinType::JOIN_NONE};
                forkp->immediateStart(ସତ୍ୟ);
                forkp->addForksp(ନୂତନ AstBegin{flp, "", assignp->unlinkFrBack(), ସତ୍ୟ});
                ifp->addThensp(forkp);
                // Read the clockvar in Postponed, so a drive late in the time step is applied
                // at this event plus the skew (IEEE 1800-2023 14.16)
                m_clockingp->addNextHere(ନୂତନ AstAlwaysPostponed{flp, ifp});
                ଯଦି (v3Global.opt.timing().isSetTrue()) {
                    AstDelay* ଅଚଳ delayp = ନୂତନ AstDelay{flp, skewp->unlinkFrBack(), ମିଥ୍ୟା};
                    delayp->timeunit(m_modp->timeunit());
                    assignp->timingControlp(delayp);
                } ଅନ୍ୟଥା ଯଦି (v3Global.opt.timing().isSetFalse()) {
                    nodep->v3warn(E_NOTIMING,
                                  "Clocking output skew greater than #0 requires --timing");
                } ଅନ୍ୟଥା {
                    nodep->v3warn(E_NEEDTIMINGOPT,
                                  "Use --timing or --no-timing to specify how "
                                  "clocking output skew greater than #0 should be handled");
                }
            }
        } ଅନ୍ୟଥା ଯଦି (nodep->direction() == VDirection::ନିବେଶ) {
            // Ref to the clockvar
            AstVarRef* ଅଚଳ refp = ନୂତନ AstVarRef{flp, varp, VAccess::WRITE};
            refp->user1(ସତ୍ୟ);
            ଯଦି (skewp->ଅଙ୍କ().is1Step()) {
                // #1step means the value that is sampled is always the signal's last value
                // before the clock edge (IEEE 1800-2023 14.4)
                AstSampled* ଅଚଳ sampledp
                    = ନୂତନ AstSampled{flp, exprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), exprp->dtypep(), ସତ୍ୟ};
                AstAssign* ଅଚଳ assignp = ନୂତନ AstAssign{flp, refp, sampledp};
                m_clockingp->addNextHere(ନୂତନ AstAlways{
                    flp, VAlwaysKwd::ALWAYS, m_clockingp->sentreep()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), assignp});
            } ଅନ୍ୟଥା ଯଦି (skewp->isZero()) {
                // #0 means the var has to be sampled in Observed (IEEE 1800-2023 14.13)
                AstAssign* ଅଚଳ assignp = ନୂତନ AstAssign{flp, refp, exprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
                m_clockingp->addNextHere(ନୂତନ AstAlwaysObserved{
                    flp, m_clockingp->sentreep()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), assignp});
            } ଅନ୍ୟଥା {
                // Create a queue where we'll store sampled values with timestamps
                AstSampleQueueDType* ଅଚଳ queueDtp
                    = ନୂତନ AstSampleQueueDType{flp, exprp->dtypep()};
                m_netlistp->typeTablep()->addTypesp(queueDtp);
                AstVar* ଅଚଳ queueVarp
                    = ନୂତନ AstVar{flp, VVarType::MODULETEMP, "__Vqueue__" + varp->name(), queueDtp};
                queueVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_clockingp->addNextHere(queueVarp);
                // Create a process like this:
                //     always queue.push(<sampled var>);
                AstCMethodHard* ଅଚଳ pushp = ନୂତନ AstCMethodHard{
                    flp, ନୂତନ AstVarRef{flp, queueVarp, VAccess::WRITE}, VCMethod::DYN_ଠେଲ,
                    ନୂତନ AstTime{nodep->fileline(), m_modp->timeunit()}};
                pushp->addPinsp(exprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା));
                pushp->dtypeSetVoid();
                m_clockingp->addNextHere(
                    ନୂତନ AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, pushp->makeStmt()});
                // Create a process like this:
                //     always @<clocking event> queue.pop(<skew>, /*out*/<skewed var>);
                AstCMethodHard* ଅଚଳ popp = ନୂତନ AstCMethodHard{
                    flp, ନୂତନ AstVarRef{flp, queueVarp, VAccess::READWRITE}, VCMethod::DYN_କାଢ଼,
                    ନୂତନ AstTime{nodep->fileline(), m_modp->timeunit()}};
                popp->addPinsp(skewp->unlinkFrBack());
                refp->access(VAccess::READWRITE);  // Only conditionally assigned
                popp->addPinsp(refp);
                popp->dtypeSetVoid();
                m_clockingp->addNextHere(ନୂତନ AstAlways{flp, VAlwaysKwd::ALWAYS,
                                                       m_clockingp->sentreep()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା),
                                                       popp->makeStmt()});
            }
        } ଅନ୍ୟଥା {
            nodep->v3fatalSrc("Invalid direction");
        }
    }
    ଶୂନ୍ୟ visit(AstDelay* nodep) override {
        m_hasCycleDelay |= nodep->isCycleDelay();
        // Only cycle delays are relevant in this stage; also only process once
        ଯଦି (!nodep->isCycleDelay()) {
            ଯଦି (m_inSynchDrive) {
                nodep->v3error("Only cycle delays can be used in synchronous drives"
                               " (IEEE 1800-2023 14.16)");
            }
            iterateChildren(nodep);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (m_inAssign && !m_inSynchDrive) {
            nodep->v3error("Cycle delays not allowed as intra-assignment delays"
                           " (IEEE 1800-2023 14.11)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBackWithNext());
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep = V3Const::constifyEdit(nodep->lhsp()->unlinkFrBack());
        ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(valuep, Const);
        ଯଦି (!constp && m_pexprp) {
            // V3AssertNfa handles non-const delays in sequences before this pass and
            // replaces the property; this branch should never be reached.
            nodep->v3fatalSrc("Non-constant cycle delay in assertion: "
                              "should have been caught by V3AssertNfa");
        } ଅନ୍ୟଥା ଯଦି (constp && constp->isZero()) {
            VL_DO_DANGLING(ଠେଲ_Deletep(valuep), valuep);
            ଯଦି (m_inSynchDrive) {
                // ##0 has no effect in synchronous drives (IEEE 1800-2023 14.11)
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
                ଫେରନ୍ତୁ;
            }
            ଯଦି (m_pexprp) {
                // ##0 in sequence context = zero delay = same clock tick
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
                ଫେରନ୍ତୁ;
            }
            // Procedural ##0: synchronize with default clocking event (IEEE 1800-2023 14.11)
            ଯଦି (!m_defaultClockingp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
                ଫେରନ୍ତୁ;
            }
            nodep->replaceWith(newCycleDelayZerop(flp));
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ;
        }
        Ast_Sen_ବୃକ୍ଷ* sentreep = nullptr;
        ଯଦି (!m_drives.empty()) {
            ଯଦି (m_inConcatAssign) {
                // Also the clockvars may be of different clocking blocks
                nodep->v3error("Cycle delays cannot be used in synchronous drives to a"
                               " concatenation (IEEE 1800-2023 14.16)");
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(ଠେଲ_Deletep(valuep), valuep);
                ଫେରନ୍ତୁ;
            }
            // Count the cycles of the driven clockvar's clocking block (IEEE 1800-2023 14.16)
            sentreep = newDriveSentreep(nodep, m_drives.front());
            ଯଦି (!sentreep) {
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(ଠେଲ_Deletep(valuep), valuep);
                ଫେରନ୍ତୁ;
            }
        } ଅନ୍ୟଥା ଯଦି (!m_defaultClockingp) {
            ଯଦି (!m_pexprp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(ଠେଲ_Deletep(valuep), valuep);
                ଫେରନ୍ତୁ;
            }
            sentreep = ନୂତନ Ast_Sen_ବୃକ୍ଷ{flp, m_senip->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)};
        } ଅନ୍ୟଥା {
            sentreep = m_defaultClockingp->sentreep()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
        }
        AstEventControl* ଅଚଳ controlp
            = ନୂତନ AstEventControl{nodep->fileline(), sentreep, nullptr};
        ଅଚଳ std::string delayName = m_cycleDlyNames.get(nodep);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* throughoutp
            = nodep->throughoutp() ? nodep->throughoutp()->unlinkFrBack() : nullptr;

        AstVar* ଅଚଳ cntVarp = ନୂତନ AstVar{flp, VVarType::BLOCKTEMP, delayName + "__counter",
                                           nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstBegin* ଅଚଳ beginp = ନୂତନ AstBegin{flp, delayName + "__block", cntVarp, ସତ୍ୟ};
        // A non-constant delay is counted at runtime. The count is integral of 64 bits, as
        // V3Width makes delays, or real, which is rounded (IEEE 1800-2023 6.12.1)
        ଯଦି (valuep->isDouble()) {
            valuep = ନୂତନ AstRToIRoundS{flp, valuep};
            valuep->dtypeFrom(cntVarp);
        } ଅନ୍ୟଥା {
            valuep = ନୂତନ AstSel{flp, valuep, 0, cntVarp->width()};
        }
        beginp->addStmtsp(ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, cntVarp, VAccess::WRITE}, valuep});
        // If the count is zero, a synchronous drive waits for none, like with a constant '##0',
        // and a procedural delay waits like a procedural '##0' (IEEE 1800-2023 14.11)
        ଯଦି (!constp && !m_inSynchDrive) {
            beginp->addStmtsp(ନୂତନ AstIf{
                flp,
                ନୂତନ AstEq{flp, ନୂତନ AstVarRef{flp, cntVarp, VAccess::READ}, ନୂତନ AstConst{flp, 0}},
                newCycleDelayZerop(flp)});
        }

        // Throughout: create flag tracking whether condition held every tick
        AstVar* throughoutOkp = nullptr;
        ଯଦି (throughoutp) {
            throughoutOkp = ନୂତନ AstVar{flp, VVarType::BLOCKTEMP, delayName + "__throughoutOk",
                                       nodep->findBasicDType(VBasicDTypeKwd::LOGIC_IMPLICIT)};
            throughoutOkp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->addStmtsp(throughoutOkp);
            beginp->addStmtsp(ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            ନୂତନ AstConst{flp, AstConst::BitTrue{}}});
            // Check condition at tick 0 (sequence start, before entering loop)
            beginp->addStmtsp(
                ନୂତନ AstIf{flp, ନୂତନ AstLogNot{flp, throughoutp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)},
                          ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                        ନୂତନ AstConst{flp, AstConst::BitFalse{}}}});
        }

        {
            AstLoop* ଅଚଳ loopp = ନୂତନ AstLoop{flp};
            // When throughout is present, exit loop early if condition fails
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* loopCondp
                = ନୂତନ AstGt{flp, ନୂତନ AstVarRef{flp, cntVarp, VAccess::READ}, ନୂତନ AstConst{flp, 0}};
            ଯଦି (throughoutOkp) {
                loopCondp = ନୂତନ AstLogAnd{flp, loopCondp,
                                          ନୂତନ AstVarRef{flp, throughoutOkp, VAccess::READ}};
            }
            loopp->addStmtsp(ନୂତନ AstLoopTest{flp, loopp, loopCondp});
            loopp->addStmtsp(controlp);
            loopp->addStmtsp(
                ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, cntVarp, VAccess::WRITE},
                              ନୂତନ AstSub{flp, ନୂତନ AstVarRef{flp, cntVarp, VAccess::READ},
                                         ନୂତନ AstConst{flp, 1}}});
            // Check throughout condition at each tick during delay (IEEE 1800-2023 16.9.9)
            ଯଦି (throughoutp) {
                loopp->addStmtsp(
                    ନୂତନ AstIf{flp, ନୂତନ AstLogNot{flp, throughoutp},
                              ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            ନୂତନ AstConst{flp, AstConst::BitFalse{}}}});
            }
            beginp->addStmtsp(loopp);
        }
        // Compose wrappers on remaining sequence: throughout gate (inner), disable iff (outer)
        Ast_ଗଣ୍ଠି* remainp = nodep->nextp() ? nodep->nextp()->unlinkFrBackWithNext() : nullptr;
        ଯଦି (throughoutOkp) {
            // If condition failed during delay, fail assertion
            remainp = ନୂତନ AstIf{flp, ନୂତନ AstVarRef{flp, throughoutOkp, VAccess::READ}, remainp,
                                ନୂତନ AstPExprClause{flp, /*pass=*/ମିଥ୍ୟା}};
        }
        ଯଦି (m_disableSeqIfp && remainp) {
            AstIf* ଅଚଳ disableSeqIfp = m_disableSeqIfp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
            disableSeqIfp->addThensp(remainp);
            remainp = disableSeqIfp;
        }
        ଯଦି (remainp) {
            ଯଦି (throughoutOkp) {
                // throughoutOkp is declared in beginp scope -- check must be inside it
                beginp->addStmtsp(remainp);
            } ଅନ୍ୟଥା {
                nodep->addNextHere(remainp);
            }
        }
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
    }
    ଶୂନ୍ୟ visit(Ast_Sen_ବୃକ୍ଷ* nodep) override {
        ଯଦି (m_inSynchDrive) {
            nodep->v3error("Event controls cannot be used in "
                           "synchronous drives (IEEE 1800-2023 14.16)");
        }

        ଅଚଳ AstSampled* sampledp;
        ଯଦି (nodep->exists([&sampledp](ଅଚଳ AstSampled* ଅଚଳ sp) {
                sampledp = sp;
                ଫେରନ୍ତୁ ସତ୍ୟ;
            })) {
            sampledp->v3warn(E_UNSUPPORTED, "Unsupported: $sampled inside sensitivity list");
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Var_Ref* nodep) override {
        UINFO(8, " -varref:  " << nodep);
        UINFO(8, " -varref-var-back:  " << nodep->varp()->backp());
        UINFO(8, " -varref-var-user1:  " << nodep->varp()->user1p());
        ଯଦି (AstClockingItem* ଅଚଳ itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->firstAbovep(),
                ClockingItem)) {
            ଯଦି (nodep->user1()) ଫେରନ୍ତୁ;

            // ensure linking still works, this has to be done only once
            ଯଦି (AstVarXRef* xrefp = VN_CAST(nodep, VarXRef)) {
                UINFO(8, " -clockvarxref-in:  " << xrefp);
                string dotted = xrefp->dotted();
                ଅଚଳ size_t dotPos = dotted.rfind('.');
                dotted.erase(dotPos, string::npos);
                xrefp->dotted(dotted);
                UINFO(8, " -clockvarxref-out: " << xrefp);
                m_xrefsp.emplace_back(xrefp);
            }

            ଯଦି (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            ଯଦି (nodep->access().isWriteOrRW()) {
                ଯଦି (itemp->direction() == VDirection::OUTPUT) {
                    ଯଦି (!m_inAssignDlyLhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    ଯଦି (m_inAssign) {
                        m_inSynchDrive = ସତ୍ୟ;
                        m_drives.ଠେଲ_back({itemp, nodep});
                    }
                } ଅନ୍ୟଥା ଯଦି (itemp->direction() == VDirection::ନିବେଶ) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
            nodep->user1(ସତ୍ୟ);
        }
    }
    ଶୂନ୍ୟ visit(AstMemberSel* nodep) override {
        ଯଦି (AstClockingItem* ଅଚଳ itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->firstAbovep(),
                ClockingItem)) {
            ଯଦି (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            ଯଦି (nodep->access().isWriteOrRW()) {
                ଯଦି (itemp->direction() == VDirection::OUTPUT) {
                    ଯଦି (!m_inAssignDlyLhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    ଯଦି (m_inAssign) {
                        m_inSynchDrive = ସତ୍ୟ;
                        m_drives.ଠେଲ_back({itemp, nodep});
                    }
                } ଅନ୍ୟଥା ଯଦି (itemp->direction() == VDirection::ନିବେଶ) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Assign* nodep) override {
        ଯଦି (nodep->user1()) ଫେରନ୍ତୁ;
        VL_RESTORER(m_inAssign);
        VL_RESTORER(m_inSynchDrive);
        VL_RESTORER(m_inConcatAssign);
        VL_RESTORER_CLEAR(m_drives);
        m_inAssign = ସତ୍ୟ;
        m_inSynchDrive = ମିଥ୍ୟା;
        {
            // The braces of a concatenation are a replication, by one
            ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp = nodep->lhsp();
            ଯଦି (ଅଚଳ AstReplicate* ଅଚଳ repp = VN_CAST(lhsp, Replicate)) lhsp = repp->srcp();
            m_inConcatAssign = VN_IS(lhsp, Concat);
        }
        {
            VL_RESTORER(m_inAssignDlyLhs);
            m_inAssignDlyLhs = VN_IS(nodep, AssignDly);
            iterate(nodep->lhsp());
        }
        iterate(nodep->rhsp());
        ଯଦି (nodep->timingControlp()) iterate(nodep->timingControlp());
        // A drive without a cycle delay, also after removing '##0', updates the clockvar now
        ଯଦି (m_inSynchDrive && !nodep->timingControlp()) {
            AstAssign* ଅଚଳ assignp = ନୂତନ AstAssign{
                nodep->fileline(), nodep->lhsp()->unlinkFrBack(), nodep->rhsp()->unlinkFrBack()};
            assignp->user1(ସତ୍ୟ);
            nodep->replaceWith(assignp);
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
            nodep = assignp;
        }
        // Note the drives when they take effect, after the cycle delay if any. Each clockvar
        // of the LHS is driven, e.g. in a concatenation which other simulators accept, although
        // IEEE 1800-2023 14.16 does not allow it
        Ast_ଗଣ୍ଠି* setsp = nullptr;
        ପାଇଁ (ଅଚଳ SynchDrive& drive : m_drives) {
            setsp = Ast_ଗଣ୍ଠି::addNext(setsp, newDrivenSetp(nodep->fileline(), drive));
        }
        ଯଦି (!setsp) ଫେରନ୍ତୁ;
        ଯଦି (AstBegin* ଅଚଳ beginp = VN_CAST(nodep->timingControlp(), Begin)) {
            beginp->addStmtsp(setsp);
        } ଅନ୍ୟଥା {
            nodep->addNextHere(setsp);
        }
    }
    ଶୂନ୍ୟ visit(AstAlways* nodep) override {
        iterateAndNextNull(nodep->sentreep());
        ଯଦି (nodep->sentreep()) m_seniAlwaysp = nodep->sentreep()->sensesp();
        iterateAndNextNull(nodep->stmtsp());
        m_seniAlwaysp = nullptr;
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Cover_Or_Assert* nodep) override {
        ଯଦି (nodep->sentreep()) ଫେରନ୍ତୁ;  // Already processed

        VL_RESTORER(m_senip);
        VL_RESTORER(m_disablep);
        m_senip = nullptr;
        m_disablep = nullptr;

        // Find Clocking's buried under nodep->exprsp
        iterateChildren(nodep);
        ଯଦି (!nodep->immediate()) nodep->sentreep(new_Sen_ବୃକ୍ଷ(nodep, nullptr, nodep));
    }
    ଶୂନ୍ୟ visit(AstFalling* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp = nodep->exprp()->unlinkFrBack();
        ଯଦି (exprp->width() > 1) exprp = ନୂତନ AstSel{fl, exprp, 0, 1};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ futurep = ନୂତନ AstFuture{fl, exprp, new_Sen_ବୃକ୍ଷ(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = ନୂତନ AstAnd{fl, exprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), ନୂତନ AstNot{fl, futurep}};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(AstFell* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp = nodep->exprp()->unlinkFrBack();
        ଯଦି (exprp->width() > 1) exprp = ନୂତନ AstSel{fl, exprp, 0, 1};
        Ast_Sen_ବୃକ୍ଷ* sentreep = nodep->sentreep();
        ଯଦି (sentreep) sentreep->unlinkFrBack();
        AstPast* ଅଚଳ pastp = ନୂତନ AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_ବୃକ୍ଷ(nodep, sentreep));
        exprp = ନୂତନ AstAnd{fl, pastp, ନୂତନ AstNot{fl, exprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)}};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(AstFuture* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        Ast_Sen_ବୃକ୍ଷ* ଅଚଳ sentreep = nodep->sentreep();
        ଯଦି (sentreep) VL_DO_DANGLING(ଠେଲ_Deletep(sentreep->unlinkFrBack()), sentreep);
        nodep->sentreep(new_Sen_ବୃକ୍ଷ(nodep));
    }
    ଶୂନ୍ୟ visit(AstPast* nodep) override {
        checkSamplingFuncDType(nodep, nodep->exprp());
        ଯଦି (nodep->sentreep()) ଫେରନ୍ତୁ;  // Already processed
        iterateChildren(nodep);
        nodep->sentreep(new_Sen_ବୃକ୍ଷ(nodep));
    }
    ଶୂନ୍ୟ visit(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- Lower standalone exact [*N] (N >= 2) via saturating counter.
        // Range/unbounded forms and SExpr-contained forms are lowered by V3AssertNfa.
        iterateChildren(nodep);
        // V3AssertNfa handles unbounded/ranged forms upstream, so this fast-path
        // is effectively unreachable when NFA is enabled.
        ଯଦି (nodep->unbounded() || nodep->maxCountp()) ଫେରନ୍ତୁ;  // LCOV_EXCL_LINE
        ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(nodep->countp(), Const);
        ଯଦି (VL_UNLIKELY(!constp || constp->toSInt() < 1)) {
            nodep->v3fatalSrc("Consecutive repetition count must be a positive constant"
                              " (should have been caught by V3Width)");
            ଫେରନ୍ତୁ;
        }
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା n = constp->toSInt();
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = nodep->exprp()->unlinkFrBack();
        ଯଦି (n == 1) {
            nodep->replaceWith(exprp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ;
        }
        // Saturating counter: if (expr) cnt <= min(cnt+1, N); else cnt <= 0;
        AstVar* ଅଚଳ cntVarp = ନୂତନ AstVar{flp, VVarType::MODULETEMP, m_consRepNames.get(""),
                                           nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(cntVarp);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ ପଦସମଷ୍ଟି_Clonep = exprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ saturatingIncrp = ନୂତନ AstCond{
            flp,
            ନୂତନ AstLt{flp, ନୂତନ AstVarRef{flp, cntVarp, VAccess::READ},
                      ନୂତନ AstConst{flp, static_cast<uint32_t>(n)}},
            ନୂତନ AstAdd{flp, ନୂତନ AstVarRef{flp, cntVarp, VAccess::READ}, ନୂତନ AstConst{flp, 1u}},
            ନୂତନ AstConst{flp, static_cast<uint32_t>(n)}};
        AstAssignDly* ଅଚଳ incrAssignp
            = ନୂତନ AstAssignDly{flp, ନୂତନ AstVarRef{flp, cntVarp, VAccess::WRITE}, saturatingIncrp};
        AstAssignDly* ଅଚଳ resetAssignp = ନୂତନ AstAssignDly{
            flp, ନୂତନ AstVarRef{flp, cntVarp, VAccess::WRITE}, ନୂତନ AstConst{flp, 0u}};
        AstIf* ଅଚଳ ifp = ନୂତନ AstIf{flp, ପଦସମଷ୍ଟି_Clonep, incrAssignp, resetAssignp};
        Ast_Sen_ବୃକ୍ଷ* ଅଚଳ senTreep = new_Sen_ବୃକ୍ଷ(nodep);
        AstAlways* ଅଚଳ alwaysp = ନୂତନ AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, ifp};
        cntVarp->addNextHere(alwaysp);
        // Match: cnt >= N-1 (previous cycles via NBA) && expr (current cycle)
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ cntCheckp = ନୂତନ AstGte{flp, ନୂତନ AstVarRef{flp, cntVarp, VAccess::READ},
                                                  ନୂତନ AstConst{flp, static_cast<uint32_t>(n - 1)}};
        cntCheckp->dtypeSetBit();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ matchp = ନୂତନ AstAnd{flp, cntCheckp, exprp};
        matchp->dtypeSetBit();
        nodep->replaceWith(matchp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(AstRising* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp = nodep->exprp()->unlinkFrBack();
        ଯଦି (exprp->width() > 1) exprp = ନୂତନ AstSel{fl, exprp, 0, 1};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ futurep = ନୂତନ AstFuture{fl, exprp, new_Sen_ବୃକ୍ଷ(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = ନୂତନ AstAnd{fl, ନୂତନ AstNot{fl, exprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)}, futurep};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(AstRose* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp = nodep->exprp()->unlinkFrBack();
        ଯଦି (exprp->width() > 1) exprp = ନୂତନ AstSel{fl, exprp, 0, 1};
        Ast_Sen_ବୃକ୍ଷ* sentreep = nodep->sentreep();
        ଯଦି (sentreep) sentreep->unlinkFrBack();
        AstPast* ଅଚଳ pastp = ନୂତନ AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_ବୃକ୍ଷ(nodep, sentreep));
        exprp = ନୂତନ AstAnd{fl, ନୂତନ AstNot{fl, pastp}, exprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ସ୍ଥିର AstAssocArrayDType* getProcessAssocArrayType(File_ଧାଡ଼ି* ଅଚଳ flp) {
        // Type of __VassertsActive___x[std::process]
        Ast_ଗଣ୍ଠି_DType* valp
            = v3Global.rootp()->typeTablep()->findBasicDType(flp, VBasicDTypeKwd::BIT);
        AstClassRefDType* keyp
            = ନୂତନ AstClassRefDType{flp, v3Global.rootp()->stdPackageProcessp(), nullptr};
        keyp->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        v3Global.rootp()->typeTablep()->addTypesp(keyp);
        AstAssocArrayDType* ଅଚଳ typep = ନୂତନ AstAssocArrayDType{flp, valp, keyp};
        typep->dtypep(typep);
        v3Global.rootp()->typeTablep()->addTypesp(typep);
        ଫେରନ୍ତୁ typep;
    }
    ସ୍ଥିର Ast_Stmt_ପଦସମଷ୍ଟି* getProcessAssocArrayDelete(AstVarRef* ଅଚଳ refp) {
        // Constructs refp.delete(std::process::self()) statement
        File_ଧାଡ଼ି* ଅଚଳ flp = refp->fileline();
        AstCMethodHard* ଅଚଳ deletep = ନୂତନ AstCMethodHard{
            flp, refp, VCMethod::ASSOC_ERASE, v3Global.rootp()->stdPackageProcessSelfp(flp)};
        deletep->dtypep(refp->findVoidDType());
        ଫେରନ୍ତୁ ନୂତନ Ast_Stmt_ପଦସମଷ୍ଟି{flp, deletep};
    }
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* getProcessAssocArraySize(AstVarRef* ଅଚଳ refp) {
        // Constructs refp.size() statement
        AstCMethodHard* ଅଚଳ sizep
            = ନୂତନ AstCMethodHard{refp->fileline(), refp, VCMethod::ASSOC_SIZE};
        sizep->dtypep(refp->findBasicDType(VBasicDTypeKwd::UINT32));
        ଫେରନ୍ତୁ sizep;
    }
    ଶୂନ୍ୟ visit(AstSEventually* nodep) override {
        UASSERT(v3Global.rootp()->stdPackagep(), "Should be imported");

        Ast_Sen_ବୃକ୍ଷ* ଅଚଳ sentreep = new_Sen_ବୃକ୍ଷ(nodep);
        ଯଦି (!sentreep->sensesp()) {
            VL_DO_DANGLING(ଠେଲ_Deletep(sentreep), sentreep);
            nodep->replaceWith(ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ;
        }

        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();

        // Track active assertions
        AstVar* ଅଚଳ activep = ନୂତନ AstVar{flp, VVarType::MODULETEMP, m_activeNames.get(""),
                                           getProcessAssocArrayType(flp)};
        activep->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(activep);

        // Assertion condition check
        AstLoop* ଅଚଳ loopp = ନୂତନ AstLoop{flp};
        AstSampled* ଅଚଳ condp
            = ନୂତନ AstSampled{flp, nodep->exprp()->unlinkFrBack(), nullptr, ସତ୍ୟ};
        loopp->addStmtsp(ନୂତନ AstLoopTest{flp, loopp, ନୂତନ AstLogNot{flp, condp}});
        loopp->addStmtsp(ନୂତନ AstEventControl{flp, sentreep, nullptr});

        // Add assertion to the active set
        AstAssocSel* ଅଚଳ selp = ନୂତନ AstAssocSel{flp, ନୂତନ AstVarRef{flp, activep, VAccess::WRITE},
                                                  v3Global.rootp()->stdPackageProcessSelfp(flp)};
        AstAssign* ଅଚଳ incrementp = ନୂତନ AstAssign{flp, selp, ନୂତନ AstConst{flp, 1}};
        AstPExprClause* ଅଚଳ clausep = ନୂତନ AstPExprClause{flp};
        Ast_Stmt_ପଦସମଷ୍ଟି* ଅଚଳ deletep
            = getProcessAssocArrayDelete(ନୂତନ AstVarRef{flp, activep, VAccess::WRITE});

        // Main assertion block
        AstBegin* ଅଚଳ bodyp = ନୂତନ AstBegin{flp, m_blockNames.get(""), nullptr, ସତ୍ୟ};
        bodyp->addStmtsp(incrementp);
        bodyp->addStmtsp(loopp);
        bodyp->addStmtsp(clausep);
        bodyp->addStmtsp(deletep);

        // Validate assertion condition for each active assert
        AstVar* ଅଚଳ activeCountp = ନୂତନ AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        activeCountp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstAssign* ଅଚଳ initActiveCountp
            = ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, activeCountp, VAccess::WRITE},
                            getProcessAssocArraySize(ନୂତନ AstVarRef{flp, activep, VAccess::READ})};
        AstLoop* ଅଚଳ finalLoopp = ନୂତନ AstLoop{flp};
        AstIf* ଅଚଳ finalBodypCondp
            = ନୂତନ AstIf{flp, condp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), ନୂତନ AstPExprClause{flp},
                        ନୂତନ AstPExprClause{flp, ମିଥ୍ୟା}};
        finalLoopp->addStmtsp(
            ନୂତନ AstLoopTest{flp, finalLoopp,
                            ନୂତନ AstNeq{flp, ନୂତନ AstVarRef{flp, activeCountp, VAccess::READ},
                                       ନୂତନ AstConst{flp, 0}}});
        finalLoopp->addStmtsp(finalBodypCondp);
        finalLoopp->addStmtsp(
            ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, activeCountp, VAccess::WRITE},
                          ନୂତନ AstSub{flp, ନୂତନ AstVarRef{flp, activeCountp, VAccess::READ},
                                     ନୂତନ AstConst{flp, 1}}});

        // Final assertion block
        AstBegin* ଅଚଳ finalp = ନୂତନ AstBegin{flp, m_blockNames.get(""), nullptr, ସତ୍ୟ};
        finalp->addStmtsp(activeCountp);
        finalp->addStmtsp(initActiveCountp);
        finalp->addStmtsp(finalLoopp);

        VL_RESTORER(m_pexprp);
        m_pexprp = ନୂତନ AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
        VL_RESTORER(m_hasCycleDelay);
        m_hasCycleDelay = ମିଥ୍ୟା;
        iterate(bodyp);
        iterate(finalp);

        ଯଦି (m_hasCycleDelay) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in s_eventually");
            nodep->replaceWith(ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            VL_DO_DANGLING(m_pexprp->delete_ବୃକ୍ଷ(), m_pexprp);
            ଫେରନ୍ତୁ;
        }

        nodep->replaceWith(m_pexprp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(AstStable* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp = nodep->exprp()->unlinkFrBack();
        Ast_Sen_ବୃକ୍ଷ* sentreep = nodep->sentreep();
        ଯଦି (sentreep) sentreep->unlinkFrBack();
        AstPast* ଅଚଳ pastp = ନୂତନ AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_ବୃକ୍ଷ(nodep, sentreep));
        exprp = ନୂତନ AstEq{fl, pastp, exprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(AstSteady* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp = nodep->exprp()->unlinkFrBack();
        ଯଦି (exprp->width() > 1) exprp = ନୂତନ AstSel{fl, exprp, 0, 1};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ futurep = ନୂତନ AstFuture{fl, exprp, new_Sen_ବୃକ୍ଷ(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = ନୂତନ AstEq{fl, exprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), futurep};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(AstSampled* nodep) override {
        iterateChildren(nodep);
        ଯଦି (!nodep->internal()) checkSamplingFuncDType(nodep, nodep->exprp());
    }

    // Validate repetition count: must be a non-negative elaboration-time constant.
    // Shared by goto [->N] and nonconsecutive [=N] repetition.
    // On error, replaces nodep with BitFalse placeholder and returns nullptr.
    ଅଚଳ AstConst* validate_Rep_ଗଣନା(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*& countp) {
        countp = V3Const::constifyEdit(countp);
        ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(countp, Const);
        ଯଦି (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ଠେଲ_Deletep(countp), countp);
            nodep->replaceWith(ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ nullptr;
        }
        ଯଦି (constp->toSInt() < 0) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ଠେଲ_Deletep(countp), countp);
            nodep->replaceWith(ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ nullptr;
        }
        ଯଦି (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ଠେଲ_Deletep(countp), countp);
            nodep->replaceWith(ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ nullptr;
        }
        ଫେରନ୍ତୁ constp;
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
    AstPExpr* createRepPExpr(File_ଧାଡ଼ି* flp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* countp,
                             Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp, ତାର୍କିକ isOverlapped, ତାର୍କିକ isNonConsec) {
        AstSenItem* ଅଚଳ sensesp = m_senip;
        UASSERT_OBJ(sensesp, exprp, "Repetition requires a clock");

        ଅଚଳ std::string name
            = isNonConsec ? m_nonConsRepNames.get(exprp) : m_gotoRepNames.get(exprp);
        AstVar* ଅଚଳ cntVarp = ନୂତନ AstVar{flp, VVarType::BLOCKTEMP, name + "__counter",
                                           exprp->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstBegin* ଅଚଳ beginp = ନୂତନ AstBegin{flp, name + "__block", cntVarp, ସତ୍ୟ};
        beginp->addStmtsp(
            ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, cntVarp, VAccess::WRITE}, ନୂତନ AstConst{flp, 0}});

        // Counting loop: find N occurrences of expr (shared by goto and nonconsec)
        AstLoop* ଅଚଳ loopp = ନୂତନ AstLoop{flp};
        loopp->addStmtsp(ନୂତନ AstLoopTest{flp, loopp,
                                         ନୂତନ AstLt{flp, ନୂତନ AstVarRef{flp, cntVarp, VAccess::READ},
                                                   countp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)}});
        // if (expr) cnt++ -- sampled is applied to whole property expr by V3Assert
        loopp->addStmtsp(
            ନୂତନ AstIf{flp, exprp,
                      ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, cntVarp, VAccess::WRITE},
                                    ନୂତନ AstAdd{flp, ନୂତନ AstVarRef{flp, cntVarp, VAccess::READ},
                                               ନୂତନ AstConst{flp, 1}}}});
        loopp->addStmtsp(ନୂତନ AstIf{
            flp, ନୂତନ AstLt{flp, ନୂତନ AstVarRef{flp, cntVarp, VAccess::READ}, countp},
            ନୂତନ AstEventControl{flp, ନୂତନ Ast_Sen_ବୃକ୍ଷ{flp, sensesp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)}, nullptr}});

        beginp->addStmtsp(loopp);

        ଯଦି (isNonConsec && rhsp) {
            // IEEE 16.9.2: b[=N] = b[->N] ##1 !b[*0:$]
            // Trailing window: ##1 advance, then scan !expr cycles checking rhs.
            // Window closes when expr becomes true again (fail if rhs was never true).
            beginp->addStmtsp(ନୂତନ AstEventControl{
                flp, ନୂତନ Ast_Sen_ବୃକ୍ଷ{flp, sensesp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)}, nullptr});  // ##1
            ଯଦି (!isOverlapped) {
                beginp->addStmtsp(ନୂତନ AstEventControl{
                    flp, ନୂତନ Ast_Sen_ବୃକ୍ଷ{flp, sensesp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)}, nullptr});  // |=>
            }
            // Window loop: check rhs at each !expr cycle (done variable for termination)
            AstVar* ଅଚଳ doneVarp
                = ନୂତନ AstVar{flp, VVarType::BLOCKTEMP, name + "__done", exprp->findBitDType()};
            doneVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->addStmtsp(doneVarp);
            beginp->addStmtsp(ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, doneVarp, VAccess::WRITE},
                                            ନୂତନ AstConst{flp, AstConst::BitFalse{}}});
            auto setDone = [&]() {
                ଫେରନ୍ତୁ ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, doneVarp, VAccess::WRITE},
                                     ନୂତନ AstConst{flp, AstConst::BitTrue{}}};
            };
            AstLoop* ଅଚଳ windowp = ନୂତନ AstLoop{flp};
            // LoopTest: continue while !done
            windowp->addStmtsp(ନୂତନ AstLoopTest{
                flp, windowp, ନୂତନ AstNot{flp, ନୂତନ AstVarRef{flp, doneVarp, VAccess::READ}}});
            // if (expr) { fail; done = 1; } -- window closed, expr true again
            Ast_ଗଣ୍ଠି* ଅଚଳ failsp = ନୂତନ AstPExprClause{flp, ମିଥ୍ୟା};
            failsp->addNext(setDone());
            windowp->addStmtsp(ନୂତନ AstIf{flp, exprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), failsp});
            // if (rhs) { pass; done = 1; } -- consequent true at this !expr endpoint
            Ast_ଗଣ୍ଠି* ଅଚଳ passsp = ନୂତନ AstPExprClause{flp, ସତ୍ୟ};
            passsp->addNext(setDone());
            windowp->addStmtsp(ନୂତନ AstIf{flp, rhsp, passsp});
            // @(clk) -- advance to next cycle in window
            windowp->addStmtsp(
                ନୂତନ AstEventControl{flp, ନୂତନ Ast_Sen_ବୃକ୍ଷ{flp, sensesp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)}, nullptr});
            beginp->addStmtsp(windowp);
        } ଅନ୍ୟଥା ଯଦି (isNonConsec) {
            // Standalone nonconsec: ##1 into window, then pass
            beginp->addStmtsp(
                ନୂତନ AstEventControl{flp, ନୂତନ Ast_Sen_ବୃକ୍ଷ{flp, sensesp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)}, nullptr});
            beginp->addStmtsp(ନୂତନ AstPExprClause{flp, ସତ୍ୟ});
        } ଅନ୍ୟଥା ଯଦି (rhsp) {
            // Goto rep: check consequent once at match endpoint
            ଯଦି (!isOverlapped) {
                beginp->addStmtsp(ନୂତନ AstEventControl{
                    flp, ନୂତନ Ast_Sen_ବୃକ୍ଷ{flp, sensesp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)}, nullptr});
            }
            beginp->addStmtsp(ନୂତନ AstIf{flp, rhsp, ନୂତନ AstPExprClause{flp, ସତ୍ୟ},
                                        ନୂତନ AstPExprClause{flp, ମିଥ୍ୟା}});
        } ଅନ୍ୟଥା {
            // Standalone goto: pass after counting
            beginp->addStmtsp(ନୂତନ AstPExprClause{flp, ସତ୍ୟ});
        }

        ଫେରନ୍ତୁ ନୂତନ AstPExpr{flp, beginp, exprp->findBitDType()};
    }

    // Reject [=M:N] nonconsecutive range that reached V3AssertPre.
    // [->M:N] is fully handled upstream (NFA path); only [=M:N] still lands
    // here because AstSNonConsRep lowering is not yet implemented there.
    // Replaces the node with BitFalse so lowering continues after the warning.
    ତାର୍କିକ rejectNonconsecRange(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* maxCountp) {
        ଯଦି (!maxCountp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: [=M:N] nonconsecutive range repetition"
                                     " (IEEE 1800-2023 16.9.2)");
        nodep->replaceWith(ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalse{}});
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    ଶୂନ୍ୟ visit(AstSGotoRep* nodep) override {
        // Standalone goto rep (not inside implication antecedent)
        iterateChildren(nodep);
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* countp = nodep->countp()->unlinkFrBack();
        ଯଦି (!validate_Rep_ଗଣନା(nodep, countp)) ଫେରନ୍ତୁ;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = nodep->exprp()->unlinkFrBack();
        AstPExpr* ଅଚଳ pexprp = createRepPExpr(flp, exprp, countp, nullptr, ସତ୍ୟ, ମିଥ୍ୟା);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }

    ଶୂନ୍ୟ visit(AstSNonConsRep* nodep) override {
        // Standalone nonconsecutive rep (not inside implication antecedent)
        iterateChildren(nodep);
        ଯଦି (rejectNonconsecRange(nodep, nodep->maxCountp())) ଫେରନ୍ତୁ;
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* countp = nodep->countp()->unlinkFrBack();
        ଯଦି (!validate_Rep_ଗଣନା(nodep, countp)) ଫେରନ୍ତୁ;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = nodep->exprp()->unlinkFrBack();
        AstPExpr* ଅଚଳ pexprp = createRepPExpr(flp, exprp, countp, nullptr, ସତ୍ୟ, ସତ୍ୟ);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }

    ଶୂନ୍ୟ visit(AstFuncRef* nodep) override {
        // IEEE 1800-2023 16.8: Inline sequence instances wherever they appear
        // in the expression tree (inside implications, boolean ops, nested refs, etc.)
        ଯଦି (AstSequence* ଅଚଳ seqp = VN_CAST(nodep->taskp(), Sequence)) {
            substituteSequenceCall(nodep, seqp);
            // The FuncRef has been replaced; do not access nodep after this point.
            // The replacement node will be visited by the parent's iterateChildren.
            ଫେରନ୍ତୁ;
        }
        ଯଦି (ଅଚଳ AstProperty* ଅଚଳ propp = VN_CAST(nodep->taskp(), Property)) {
            substituteNestedPropertyCall(nodep, propp);
            ଫେରନ୍ତୁ;
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstImplication* nodep) override {
        ଯଦି (nodep->sentreep()) ଫେରନ୍ତୁ;  // Already processed

        // Top-level followed-by is claimed by V3AssertNfa; anything reaching here
        // is nested inside iff/implies/or. IEEE 1800-2023 16.12.9 permits the
        // composition, but silent lowering would drop non-vacuous-fail semantics.
        ଯଦି (nodep->isFollowedBy()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: followed-by (#-# / #=#) nested inside property operator"
                          " (iff/implies/or) (IEEE 1800-2023 16.12.9)");
            nodep->replaceWith(ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ;
        }

        // Handle goto repetition as antecedent before iterateChildren,
        // so the standalone AstSGotoRep visitor doesn't process it
        ଯଦି (AstSGotoRep* ଅଚଳ gotop = VN_CAST(nodep->lhsp(), SGotoRep)) {
            iterateChildren(gotop);
            iterateAndNextNull(nodep->rhsp());
            File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* countp = gotop->countp()->unlinkFrBack();
            ଯଦି (!validate_Rep_ଗଣନା(nodep, countp)) ଫେରନ୍ତୁ;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = gotop->exprp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = nodep->rhsp()->unlinkFrBack();
            AstPExpr* ଅଚଳ pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), ମିଥ୍ୟା);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ;
        }

        // Handle nonconsecutive repetition as antecedent before iterateChildren,
        // so the standalone AstSNonConsRep visitor doesn't process it
        ଯଦି (AstSNonConsRep* ଅଚଳ ncrp = VN_CAST(nodep->lhsp(), SNonConsRep)) {
            iterateChildren(ncrp);
            iterateAndNextNull(nodep->rhsp());
            ଯଦି (rejectNonconsecRange(nodep, ncrp->maxCountp())) ଫେରନ୍ତୁ;
            File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* countp = ncrp->countp()->unlinkFrBack();
            ଯଦି (!validate_Rep_ଗଣନା(nodep, countp)) ଫେରନ୍ତୁ;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = ncrp->exprp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = nodep->rhsp()->unlinkFrBack();
            AstPExpr* ଅଚଳ pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), ସତ୍ୟ);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ;
        }

        iterateChildren(nodep);

        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp = nodep->lhsp()->unlinkFrBack();

        // Lower sequence match items (IEEE 16.11): (expr, var = val, ...) |-> / |=>
        ଯଦି (Ast_ପଦସମଷ୍ଟି_Stmt* ଅଚଳ ପଦସମଷ୍ଟି_Stmtp = VN_CAST(lhsp, ପଦସମଷ୍ଟି_Stmt)) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ antExprp = ପଦସମଷ୍ଟି_Stmtp->resultp()->unlinkFrBack();

            ଯଦି (nodep->isOverlapped()) {
                // |-> : assign to __Vpropvar via always_comb (continuous).
                // The assign evaluates RHS once; V3Sampled snapshots the
                // result so all consequent refs read the same value.
                Ast_ଗଣ୍ଠି* matchAssignsp = nullptr;
                ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = ପଦସମଷ୍ଟି_Stmtp->stmtsp(); stmtp;) {
                    Ast_ଗଣ୍ଠି* ଅଚଳ nextp = stmtp->nextp();
                    ଯଦି (AstAssign* ଅଚଳ assignp = VN_CAST(stmtp, Assign)) {
                        assignp->unlinkFrBack();
                        matchAssignsp = Ast_ଗଣ୍ଠି::addNextNull(matchAssignsp, assignp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(ଠେଲ_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                ଯଦି (matchAssignsp) {
                    AstAlways* ଅଚଳ alwaysp
                        = ନୂତନ AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, matchAssignsp};
                    m_modp->addStmtsp(alwaysp);
                }
            } ଅନ୍ୟଥା {
                // |=> : assign to __Vpropvar via NBA in a clocked always block.
                // The NBA commits before the next cycle's sampled snapshot,
                // so the consequent (which already references __Vpropvar)
                // sees the captured value.
                Ast_ଗଣ୍ଠି* matchAssignsp = nullptr;
                ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = ପଦସମଷ୍ଟି_Stmtp->stmtsp(); stmtp;) {
                    Ast_ଗଣ୍ଠି* ଅଚଳ nextp = stmtp->nextp();
                    ଯଦି (AstAssign* ଅଚଳ assignp = VN_CAST(stmtp, Assign)) {
                        assignp->unlinkFrBack();
                        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ assignLhsp = assignp->lhsp()->unlinkFrBack();
                        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ assignRhsp = assignp->rhsp()->unlinkFrBack();
                        AstAssignDly* ଅଚଳ dlyp = ନୂତନ AstAssignDly{flp, assignLhsp, assignRhsp};
                        VL_DO_DANGLING(ଠେଲ_Deletep(assignp), assignp);
                        matchAssignsp = Ast_ଗଣ୍ଠି::addNextNull(matchAssignsp, dlyp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(ଠେଲ_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                ଯଦି (matchAssignsp) {
                    AstIf* ଅଚଳ condp
                        = ନୂତନ AstIf{flp, antExprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), matchAssignsp};
                    AstAlways* ଅଚଳ alwaysp
                        = ନୂତନ AstAlways{flp, VAlwaysKwd::ALWAYS, new_Sen_ବୃକ୍ଷ(nodep), condp};
                    m_modp->addStmtsp(alwaysp);
                }
            }
        }

        ଯଦି (AstPExpr* ଅଚଳ pexprp = VN_CAST(rhsp, PExpr)) {
            // Implication with sequence expression on RHS (IEEE 1800-2023 16.11, 16.12.7).
            // The PExpr was lowered from the property expression earlier in this pass.
            // Wrap the PExpr body with the antecedent check so the sequence only
            // starts when the antecedent holds.
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp;
            ଯଦି (nodep->isOverlapped()) {
                // Overlapped implication (|->): check antecedent on same cycle.
                // disable iff is applied at the assertion level, not at the
                // antecedent gate, matching the existing non-PExpr overlapped path.
                condp = lhsp;
            } ଅନ୍ୟଥା {
                // Non-overlapped implication (|=>): check antecedent from previous cycle
                ଯଦି (m_disablep) {
                    lhsp
                        = ନୂତନ AstAnd{flp, ନୂତନ AstNot{flp, m_disablep->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)}, lhsp};
                }
                AstPast* ଅଚଳ pastp
                    = ନୂତନ AstPast{flp, lhsp, nullptr, nullptr, /* propertyTiming */ ସତ୍ୟ};
                pastp->dtypeFrom(lhsp);
                pastp->sentreep(new_Sen_ବୃକ୍ଷ(nodep));
                condp = pastp;
            }
            // Wrap existing PExpr body: if (antecedent) { <original body> } else { /* vacuous pass
            // */ }
            // Only the statements are guarded; declarations stay directly in the block. After
            // V3Fork moves the block's statements into a task, V3Task only handles variables
            // that are direct statements of the task.
            AstBegin* ଅଚଳ bodyp = pexprp->bodyp();
            AstIf* ଅଚଳ guardp = ନୂତନ AstIf{flp, condp};
            ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = bodyp->stmtsp(); stmtp;) {
                Ast_ଗଣ୍ଠି* ଅଚଳ nextp = stmtp->nextp();
                ଯଦି (!VN_IS(stmtp, Var)) guardp->addThensp(stmtp->unlinkFrBack());
                stmtp = nextp;
            }
            bodyp->addStmtsp(guardp);
            nodep->replaceWith(pexprp);
            // Don't iterate pexprp here -- it was already iterated when created
            // (in visit(AstSExpr*)), so delays and disable iff are already processed.
        } ଅନ୍ୟଥା ଯଦି (nodep->isOverlapped()) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp
                = m_underCover ? static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(ନୂତନ AstLogAnd{flp, lhsp, rhsp})
                               : ନୂତନ AstLogOr{flp, ନୂତନ AstLogNot{flp, lhsp}, rhsp};
            nodep->replaceWith(exprp);
        } ଅନ୍ୟଥା {
            ଯଦି (m_disablep) {
                lhsp = ନୂତନ AstAnd{flp, ନୂତନ AstNot{flp, m_disablep->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)}, lhsp};
            }

            AstPast* ଅଚଳ pastp
                = ନୂତନ AstPast{flp, lhsp, nullptr, nullptr, /* propertyTiming */ ସତ୍ୟ};
            pastp->dtypeFrom(lhsp);
            pastp->sentreep(new_Sen_ବୃକ୍ଷ(nodep));
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp
                = m_underCover ? static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(ନୂତନ AstAnd{flp, pastp, rhsp})
                               : ନୂତନ AstOr{flp, ନୂତନ AstNot{flp, pastp}, rhsp};
            exprp->dtypeSetBit();
            nodep->replaceWith(exprp);
        }
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(AstUntil* nodep) override {
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        UASSERT_OBJ(
            !m_pexprp, nodep,
            "'" << nodep->verilogKwd()
                << "' in complex property expression should have been rejected by V3AssertNfa");
        ଯଦି (nodep->isStrong()
            && (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue())) {
            nodep->v3warn(E_NOTIMING, nodep->verilogKwd() << " requires --timing");
            nodep->replaceWith(ନୂତନ AstConst{flp, AstConst::BitFalse{}});
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (nodep->isStrong()) {
            // p s_until q / p s_until_with q: q must eventually be true. Until then, p must
            // be true on every sampled tick. For s_until, check q first: when q is true on
            // this tick, p is not required. For s_until_with, p must be true on the q tick too.
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rawLhsp = nodep->lhsp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rawRhsp = nodep->rhsp()->unlinkFrBack();
            AstSampled* ଅଚଳ lhsp = ନୂତନ AstSampled{flp, rawLhsp, rawLhsp->dtypep(), ସତ୍ୟ};
            AstSampled* ଅଚଳ rhsp = ନୂତନ AstSampled{flp, rawRhsp, rawRhsp->dtypep(), ସତ୍ୟ};
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* finalCondp = rhsp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
            ଯଦି (nodep->isOverlapping()) {
                finalCondp = ନୂତନ AstLogAnd{flp, lhsp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), finalCondp};
            }

            // Track active assertion attempts. Only the count is needed to emit final failures.
            AstVar* ଅଚଳ activep = ନୂତନ AstVar{flp, VVarType::MODULETEMP, m_activeNames.get(""),
                                               nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            activep->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(activep);

            AstVar* ଅଚଳ donep
                = ନୂତନ AstVar{flp, VVarType::BLOCKTEMP, "__VassertDone", nodep->findBitDType()};
            donep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            auto setDone = [&]() {
                ଫେରନ୍ତୁ ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, donep, VAccess::WRITE},
                                     ନୂତନ AstConst{flp, AstConst::BitTrue{}}};
            };

            AstLoop* ଅଚଳ loopp = ନୂତନ AstLoop{flp};
            AstAssign* ଅଚଳ decrementVar = ନୂତନ AstAssign{
                flp, ନୂତନ AstVarRef{flp, activep, VAccess::WRITE},
                ନୂତନ AstSub{flp, ନୂତନ AstVarRef{flp, activep, VAccess::READ}, ନୂତନ AstConst{flp, 1}}};
            loopp->addStmtsp(ନୂତନ AstLoopTest{
                flp, loopp, ନୂତନ AstLogNot{flp, ନୂତନ AstVarRef{flp, donep, VAccess::READ}}});
            {
                Ast_ଗଣ୍ଠି* ଅଚଳ passp = ନୂତନ AstPExprClause{flp};
                passp->addNext(decrementVar);
                passp->addNext(setDone());
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* passCondp = rhsp;
                ଯଦି (nodep->isOverlapping()) {
                    passCondp = ନୂତନ AstLogAnd{flp, lhsp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), passCondp};
                }
                loopp->addStmtsp(ନୂତନ AstIf{flp, passCondp, passp});
            }
            {
                Ast_ଗଣ୍ଠି* ଅଚଳ failp = ନୂତନ AstPExprClause{flp, ମିଥ୍ୟା};
                failp->addNext(decrementVar->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
                failp->addNext(setDone());
                loopp->addStmtsp(ନୂତନ AstIf{
                    flp,
                    ନୂତନ AstLogAnd{flp,
                                  ନୂତନ AstLogNot{flp, ନୂତନ AstVarRef{flp, donep, VAccess::READ}},
                                  ନୂତନ AstLogNot{flp, lhsp}},
                    failp});
            }
            AstDelay* ଅଚଳ delayp = ନୂତନ AstDelay{flp, ନୂତନ AstConst{flp, 1}, ମିଥ୍ୟା};
            delayp->timeunit(m_modp->timeunit());
            loopp->addStmtsp(ନୂତନ AstIf{
                flp, ନୂତନ AstLogNot{flp, ନୂତନ AstVarRef{flp, donep, VAccess::READ}}, delayp});

            // Main assertion block
            AstBegin* ଅଚଳ bodyp = ନୂତନ AstBegin{flp, m_blockNames.get(""), nullptr, ସତ୍ୟ};
            bodyp->addStmtsp(donep);
            bodyp->addStmtsp(ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, donep, VAccess::WRITE},
                                           ନୂତନ AstConst{flp, AstConst::BitFalse{}}});
            File_ଧାଡ଼ି* ଅଚଳ flp = activep->fileline();
            bodyp->addStmtsp(
                ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, activep, VAccess::WRITE},
                              ନୂତନ AstAdd{flp, ନୂତନ AstVarRef{flp, activep, VAccess::READ},
                                         ନୂତନ AstConst{flp, 1}}});
            bodyp->addStmtsp(loopp);

            // Validate assertion condition for each active assert
            AstVar* ଅଚଳ activeCountp = ନୂତନ AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                    nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            activeCountp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

            AstAssign* ଅଚଳ initActiveCountp
                = ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, activeCountp, VAccess::WRITE},
                                ନୂତନ AstVarRef{flp, activep, VAccess::READ}};
            AstLoop* ଅଚଳ finalLoopp = ନୂତନ AstLoop{flp};
            finalLoopp->addStmtsp(
                ନୂତନ AstLoopTest{flp, finalLoopp,
                                ନୂତନ AstNeq{flp, ନୂତନ AstVarRef{flp, activeCountp, VAccess::READ},
                                           ନୂତନ AstConst{flp, 0}}});
            finalLoopp->addStmtsp(ନୂତନ AstIf{flp, finalCondp, ନୂତନ AstPExprClause{flp},
                                            ନୂତନ AstPExprClause{flp, ମିଥ୍ୟା}});
            finalLoopp->addStmtsp(
                ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, activeCountp, VAccess::WRITE},
                              ନୂତନ AstSub{flp, ନୂତନ AstVarRef{flp, activeCountp, VAccess::READ},
                                         ନୂତନ AstConst{flp, 1}}});

            // Final assertion block
            AstBegin* ଅଚଳ finalp = ନୂତନ AstBegin{flp, m_blockNames.get(""), nullptr, ସତ୍ୟ};
            finalp->addStmtsp(activeCountp);
            finalp->addStmtsp(initActiveCountp);
            finalp->addStmtsp(finalLoopp);

            AstPExpr* ଅଚଳ pexprp = ନୂତନ AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
            VL_RESTORER(m_pexprp);
            VL_RESTORER(m_hasCycleDelay);
            m_pexprp = pexprp;
            m_hasCycleDelay = ମିଥ୍ୟା;
            iterate(bodyp);
            iterate(finalp);

            UASSERT_OBJ(!m_hasCycleDelay, nodep,
                        "s_until cycle delay should have been handled by V3AssertNfa");

            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ;
        }
        AstLoop* ଅଚଳ loopp = ନୂତନ AstLoop{flp};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = nodep->lhsp()->unlinkFrBack();
        AstLogAnd* ଅଚଳ loopCondp = ନୂତନ AstLogAnd{flp, lhsp, ନୂତନ AstLogNot{flp, rhsp}};
        loopp->addStmtsp(ନୂତନ AstLoopTest{flp, loopp, loopCondp});
        loopp->addStmtsp(ନୂତନ AstEventControl{flp, new_Sen_ବୃକ୍ଷ(nodep), nullptr});

        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsCopyp = rhsp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ passCondp
            = nodep->isOverlapping() ? ନୂତନ AstLogAnd{flp, lhsp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), rhsCopyp}
                                     : rhsCopyp;
        AstBegin* ଅଚଳ beginp = ନୂତନ AstBegin{flp, m_blockNames.get(""), loopp, ସତ୍ୟ};
        beginp->addStmtsp(
            ନୂତନ AstIf{flp, passCondp, ନୂତନ AstPExprClause{flp}, ନୂତନ AstPExprClause{flp, ମିଥ୍ୟା}});

        AstPExpr* ଅଚଳ pexprp = ନୂତନ AstPExpr{flp, beginp, nodep->dtypep()};
        pexprp->user1(1);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }

    ଶୂନ୍ୟ visit(AstDefaultDisable* nodep) override {
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ଶୂନ୍ୟ visit(AstInferredDisable* nodep) override {
        Ast_ଗଣ୍ଠି* newp;
        ଯଦି (m_defaultDisablep) {
            newp = m_defaultDisablep->condp()->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ);
        } ଅନ୍ୟଥା {
            newp = ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalse{}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }

    ଶୂନ୍ୟ visit(AstPropSpec* nodep) override {
        nodep = substitutePropertyCall(nodep);
        iterateAndNextNull(nodep->sensesp());
        ଯଦି (m_senip && m_senip != nodep->sensesp())
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Only one PSL clock allowed per assertion");
        ଅଚଳ AstCover* ଅଚଳ coverp = VN_CAST(nodep->backp(), Cover);
        ଅଚଳ ତାର୍କିକ seqEvent = coverp && coverp->isSeqEvent();
        ଯଦି (!nodep->disablep() && m_defaultDisablep && !seqEvent) {
            nodep->disablep(m_defaultDisablep->condp()->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ));
        }
        m_disablep = nodep->disablep();
        // Unlink and just keep a pointer to it, convert to sentree as needed
        m_senip = nodep->sensesp();
        iterateNull(nodep->disablep());
        ଯଦି (!VN_AS(nodep->backp(), ଗଣ୍ଠି_Cover_Or_Assert)->immediate()) {
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ propDtp = nodep->propp()->dtypep();
            nodep->propp(ନୂତନ AstSampled{nodep->fileline(), nodep->propp()->unlinkFrBack(),
                                        propDtp->dtypep(), ସତ୍ୟ});
        }
        // cover counts non-vacuous matches only (IEEE 1800-2023 16.15.2), so an
        // implication antecedent must hold; assert passes vacuously instead.
        VL_RESTORER(m_underCover);
        m_underCover = VN_IS(nodep->backp(), Cover);
        iterate(nodep->propp());
        iterateNull(nodep->matchCountp());
    }
    ଶୂନ୍ୟ visit(AstPExpr* nodep) override {
        // V3AssertNfa handles multi-cycle property expressions before this pass,
        // so the following unsupported paths are defensive and typically unreached.
        ଯଦି (m_pexprp && m_pexprp->user1()) {  // LCOV_EXCL_START
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Complex property expression inside 'until''");
            nodep->replaceWith(ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ;
        }  // LCOV_EXCL_STOP
        ଯଦି (AstLogNot* ଅଚଳ notp = VN_CAST(nodep->backp(), LogNot)) {
            notp->replaceWith(nodep->unlinkFrBack());
            VL_DO_DANGLING(ଠେଲ_Deletep(notp), notp);
            iterate(nodep);
            ଫେରନ୍ତୁ;
        }
        // Sequence expression as antecedent of implication is not yet supported
        ଯଦି (AstImplication* ଅଚଳ implp = VN_CAST(nodep->backp(), Implication)) {
            ଯଦି (implp->lhsp() == nodep) {  // LCOV_EXCL_START
                implp->v3warn(E_UNSUPPORTED,
                              "Unsupported: Implication with sequence expression as antecedent");
                nodep->replaceWith(ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                ଫେରନ୍ତୁ;
            }  // LCOV_EXCL_STOP
        }
        VL_RESTORER(m_pexprp);
        VL_RESTORER(m_disableSeqIfp);
        m_pexprp = nodep;

        ଯଦି (m_disablep) {
            ଅଚଳ AstSampled* sampledp = nullptr;
            ଯଦି (m_disablep->exists([&sampledp](ଅଚଳ AstSampled* ଅଚଳ sp) {
                    sampledp = sp;
                    ଫେରନ୍ତୁ ସତ୍ୟ;
                })) {
                sampledp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: $sampled inside disabled condition of a sequence");
                m_disablep = ନୂତନ AstConst{m_disablep->fileline(), AstConst::BitFalse{}};
                // always a copy is used, so remove it now
                ଠେଲ_Deletep(m_disablep);  // m_disablep used later
            }
            File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
            // Add counter which counts times the condition turned true
            AstVar* ଅଚଳ disableCntp
                = ନୂତନ AstVar{flp, VVarType::MODULETEMP, m_disableCntNames.get(""),
                             nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            disableCntp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(disableCntp);
            AstVarRef* ଅଚଳ readCntRefp = ନୂତନ AstVarRef{flp, disableCntp, VAccess::READ};
            AstVarRef* ଅଚଳ writeCntRefp = ନୂତନ AstVarRef{flp, disableCntp, VAccess::WRITE};
            AstAssign* ଅଚଳ incrStmtp = ନୂତନ AstAssign{
                flp, writeCntRefp, ନୂତନ AstAdd{flp, readCntRefp, ନୂତନ AstConst{flp, 1}}};
            AstAlways* ଅଚଳ alwaysp
                = ନୂତନ AstAlways{flp, VAlwaysKwd::ALWAYS,
                                ନୂତନ Ast_Sen_ବୃକ୍ଷ{flp, ନୂତନ AstSenItem{flp, VEdgeType::ET_POSEDGE,
                                                                   m_disablep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)}},
                                incrStmtp};
            disableCntp->addNextHere(alwaysp);

            // Store value of that counter at the beginning of sequence evaluation
            AstBegin* ଅଚଳ bodyp = nodep->bodyp();
            AstVar* ଅଚଳ initialCntp = ନୂତନ AstVar{flp, VVarType::BLOCKTEMP, "__VinitialCnt",
                                                   nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            initialCntp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            AstAssign* ଅଚଳ assignp
                = ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, initialCntp, VAccess::WRITE},
                                readCntRefp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)};
            // Prepend to the sequence body to keep statement list structure valid.
            UASSERT_OBJ(!bodyp->name().empty(), bodyp, "Sequence body should be named");
            Ast_ଗଣ୍ଠି* ଅଚଳ newStmtsp = Ast_ଗଣ୍ଠି::addNext<Ast_ଗଣ୍ଠି, Ast_ଗଣ୍ଠି>(initialCntp, assignp);
            bodyp->prependStmtsp(newStmtsp);

            m_disableSeqIfp
                = ନୂତନ AstIf{flp, ନୂତନ AstEq{flp, ନୂତନ AstVarRef{flp, initialCntp, VAccess::READ},
                                           readCntRefp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)}};
            // Delete it, because it is always copied before insetion to the AST
            ଠେଲ_Deletep(m_disableSeqIfp);  // m_disableSeqIfp used later
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultClkEvtVarp);
        VL_RESTORER(m_defaultDisablep);
        VL_RESTORER(m_modp);
        ଯଦି (!VN_IS(nodep, Class)) {
            // A class nested in a module is in the scope of the module's default clocking
            m_defaultClockingp = nullptr;
            m_defaultClkEvtVarp = nullptr;
        }
        m_defaultDisablep = nodep->defaultDisablep();
        m_modp = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstProperty* nodep) override {
        // The body will be visited when will be substituted in place of property reference
        // (AstFuncRef)
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ଶୂନ୍ୟ visit(AstSequence* nodep) override {
        // Sequence declarations are not visited directly; their bodies are inlined
        // at call sites by visit(AstFuncRef*). Collect for post-traversal cleanup.
        m_seqsToCleanup.ଠେଲ_back(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit AssertPreVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        { AssertPreClockingVisitor{nodep}; }
        // Process
        iterate(nodep);
        // Fix up varref names
        ପାଇଁ (AstVarXRef* xrefp : m_xrefsp) xrefp->name(xrefp->varp()->name());
        // Clean up sequence declarations after inlining.
        // Referenced sequences that were inlined have isReferenced cleared.
        // Remaining referenced sequences are in unsupported contexts (e.g. @seq event).
        ପାଇଁ (AstSequence* seqp : m_seqsToCleanup) {
            ଯଦି (seqp->isReferenced()) {
                seqp->v3warn(E_UNSUPPORTED,
                             "Unsupported: sequence referenced outside assertion property");
            } ଅନ୍ୟଥା {
                VL_DO_DANGLING(seqp->unlinkFrBack()->delete_ବୃକ୍ଷ(), seqp);
            }
        }
    }
    ~AssertPreVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Top Assert class

ଶୂନ୍ୟ V3AssertPre::assertPreAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { AssertPreVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("assertpre", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
