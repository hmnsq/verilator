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
    शून्य दर्शय(AstNetlist* nodep) override { iterate_And_अग्र_Const_Null(nodep->modulesp()); }
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override { iterate_And_अग्र_Const_Null(nodep->stmtsp()); }
    शून्य दर्शय(Ast_Gen_खण्डम्* nodep) override { iterate_And_अग्र_Const_Null(nodep->itemsp()); }
    शून्य दर्शय(AstClocking* nodep) override {
        प्रति (Ast_ग्रन्थिः* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            यदि (AstClockingItem* ध्रुव citemp = VN_CAST(itemp, ClockingItem)) {
                citemp->user3p(nodep);
            }
        }
    }
    // Clocking blocks are only in modules and their generate blocks
    शून्य दर्शय(Ast_ग्रन्थिः*) override {}

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट AssertPreClockingVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~AssertPreClockingVisitor() override = स्वभाव;
};

//######################################################################
// Assert class functions

वर्ग AssertPreVisitor final : सार्वजनिक VNVisitor {
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
    ध्रुव VNUser1_अन्तर्_Use m_inuser1;
    ध्रुव VNUser2_अन्तर्_Use m_inuser2;
    ध्रुव VNUser3_अन्तर्_Use m_inuser3;
    // STATE
    // Current context:
    AstNetlist* ध्रुव m_netlistp = शून्यसूचकः;  // Current netlist
    Ast_ग्रन्थिः_Module* m_modp = शून्यसूचकः;  // Current module
    AstClocking* m_clockingp = शून्यसूचकः;  // Current clocking block
    // Reset each module:
    AstClocking* m_defaultClockingp = शून्यसूचकः;  // Default clocking for current module
    AstVar* m_defaultClkEvtVarp = शून्यसूचकः;  // Event var for default clocking (for ##0)
    AstDefaultDisable* m_defaultDisablep = शून्यसूचकः;  // Default disable for current module
    // Reset each assertion:
    AstSenItem* m_senip = शून्यसूचकः;  // Last sensitivity
    // Reset each always:
    AstSenItem* m_seniAlwaysp = शून्यसूचकः;  // Last sensitivity in always
    // Reset each assertion:
    Ast_ग्रन्थिः_व्यञ्जकम्* m_disablep = शून्यसूचकः;  // Last disable
    AstIf* m_disableSeqIfp = शून्यसूचकः;  // Used for handling disable iff in sequences
    AstPExpr* m_pexprp = शून्यसूचकः;  // Last AstPExpr
    तार्किक m_underCover = असत्य;  // True if the enclosing assertion is a cover
    // Other:
    V3_Unique_नामानि m_आवृत्तिः_Dly_नामानि{"__VcycleDly"};  // Cycle delay counter name generator
    V3_Unique_नामानि m_cons_Rep_नामानि{"__VconsRep"};  // Consecutive repetition counter name generator
    V3_Unique_नामानि m_goto_Rep_नामानि{"__VgotoRep"};  // Goto repetition counter name generator
    V3_Unique_नामानि m_non_Cons_Rep_नामानि{"__VnonConsRep"};  // Nonconsecutive rep name generator
    V3_Unique_नामानि m_disable_Cnt_नामानि{"__VdisableCnt"};  // Disable condition counter name generator
    V3_Unique_नामानि m_खण्डम्_नामानि{"__VassertBlock"};  // Names of blocks with temporaries
    V3_Unique_नामानि m_prop_Var_नामानि{"__Vpropvar"};  // Property-local variable name generator
    V3_Unique_नामानि m_सक्रिय_नामानि{"__VassertsActive"};  // Active asserts map name generator
    V3_Unique_नामानि m_driven_नामानि{"__VclockingDriven"};  // Clockvar drive flag name generator
    तार्किक m_अन्तर्_नियोजय = असत्य;  // True if in an AssignNode
    तार्किक m_अन्तर्_नियोजय_Dly_Lhs = असत्य;  // True if in AssignDly's LHS
    तार्किक m_अन्तर्_Synch_Drive = असत्य;  // True if in synchronous drive
    तार्किक m_अन्तर्_Concat_नियोजय = असत्य;  // True if in assignment to a concatenation
    std::सदिशम्<SynchDrive> m_drives;  // Clockvars written by the synchronous drive
    तार्किक m_has_आवृत्तिः_Delay = असत्य;  // True if node has cycle delay beneath
    std::सदिशम्<AstVarXRef*> m_xrefsp;  // list of xrefs that need name fixup
    std::सदिशम्<AstSequence*> m_seqs_गन्तव्यम्_Cleanup;  // Sequences to clean up after traversal

    // METHODS

    // Flag set by each drive of an output clockvar, added to the module with its clocking item
    AstVar* getCreateDrivenVarp(AstClockingItem* itemp) {
        यदि (!itemp->user2p()) {
            AstVar* ध्रुव varp = नूतन AstVar{itemp->fileline(), VVarType::MODULETEMP,
                                            m_driven_नामानि.get(itemp), itemp->findBitDType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            itemp->user2p(varp);
        }
        प्रत्यागम VN_AS(itemp->user2p(), Var);
    }
    // Reference to a variable in the scope of a driven clockvar, made like the clockvar's
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* newDriveRefp(File_पङ्क्तिः* flp, ध्रुव SynchDrive& drive, AstVar* varp,
                                     ध्रुव VAccess& access) {
        यदि (ध्रुव AstVarXRef* ध्रुव xrefp = VN_CAST(drive.refp, VarXRef)) {
            प्रत्यागम नूतन AstVarXRef{flp, varp, xrefp->dotted(), access};
        } अन्यथा यदि (ध्रुव AstMemberSel* ध्रुव selp = VN_CAST(drive.refp, MemberSel)) {
            // The interface expression is evaluated again, so it must not have side effects;
            // cloneTreePure warns if it has any
            AstMemberSel* ध्रुव newSelp
                = नूतन AstMemberSel{flp, selp->fromp()->clone_वृक्षः_विशुद्धम्(असत्य), varp};
            newSelp->access(access);
            प्रत्यागम newSelp;
        }
        प्रत्यागम नूतन AstVarRef{flp, varp, access};
    }
    // Assignment setting the drive flag of a driven clockvar
    Ast_नियोजय* newDrivenSetp(File_पङ्क्तिः* flp, ध्रुव SynchDrive& drive) {
        AstVar* ध्रुव varp = getCreateDrivenVarp(drive.itemp);
        Ast_नियोजय* ध्रुव setp = नूतन Ast_नियोजय{flp, newDriveRefp(flp, drive, varp, VAccess::WRITE),
                                              नूतन AstConst{flp, AstConst::BitTrue{}}};
        setp->user1(सत्य);
        प्रत्यागम setp;
    }
    // Clocking block of a clocking item
    स्थिर AstClocking* clocking_सम्बन्धि(ध्रुव AstClockingItem* itemp) {
        UASSERT_OBJ(itemp->user3p(), itemp, "Clocking item not captured in a clocking block");
        प्रत्यागम VN_AS(itemp->user3p(), Clocking);
    }
    // Clocking event of a driven clockvar, referenced from the drive like the clockvar, or
    // nullptr if unsupported, which is reported on the drive's cycle delay
    Ast_Sen_वृक्षः* newDriveSentreep(AstDelay* delayp, ध्रुव SynchDrive& drive) {
        Ast_Sen_वृक्षः* ध्रुव origp = clocking_सम्बन्धि(drive.itemp)->sentreep();
        यदि (VN_IS(drive.refp, VarRef)) प्रत्यागम origp->clone_वृक्षः(असत्य);
        यदि (origp->exists([](ध्रुव AstVarXRef*) { प्रत्यागम सत्य; })) {
            // The reference is relative to the clocking block
            delayp->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in synchronous drive to"
                                          " clockvar in another scope, whose clocking event"
                                          " has a hierarchical reference");
            प्रत्यागम शून्यसूचकः;
        }
        ध्रुव AstMemberSel* ध्रुव selp = VN_CAST(drive.refp, MemberSel);
        // Processes outside classes evaluate their events also when not waiting on them,
        // possibly before the interface reference is set, so the event is guarded against a
        // null reference, which needs an integral value; scheduling does not guard it
        ध्रुव तार्किक guardNull = selp && !VN_IS(m_modp, Class);
        यदि (guardNull) {
            प्रति (AstSenItem* itemp = origp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                यदि (!itemp->sensp()->dtypep()->skipRefp()->isIntegralOrPacked()) {
                    delayp->v3warn(E_UNSUPPORTED,
                                   "Unsupported: cycle delay in synchronous drive through"
                                   " an interface reference outside a class, to clocking"
                                   " block with non-integral clocking event");
                    प्रत्यागम शून्यसूचकः;
                }
            }
        }
        // Interface declaring the clocking block, if driven through an interface reference
        AstIface* ध्रुव ifacep
            = selp ? VN_AS(selp->fromp()->dtypep()->skipRefp(), IfaceRefDType)->iface_द्वारा_Cellp()
                   : शून्यसूचकः;
        Ast_Sen_वृक्षः* ध्रुव sentreep = origp->clone_वृक्षः(असत्य);
        std::सदिशम्<AstVarRef*> refps;
        sentreep->foreach([&](AstVarRef* refp) {
            यदि (!refp->classOrPackagep()) refps.प्रक्षिप_पृष्ठभागः(refp);
        });
        प्रति (AstVarRef* ध्रुव refp : refps) {
            AstVar* ध्रुव varp = refp->varp();
            यदि (ifacep) {
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
        यदि (guardNull) {
            प्रति (AstSenItem* itemp = sentreep->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                guardNullIface(itemp, selp->fromp());
            }
        }
        प्रत्यागम sentreep;
    }
    // Make an integral clocking event through an interface reference false while the reference
    // is null
    स्थिर शून्य guardNullIface(AstSenItem* sensesp, Ast_ग्रन्थिः_व्यञ्जकम्* fromp) {
        File_पङ्क्तिः* ध्रुव flp = sensesp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव sensp = sensesp->sensp()->unlink_Fr_पृष्ठभागः();
        sensesp->sensp(नूतन AstCond{flp, newNotNullp(flp, fromp), sensp,
                                   नूतन AstConst{flp, AstConst::DTyped{}, sensp->dtypep()}});
        यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = sensesp->condp()) {
            condp->unlink_Fr_पृष्ठभागः();
            sensesp->condp(नूतन AstLogAnd{flp, newNotNullp(flp, fromp), condp});
        }
    }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* newNotNullp(File_पङ्क्तिः* flp, Ast_ग्रन्थिः_व्यञ्जकम्* fromp) {
        प्रत्यागम नूतन AstNeq{flp, fromp->clone_वृक्षः_विशुद्धम्(असत्य), नूतन AstConst{flp, AstConst::Null{}}};
    }
    // Procedural '##0': if the default clocking event has not yet occurred in this time step,
    // wait for it; otherwise continue without suspension (IEEE 1800-2023 14.11)
    AstIf* new_आवृत्तिः_Delay_Zerop(File_पङ्क्तिः* flp) ध्रुव {
        AstVar* ध्रुव evtVarp = m_defaultClkEvtVarp;
        UASSERT_OBJ(evtVarp, flp, "Default clocking event var not pre-created");
        AstCMethodHard* ध्रुव isTriggeredp = नूतन AstCMethodHard{
            flp, नूतन AstVarRef{flp, evtVarp, VAccess::READ}, VCMethod::EVENT_IS_TRIGGERED};
        isTriggeredp->dtype_समुच्चयः_Bit();
        AstEventControl* ध्रुव waitp = नूतन AstEventControl{
            flp,
            नूतन Ast_Sen_वृक्षः{flp, नूतन AstSenItem{flp, VEdgeType::ET_EVENT,
                                               नूतन AstVarRef{flp, evtVarp, VAccess::READ}}},
            शून्यसूचकः};
        प्रत्यागम नूतन AstIf{flp, नूतन AstNot{flp, isTriggeredp}, waitp};
    }

    स्थिर शून्य checkSamplingFuncDType(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, ध्रुव Ast_ग्रन्थिः* exprp) {
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = exprp->dtypep()->skipRefp();
        यदि (!dtypep->isIntegralOrPacked()) {
            nodep->v3error("Expected numeric type, but got a " << dtypep->prettyDTypeNameQ()
                                                               << " data type");
        }
    }

    Ast_Sen_वृक्षः* new_Sen_वृक्षः(Ast_ग्रन्थिः* nodep, Ast_Sen_वृक्षः* useTreep = शून्यसूचकः,
                           Ast_ग्रन्थिः_Cover_Or_Assert* cassertp = शून्यसूचकः) {
        // Create sentree based on clocked or default clock
        // Return nullptr for always
        यदि (useTreep) प्रत्यागम useTreep;
        Ast_Sen_वृक्षः* newp = शून्यसूचकः;
        AstSenItem* senip = m_senip;
        तार्किक fromAlways = असत्य;
        यदि (!senip && m_defaultClockingp) senip = m_defaultClockingp->sentreep()->sensesp();
        यदि (!senip) {
            senip = m_seniAlwaysp;
            fromAlways = सत्य;
        }
        यदि (!senip) {
            nodep->v3error("Concurrent assertion has no clock (IEEE 1800-2023 16.16)\n"
                           << nodep->warn_अधिकम्()
                           << "... Suggest provide a clocking event, a default"
                              " clocking, or a clocked procedural context");
            newp = नूतन Ast_Sen_वृक्षः{nodep->fileline(), शून्यसूचकः};
        } अन्यथा {
            यदि (cassertp && fromAlways) cassertp->senFromAlways(सत्य);
            newp = नूतन Ast_Sen_वृक्षः{nodep->fileline(), senip->clone_वृक्षः(सत्य)};
        }
        प्रत्यागम newp;
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* getSequenceBodyExprp(ध्रुव AstSequence* ध्रुव seqp) ध्रुव {
        // The statements in AstSequence are optional AstVar (ports) followed by body expr.
        Ast_ग्रन्थिः* bodyp = seqp->stmtsp();
        यावत् (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        प्रत्यागम VN_CAST(bodyp, ग्रन्थिः_व्यञ्जकम्);
    }
    AstPropSpec* getPropertyExprp(ध्रुव AstProperty* ध्रुव propp) {
        // Statements in AstProperty: AstVar (ports/local vars),
        // AstInitialStaticStmt/AstInitialAutomaticStmt (var init), AstPropSpec (body).
        Ast_ग्रन्थिः* propExprp = propp->stmtsp();
        यावत् (propExprp
               && (VN_IS(propExprp, Var) || VN_IS(propExprp, InitialStaticStmt)
                   || VN_IS(propExprp, InitialAutomaticStmt))) {
            propExprp = propExprp->nextp();
        }
        प्रत्यागम VN_CAST(propExprp, PropSpec);
    }
    शून्य substituteSequenceCall(AstFuncRef* funcrefp, AstSequence* seqp) {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Inline the sequence body at the call site, replacing the FuncRef
        Ast_ग्रन्थिः_व्यञ्जकम्* bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        // Clone the body expression since the sequence may be referenced multiple times
        Ast_ग्रन्थिः_व्यञ्जकम्* clonedp = bodyExprp->clone_वृक्षः(असत्य);
        // Build substitution map, then do a single traversal to replace all formals
        // (textual substitution per IEEE 16.8.2).
        ध्रुव V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<ध्रुव AstVar*, Ast_ग्रन्थिः_व्यञ्जकम्*> पत्तनम्_Map;
        प्रति (ध्रुव स्वचालित& tconnect : tconnects) {
            पत्तनम्_Map[tconnect.first] = tconnect.second->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            ध्रुव स्वचालित it = पत्तनम्_Map.find(refp->varp());
            यदि (it != पत्तनम्_Map.अन्तः_2()) {
                refp->replaceWith(it->second->clone_वृक्षः(असत्य));
                VL_DO_DANGLING(प्रक्षिप_Deletep(refp), refp);
            }
        });
        प्रति (ध्रुव स्वचालित& tconnect : tconnects) {
            प्रक्षिप_Deletep(tconnect.second->exprp()->unlink_Fr_पृष्ठभागः());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        // Replace the FuncRef with the inlined body
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(funcrefp), funcrefp);
        // Clear referenced flag; sequences with isReferenced==false are deleted in assertPreAll
        seqp->isReferenced(असत्य);
    }
    AstPropSpec* clonePropertyBody(AstFuncRef* funcrefp, ध्रुव AstProperty* propp) {
        AstPropSpec* propExprp = getPropertyExprp(propp);
        // Substitute inner property call before copying in order to not doing the same for
        // each call of outer property call.
        propExprp = substitutePropertyCall(propExprp);
        // Clone subtree after substitution. It is needed, because property might be called
        // multiple times with different arguments.
        propExprp = propExprp->clone_वृक्षः(असत्य);
        // Build substitution maps for formal arguments and property-local
        // variables, then perform a single foreach to apply all replacements.
        // Map port vars to their actual argument expressions
        ध्रुव V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propp->stmtsp());
        std::unordered_map<ध्रुव AstVar*, Ast_ग्रन्थिः_व्यञ्जकम्*> पत्तनम्_Map;
        प्रति (ध्रुव स्वचालित& tconnect : tconnects) {
            पत्तनम्_Map[tconnect.first] = tconnect.second->exprp();
        }

        // Promote property-local variables (non-port vars, IEEE 16.10) to
        // module-level __Vpropvar temps. Cross-cycle persistence is handled
        // by the match item lowering in visit(AstImplication*).
        std::unordered_map<ध्रुव AstVar*, AstVar*> localVarMap;
        प्रति (Ast_ग्रन्थिः* stmtp = propp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (AstVar* ध्रुव varp = VN_CAST(stmtp, Var)) {
                यदि (!varp->isIO()) {
                    ध्रुव string newName = m_prop_Var_नामानि.get(varp);
                    AstVar* ध्रुव newVarp = नूतन AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->सङ्कलय_Stmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        // Single traversal: substitute ports and update local var references
        propExprp->foreach([&](AstVarRef* refp) {
            ध्रुव स्वचालित पत्तनम्_It = पत्तनम्_Map.find(refp->varp());
            यदि (पत्तनम्_It != पत्तनम्_Map.अन्तः_2()) {
                refp->replaceWith(पत्तनम्_It->second->clone_वृक्षः(असत्य));
                VL_DO_DANGLING(प्रक्षिप_Deletep(refp), refp);
                प्रत्यागम;
            }
            ध्रुव स्वचालित localIt = localVarMap.find(refp->varp());
            यदि (localIt != localVarMap.अन्तः_2()) refp->varp(localIt->second);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        // Clean up argument expressions
        प्रति (ध्रुव स्वचालित& tconnect : tconnects) {
            प्रक्षिप_Deletep(tconnect.second->exprp()->unlink_Fr_पृष्ठभागः());
        }
        V3AssertCommon::checkSvaInstanceDelays(propExprp, funcrefp);
        प्रत्यागम propExprp;
    }
    AstPropSpec* substitutePropertyCall(AstPropSpec* nodep) {
        यदि (AstFuncRef* ध्रुव funcrefp = VN_CAST(nodep->propp(), FuncRef)) {
            यदि (ध्रुव AstProperty* ध्रुव propp = VN_CAST(funcrefp->taskp(), Property)) {
                AstPropSpec* ध्रुव propExprp = clonePropertyBody(funcrefp, propp);

                // Handle case with 2 disable iff statement (IEEE 1800-2023 16.12.1)
                यदि (nodep->disablep() && propExprp->disablep()) {
                    nodep->v3error("disable iff expression before property call and in its "
                                   "body is not legal");
                    प्रक्षिप_Deletep(propExprp->disablep()->unlink_Fr_पृष्ठभागः());
                }
                // If disable iff is in outer property, move it to inner
                यदि (nodep->disablep()) {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव disablep = nodep->disablep()->unlink_Fr_पृष्ठभागः();
                    propExprp->disablep(disablep);
                }

                यदि (nodep->sensesp() && propExprp->sensesp()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Clock event before property call and in its body");
                    प्रक्षिप_Deletep(propExprp->sensesp()->unlink_Fr_पृष्ठभागः());
                }
                // If clock event is in outer property, move it to inner
                यदि (nodep->sensesp()) {
                    AstSenItem* ध्रुव sensesp = nodep->sensesp();
                    sensesp->unlink_Fr_पृष्ठभागः();
                    propExprp->sensesp(sensesp);
                }

                // Now substitute property reference with property body
                nodep->replaceWith(propExprp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                प्रत्यागम propExprp;
            }
        }
        प्रत्यागम nodep;
    }

    शून्य substituteNestedPropertyCall(AstFuncRef* funcrefp, ध्रुव AstProperty* propp) {
        AstPropSpec* ध्रुव specp = clonePropertyBody(funcrefp, propp);
        Ast_ग्रन्थिः* replp = शून्यसूचकः;
        यदि (specp->disablep()) {
            funcrefp->v3error("Illegal 'disable iff' in a nested property instance"
                              " (IEEE 1800-2023 16.12.1)");
        } अन्यथा यदि (specp->sensesp()) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: clock event in a nested property instance");
        } अन्यथा यदि (specp->propp()->exists(
                       [](ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* np) { प्रत्यागम np->is_Multi_आवृत्तिः_Sva(); })) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: multi-cycle body in a nested property instance");
        } अन्यथा {
            replp = specp->propp()->unlink_Fr_पृष्ठभागः();
        }
        यदि (!replp) replp = नूतन AstConst{funcrefp->fileline(), AstConst::BitFalse{}};
        funcrefp->replaceWith(replp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(funcrefp), funcrefp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(specp), specp);
    }

    // VISITORS
    //========== Statements
    शून्य दर्शय(AstClocking* ध्रुव nodep) override {
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        UINFO(8, "   CLOCKING" << nodep);
        यदि (nodep->isDefault()) {
            यदि (m_defaultClockingp) {
                nodep->v3error("Only one default clocking block allowed per module"
                               " (IEEE 1800-2023 14.12)");
            } अन्यथा {
                m_defaultClockingp = nodep;
                m_defaultClkEvtVarp = nodep->ensureEventp();
            }
        }
        iterateChildren(nodep);
        यदि (nodep->eventp()) nodep->सङ्कलय_अग्र_अत्र(nodep->eventp()->unlink_Fr_पृष्ठभागः());
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
    }
    शून्य दर्शय(AstModportClockingRef* ध्रुव nodep) override {
        // It has to be converted to a list of ModportClockingVarRefs,
        // because clocking blocks are removed in this pass
        प्रति (Ast_ग्रन्थिः* itemp = nodep->clockingp()->itemsp(); itemp; itemp = itemp->nextp()) {
            यदि (AstClockingItem* ध्रुव citemp = VN_CAST(itemp, ClockingItem)) {
                यदि (AstVar* ध्रुव varp
                    = citemp->varp() ? citemp->varp() : VN_AS(citemp->user1p(), Var)) {
                    AstModportVarRef* ध्रुव modVarp = नूतन AstModportVarRef{
                        nodep->fileline(), varp->name(), citemp->direction()};
                    modVarp->varp(varp);
                    nodep->सङ्कलय_अग्र_अत्र(modVarp);
                    यदि (citemp->direction().isOutput()) {
                        // Drives through the modport set the drive flag
                        AstVar* ध्रुव drivenVarp = getCreateDrivenVarp(citemp);
                        AstModportVarRef* ध्रुव drivenRefp = नूतन AstModportVarRef{
                            nodep->fileline(), drivenVarp->name(), VDirection::OUTPUT};
                        drivenRefp->varp(drivenVarp);
                        nodep->सङ्कलय_अग्र_अत्र(drivenRefp);
                    }
                }
            }
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
    }
    शून्य दर्शय(AstClockingItem* ध्रुव nodep) override {
        // Get a ref to the sampled/driven variable
        AstVar* ध्रुव varp = nodep->varp();
        यदि (!varp) {
            // Unused item
            प्रत्यागम;
        }
        // Flag set by drives of an output clockvar, possibly visited before this item
        यदि (nodep->direction().isOutput()) m_modp->सङ्कलय_Stmtsp(getCreateDrivenVarp(nodep));
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        V3Const::constifyEdit(nodep->skewp());
        यदि (!VN_IS(nodep->skewp(), Const)) {
            nodep->skewp()->v3error("Skew must be constant (IEEE 1800-2023 14.4)");
            प्रत्यागम;
        }
        AstConst* ध्रुव skewp = VN_AS(nodep->skewp(), Const);
        यदि (skewp->अङ्कः().isNegative()) skewp->v3error("Skew cannot be negative");
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = nodep->exprp();
        varp->name(m_clockingp->name() + "__DOT__" + varp->name());
        m_clockingp->सङ्कलय_अग्र_अत्र(varp->unlink_Fr_पृष्ठभागः());
        nodep->user1p(varp);
        varp->user1p(nodep);
        यदि (nodep->direction() == VDirection::OUTPUT) {
            exprp->foreach([](ध्रुव Ast_ग्रन्थिः_Var_Ref* varrefp) {
                // Prevent confusing BLKANDNBLK warnings on clockvars due to generated assignments
                varrefp->fileline()->warnOff(V3_Error_संहिता::BLKANDNBLK, सत्य);
                // Mark the driven signal so later passes (e.g. V3Inst port
                // connection) know the drive flows out from inside the cell.
                // Item expressions are plain signal references, so only the
                // driven signal is marked, never index selects.
                varrefp->varp()->isClockingDriven(सत्य);
            });
            AstVarRef* ध्रुव skewedReadRefp = नूतन AstVarRef{flp, varp, VAccess::READ};
            skewedReadRefp->user1(सत्य);
            // Initialize the clockvar
            AstVarRef* ध्रुव skewedWriteRefp = नूतन AstVarRef{flp, varp, VAccess::WRITE};
            skewedWriteRefp->user1(सत्य);
            AstInitialStatic* ध्रुव initClockvarp = नूतन AstInitialStatic{
                flp, नूतन Ast_नियोजय{flp, skewedWriteRefp, exprp->clone_वृक्षः_विशुद्धम्(असत्य)}};
            m_modp->सङ्कलय_Stmtsp(initClockvarp);
            // Each drive sets a flag, so the signal is also assigned when driven with the same
            // value again, e.g. after another assignment to the signal (IEEE 1800-2023 14.16.2)
            AstVar* ध्रुव drivenVarp = getCreateDrivenVarp(nodep);
            m_modp->सङ्कलय_Stmtsp(नूतन AstInitialStatic{
                flp, नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                   नूतन AstConst{flp, AstConst::BitFalse{}}}});
            // Assign the clockvar to the actual var if it was driven
            Ast_नियोजय* ध्रुव assignp
                = नूतन Ast_नियोजय{flp, exprp->clone_वृक्षः_विशुद्धम्(असत्य), skewedReadRefp};
            AstIf* ध्रुव ifp
                = नूतन AstIf{flp, नूतन AstVarRef{flp, drivenVarp, VAccess::READ},
                            नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                          नूतन AstConst{flp, AstConst::BitFalse{}}}};
            ifp->सङ्कलय_Thensp(assignp);
            // Only in the time step of the clocking event
            AstCMethodHard* ध्रुव trigp = नूतन AstCMethodHard{
                nodep->fileline(), नूतन AstVarRef{flp, m_clockingp->ensureEventp(), VAccess::READ},
                VCMethod::EVENT_IS_TRIGGERED};
            trigp->dtype_समुच्चयः_Bit();
            ifp->condp(नूतन AstLogAnd{flp, ifp->condp()->unlink_Fr_पृष्ठभागः(), trigp});
            // A skew rounding to no time precision ticks is a zero skew
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव ticksp
                = V3Timing::delay_गन्तव्यम्_Ticks(skewp->clone_वृक्षः(असत्य), m_modp->timeunit());
            ध्रुव तार्किक zeroSkew = ticksp->isZero();
            VL_DO_DANGLING(ticksp->delete_वृक्षः(), ticksp);
            यदि (zeroSkew || !skewp->fileline()->timingOn()) {
                // Drive the var in Re-NBA (IEEE 1800-2023 14.16), ignoring skews where timing is
                // turned off
                Ast_Sen_वृक्षः* senTreep = m_clockingp->sentreep()->clone_वृक्षः(असत्य);
                senTreep->सङ्कलय_Sensesp(नूतन AstSenItem{
                    flp, VEdgeType::ET_परिवर्तितम्, नूतन AstVarRef{flp, drivenVarp, VAccess::READ}});
                m_clockingp->सङ्कलय_अग्र_अत्र(नूतन AstAlwaysReactive{flp, senTreep, ifp});
            } अन्यथा {
                // Fork only the assignment, so this AlwaysPostponed can be retriggered
                // before the assignment happens
                AstFork* ध्रुव forkp = नूतन AstFork{flp, VJoinType::JOIN_NONE};
                forkp->तात्कालिकम्_Start(सत्य);
                forkp->सङ्कलय_Forksp(नूतन AstBegin{flp, "", assignp->unlink_Fr_पृष्ठभागः(), सत्य});
                ifp->सङ्कलय_Thensp(forkp);
                // Read the clockvar in Postponed, so a drive late in the time step is applied
                // at this event plus the skew (IEEE 1800-2023 14.16)
                m_clockingp->सङ्कलय_अग्र_अत्र(नूतन AstAlwaysPostponed{flp, ifp});
                यदि (v3Global.opt.timing().is_समुच्चयः_True()) {
                    AstDelay* ध्रुव delayp = नूतन AstDelay{flp, skewp->unlink_Fr_पृष्ठभागः(), असत्य};
                    delayp->timeunit(m_modp->timeunit());
                    assignp->timingControlp(delayp);
                } अन्यथा यदि (v3Global.opt.timing().is_समुच्चयः_False()) {
                    nodep->v3warn(E_NOTIMING,
                                  "Clocking output skew greater than #0 requires --timing");
                } अन्यथा {
                    nodep->v3warn(E_NEEDTIMINGOPT,
                                  "Use --timing or --no-timing to specify how "
                                  "clocking output skew greater than #0 should be handled");
                }
            }
        } अन्यथा यदि (nodep->direction() == VDirection::निवेशः) {
            // Ref to the clockvar
            AstVarRef* ध्रुव refp = नूतन AstVarRef{flp, varp, VAccess::WRITE};
            refp->user1(सत्य);
            यदि (skewp->अङ्कः().is1_चरणः()) {
                // #1step means the value that is sampled is always the signal's last value
                // before the clock edge (IEEE 1800-2023 14.4)
                AstSampled* ध्रुव sampledp
                    = नूतन AstSampled{flp, exprp->clone_वृक्षः_विशुद्धम्(असत्य), exprp->dtypep(), सत्य};
                Ast_नियोजय* ध्रुव assignp = नूतन Ast_नियोजय{flp, refp, sampledp};
                m_clockingp->सङ्कलय_अग्र_अत्र(नूतन AstAlways{
                    flp, VAlwaysKwd::ALWAYS, m_clockingp->sentreep()->clone_वृक्षः(असत्य), assignp});
            } अन्यथा यदि (skewp->isZero()) {
                // #0 means the var has to be sampled in Observed (IEEE 1800-2023 14.13)
                Ast_नियोजय* ध्रुव assignp = नूतन Ast_नियोजय{flp, refp, exprp->clone_वृक्षः_विशुद्धम्(असत्य)};
                m_clockingp->सङ्कलय_अग्र_अत्र(नूतन AstAlwaysObserved{
                    flp, m_clockingp->sentreep()->clone_वृक्षः(असत्य), assignp});
            } अन्यथा {
                // Create a queue where we'll store sampled values with timestamps
                AstSampleQueueDType* ध्रुव queueDtp
                    = नूतन AstSampleQueueDType{flp, exprp->dtypep()};
                m_netlistp->typeTablep()->सङ्कलय_Typesp(queueDtp);
                AstVar* ध्रुव queueVarp
                    = नूतन AstVar{flp, VVarType::MODULETEMP, "__Vqueue__" + varp->name(), queueDtp};
                queueVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_clockingp->सङ्कलय_अग्र_अत्र(queueVarp);
                // Create a process like this:
                //     always queue.push(<sampled var>);
                AstCMethodHard* ध्रुव pushp = नूतन AstCMethodHard{
                    flp, नूतन AstVarRef{flp, queueVarp, VAccess::WRITE}, VCMethod::DYN_प्रक्षिप,
                    नूतन AstTime{nodep->fileline(), m_modp->timeunit()}};
                pushp->सङ्कलय_Pinsp(exprp->clone_वृक्षः_विशुद्धम्(असत्य));
                pushp->dtype_समुच्चयः_Void();
                m_clockingp->सङ्कलय_अग्र_अत्र(
                    नूतन AstAlways{flp, VAlwaysKwd::ALWAYS, शून्यसूचकः, pushp->makeStmt()});
                // Create a process like this:
                //     always @<clocking event> queue.pop(<skew>, /*out*/<skewed var>);
                AstCMethodHard* ध्रुव popp = नूतन AstCMethodHard{
                    flp, नूतन AstVarRef{flp, queueVarp, VAccess::READWRITE}, VCMethod::DYN_निष्कासय,
                    नूतन AstTime{nodep->fileline(), m_modp->timeunit()}};
                popp->सङ्कलय_Pinsp(skewp->unlink_Fr_पृष्ठभागः());
                refp->access(VAccess::READWRITE);  // Only conditionally assigned
                popp->सङ्कलय_Pinsp(refp);
                popp->dtype_समुच्चयः_Void();
                m_clockingp->सङ्कलय_अग्र_अत्र(नूतन AstAlways{flp, VAlwaysKwd::ALWAYS,
                                                       m_clockingp->sentreep()->clone_वृक्षः(असत्य),
                                                       popp->makeStmt()});
            }
        } अन्यथा {
            nodep->v3fatalSrc("Invalid direction");
        }
    }
    शून्य दर्शय(AstDelay* nodep) override {
        m_has_आवृत्तिः_Delay |= nodep->is_आवृत्तिः_Delay();
        // Only cycle delays are relevant in this stage; also only process once
        यदि (!nodep->is_आवृत्तिः_Delay()) {
            यदि (m_अन्तर्_Synch_Drive) {
                nodep->v3error("Only cycle delays can be used in synchronous drives"
                               " (IEEE 1800-2023 14.16)");
            }
            iterateChildren(nodep);
            प्रत्यागम;
        }
        यदि (m_अन्तर्_नियोजय && !m_अन्तर्_Synch_Drive) {
            nodep->v3error("Cycle delays not allowed as intra-assignment delays"
                           " (IEEE 1800-2023 14.11)");
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
            प्रत्यागम;
        }
        यदि (nodep->stmtsp()) nodep->सङ्कलय_अग्र_अत्र(nodep->stmtsp()->unlink_Fr_पृष्ठभागः_With_अग्र());
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* valuep = V3Const::constifyEdit(nodep->lhsp()->unlink_Fr_पृष्ठभागः());
        ध्रुव AstConst* ध्रुव constp = VN_CAST(valuep, Const);
        यदि (!constp && m_pexprp) {
            // V3AssertNfa handles non-const delays in sequences before this pass and
            // replaces the property; this branch should never be reached.
            nodep->v3fatalSrc("Non-constant cycle delay in assertion: "
                              "should have been caught by V3AssertNfa");
        } अन्यथा यदि (constp && constp->isZero()) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(valuep), valuep);
            यदि (m_अन्तर्_Synch_Drive) {
                // ##0 has no effect in synchronous drives (IEEE 1800-2023 14.11)
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
                प्रत्यागम;
            }
            यदि (m_pexprp) {
                // ##0 in sequence context = zero delay = same clock tick
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
                प्रत्यागम;
            }
            // Procedural ##0: synchronize with default clocking event (IEEE 1800-2023 14.11)
            यदि (!m_defaultClockingp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
                प्रत्यागम;
            }
            nodep->replaceWith(new_आवृत्तिः_Delay_Zerop(flp));
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            प्रत्यागम;
        }
        Ast_Sen_वृक्षः* sentreep = शून्यसूचकः;
        यदि (!m_drives.रिक्तानि()) {
            यदि (m_अन्तर्_Concat_नियोजय) {
                // Also the clockvars may be of different clocking blocks
                nodep->v3error("Cycle delays cannot be used in synchronous drives to a"
                               " concatenation (IEEE 1800-2023 14.16)");
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
                VL_DO_DANGLING(प्रक्षिप_Deletep(valuep), valuep);
                प्रत्यागम;
            }
            // Count the cycles of the driven clockvar's clocking block (IEEE 1800-2023 14.16)
            sentreep = newDriveSentreep(nodep, m_drives.front());
            यदि (!sentreep) {
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
                VL_DO_DANGLING(प्रक्षिप_Deletep(valuep), valuep);
                प्रत्यागम;
            }
        } अन्यथा यदि (!m_defaultClockingp) {
            यदि (!m_pexprp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
                VL_DO_DANGLING(प्रक्षिप_Deletep(valuep), valuep);
                प्रत्यागम;
            }
            sentreep = नूतन Ast_Sen_वृक्षः{flp, m_senip->clone_वृक्षः(असत्य)};
        } अन्यथा {
            sentreep = m_defaultClockingp->sentreep()->clone_वृक्षः(असत्य);
        }
        AstEventControl* ध्रुव controlp
            = नूतन AstEventControl{nodep->fileline(), sentreep, शून्यसूचकः};
        ध्रुव std::string delayName = m_आवृत्तिः_Dly_नामानि.get(nodep);
        Ast_ग्रन्थिः_व्यञ्जकम्* throughoutp
            = nodep->throughoutp() ? nodep->throughoutp()->unlink_Fr_पृष्ठभागः() : शून्यसूचकः;

        AstVar* ध्रुव cntVarp = नूतन AstVar{flp, VVarType::BLOCKTEMP, delayName + "__counter",
                                           nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstBegin* ध्रुव beginp = नूतन AstBegin{flp, delayName + "__block", cntVarp, सत्य};
        // A non-constant delay is counted at runtime. The count is integral of 64 bits, as
        // V3Width makes delays, or real, which is rounded (IEEE 1800-2023 6.12.1)
        यदि (valuep->isDouble()) {
            valuep = नूतन AstRToIRoundS{flp, valuep};
            valuep->dtypeFrom(cntVarp);
        } अन्यथा {
            valuep = नूतन AstSel{flp, valuep, 0, cntVarp->विस्तारः()};
        }
        beginp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, cntVarp, VAccess::WRITE}, valuep});
        // If the count is zero, a synchronous drive waits for none, like with a constant '##0',
        // and a procedural delay waits like a procedural '##0' (IEEE 1800-2023 14.11)
        यदि (!constp && !m_अन्तर्_Synch_Drive) {
            beginp->सङ्कलय_Stmtsp(नूतन AstIf{
                flp,
                नूतन AstEq{flp, नूतन AstVarRef{flp, cntVarp, VAccess::READ}, नूतन AstConst{flp, 0}},
                new_आवृत्तिः_Delay_Zerop(flp)});
        }

        // Throughout: create flag tracking whether condition held every tick
        AstVar* throughoutOkp = शून्यसूचकः;
        यदि (throughoutp) {
            throughoutOkp = नूतन AstVar{flp, VVarType::BLOCKTEMP, delayName + "__throughoutOk",
                                       nodep->findBasicDType(VBasicDTypeKwd::LOGIC_IMPLICIT)};
            throughoutOkp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->सङ्कलय_Stmtsp(throughoutOkp);
            beginp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            नूतन AstConst{flp, AstConst::BitTrue{}}});
            // Check condition at tick 0 (sequence start, before entering loop)
            beginp->सङ्कलय_Stmtsp(
                नूतन AstIf{flp, नूतन AstLogNot{flp, throughoutp->clone_वृक्षः_विशुद्धम्(असत्य)},
                          नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                        नूतन AstConst{flp, AstConst::BitFalse{}}}});
        }

        {
            AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
            // When throughout is present, exit loop early if condition fails
            Ast_ग्रन्थिः_व्यञ्जकम्* loopCondp
                = नूतन AstGt{flp, नूतन AstVarRef{flp, cntVarp, VAccess::READ}, नूतन AstConst{flp, 0}};
            यदि (throughoutOkp) {
                loopCondp = नूतन AstLogAnd{flp, loopCondp,
                                          नूतन AstVarRef{flp, throughoutOkp, VAccess::READ}};
            }
            loopp->सङ्कलय_Stmtsp(नूतन AstLoopTest{flp, loopp, loopCondp});
            loopp->सङ्कलय_Stmtsp(controlp);
            loopp->सङ्कलय_Stmtsp(
                नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, cntVarp, VAccess::WRITE},
                              नूतन AstSub{flp, नूतन AstVarRef{flp, cntVarp, VAccess::READ},
                                         नूतन AstConst{flp, 1}}});
            // Check throughout condition at each tick during delay (IEEE 1800-2023 16.9.9)
            यदि (throughoutp) {
                loopp->सङ्कलय_Stmtsp(
                    नूतन AstIf{flp, नूतन AstLogNot{flp, throughoutp},
                              नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            नूतन AstConst{flp, AstConst::BitFalse{}}}});
            }
            beginp->सङ्कलय_Stmtsp(loopp);
        }
        // Compose wrappers on remaining sequence: throughout gate (inner), disable iff (outer)
        Ast_ग्रन्थिः* remainp = nodep->nextp() ? nodep->nextp()->unlink_Fr_पृष्ठभागः_With_अग्र() : शून्यसूचकः;
        यदि (throughoutOkp) {
            // If condition failed during delay, fail assertion
            remainp = नूतन AstIf{flp, नूतन AstVarRef{flp, throughoutOkp, VAccess::READ}, remainp,
                                नूतन AstPExprClause{flp, /*pass=*/असत्य}};
        }
        यदि (m_disableSeqIfp && remainp) {
            AstIf* ध्रुव disableSeqIfp = m_disableSeqIfp->clone_वृक्षः(असत्य);
            disableSeqIfp->सङ्कलय_Thensp(remainp);
            remainp = disableSeqIfp;
        }
        यदि (remainp) {
            यदि (throughoutOkp) {
                // throughoutOkp is declared in beginp scope -- check must be inside it
                beginp->सङ्कलय_Stmtsp(remainp);
            } अन्यथा {
                nodep->सङ्कलय_अग्र_अत्र(remainp);
            }
        }
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }
    शून्य दर्शय(Ast_Sen_वृक्षः* nodep) override {
        यदि (m_अन्तर्_Synch_Drive) {
            nodep->v3error("Event controls cannot be used in "
                           "synchronous drives (IEEE 1800-2023 14.16)");
        }

        ध्रुव AstSampled* sampledp;
        यदि (nodep->exists([&sampledp](ध्रुव AstSampled* ध्रुव sp) {
                sampledp = sp;
                प्रत्यागम सत्य;
            })) {
            sampledp->v3warn(E_UNSUPPORTED, "Unsupported: $sampled inside sensitivity list");
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        UINFO(8, " -varref:  " << nodep);
        UINFO(8, " -varref-var-back:  " << nodep->varp()->backp());
        UINFO(8, " -varref-var-user1:  " << nodep->varp()->user1p());
        यदि (AstClockingItem* ध्रुव itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->firstAbovep(),
                ClockingItem)) {
            यदि (nodep->user1()) प्रत्यागम;

            // ensure linking still works, this has to be done only once
            यदि (AstVarXRef* xrefp = VN_CAST(nodep, VarXRef)) {
                UINFO(8, " -clockvarxref-in:  " << xrefp);
                string dotted = xrefp->dotted();
                ध्रुव आकार_t dotPos = dotted.rfind('.');
                dotted.erase(dotPos, string::npos);
                xrefp->dotted(dotted);
                UINFO(8, " -clockvarxref-out: " << xrefp);
                m_xrefsp.emplace_पृष्ठभागः(xrefp);
            }

            यदि (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            यदि (nodep->access().isWriteOrRW()) {
                यदि (itemp->direction() == VDirection::OUTPUT) {
                    यदि (!m_अन्तर्_नियोजय_Dly_Lhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    यदि (m_अन्तर्_नियोजय) {
                        m_अन्तर्_Synch_Drive = सत्य;
                        m_drives.प्रक्षिप_पृष्ठभागः({itemp, nodep});
                    }
                } अन्यथा यदि (itemp->direction() == VDirection::निवेशः) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
            nodep->user1(सत्य);
        }
    }
    शून्य दर्शय(AstMemberSel* nodep) override {
        यदि (AstClockingItem* ध्रुव itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->firstAbovep(),
                ClockingItem)) {
            यदि (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            यदि (nodep->access().isWriteOrRW()) {
                यदि (itemp->direction() == VDirection::OUTPUT) {
                    यदि (!m_अन्तर्_नियोजय_Dly_Lhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    यदि (m_अन्तर्_नियोजय) {
                        m_अन्तर्_Synch_Drive = सत्य;
                        m_drives.प्रक्षिप_पृष्ठभागः({itemp, nodep});
                    }
                } अन्यथा यदि (itemp->direction() == VDirection::निवेशः) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_नियोजय* nodep) override {
        यदि (nodep->user1()) प्रत्यागम;
        VL_RESTORER(m_अन्तर्_नियोजय);
        VL_RESTORER(m_अन्तर्_Synch_Drive);
        VL_RESTORER(m_अन्तर्_Concat_नियोजय);
        VL_RESTORER_CLEAR(m_drives);
        m_अन्तर्_नियोजय = सत्य;
        m_अन्तर्_Synch_Drive = असत्य;
        {
            // The braces of a concatenation are a replication, by one
            ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* lhsp = nodep->lhsp();
            यदि (ध्रुव AstReplicate* ध्रुव repp = VN_CAST(lhsp, Replicate)) lhsp = repp->srcp();
            m_अन्तर्_Concat_नियोजय = VN_IS(lhsp, Concat);
        }
        {
            VL_RESTORER(m_अन्तर्_नियोजय_Dly_Lhs);
            m_अन्तर्_नियोजय_Dly_Lhs = VN_IS(nodep, नियोजय_Dly);
            iterate(nodep->lhsp());
        }
        iterate(nodep->rhsp());
        यदि (nodep->timingControlp()) iterate(nodep->timingControlp());
        // A drive without a cycle delay, also after removing '##0', updates the clockvar now
        यदि (m_अन्तर्_Synch_Drive && !nodep->timingControlp()) {
            Ast_नियोजय* ध्रुव assignp = नूतन Ast_नियोजय{
                nodep->fileline(), nodep->lhsp()->unlink_Fr_पृष्ठभागः(), nodep->rhsp()->unlink_Fr_पृष्ठभागः()};
            assignp->user1(सत्य);
            nodep->replaceWith(assignp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            nodep = assignp;
        }
        // Note the drives when they take effect, after the cycle delay if any. Each clockvar
        // of the LHS is driven, e.g. in a concatenation which other simulators accept, although
        // IEEE 1800-2023 14.16 does not allow it
        Ast_ग्रन्थिः* setsp = शून्यसूचकः;
        प्रति (ध्रुव SynchDrive& drive : m_drives) {
            setsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(setsp, newDrivenSetp(nodep->fileline(), drive));
        }
        यदि (!setsp) प्रत्यागम;
        यदि (AstBegin* ध्रुव beginp = VN_CAST(nodep->timingControlp(), Begin)) {
            beginp->सङ्कलय_Stmtsp(setsp);
        } अन्यथा {
            nodep->सङ्कलय_अग्र_अत्र(setsp);
        }
    }
    शून्य दर्शय(AstAlways* nodep) override {
        iterate_And_अग्र_Null(nodep->sentreep());
        यदि (nodep->sentreep()) m_seniAlwaysp = nodep->sentreep()->sensesp();
        iterate_And_अग्र_Null(nodep->stmtsp());
        m_seniAlwaysp = शून्यसूचकः;
    }

    शून्य दर्शय(Ast_ग्रन्थिः_Cover_Or_Assert* nodep) override {
        यदि (nodep->sentreep()) प्रत्यागम;  // Already processed

        VL_RESTORER(m_senip);
        VL_RESTORER(m_disablep);
        m_senip = शून्यसूचकः;
        m_disablep = शून्यसूचकः;

        // Find Clocking's buried under nodep->exprsp
        iterateChildren(nodep);
        यदि (!nodep->तात्कालिकम्()) nodep->sentreep(new_Sen_वृक्षः(nodep, शून्यसूचकः, nodep));
    }
    शून्य दर्शय(AstFalling* nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* exprp = nodep->exprp()->unlink_Fr_पृष्ठभागः();
        यदि (exprp->विस्तारः() > 1) exprp = नूतन AstSel{fl, exprp, 0, 1};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव futurep = नूतन AstFuture{fl, exprp, new_Sen_वृक्षः(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = नूतन AstAnd{fl, exprp->clone_वृक्षः_विशुद्धम्(असत्य), नूतन AstNot{fl, futurep}};
        exprp->dtype_समुच्चयः_Bit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(AstFell* nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* exprp = nodep->exprp()->unlink_Fr_पृष्ठभागः();
        यदि (exprp->विस्तारः() > 1) exprp = नूतन AstSel{fl, exprp, 0, 1};
        Ast_Sen_वृक्षः* sentreep = nodep->sentreep();
        यदि (sentreep) sentreep->unlink_Fr_पृष्ठभागः();
        AstPast* ध्रुव pastp = नूतन AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_वृक्षः(nodep, sentreep));
        exprp = नूतन AstAnd{fl, pastp, नूतन AstNot{fl, exprp->clone_वृक्षः_विशुद्धम्(असत्य)}};
        exprp->dtype_समुच्चयः_Bit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(AstFuture* nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        Ast_Sen_वृक्षः* ध्रुव sentreep = nodep->sentreep();
        यदि (sentreep) VL_DO_DANGLING(प्रक्षिप_Deletep(sentreep->unlink_Fr_पृष्ठभागः()), sentreep);
        nodep->sentreep(new_Sen_वृक्षः(nodep));
    }
    शून्य दर्शय(AstPast* nodep) override {
        checkSamplingFuncDType(nodep, nodep->exprp());
        यदि (nodep->sentreep()) प्रत्यागम;  // Already processed
        iterateChildren(nodep);
        nodep->sentreep(new_Sen_वृक्षः(nodep));
    }
    शून्य दर्शय(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- Lower standalone exact [*N] (N >= 2) via saturating counter.
        // Range/unbounded forms and SExpr-contained forms are lowered by V3AssertNfa.
        iterateChildren(nodep);
        // V3AssertNfa handles unbounded/ranged forms upstream, so this fast-path
        // is effectively unreachable when NFA is enabled.
        यदि (nodep->unbounded() || nodep->maxCountp()) प्रत्यागम;  // LCOV_EXCL_LINE
        ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep->countp(), Const);
        यदि (VL_UNLIKELY(!constp || constp->गन्तव्यम्_SInt() < 1)) {
            nodep->v3fatalSrc("Consecutive repetition count must be a positive constant"
                              " (should have been caught by V3Width)");
            प्रत्यागम;
        }
        ध्रुव पूर्णाङ्क n = constp->गन्तव्यम्_SInt();
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = nodep->exprp()->unlink_Fr_पृष्ठभागः();
        यदि (n == 1) {
            nodep->replaceWith(exprp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            प्रत्यागम;
        }
        // Saturating counter: if (expr) cnt <= min(cnt+1, N); else cnt <= 0;
        AstVar* ध्रुव cntVarp = नूतन AstVar{flp, VVarType::MODULETEMP, m_cons_Rep_नामानि.get(""),
                                           nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->सङ्कलय_Stmtsp(cntVarp);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव व्यञ्जकम्_Clonep = exprp->clone_वृक्षः_विशुद्धम्(असत्य);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव saturatingIncrp = नूतन AstCond{
            flp,
            नूतन AstLt{flp, नूतन AstVarRef{flp, cntVarp, VAccess::READ},
                      नूतन AstConst{flp, static_cast<uint32_t>(n)}},
            नूतन Ast_सङ्कलय{flp, नूतन AstVarRef{flp, cntVarp, VAccess::READ}, नूतन AstConst{flp, 1u}},
            नूतन AstConst{flp, static_cast<uint32_t>(n)}};
        Ast_नियोजय_Dly* ध्रुव incrAssignp
            = नूतन Ast_नियोजय_Dly{flp, नूतन AstVarRef{flp, cntVarp, VAccess::WRITE}, saturatingIncrp};
        Ast_नियोजय_Dly* ध्रुव resetAssignp = नूतन Ast_नियोजय_Dly{
            flp, नूतन AstVarRef{flp, cntVarp, VAccess::WRITE}, नूतन AstConst{flp, 0u}};
        AstIf* ध्रुव ifp = नूतन AstIf{flp, व्यञ्जकम्_Clonep, incrAssignp, resetAssignp};
        Ast_Sen_वृक्षः* ध्रुव senTreep = new_Sen_वृक्षः(nodep);
        AstAlways* ध्रुव alwaysp = नूतन AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, ifp};
        cntVarp->सङ्कलय_अग्र_अत्र(alwaysp);
        // Match: cnt >= N-1 (previous cycles via NBA) && expr (current cycle)
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव cntCheckp = नूतन AstGte{flp, नूतन AstVarRef{flp, cntVarp, VAccess::READ},
                                                  नूतन AstConst{flp, static_cast<uint32_t>(n - 1)}};
        cntCheckp->dtype_समुच्चयः_Bit();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव matchp = नूतन AstAnd{flp, cntCheckp, exprp};
        matchp->dtype_समुच्चयः_Bit();
        nodep->replaceWith(matchp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(AstRising* nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* exprp = nodep->exprp()->unlink_Fr_पृष्ठभागः();
        यदि (exprp->विस्तारः() > 1) exprp = नूतन AstSel{fl, exprp, 0, 1};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव futurep = नूतन AstFuture{fl, exprp, new_Sen_वृक्षः(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = नूतन AstAnd{fl, नूतन AstNot{fl, exprp->clone_वृक्षः_विशुद्धम्(असत्य)}, futurep};
        exprp->dtype_समुच्चयः_Bit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(AstRose* nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* exprp = nodep->exprp()->unlink_Fr_पृष्ठभागः();
        यदि (exprp->विस्तारः() > 1) exprp = नूतन AstSel{fl, exprp, 0, 1};
        Ast_Sen_वृक्षः* sentreep = nodep->sentreep();
        यदि (sentreep) sentreep->unlink_Fr_पृष्ठभागः();
        AstPast* ध्रुव pastp = नूतन AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_वृक्षः(nodep, sentreep));
        exprp = नूतन AstAnd{fl, नूतन AstNot{fl, pastp}, exprp->clone_वृक्षः_विशुद्धम्(असत्य)};
        exprp->dtype_समुच्चयः_Bit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    स्थिर AstAssocArrayDType* getProcessAssocArrayType(File_पङ्क्तिः* ध्रुव flp) {
        // Type of __VassertsActive___x[std::process]
        Ast_ग्रन्थिः_DType* valp
            = v3Global.rootp()->typeTablep()->findBasicDType(flp, VBasicDTypeKwd::BIT);
        AstClassRefDType* keyp
            = नूतन AstClassRefDType{flp, v3Global.rootp()->stdPackageProcessp(), शून्यसूचकः};
        keyp->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        v3Global.rootp()->typeTablep()->सङ्कलय_Typesp(keyp);
        AstAssocArrayDType* ध्रुव typep = नूतन AstAssocArrayDType{flp, valp, keyp};
        typep->dtypep(typep);
        v3Global.rootp()->typeTablep()->सङ्कलय_Typesp(typep);
        प्रत्यागम typep;
    }
    स्थिर Ast_Stmt_व्यञ्जकम्* getProcessAssocArrayDelete(AstVarRef* ध्रुव refp) {
        // Constructs refp.delete(std::process::self()) statement
        File_पङ्क्तिः* ध्रुव flp = refp->fileline();
        AstCMethodHard* ध्रुव deletep = नूतन AstCMethodHard{
            flp, refp, VCMethod::ASSOC_ERASE, v3Global.rootp()->stdPackageProcessSelfp(flp)};
        deletep->dtypep(refp->findVoidDType());
        प्रत्यागम नूतन Ast_Stmt_व्यञ्जकम्{flp, deletep};
    }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* get_Process_Assoc_Array_आकार(AstVarRef* ध्रुव refp) {
        // Constructs refp.size() statement
        AstCMethodHard* ध्रुव sizep
            = नूतन AstCMethodHard{refp->fileline(), refp, VCMethod::ASSOC_आकार};
        sizep->dtypep(refp->findBasicDType(VBasicDTypeKwd::UINT32));
        प्रत्यागम sizep;
    }
    शून्य दर्शय(AstSEventually* nodep) override {
        UASSERT(v3Global.rootp()->stdPackagep(), "Should be imported");

        Ast_Sen_वृक्षः* ध्रुव sentreep = new_Sen_वृक्षः(nodep);
        यदि (!sentreep->sensesp()) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(sentreep), sentreep);
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            प्रत्यागम;
        }

        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();

        // Track active assertions
        AstVar* ध्रुव activep = नूतन AstVar{flp, VVarType::MODULETEMP, m_सक्रिय_नामानि.get(""),
                                           getProcessAssocArrayType(flp)};
        activep->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->सङ्कलय_Stmtsp(activep);

        // Assertion condition check
        AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
        AstSampled* ध्रुव condp
            = नूतन AstSampled{flp, nodep->exprp()->unlink_Fr_पृष्ठभागः(), शून्यसूचकः, सत्य};
        loopp->सङ्कलय_Stmtsp(नूतन AstLoopTest{flp, loopp, नूतन AstLogNot{flp, condp}});
        loopp->सङ्कलय_Stmtsp(नूतन AstEventControl{flp, sentreep, शून्यसूचकः});

        // Add assertion to the active set
        AstAssocSel* ध्रुव selp = नूतन AstAssocSel{flp, नूतन AstVarRef{flp, activep, VAccess::WRITE},
                                                  v3Global.rootp()->stdPackageProcessSelfp(flp)};
        Ast_नियोजय* ध्रुव incrementp = नूतन Ast_नियोजय{flp, selp, नूतन AstConst{flp, 1}};
        AstPExprClause* ध्रुव clausep = नूतन AstPExprClause{flp};
        Ast_Stmt_व्यञ्जकम्* ध्रुव deletep
            = getProcessAssocArrayDelete(नूतन AstVarRef{flp, activep, VAccess::WRITE});

        // Main assertion block
        AstBegin* ध्रुव bodyp = नूतन AstBegin{flp, m_खण्डम्_नामानि.get(""), शून्यसूचकः, सत्य};
        bodyp->सङ्कलय_Stmtsp(incrementp);
        bodyp->सङ्कलय_Stmtsp(loopp);
        bodyp->सङ्कलय_Stmtsp(clausep);
        bodyp->सङ्कलय_Stmtsp(deletep);

        // Validate assertion condition for each active assert
        AstVar* ध्रुव सक्रिय_Countp = नूतन AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        सक्रिय_Countp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        Ast_नियोजय* ध्रुव init_सक्रिय_Countp
            = नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, सक्रिय_Countp, VAccess::WRITE},
                            get_Process_Assoc_Array_आकार(नूतन AstVarRef{flp, activep, VAccess::READ})};
        AstLoop* ध्रुव finalLoopp = नूतन AstLoop{flp};
        AstIf* ध्रुव finalBodypCondp
            = नूतन AstIf{flp, condp->clone_वृक्षः_विशुद्धम्(असत्य), नूतन AstPExprClause{flp},
                        नूतन AstPExprClause{flp, असत्य}};
        finalLoopp->सङ्कलय_Stmtsp(
            नूतन AstLoopTest{flp, finalLoopp,
                            नूतन AstNeq{flp, नूतन AstVarRef{flp, सक्रिय_Countp, VAccess::READ},
                                       नूतन AstConst{flp, 0}}});
        finalLoopp->सङ्कलय_Stmtsp(finalBodypCondp);
        finalLoopp->सङ्कलय_Stmtsp(
            नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, सक्रिय_Countp, VAccess::WRITE},
                          नूतन AstSub{flp, नूतन AstVarRef{flp, सक्रिय_Countp, VAccess::READ},
                                     नूतन AstConst{flp, 1}}});

        // Final assertion block
        AstBegin* ध्रुव finalp = नूतन AstBegin{flp, m_खण्डम्_नामानि.get(""), शून्यसूचकः, सत्य};
        finalp->सङ्कलय_Stmtsp(सक्रिय_Countp);
        finalp->सङ्कलय_Stmtsp(init_सक्रिय_Countp);
        finalp->सङ्कलय_Stmtsp(finalLoopp);

        VL_RESTORER(m_pexprp);
        m_pexprp = नूतन AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
        VL_RESTORER(m_has_आवृत्तिः_Delay);
        m_has_आवृत्तिः_Delay = असत्य;
        iterate(bodyp);
        iterate(finalp);

        यदि (m_has_आवृत्तिः_Delay) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in s_eventually");
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            VL_DO_DANGLING(m_pexprp->delete_वृक्षः(), m_pexprp);
            प्रत्यागम;
        }

        nodep->replaceWith(m_pexprp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(AstStable* nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* exprp = nodep->exprp()->unlink_Fr_पृष्ठभागः();
        Ast_Sen_वृक्षः* sentreep = nodep->sentreep();
        यदि (sentreep) sentreep->unlink_Fr_पृष्ठभागः();
        AstPast* ध्रुव pastp = नूतन AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_वृक्षः(nodep, sentreep));
        exprp = नूतन AstEq{fl, pastp, exprp->clone_वृक्षः_विशुद्धम्(असत्य)};
        exprp->dtype_समुच्चयः_Bit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(AstSteady* nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* exprp = nodep->exprp()->unlink_Fr_पृष्ठभागः();
        यदि (exprp->विस्तारः() > 1) exprp = नूतन AstSel{fl, exprp, 0, 1};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव futurep = नूतन AstFuture{fl, exprp, new_Sen_वृक्षः(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = नूतन AstEq{fl, exprp->clone_वृक्षः_विशुद्धम्(असत्य), futurep};
        exprp->dtype_समुच्चयः_Bit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(AstSampled* nodep) override {
        iterateChildren(nodep);
        यदि (!nodep->आन्तरिकम्_2()) checkSamplingFuncDType(nodep, nodep->exprp());
    }

    // Validate repetition count: must be a non-negative elaboration-time constant.
    // Shared by goto [->N] and nonconsecutive [=N] repetition.
    // On error, replaces nodep with BitFalse placeholder and returns nullptr.
    ध्रुव AstConst* validate_Rep_गणनम्(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः_व्यञ्जकम्*& countp) {
        countp = V3Const::constifyEdit(countp);
        ध्रुव AstConst* ध्रुव constp = VN_CAST(countp, Const);
        यदि (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(प्रक्षिप_Deletep(countp), countp);
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            प्रत्यागम शून्यसूचकः;
        }
        यदि (constp->गन्तव्यम्_SInt() < 0) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(प्रक्षिप_Deletep(countp), countp);
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            प्रत्यागम शून्यसूचकः;
        }
        यदि (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(प्रक्षिप_Deletep(countp), countp);
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            प्रत्यागम शून्यसूचकः;
        }
        प्रत्यागम constp;
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
            = isNonConsec ? m_non_Cons_Rep_नामानि.get(exprp) : m_goto_Rep_नामानि.get(exprp);
        AstVar* ध्रुव cntVarp = नूतन AstVar{flp, VVarType::BLOCKTEMP, name + "__counter",
                                           exprp->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstBegin* ध्रुव beginp = नूतन AstBegin{flp, name + "__block", cntVarp, सत्य};
        beginp->सङ्कलय_Stmtsp(
            नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, cntVarp, VAccess::WRITE}, नूतन AstConst{flp, 0}});

        // Counting loop: find N occurrences of expr (shared by goto and nonconsec)
        AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
        loopp->सङ्कलय_Stmtsp(नूतन AstLoopTest{flp, loopp,
                                         नूतन AstLt{flp, नूतन AstVarRef{flp, cntVarp, VAccess::READ},
                                                   countp->clone_वृक्षः_विशुद्धम्(असत्य)}});
        // if (expr) cnt++ -- sampled is applied to whole property expr by V3Assert
        loopp->सङ्कलय_Stmtsp(
            नूतन AstIf{flp, exprp,
                      नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, cntVarp, VAccess::WRITE},
                                    नूतन Ast_सङ्कलय{flp, नूतन AstVarRef{flp, cntVarp, VAccess::READ},
                                               नूतन AstConst{flp, 1}}}});
        loopp->सङ्कलय_Stmtsp(नूतन AstIf{
            flp, नूतन AstLt{flp, नूतन AstVarRef{flp, cntVarp, VAccess::READ}, countp},
            नूतन AstEventControl{flp, नूतन Ast_Sen_वृक्षः{flp, sensesp->clone_वृक्षः(असत्य)}, शून्यसूचकः}});

        beginp->सङ्कलय_Stmtsp(loopp);

        यदि (isNonConsec && rhsp) {
            // IEEE 16.9.2: b[=N] = b[->N] ##1 !b[*0:$]
            // Trailing window: ##1 advance, then scan !expr cycles checking rhs.
            // Window closes when expr becomes true again (fail if rhs was never true).
            beginp->सङ्कलय_Stmtsp(नूतन AstEventControl{
                flp, नूतन Ast_Sen_वृक्षः{flp, sensesp->clone_वृक्षः(असत्य)}, शून्यसूचकः});  // ##1
            यदि (!isOverlapped) {
                beginp->सङ्कलय_Stmtsp(नूतन AstEventControl{
                    flp, नूतन Ast_Sen_वृक्षः{flp, sensesp->clone_वृक्षः(असत्य)}, शून्यसूचकः});  // |=>
            }
            // Window loop: check rhs at each !expr cycle (done variable for termination)
            AstVar* ध्रुव समाप्तम्_Varp
                = नूतन AstVar{flp, VVarType::BLOCKTEMP, name + "__done", exprp->findBitDType()};
            समाप्तम्_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->सङ्कलय_Stmtsp(समाप्तम्_Varp);
            beginp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, समाप्तम्_Varp, VAccess::WRITE},
                                            नूतन AstConst{flp, AstConst::BitFalse{}}});
            स्वचालित समुच्चयः_समाप्तम् = [&]() {
                प्रत्यागम नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, समाप्तम्_Varp, VAccess::WRITE},
                                     नूतन AstConst{flp, AstConst::BitTrue{}}};
            };
            AstLoop* ध्रुव windowp = नूतन AstLoop{flp};
            // LoopTest: continue while !done
            windowp->सङ्कलय_Stmtsp(नूतन AstLoopTest{
                flp, windowp, नूतन AstNot{flp, नूतन AstVarRef{flp, समाप्तम्_Varp, VAccess::READ}}});
            // if (expr) { fail; done = 1; } -- window closed, expr true again
            Ast_ग्रन्थिः* ध्रुव failsp = नूतन AstPExprClause{flp, असत्य};
            failsp->सङ्कलय_अग्र(समुच्चयः_समाप्तम्());
            windowp->सङ्कलय_Stmtsp(नूतन AstIf{flp, exprp->clone_वृक्षः_विशुद्धम्(असत्य), failsp});
            // if (rhs) { pass; done = 1; } -- consequent true at this !expr endpoint
            Ast_ग्रन्थिः* ध्रुव passsp = नूतन AstPExprClause{flp, सत्य};
            passsp->सङ्कलय_अग्र(समुच्चयः_समाप्तम्());
            windowp->सङ्कलय_Stmtsp(नूतन AstIf{flp, rhsp, passsp});
            // @(clk) -- advance to next cycle in window
            windowp->सङ्कलय_Stmtsp(
                नूतन AstEventControl{flp, नूतन Ast_Sen_वृक्षः{flp, sensesp->clone_वृक्षः(असत्य)}, शून्यसूचकः});
            beginp->सङ्कलय_Stmtsp(windowp);
        } अन्यथा यदि (isNonConsec) {
            // Standalone nonconsec: ##1 into window, then pass
            beginp->सङ्कलय_Stmtsp(
                नूतन AstEventControl{flp, नूतन Ast_Sen_वृक्षः{flp, sensesp->clone_वृक्षः(असत्य)}, शून्यसूचकः});
            beginp->सङ्कलय_Stmtsp(नूतन AstPExprClause{flp, सत्य});
        } अन्यथा यदि (rhsp) {
            // Goto rep: check consequent once at match endpoint
            यदि (!isOverlapped) {
                beginp->सङ्कलय_Stmtsp(नूतन AstEventControl{
                    flp, नूतन Ast_Sen_वृक्षः{flp, sensesp->clone_वृक्षः(असत्य)}, शून्यसूचकः});
            }
            beginp->सङ्कलय_Stmtsp(नूतन AstIf{flp, rhsp, नूतन AstPExprClause{flp, सत्य},
                                        नूतन AstPExprClause{flp, असत्य}});
        } अन्यथा {
            // Standalone goto: pass after counting
            beginp->सङ्कलय_Stmtsp(नूतन AstPExprClause{flp, सत्य});
        }

        प्रत्यागम नूतन AstPExpr{flp, beginp, exprp->findBitDType()};
    }

    // Reject [=M:N] nonconsecutive range that reached V3AssertPre.
    // [->M:N] is fully handled upstream (NFA path); only [=M:N] still lands
    // here because AstSNonConsRep lowering is not yet implemented there.
    // Replaces the node with BitFalse so lowering continues after the warning.
    तार्किक rejectNonconsecRange(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः_व्यञ्जकम्* maxCountp) {
        यदि (!maxCountp) प्रत्यागम असत्य;
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: [=M:N] nonconsecutive range repetition"
                                     " (IEEE 1800-2023 16.9.2)");
        nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}});
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        प्रत्यागम सत्य;
    }

    शून्य दर्शय(AstSGotoRep* nodep) override {
        // Standalone goto rep (not inside implication antecedent)
        iterateChildren(nodep);
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* countp = nodep->countp()->unlink_Fr_पृष्ठभागः();
        यदि (!validate_Rep_गणनम्(nodep, countp)) प्रत्यागम;
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = nodep->exprp()->unlink_Fr_पृष्ठभागः();
        AstPExpr* ध्रुव pexprp = createRepPExpr(flp, exprp, countp, शून्यसूचकः, सत्य, असत्य);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    शून्य दर्शय(AstSNonConsRep* nodep) override {
        // Standalone nonconsecutive rep (not inside implication antecedent)
        iterateChildren(nodep);
        यदि (rejectNonconsecRange(nodep, nodep->maxCountp())) प्रत्यागम;
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* countp = nodep->countp()->unlink_Fr_पृष्ठभागः();
        यदि (!validate_Rep_गणनम्(nodep, countp)) प्रत्यागम;
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = nodep->exprp()->unlink_Fr_पृष्ठभागः();
        AstPExpr* ध्रुव pexprp = createRepPExpr(flp, exprp, countp, शून्यसूचकः, सत्य, सत्य);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    शून्य दर्शय(AstFuncRef* nodep) override {
        // IEEE 1800-2023 16.8: Inline sequence instances wherever they appear
        // in the expression tree (inside implications, boolean ops, nested refs, etc.)
        यदि (AstSequence* ध्रुव seqp = VN_CAST(nodep->taskp(), Sequence)) {
            substituteSequenceCall(nodep, seqp);
            // The FuncRef has been replaced; do not access nodep after this point.
            // The replacement node will be visited by the parent's iterateChildren.
            प्रत्यागम;
        }
        यदि (ध्रुव AstProperty* ध्रुव propp = VN_CAST(nodep->taskp(), Property)) {
            substituteNestedPropertyCall(nodep, propp);
            प्रत्यागम;
        }
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstImplication* nodep) override {
        यदि (nodep->sentreep()) प्रत्यागम;  // Already processed

        // Top-level followed-by is claimed by V3AssertNfa; anything reaching here
        // is nested inside iff/implies/or. IEEE 1800-2023 16.12.9 permits the
        // composition, but silent lowering would drop non-vacuous-fail semantics.
        यदि (nodep->isFollowedBy()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: followed-by (#-# / #=#) nested inside property operator"
                          " (iff/implies/or) (IEEE 1800-2023 16.12.9)");
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            प्रत्यागम;
        }

        // Handle goto repetition as antecedent before iterateChildren,
        // so the standalone AstSGotoRep visitor doesn't process it
        यदि (AstSGotoRep* ध्रुव gotop = VN_CAST(nodep->lhsp(), SGotoRep)) {
            iterateChildren(gotop);
            iterate_And_अग्र_Null(nodep->rhsp());
            File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
            Ast_ग्रन्थिः_व्यञ्जकम्* countp = gotop->countp()->unlink_Fr_पृष्ठभागः();
            यदि (!validate_Rep_गणनम्(nodep, countp)) प्रत्यागम;
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = gotop->exprp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
            AstPExpr* ध्रुव pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), असत्य);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            प्रत्यागम;
        }

        // Handle nonconsecutive repetition as antecedent before iterateChildren,
        // so the standalone AstSNonConsRep visitor doesn't process it
        यदि (AstSNonConsRep* ध्रुव ncrp = VN_CAST(nodep->lhsp(), SNonConsRep)) {
            iterateChildren(ncrp);
            iterate_And_अग्र_Null(nodep->rhsp());
            यदि (rejectNonconsecRange(nodep, ncrp->maxCountp())) प्रत्यागम;
            File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
            Ast_ग्रन्थिः_व्यञ्जकम्* countp = ncrp->countp()->unlink_Fr_पृष्ठभागः();
            यदि (!validate_Rep_गणनम्(nodep, countp)) प्रत्यागम;
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = ncrp->exprp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
            AstPExpr* ध्रुव pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), सत्य);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            प्रत्यागम;
        }

        iterateChildren(nodep);

        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* lhsp = nodep->lhsp()->unlink_Fr_पृष्ठभागः();

        // Lower sequence match items (IEEE 16.11): (expr, var = val, ...) |-> / |=>
        यदि (Ast_व्यञ्जकम्_Stmt* ध्रुव व्यञ्जकम्_Stmtp = VN_CAST(lhsp, व्यञ्जकम्_Stmt)) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव antExprp = व्यञ्जकम्_Stmtp->resultp()->unlink_Fr_पृष्ठभागः();

            यदि (nodep->isOverlapped()) {
                // |-> : assign to __Vpropvar via always_comb (continuous).
                // The assign evaluates RHS once; V3Sampled snapshots the
                // result so all consequent refs read the same value.
                Ast_ग्रन्थिः* मेलनम्_Assignsp = शून्यसूचकः;
                प्रति (Ast_ग्रन्थिः* stmtp = व्यञ्जकम्_Stmtp->stmtsp(); stmtp;) {
                    Ast_ग्रन्थिः* ध्रुव nextp = stmtp->nextp();
                    यदि (Ast_नियोजय* ध्रुव assignp = VN_CAST(stmtp, नियोजय)) {
                        assignp->unlink_Fr_पृष्ठभागः();
                        मेलनम्_Assignsp = Ast_ग्रन्थिः::सङ्कलय_अग्र_Null(मेलनम्_Assignsp, assignp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(प्रक्षिप_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                यदि (मेलनम्_Assignsp) {
                    AstAlways* ध्रुव alwaysp
                        = नूतन AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, शून्यसूचकः, मेलनम्_Assignsp};
                    m_modp->सङ्कलय_Stmtsp(alwaysp);
                }
            } अन्यथा {
                // |=> : assign to __Vpropvar via NBA in a clocked always block.
                // The NBA commits before the next cycle's sampled snapshot,
                // so the consequent (which already references __Vpropvar)
                // sees the captured value.
                Ast_ग्रन्थिः* मेलनम्_Assignsp = शून्यसूचकः;
                प्रति (Ast_ग्रन्थिः* stmtp = व्यञ्जकम्_Stmtp->stmtsp(); stmtp;) {
                    Ast_ग्रन्थिः* ध्रुव nextp = stmtp->nextp();
                    यदि (Ast_नियोजय* ध्रुव assignp = VN_CAST(stmtp, नियोजय)) {
                        assignp->unlink_Fr_पृष्ठभागः();
                        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव नियोजय_Lhsp = assignp->lhsp()->unlink_Fr_पृष्ठभागः();
                        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव नियोजय_Rhsp = assignp->rhsp()->unlink_Fr_पृष्ठभागः();
                        Ast_नियोजय_Dly* ध्रुव dlyp = नूतन Ast_नियोजय_Dly{flp, नियोजय_Lhsp, नियोजय_Rhsp};
                        VL_DO_DANGLING(प्रक्षिप_Deletep(assignp), assignp);
                        मेलनम्_Assignsp = Ast_ग्रन्थिः::सङ्कलय_अग्र_Null(मेलनम्_Assignsp, dlyp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(प्रक्षिप_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                यदि (मेलनम्_Assignsp) {
                    AstIf* ध्रुव condp
                        = नूतन AstIf{flp, antExprp->clone_वृक्षः_विशुद्धम्(असत्य), मेलनम्_Assignsp};
                    AstAlways* ध्रुव alwaysp
                        = नूतन AstAlways{flp, VAlwaysKwd::ALWAYS, new_Sen_वृक्षः(nodep), condp};
                    m_modp->सङ्कलय_Stmtsp(alwaysp);
                }
            }
        }

        यदि (AstPExpr* ध्रुव pexprp = VN_CAST(rhsp, PExpr)) {
            // Implication with sequence expression on RHS (IEEE 1800-2023 16.11, 16.12.7).
            // The PExpr was lowered from the property expression earlier in this pass.
            // Wrap the PExpr body with the antecedent check so the sequence only
            // starts when the antecedent holds.
            Ast_ग्रन्थिः_व्यञ्जकम्* condp;
            यदि (nodep->isOverlapped()) {
                // Overlapped implication (|->): check antecedent on same cycle.
                // disable iff is applied at the assertion level, not at the
                // antecedent gate, matching the existing non-PExpr overlapped path.
                condp = lhsp;
            } अन्यथा {
                // Non-overlapped implication (|=>): check antecedent from previous cycle
                यदि (m_disablep) {
                    lhsp
                        = नूतन AstAnd{flp, नूतन AstNot{flp, m_disablep->clone_वृक्षः_विशुद्धम्(असत्य)}, lhsp};
                }
                AstPast* ध्रुव pastp
                    = नूतन AstPast{flp, lhsp, शून्यसूचकः, शून्यसूचकः, /* propertyTiming */ सत्य};
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
            प्रति (Ast_ग्रन्थिः* stmtp = bodyp->stmtsp(); stmtp;) {
                Ast_ग्रन्थिः* ध्रुव nextp = stmtp->nextp();
                यदि (!VN_IS(stmtp, Var)) guardp->सङ्कलय_Thensp(stmtp->unlink_Fr_पृष्ठभागः());
                stmtp = nextp;
            }
            bodyp->सङ्कलय_Stmtsp(guardp);
            nodep->replaceWith(pexprp);
            // Don't iterate pexprp here -- it was already iterated when created
            // (in visit(AstSExpr*)), so delays and disable iff are already processed.
        } अन्यथा यदि (nodep->isOverlapped()) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp
                = m_underCover ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstLogAnd{flp, lhsp, rhsp})
                               : नूतन AstLogOr{flp, नूतन AstLogNot{flp, lhsp}, rhsp};
            nodep->replaceWith(exprp);
        } अन्यथा {
            यदि (m_disablep) {
                lhsp = नूतन AstAnd{flp, नूतन AstNot{flp, m_disablep->clone_वृक्षः_विशुद्धम्(असत्य)}, lhsp};
            }

            AstPast* ध्रुव pastp
                = नूतन AstPast{flp, lhsp, शून्यसूचकः, शून्यसूचकः, /* propertyTiming */ सत्य};
            pastp->dtypeFrom(lhsp);
            pastp->sentreep(new_Sen_वृक्षः(nodep));
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp
                = m_underCover ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstAnd{flp, pastp, rhsp})
                               : नूतन AstOr{flp, नूतन AstNot{flp, pastp}, rhsp};
            exprp->dtype_समुच्चयः_Bit();
            nodep->replaceWith(exprp);
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(AstUntil* nodep) override {
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        UASSERT_OBJ(
            !m_pexprp, nodep,
            "'" << nodep->verilogKwd()
                << "' in complex property expression should have been rejected by V3AssertNfa");
        यदि (nodep->isStrong()
            && (v3Global.opt.timing().is_समुच्चयः_False() || !v3Global.opt.timing().is_समुच्चयः_True())) {
            nodep->v3warn(E_NOTIMING, nodep->verilogKwd() << " requires --timing");
            nodep->replaceWith(नूतन AstConst{flp, AstConst::BitFalse{}});
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            प्रत्यागम;
        }
        यदि (nodep->isStrong()) {
            // p s_until q / p s_until_with q: q must eventually be true. Until then, p must
            // be true on every sampled tick. For s_until, check q first: when q is true on
            // this tick, p is not required. For s_until_with, p must be true on the q tick too.
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rawLhsp = nodep->lhsp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rawRhsp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
            AstSampled* ध्रुव lhsp = नूतन AstSampled{flp, rawLhsp, rawLhsp->dtypep(), सत्य};
            AstSampled* ध्रुव rhsp = नूतन AstSampled{flp, rawRhsp, rawRhsp->dtypep(), सत्य};
            Ast_ग्रन्थिः_व्यञ्जकम्* finalCondp = rhsp->clone_वृक्षः_विशुद्धम्(असत्य);
            यदि (nodep->isOverlapping()) {
                finalCondp = नूतन AstLogAnd{flp, lhsp->clone_वृक्षः_विशुद्धम्(असत्य), finalCondp};
            }

            // Track active assertion attempts. Only the count is needed to emit final failures.
            AstVar* ध्रुव activep = नूतन AstVar{flp, VVarType::MODULETEMP, m_सक्रिय_नामानि.get(""),
                                               nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            activep->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->सङ्कलय_Stmtsp(activep);

            AstVar* ध्रुव donep
                = नूतन AstVar{flp, VVarType::BLOCKTEMP, "__VassertDone", nodep->findBitDType()};
            donep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            स्वचालित समुच्चयः_समाप्तम् = [&]() {
                प्रत्यागम नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, donep, VAccess::WRITE},
                                     नूतन AstConst{flp, AstConst::BitTrue{}}};
            };

            AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
            Ast_नियोजय* ध्रुव decrementVar = नूतन Ast_नियोजय{
                flp, नूतन AstVarRef{flp, activep, VAccess::WRITE},
                नूतन AstSub{flp, नूतन AstVarRef{flp, activep, VAccess::READ}, नूतन AstConst{flp, 1}}};
            loopp->सङ्कलय_Stmtsp(नूतन AstLoopTest{
                flp, loopp, नूतन AstLogNot{flp, नूतन AstVarRef{flp, donep, VAccess::READ}}});
            {
                Ast_ग्रन्थिः* ध्रुव passp = नूतन AstPExprClause{flp};
                passp->सङ्कलय_अग्र(decrementVar);
                passp->सङ्कलय_अग्र(समुच्चयः_समाप्तम्());
                Ast_ग्रन्थिः_व्यञ्जकम्* passCondp = rhsp;
                यदि (nodep->isOverlapping()) {
                    passCondp = नूतन AstLogAnd{flp, lhsp->clone_वृक्षः_विशुद्धम्(असत्य), passCondp};
                }
                loopp->सङ्कलय_Stmtsp(नूतन AstIf{flp, passCondp, passp});
            }
            {
                Ast_ग्रन्थिः* ध्रुव failp = नूतन AstPExprClause{flp, असत्य};
                failp->सङ्कलय_अग्र(decrementVar->clone_वृक्षः(असत्य));
                failp->सङ्कलय_अग्र(समुच्चयः_समाप्तम्());
                loopp->सङ्कलय_Stmtsp(नूतन AstIf{
                    flp,
                    नूतन AstLogAnd{flp,
                                  नूतन AstLogNot{flp, नूतन AstVarRef{flp, donep, VAccess::READ}},
                                  नूतन AstLogNot{flp, lhsp}},
                    failp});
            }
            AstDelay* ध्रुव delayp = नूतन AstDelay{flp, नूतन AstConst{flp, 1}, असत्य};
            delayp->timeunit(m_modp->timeunit());
            loopp->सङ्कलय_Stmtsp(नूतन AstIf{
                flp, नूतन AstLogNot{flp, नूतन AstVarRef{flp, donep, VAccess::READ}}, delayp});

            // Main assertion block
            AstBegin* ध्रुव bodyp = नूतन AstBegin{flp, m_खण्डम्_नामानि.get(""), शून्यसूचकः, सत्य};
            bodyp->सङ्कलय_Stmtsp(donep);
            bodyp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, donep, VAccess::WRITE},
                                           नूतन AstConst{flp, AstConst::BitFalse{}}});
            File_पङ्क्तिः* ध्रुव flp = activep->fileline();
            bodyp->सङ्कलय_Stmtsp(
                नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, activep, VAccess::WRITE},
                              नूतन Ast_सङ्कलय{flp, नूतन AstVarRef{flp, activep, VAccess::READ},
                                         नूतन AstConst{flp, 1}}});
            bodyp->सङ्कलय_Stmtsp(loopp);

            // Validate assertion condition for each active assert
            AstVar* ध्रुव सक्रिय_Countp = नूतन AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                    nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            सक्रिय_Countp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

            Ast_नियोजय* ध्रुव init_सक्रिय_Countp
                = नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, सक्रिय_Countp, VAccess::WRITE},
                                नूतन AstVarRef{flp, activep, VAccess::READ}};
            AstLoop* ध्रुव finalLoopp = नूतन AstLoop{flp};
            finalLoopp->सङ्कलय_Stmtsp(
                नूतन AstLoopTest{flp, finalLoopp,
                                नूतन AstNeq{flp, नूतन AstVarRef{flp, सक्रिय_Countp, VAccess::READ},
                                           नूतन AstConst{flp, 0}}});
            finalLoopp->सङ्कलय_Stmtsp(नूतन AstIf{flp, finalCondp, नूतन AstPExprClause{flp},
                                            नूतन AstPExprClause{flp, असत्य}});
            finalLoopp->सङ्कलय_Stmtsp(
                नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, सक्रिय_Countp, VAccess::WRITE},
                              नूतन AstSub{flp, नूतन AstVarRef{flp, सक्रिय_Countp, VAccess::READ},
                                         नूतन AstConst{flp, 1}}});

            // Final assertion block
            AstBegin* ध्रुव finalp = नूतन AstBegin{flp, m_खण्डम्_नामानि.get(""), शून्यसूचकः, सत्य};
            finalp->सङ्कलय_Stmtsp(सक्रिय_Countp);
            finalp->सङ्कलय_Stmtsp(init_सक्रिय_Countp);
            finalp->सङ्कलय_Stmtsp(finalLoopp);

            AstPExpr* ध्रुव pexprp = नूतन AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
            VL_RESTORER(m_pexprp);
            VL_RESTORER(m_has_आवृत्तिः_Delay);
            m_pexprp = pexprp;
            m_has_आवृत्तिः_Delay = असत्य;
            iterate(bodyp);
            iterate(finalp);

            UASSERT_OBJ(!m_has_आवृत्तिः_Delay, nodep,
                        "s_until cycle delay should have been handled by V3AssertNfa");

            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            प्रत्यागम;
        }
        AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp()->unlink_Fr_पृष्ठभागः();
        AstLogAnd* ध्रुव loopCondp = नूतन AstLogAnd{flp, lhsp, नूतन AstLogNot{flp, rhsp}};
        loopp->सङ्कलय_Stmtsp(नूतन AstLoopTest{flp, loopp, loopCondp});
        loopp->सङ्कलय_Stmtsp(नूतन AstEventControl{flp, new_Sen_वृक्षः(nodep), शून्यसूचकः});

        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsCopyp = rhsp->clone_वृक्षः_विशुद्धम्(असत्य);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव passCondp
            = nodep->isOverlapping() ? नूतन AstLogAnd{flp, lhsp->clone_वृक्षः_विशुद्धम्(असत्य), rhsCopyp}
                                     : rhsCopyp;
        AstBegin* ध्रुव beginp = नूतन AstBegin{flp, m_खण्डम्_नामानि.get(""), loopp, सत्य};
        beginp->सङ्कलय_Stmtsp(
            नूतन AstIf{flp, passCondp, नूतन AstPExprClause{flp}, नूतन AstPExprClause{flp, असत्य}});

        AstPExpr* ध्रुव pexprp = नूतन AstPExpr{flp, beginp, nodep->dtypep()};
        pexprp->user1(1);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    शून्य दर्शय(AstDefaultDisable* nodep) override {
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
    }
    शून्य दर्शय(AstInferredDisable* nodep) override {
        Ast_ग्रन्थिः* newp;
        यदि (m_defaultDisablep) {
            newp = m_defaultDisablep->condp()->clone_वृक्षः_विशुद्धम्(सत्य);
        } अन्यथा {
            newp = नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    शून्य दर्शय(AstPropSpec* nodep) override {
        nodep = substitutePropertyCall(nodep);
        iterate_And_अग्र_Null(nodep->sensesp());
        यदि (m_senip && m_senip != nodep->sensesp())
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Only one PSL clock allowed per assertion");
        ध्रुव AstCover* ध्रुव coverp = VN_CAST(nodep->backp(), Cover);
        ध्रुव तार्किक seqEvent = coverp && coverp->isSeqEvent();
        यदि (!nodep->disablep() && m_defaultDisablep && !seqEvent) {
            nodep->disablep(m_defaultDisablep->condp()->clone_वृक्षः_विशुद्धम्(सत्य));
        }
        m_disablep = nodep->disablep();
        // Unlink and just keep a pointer to it, convert to sentree as needed
        m_senip = nodep->sensesp();
        iterateNull(nodep->disablep());
        यदि (!VN_AS(nodep->backp(), ग्रन्थिः_Cover_Or_Assert)->तात्कालिकम्()) {
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव propDtp = nodep->propp()->dtypep();
            nodep->propp(नूतन AstSampled{nodep->fileline(), nodep->propp()->unlink_Fr_पृष्ठभागः(),
                                        propDtp->dtypep(), सत्य});
        }
        // cover counts non-vacuous matches only (IEEE 1800-2023 16.15.2), so an
        // implication antecedent must hold; assert passes vacuously instead.
        VL_RESTORER(m_underCover);
        m_underCover = VN_IS(nodep->backp(), Cover);
        iterate(nodep->propp());
        iterateNull(nodep->मेलनम्_Countp());
    }
    शून्य दर्शय(AstPExpr* nodep) override {
        // V3AssertNfa handles multi-cycle property expressions before this pass,
        // so the following unsupported paths are defensive and typically unreached.
        यदि (m_pexprp && m_pexprp->user1()) {  // LCOV_EXCL_START
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Complex property expression inside 'until''");
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            प्रत्यागम;
        }  // LCOV_EXCL_STOP
        यदि (AstLogNot* ध्रुव notp = VN_CAST(nodep->backp(), LogNot)) {
            notp->replaceWith(nodep->unlink_Fr_पृष्ठभागः());
            VL_DO_DANGLING(प्रक्षिप_Deletep(notp), notp);
            iterate(nodep);
            प्रत्यागम;
        }
        // Sequence expression as antecedent of implication is not yet supported
        यदि (AstImplication* ध्रुव implp = VN_CAST(nodep->backp(), Implication)) {
            यदि (implp->lhsp() == nodep) {  // LCOV_EXCL_START
                implp->v3warn(E_UNSUPPORTED,
                              "Unsupported: Implication with sequence expression as antecedent");
                nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                प्रत्यागम;
            }  // LCOV_EXCL_STOP
        }
        VL_RESTORER(m_pexprp);
        VL_RESTORER(m_disableSeqIfp);
        m_pexprp = nodep;

        यदि (m_disablep) {
            ध्रुव AstSampled* sampledp = शून्यसूचकः;
            यदि (m_disablep->exists([&sampledp](ध्रुव AstSampled* ध्रुव sp) {
                    sampledp = sp;
                    प्रत्यागम सत्य;
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
                = नूतन AstVar{flp, VVarType::MODULETEMP, m_disable_Cnt_नामानि.get(""),
                             nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            disableCntp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->सङ्कलय_Stmtsp(disableCntp);
            AstVarRef* ध्रुव readCntRefp = नूतन AstVarRef{flp, disableCntp, VAccess::READ};
            AstVarRef* ध्रुव writeCntRefp = नूतन AstVarRef{flp, disableCntp, VAccess::WRITE};
            Ast_नियोजय* ध्रुव incrStmtp = नूतन Ast_नियोजय{
                flp, writeCntRefp, नूतन Ast_सङ्कलय{flp, readCntRefp, नूतन AstConst{flp, 1}}};
            AstAlways* ध्रुव alwaysp
                = नूतन AstAlways{flp, VAlwaysKwd::ALWAYS,
                                नूतन Ast_Sen_वृक्षः{flp, नूतन AstSenItem{flp, VEdgeType::ET_POSEDGE,
                                                                   m_disablep->clone_वृक्षः(असत्य)}},
                                incrStmtp};
            disableCntp->सङ्कलय_अग्र_अत्र(alwaysp);

            // Store value of that counter at the beginning of sequence evaluation
            AstBegin* ध्रुव bodyp = nodep->bodyp();
            AstVar* ध्रुव initialCntp = नूतन AstVar{flp, VVarType::BLOCKTEMP, "__VinitialCnt",
                                                   nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            initialCntp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            Ast_नियोजय* ध्रुव assignp
                = नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, initialCntp, VAccess::WRITE},
                                readCntRefp->clone_वृक्षः(असत्य)};
            // Prepend to the sequence body to keep statement list structure valid.
            UASSERT_OBJ(!bodyp->name().रिक्तानि(), bodyp, "Sequence body should be named");
            Ast_ग्रन्थिः* ध्रुव newStmtsp = Ast_ग्रन्थिः::सङ्कलय_अग्र<Ast_ग्रन्थिः, Ast_ग्रन्थिः>(initialCntp, assignp);
            bodyp->prependStmtsp(newStmtsp);

            m_disableSeqIfp
                = नूतन AstIf{flp, नूतन AstEq{flp, नूतन AstVarRef{flp, initialCntp, VAccess::READ},
                                           readCntRefp->clone_वृक्षः(असत्य)}};
            // Delete it, because it is always copied before insetion to the AST
            प्रक्षिप_Deletep(m_disableSeqIfp);  // m_disableSeqIfp used later
        }
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultClkEvtVarp);
        VL_RESTORER(m_defaultDisablep);
        VL_RESTORER(m_modp);
        यदि (!VN_IS(nodep, Class)) {
            // A class nested in a module is in the scope of the module's default clocking
            m_defaultClockingp = शून्यसूचकः;
            m_defaultClkEvtVarp = शून्यसूचकः;
        }
        m_defaultDisablep = nodep->defaultDisablep();
        m_modp = nodep;
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_Gen_खण्डम्* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstProperty* nodep) override {
        // The body will be visited when will be substituted in place of property reference
        // (AstFuncRef)
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
    }
    शून्य दर्शय(AstSequence* nodep) override {
        // Sequence declarations are not visited directly; their bodies are inlined
        // at call sites by visit(AstFuncRef*). Collect for post-traversal cleanup.
        m_seqs_गन्तव्यम्_Cleanup.प्रक्षिप_पृष्ठभागः(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट AssertPreVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        { AssertPreClockingVisitor{nodep}; }
        // Process
        iterate(nodep);
        // Fix up varref names
        प्रति (AstVarXRef* xrefp : m_xrefsp) xrefp->name(xrefp->varp()->name());
        // Clean up sequence declarations after inlining.
        // Referenced sequences that were inlined have isReferenced cleared.
        // Remaining referenced sequences are in unsupported contexts (e.g. @seq event).
        प्रति (AstSequence* seqp : m_seqs_गन्तव्यम्_Cleanup) {
            यदि (seqp->isReferenced()) {
                seqp->v3warn(E_UNSUPPORTED,
                             "Unsupported: sequence referenced outside assertion property");
            } अन्यथा {
                VL_DO_DANGLING(seqp->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), seqp);
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
