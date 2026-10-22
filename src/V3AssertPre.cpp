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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3AssertPre.h"

#అన్తర్భావ "V3Assert.h"
#అన్తర్భావ "V3Const.h"
#అన్తర్భావ "V3Task.h"
#అన్తర్భావ "V3Timing.h"
#అన్తర్భావ "V3UniqueNames.h"

#అన్తర్భావ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clocking blocks of clocking items

తరగతి AssertPreClockingVisitor final : బహిరంగ VNVisitorConst {
    // Captures the clocking block of each clocking item before AssertPreVisitor, which needs it
    // at synchronous drives, often visited before the clocking block, e.g. in a parent module
    // NODE STATE
    // AstClockingItem::user3p()  // AstClocking*. Clocking block of the item, user3 claimed by
    //                            //               AssertPreVisitor

    // VISITORS
    శూన్య సందర్శించు(AstNetlist* nodep) override { iterate_And_తదుపరి_Const_Null(nodep->modulesp()); }
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override { iterate_And_తదుపరి_Const_Null(nodep->stmtsp()); }
    శూన్య సందర్శించు(Ast_Gen_ఖండం* nodep) override { iterate_And_తదుపరి_Const_Null(nodep->itemsp()); }
    శూన్య సందర్శించు(AstClocking* nodep) override {
        కొరకు (Ast_కణుపు* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            ఒకవేళ (Ast_Clocking_అంశం* స్థిరరాశి citemp = VN_CAST(itemp, Clocking_అంశం)) {
                citemp->user3p(nodep);
            }
        }
    }
    // Clocking blocks are only in modules and their generate blocks
    శూన్య సందర్శించు(Ast_కణుపు*) override {}

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన AssertPreClockingVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~AssertPreClockingVisitor() override = అప్రమేయ;
};

//######################################################################
// Assert class functions

తరగతి AssertPreVisitor final : బహిరంగ VNVisitor {
    // Removes clocks and other pre-optimizations
    // Eventually inlines calls to sequences, properties, etc.
    // We're not parsing the tree, or anything more complicated.
అంతరంగ:
    // TYPES
    నిర్మాణం SynchDrive final {
        Ast_Clocking_అంశం* itemp;  // Clocking item of the driven clockvar
        Ast_కణుపు_సమాసం* refp;  // Reference to the driven clockvar
    };

    // NODE STATE
    // AstClockingItem::user1p()         // AstVar*.      varp() of ClockingItem after unlink
    // AstClockingItem::user2p()         // AstVar*.      Flag set by drives of output clockvar
    // AstClockingItem::user3p()         // AstClocking*. Clocking block of the item, captured
    //                                   //               by AssertPreClockingVisitor
    // AstPExpr::user1()                 // bool.         Created from AstUntil
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;
    స్థిరరాశి VNUser2_లోపల_Use m_inuser2;
    స్థిరరాశి VNUser3_లోపల_Use m_inuser3;
    // STATE
    // Current context:
    AstNetlist* స్థిరరాశి m_netlistp = శూన్యసూచిక;  // Current netlist
    Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Current module
    AstClocking* m_clockingp = శూన్యసూచిక;  // Current clocking block
    // Reset each module:
    AstClocking* m_defaultClockingp = శూన్యసూచిక;  // Default clocking for current module
    AstVar* m_defaultClkEvtVarp = శూన్యసూచిక;  // Event var for default clocking (for ##0)
    AstDefaultDisable* m_defaultDisablep = శూన్యసూచిక;  // Default disable for current module
    // Reset each assertion:
    Ast_Sen_అంశం* m_senip = శూన్యసూచిక;  // Last sensitivity
    // Reset each always:
    Ast_Sen_అంశం* m_seniAlwaysp = శూన్యసూచిక;  // Last sensitivity in always
    // Reset each assertion:
    Ast_కణుపు_సమాసం* m_disablep = శూన్యసూచిక;  // Last disable
    AstIf* m_disableSeqIfp = శూన్యసూచిక;  // Used for handling disable iff in sequences
    AstPExpr* m_pexprp = శూన్యసూచిక;  // Last AstPExpr
    తార్కిక m_underCover = అబద్ధం;  // True if the enclosing assertion is a cover
    // Other:
    V3_Unique_పేర్లు m_చక్రం_Dly_పేర్లు{"__VcycleDly"};  // Cycle delay counter name generator
    V3_Unique_పేర్లు m_cons_Rep_పేర్లు{"__VconsRep"};  // Consecutive repetition counter name generator
    V3_Unique_పేర్లు m_goto_Rep_పేర్లు{"__VgotoRep"};  // Goto repetition counter name generator
    V3_Unique_పేర్లు m_non_Cons_Rep_పేర్లు{"__VnonConsRep"};  // Nonconsecutive rep name generator
    V3_Unique_పేర్లు m_disable_Cnt_పేర్లు{"__VdisableCnt"};  // Disable condition counter name generator
    V3_Unique_పేర్లు m_ఖండం_పేర్లు{"__VassertBlock"};  // Names of blocks with temporaries
    V3_Unique_పేర్లు m_prop_Var_పేర్లు{"__Vpropvar"};  // Property-local variable name generator
    V3_Unique_పేర్లు m_చురుకైన_పేర్లు{"__VassertsActive"};  // Active asserts map name generator
    V3_Unique_పేర్లు m_driven_పేర్లు{"__VclockingDriven"};  // Clockvar drive flag name generator
    తార్కిక m_లోపల_కేటాయించు = అబద్ధం;  // True if in an AssignNode
    తార్కిక m_లోపల_కేటాయించు_Dly_Lhs = అబద్ధం;  // True if in AssignDly's LHS
    తార్కిక m_లోపల_Synch_Drive = అబద్ధం;  // True if in synchronous drive
    తార్కిక m_లోపల_Concat_కేటాయించు = అబద్ధం;  // True if in assignment to a concatenation
    std::సదిశ<SynchDrive> m_drives;  // Clockvars written by the synchronous drive
    తార్కిక m_has_చక్రం_Delay = అబద్ధం;  // True if node has cycle delay beneath
    std::సదిశ<AstVarXRef*> m_xrefsp;  // list of xrefs that need name fixup
    std::సదిశ<AstSequence*> m_seqs_గమ్యం_Cleanup;  // Sequences to clean up after traversal

    // METHODS

    // Flag set by each drive of an output clockvar, added to the module with its clocking item
    AstVar* getCreateDrivenVarp(Ast_Clocking_అంశం* itemp) {
        ఒకవేళ (!itemp->user2p()) {
            AstVar* స్థిరరాశి varp = కొత్త AstVar{itemp->fileline(), VVarType::MODULETEMP,
                                            m_driven_పేర్లు.get(itemp), itemp->కనుగొను_బిట్టు_DType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            itemp->user2p(varp);
        }
        తిరిగి VN_AS(itemp->user2p(), Var);
    }
    // Reference to a variable in the scope of a driven clockvar, made like the clockvar's
    స్థిర Ast_కణుపు_సమాసం* newDriveRefp(File_పంక్తి* flp, స్థిరరాశి SynchDrive& drive, AstVar* varp,
                                     స్థిరరాశి VAccess& access) {
        ఒకవేళ (స్థిరరాశి AstVarXRef* స్థిరరాశి xrefp = VN_CAST(drive.refp, VarXRef)) {
            తిరిగి కొత్త AstVarXRef{flp, varp, xrefp->dotted(), access};
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstMemberSel* స్థిరరాశి selp = VN_CAST(drive.refp, MemberSel)) {
            // The interface expression is evaluated again, so it must not have side effects;
            // cloneTreePure warns if it has any
            AstMemberSel* స్థిరరాశి newSelp
                = కొత్త AstMemberSel{flp, selp->fromp()->clone_వృక్షం_శుద్ధ(అబద్ధం), varp};
            newSelp->access(access);
            తిరిగి newSelp;
        }
        తిరిగి కొత్త AstVarRef{flp, varp, access};
    }
    // Assignment setting the drive flag of a driven clockvar
    Ast_కేటాయించు* newDrivenSetp(File_పంక్తి* flp, స్థిరరాశి SynchDrive& drive) {
        AstVar* స్థిరరాశి varp = getCreateDrivenVarp(drive.itemp);
        Ast_కేటాయించు* స్థిరరాశి setp = కొత్త Ast_కేటాయించు{flp, newDriveRefp(flp, drive, varp, VAccess::WRITE),
                                              కొత్త AstConst{flp, AstConst::బిట్టు_True{}}};
        setp->user1(నిజం);
        తిరిగి setp;
    }
    // Clocking block of a clocking item
    స్థిర AstClocking* clocking_యొక్క(స్థిరరాశి Ast_Clocking_అంశం* itemp) {
        UASSERT_OBJ(itemp->user3p(), itemp, "Clocking item not captured in a clocking block");
        తిరిగి VN_AS(itemp->user3p(), Clocking);
    }
    // Clocking event of a driven clockvar, referenced from the drive like the clockvar, or
    // nullptr if unsupported, which is reported on the drive's cycle delay
    Ast_Sen_వృక్షం* newDriveSentreep(AstDelay* delayp, స్థిరరాశి SynchDrive& drive) {
        Ast_Sen_వృక్షం* స్థిరరాశి origp = clocking_యొక్క(drive.itemp)->sentreep();
        ఒకవేళ (VN_IS(drive.refp, VarRef)) తిరిగి origp->clone_వృక్షం(అబద్ధం);
        ఒకవేళ (origp->exists([](స్థిరరాశి AstVarXRef*) { తిరిగి నిజం; })) {
            // The reference is relative to the clocking block
            delayp->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in synchronous drive to"
                                          " clockvar in another scope, whose clocking event"
                                          " has a hierarchical reference");
            తిరిగి శూన్యసూచిక;
        }
        స్థిరరాశి AstMemberSel* స్థిరరాశి selp = VN_CAST(drive.refp, MemberSel);
        // Processes outside classes evaluate their events also when not waiting on them,
        // possibly before the interface reference is set, so the event is guarded against a
        // null reference, which needs an integral value; scheduling does not guard it
        స్థిరరాశి తార్కిక guardNull = selp && !VN_IS(m_modp, Class);
        ఒకవేళ (guardNull) {
            కొరకు (Ast_Sen_అంశం* itemp = origp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Sen_అంశం)) {
                ఒకవేళ (!itemp->sensp()->dtypep()->skipRefp()->isIntegralOrPacked()) {
                    delayp->v3warn(E_UNSUPPORTED,
                                   "Unsupported: cycle delay in synchronous drive through"
                                   " an interface reference outside a class, to clocking"
                                   " block with non-integral clocking event");
                    తిరిగి శూన్యసూచిక;
                }
            }
        }
        // Interface declaring the clocking block, if driven through an interface reference
        AstIface* స్థిరరాశి ifacep
            = selp ? VN_AS(selp->fromp()->dtypep()->skipRefp(), IfaceRefDType)->iface_ద్వారా_Cellp()
                   : శూన్యసూచిక;
        Ast_Sen_వృక్షం* స్థిరరాశి sentreep = origp->clone_వృక్షం(అబద్ధం);
        std::సదిశ<AstVarRef*> refps;
        sentreep->foreach([&](AstVarRef* refp) {
            ఒకవేళ (!refp->classOrPackagep()) refps.నెట్టు_వెనుక(refp);
        });
        కొరకు (AstVarRef* స్థిరరాశి refp : refps) {
            AstVar* స్థిరరాశి varp = refp->varp();
            ఒకవేళ (ifacep) {
                // Mark the variable as read through an interface reference, as V3Width does for
                // a member select. The mark is the interface declaring the variable, whose
                // instances all share it, so drives through references to any instances agree.
                UASSERT_OBJ(!varp->sensIfacep() || varp->sensIfacep() == ifacep, refp,
                            "Variable read through references to different interfaces");
                varp->sensIfacep(ifacep);
            }
            refp->replaceWith(newDriveRefp(refp->fileline(), drive, varp, refp->access()));
            VL_DO_DANGLING(నెట్టు_Deletep(refp), refp);
        }
        ఒకవేళ (guardNull) {
            కొరకు (Ast_Sen_అంశం* itemp = sentreep->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Sen_అంశం)) {
                guardNullIface(itemp, selp->fromp());
            }
        }
        తిరిగి sentreep;
    }
    // Make an integral clocking event through an interface reference false while the reference
    // is null
    స్థిర శూన్య guardNullIface(Ast_Sen_అంశం* sensesp, Ast_కణుపు_సమాసం* fromp) {
        File_పంక్తి* స్థిరరాశి flp = sensesp->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి sensp = sensesp->sensp()->unlink_Fr_వెనుక();
        sensesp->sensp(కొత్త AstCond{flp, newNotNullp(flp, fromp), sensp,
                                   కొత్త AstConst{flp, AstConst::DTyped{}, sensp->dtypep()}});
        ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి condp = sensesp->condp()) {
            condp->unlink_Fr_వెనుక();
            sensesp->condp(కొత్త AstLogAnd{flp, newNotNullp(flp, fromp), condp});
        }
    }
    స్థిర Ast_కణుపు_సమాసం* newNotNullp(File_పంక్తి* flp, Ast_కణుపు_సమాసం* fromp) {
        తిరిగి కొత్త AstNeq{flp, fromp->clone_వృక్షం_శుద్ధ(అబద్ధం), కొత్త AstConst{flp, AstConst::Null{}}};
    }
    // Procedural '##0': if the default clocking event has not yet occurred in this time step,
    // wait for it; otherwise continue without suspension (IEEE 1800-2023 14.11)
    AstIf* new_చక్రం_Delay_Zerop(File_పంక్తి* flp) స్థిరరాశి {
        AstVar* స్థిరరాశి evtVarp = m_defaultClkEvtVarp;
        UASSERT_OBJ(evtVarp, flp, "Default clocking event var not pre-created");
        AstCMethodHard* స్థిరరాశి isTriggeredp = కొత్త AstCMethodHard{
            flp, కొత్త AstVarRef{flp, evtVarp, VAccess::READ}, VCMethod::EVENT_IS_TRIGGERED};
        isTriggeredp->dtype_సమితి_బిట్టు();
        AstEventControl* స్థిరరాశి waitp = కొత్త AstEventControl{
            flp,
            కొత్త Ast_Sen_వృక్షం{flp, కొత్త Ast_Sen_అంశం{flp, VEdgeType::ET_EVENT,
                                               కొత్త AstVarRef{flp, evtVarp, VAccess::READ}}},
            శూన్యసూచిక};
        తిరిగి కొత్త AstIf{flp, కొత్త AstNot{flp, isTriggeredp}, waitp};
    }

    స్థిర శూన్య checkSamplingFuncDType(Ast_కణుపు_సమాసం* nodep, స్థిరరాశి Ast_కణుపు* exprp) {
        స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep = exprp->dtypep()->skipRefp();
        ఒకవేళ (!dtypep->isIntegralOrPacked()) {
            nodep->v3error("Expected numeric type, but got a " << dtypep->prettyDTypeNameQ()
                                                               << " data type");
        }
    }

    Ast_Sen_వృక్షం* new_Sen_వృక్షం(Ast_కణుపు* nodep, Ast_Sen_వృక్షం* useTreep = శూన్యసూచిక,
                           Ast_కణుపు_Cover_Or_Assert* cassertp = శూన్యసూచిక) {
        // Create sentree based on clocked or default clock
        // Return nullptr for always
        ఒకవేళ (useTreep) తిరిగి useTreep;
        Ast_Sen_వృక్షం* newp = శూన్యసూచిక;
        Ast_Sen_అంశం* senip = m_senip;
        తార్కిక fromAlways = అబద్ధం;
        ఒకవేళ (!senip && m_defaultClockingp) senip = m_defaultClockingp->sentreep()->sensesp();
        ఒకవేళ (!senip) {
            senip = m_seniAlwaysp;
            fromAlways = నిజం;
        }
        ఒకవేళ (!senip) {
            nodep->v3error("Concurrent assertion has no clock (IEEE 1800-2023 16.16)\n"
                           << nodep->warn_మరిన్ని()
                           << "... Suggest provide a clocking event, a default"
                              " clocking, or a clocked procedural context");
            newp = కొత్త Ast_Sen_వృక్షం{nodep->fileline(), శూన్యసూచిక};
        } లేకపోతే {
            ఒకవేళ (cassertp && fromAlways) cassertp->senFromAlways(నిజం);
            newp = కొత్త Ast_Sen_వృక్షం{nodep->fileline(), senip->clone_వృక్షం(నిజం)};
        }
        తిరిగి newp;
    }
    Ast_కణుపు_సమాసం* getSequenceBodyExprp(స్థిరరాశి AstSequence* స్థిరరాశి seqp) స్థిరరాశి {
        // The statements in AstSequence are optional AstVar (ports) followed by body expr.
        Ast_కణుపు* bodyp = seqp->stmtsp();
        అయితే (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        తిరిగి VN_CAST(bodyp, కణుపు_సమాసం);
    }
    AstPropSpec* getPropertyExprp(స్థిరరాశి AstProperty* స్థిరరాశి propp) {
        // Statements in AstProperty: AstVar (ports/local vars),
        // AstInitialStaticStmt/AstInitialAutomaticStmt (var init), AstPropSpec (body).
        Ast_కణుపు* propExprp = propp->stmtsp();
        అయితే (propExprp
               && (VN_IS(propExprp, Var) || VN_IS(propExprp, InitialStaticStmt)
                   || VN_IS(propExprp, InitialAutomaticStmt))) {
            propExprp = propExprp->nextp();
        }
        తిరిగి VN_CAST(propExprp, PropSpec);
    }
    శూన్య substituteSequenceCall(AstFuncRef* funcrefp, AstSequence* seqp) {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Inline the sequence body at the call site, replacing the FuncRef
        Ast_కణుపు_సమాసం* bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        // Clone the body expression since the sequence may be referenced multiple times
        Ast_కణుపు_సమాసం* clonedp = bodyExprp->clone_వృక్షం(అబద్ధం);
        // Build substitution map, then do a single traversal to replace all formals
        // (textual substitution per IEEE 16.8.2).
        స్థిరరాశి V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<స్థిరరాశి AstVar*, Ast_కణుపు_సమాసం*> పోర్టు_Map;
        కొరకు (స్థిరరాశి స్వయంచాలక& tconnect : tconnects) {
            పోర్టు_Map[tconnect.మొదటి] = tconnect.రెండవ->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            స్థిరరాశి స్వయంచాలక it = పోర్టు_Map.కనుగొను(refp->varp());
            ఒకవేళ (it != పోర్టు_Map.చివర()) {
                refp->replaceWith(it->రెండవ->clone_వృక్షం(అబద్ధం));
                VL_DO_DANGLING(నెట్టు_Deletep(refp), refp);
            }
        });
        కొరకు (స్థిరరాశి స్వయంచాలక& tconnect : tconnects) {
            నెట్టు_Deletep(tconnect.రెండవ->exprp()->unlink_Fr_వెనుక());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        // Replace the FuncRef with the inlined body
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(నెట్టు_Deletep(funcrefp), funcrefp);
        // Clear referenced flag; sequences with isReferenced==false are deleted in assertPreAll
        seqp->isReferenced(అబద్ధం);
    }
    AstPropSpec* clonePropertyBody(AstFuncRef* funcrefp, స్థిరరాశి AstProperty* propp) {
        AstPropSpec* propExprp = getPropertyExprp(propp);
        // Substitute inner property call before copying in order to not doing the same for
        // each call of outer property call.
        propExprp = substitutePropertyCall(propExprp);
        // Clone subtree after substitution. It is needed, because property might be called
        // multiple times with different arguments.
        propExprp = propExprp->clone_వృక్షం(అబద్ధం);
        // Build substitution maps for formal arguments and property-local
        // variables, then perform a single foreach to apply all replacements.
        // Map port vars to their actual argument expressions
        స్థిరరాశి V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propp->stmtsp());
        std::unordered_map<స్థిరరాశి AstVar*, Ast_కణుపు_సమాసం*> పోర్టు_Map;
        కొరకు (స్థిరరాశి స్వయంచాలక& tconnect : tconnects) {
            పోర్టు_Map[tconnect.మొదటి] = tconnect.రెండవ->exprp();
        }

        // Promote property-local variables (non-port vars, IEEE 16.10) to
        // module-level __Vpropvar temps. Cross-cycle persistence is handled
        // by the match item lowering in visit(AstImplication*).
        std::unordered_map<స్థిరరాశి AstVar*, AstVar*> localVarMap;
        కొరకు (Ast_కణుపు* stmtp = propp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(stmtp, Var)) {
                ఒకవేళ (!varp->isIO()) {
                    స్థిరరాశి string newName = m_prop_Var_పేర్లు.get(varp);
                    AstVar* స్థిరరాశి newVarp = కొత్త AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->కూడు_Stmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        // Single traversal: substitute ports and update local var references
        propExprp->foreach([&](AstVarRef* refp) {
            స్థిరరాశి స్వయంచాలక పోర్టు_It = పోర్టు_Map.కనుగొను(refp->varp());
            ఒకవేళ (పోర్టు_It != పోర్టు_Map.చివర()) {
                refp->replaceWith(పోర్టు_It->రెండవ->clone_వృక్షం(అబద్ధం));
                VL_DO_DANGLING(నెట్టు_Deletep(refp), refp);
                తిరిగి;
            }
            స్థిరరాశి స్వయంచాలక localIt = localVarMap.కనుగొను(refp->varp());
            ఒకవేళ (localIt != localVarMap.చివర()) refp->varp(localIt->రెండవ);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        // Clean up argument expressions
        కొరకు (స్థిరరాశి స్వయంచాలక& tconnect : tconnects) {
            నెట్టు_Deletep(tconnect.రెండవ->exprp()->unlink_Fr_వెనుక());
        }
        V3AssertCommon::checkSvaInstanceDelays(propExprp, funcrefp);
        తిరిగి propExprp;
    }
    AstPropSpec* substitutePropertyCall(AstPropSpec* nodep) {
        ఒకవేళ (AstFuncRef* స్థిరరాశి funcrefp = VN_CAST(nodep->propp(), FuncRef)) {
            ఒకవేళ (స్థిరరాశి AstProperty* స్థిరరాశి propp = VN_CAST(funcrefp->taskp(), Property)) {
                AstPropSpec* స్థిరరాశి propExprp = clonePropertyBody(funcrefp, propp);

                // Handle case with 2 disable iff statement (IEEE 1800-2023 16.12.1)
                ఒకవేళ (nodep->disablep() && propExprp->disablep()) {
                    nodep->v3error("disable iff expression before property call and in its "
                                   "body is not legal");
                    నెట్టు_Deletep(propExprp->disablep()->unlink_Fr_వెనుక());
                }
                // If disable iff is in outer property, move it to inner
                ఒకవేళ (nodep->disablep()) {
                    Ast_కణుపు_సమాసం* స్థిరరాశి disablep = nodep->disablep()->unlink_Fr_వెనుక();
                    propExprp->disablep(disablep);
                }

                ఒకవేళ (nodep->sensesp() && propExprp->sensesp()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Clock event before property call and in its body");
                    నెట్టు_Deletep(propExprp->sensesp()->unlink_Fr_వెనుక());
                }
                // If clock event is in outer property, move it to inner
                ఒకవేళ (nodep->sensesp()) {
                    Ast_Sen_అంశం* స్థిరరాశి sensesp = nodep->sensesp();
                    sensesp->unlink_Fr_వెనుక();
                    propExprp->sensesp(sensesp);
                }

                // Now substitute property reference with property body
                nodep->replaceWith(propExprp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                తిరిగి propExprp;
            }
        }
        తిరిగి nodep;
    }

    శూన్య substituteNestedPropertyCall(AstFuncRef* funcrefp, స్థిరరాశి AstProperty* propp) {
        AstPropSpec* స్థిరరాశి specp = clonePropertyBody(funcrefp, propp);
        Ast_కణుపు* replp = శూన్యసూచిక;
        ఒకవేళ (specp->disablep()) {
            funcrefp->v3error("Illegal 'disable iff' in a nested property instance"
                              " (IEEE 1800-2023 16.12.1)");
        } లేకపోతే ఒకవేళ (specp->sensesp()) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: clock event in a nested property instance");
        } లేకపోతే ఒకవేళ (specp->propp()->exists(
                       [](స్థిరరాశి Ast_కణుపు_సమాసం* np) { తిరిగి np->is_Multi_చక్రం_Sva(); })) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: multi-cycle body in a nested property instance");
        } లేకపోతే {
            replp = specp->propp()->unlink_Fr_వెనుక();
        }
        ఒకవేళ (!replp) replp = కొత్త AstConst{funcrefp->fileline(), AstConst::బిట్టు_False{}};
        funcrefp->replaceWith(replp);
        VL_DO_DANGLING(నెట్టు_Deletep(funcrefp), funcrefp);
        VL_DO_DANGLING(నెట్టు_Deletep(specp), specp);
    }

    // VISITORS
    //========== Statements
    శూన్య సందర్శించు(AstClocking* స్థిరరాశి nodep) override {
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        UINFO(8, "   CLOCKING" << nodep);
        ఒకవేళ (nodep->isDefault()) {
            ఒకవేళ (m_defaultClockingp) {
                nodep->v3error("Only one default clocking block allowed per module"
                               " (IEEE 1800-2023 14.12)");
            } లేకపోతే {
                m_defaultClockingp = nodep;
                m_defaultClkEvtVarp = nodep->ensureEventp();
            }
        }
        iterateChildren(nodep);
        ఒకవేళ (nodep->eventp()) nodep->కూడు_తదుపరి_ఇక్కడ(nodep->eventp()->unlink_Fr_వెనుక());
        VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
    }
    శూన్య సందర్శించు(AstModportClockingRef* స్థిరరాశి nodep) override {
        // It has to be converted to a list of ModportClockingVarRefs,
        // because clocking blocks are removed in this pass
        కొరకు (Ast_కణుపు* itemp = nodep->clockingp()->itemsp(); itemp; itemp = itemp->nextp()) {
            ఒకవేళ (Ast_Clocking_అంశం* స్థిరరాశి citemp = VN_CAST(itemp, Clocking_అంశం)) {
                ఒకవేళ (AstVar* స్థిరరాశి varp
                    = citemp->varp() ? citemp->varp() : VN_AS(citemp->user1p(), Var)) {
                    AstModportVarRef* స్థిరరాశి modVarp = కొత్త AstModportVarRef{
                        nodep->fileline(), varp->name(), citemp->direction()};
                    modVarp->varp(varp);
                    nodep->కూడు_తదుపరి_ఇక్కడ(modVarp);
                    ఒకవేళ (citemp->direction().isOutput()) {
                        // Drives through the modport set the drive flag
                        AstVar* స్థిరరాశి drivenVarp = getCreateDrivenVarp(citemp);
                        AstModportVarRef* స్థిరరాశి drivenRefp = కొత్త AstModportVarRef{
                            nodep->fileline(), drivenVarp->name(), VDirection::OUTPUT};
                        drivenRefp->varp(drivenVarp);
                        nodep->కూడు_తదుపరి_ఇక్కడ(drivenRefp);
                    }
                }
            }
        }
        VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
    }
    శూన్య సందర్శించు(Ast_Clocking_అంశం* స్థిరరాశి nodep) override {
        // Get a ref to the sampled/driven variable
        AstVar* స్థిరరాశి varp = nodep->varp();
        ఒకవేళ (!varp) {
            // Unused item
            తిరిగి;
        }
        // Flag set by drives of an output clockvar, possibly visited before this item
        ఒకవేళ (nodep->direction().isOutput()) m_modp->కూడు_Stmtsp(getCreateDrivenVarp(nodep));
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        V3Const::constifyEdit(nodep->skewp());
        ఒకవేళ (!VN_IS(nodep->skewp(), Const)) {
            nodep->skewp()->v3error("Skew must be constant (IEEE 1800-2023 14.4)");
            తిరిగి;
        }
        AstConst* స్థిరరాశి skewp = VN_AS(nodep->skewp(), Const);
        ఒకవేళ (skewp->అంకె().isNegative()) skewp->v3error("Skew cannot be negative");
        Ast_కణుపు_సమాసం* స్థిరరాశి exprp = nodep->exprp();
        varp->name(m_clockingp->name() + "__DOT__" + varp->name());
        m_clockingp->కూడు_తదుపరి_ఇక్కడ(varp->unlink_Fr_వెనుక());
        nodep->user1p(varp);
        varp->user1p(nodep);
        ఒకవేళ (nodep->direction() == VDirection::OUTPUT) {
            exprp->foreach([](స్థిరరాశి Ast_కణుపు_Var_Ref* varrefp) {
                // Prevent confusing BLKANDNBLK warnings on clockvars due to generated assignments
                varrefp->fileline()->warnOff(V3_Error_సంకేతం::BLKANDNBLK, నిజం);
                // Mark the driven signal so later passes (e.g. V3Inst port
                // connection) know the drive flows out from inside the cell.
                // Item expressions are plain signal references, so only the
                // driven signal is marked, never index selects.
                varrefp->varp()->isClockingDriven(నిజం);
            });
            AstVarRef* స్థిరరాశి skewedReadRefp = కొత్త AstVarRef{flp, varp, VAccess::READ};
            skewedReadRefp->user1(నిజం);
            // Initialize the clockvar
            AstVarRef* స్థిరరాశి skewedWriteRefp = కొత్త AstVarRef{flp, varp, VAccess::WRITE};
            skewedWriteRefp->user1(నిజం);
            AstInitialStatic* స్థిరరాశి initClockvarp = కొత్త AstInitialStatic{
                flp, కొత్త Ast_కేటాయించు{flp, skewedWriteRefp, exprp->clone_వృక్షం_శుద్ధ(అబద్ధం)}};
            m_modp->కూడు_Stmtsp(initClockvarp);
            // Each drive sets a flag, so the signal is also assigned when driven with the same
            // value again, e.g. after another assignment to the signal (IEEE 1800-2023 14.16.2)
            AstVar* స్థిరరాశి drivenVarp = getCreateDrivenVarp(nodep);
            m_modp->కూడు_Stmtsp(కొత్త AstInitialStatic{
                flp, కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                   కొత్త AstConst{flp, AstConst::బిట్టు_False{}}}});
            // Assign the clockvar to the actual var if it was driven
            Ast_కేటాయించు* స్థిరరాశి assignp
                = కొత్త Ast_కేటాయించు{flp, exprp->clone_వృక్షం_శుద్ధ(అబద్ధం), skewedReadRefp};
            AstIf* స్థిరరాశి ifp
                = కొత్త AstIf{flp, కొత్త AstVarRef{flp, drivenVarp, VAccess::READ},
                            కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                          కొత్త AstConst{flp, AstConst::బిట్టు_False{}}}};
            ifp->కూడు_Thensp(assignp);
            // Only in the time step of the clocking event
            AstCMethodHard* స్థిరరాశి trigp = కొత్త AstCMethodHard{
                nodep->fileline(), కొత్త AstVarRef{flp, m_clockingp->ensureEventp(), VAccess::READ},
                VCMethod::EVENT_IS_TRIGGERED};
            trigp->dtype_సమితి_బిట్టు();
            ifp->condp(కొత్త AstLogAnd{flp, ifp->condp()->unlink_Fr_వెనుక(), trigp});
            // A skew rounding to no time precision ticks is a zero skew
            Ast_కణుపు_సమాసం* స్థిరరాశి ticksp
                = V3Timing::delay_గమ్యం_Ticks(skewp->clone_వృక్షం(అబద్ధం), m_modp->timeunit());
            స్థిరరాశి తార్కిక zeroSkew = ticksp->isZero();
            VL_DO_DANGLING(ticksp->delete_వృక్షం(), ticksp);
            ఒకవేళ (zeroSkew || !skewp->fileline()->timingOn()) {
                // Drive the var in Re-NBA (IEEE 1800-2023 14.16), ignoring skews where timing is
                // turned off
                Ast_Sen_వృక్షం* senTreep = m_clockingp->sentreep()->clone_వృక్షం(అబద్ధం);
                senTreep->కూడు_Sensesp(కొత్త Ast_Sen_అంశం{
                    flp, VEdgeType::ET_మారింది, కొత్త AstVarRef{flp, drivenVarp, VAccess::READ}});
                m_clockingp->కూడు_తదుపరి_ఇక్కడ(కొత్త AstAlwaysReactive{flp, senTreep, ifp});
            } లేకపోతే {
                // Fork only the assignment, so this AlwaysPostponed can be retriggered
                // before the assignment happens
                AstFork* స్థిరరాశి forkp = కొత్త AstFork{flp, VJoinType::JOIN_NONE};
                forkp->తక్షణ_ప్రారంభం(నిజం);
                forkp->కూడు_Forksp(కొత్త AstBegin{flp, "", assignp->unlink_Fr_వెనుక(), నిజం});
                ifp->కూడు_Thensp(forkp);
                // Read the clockvar in Postponed, so a drive late in the time step is applied
                // at this event plus the skew (IEEE 1800-2023 14.16)
                m_clockingp->కూడు_తదుపరి_ఇక్కడ(కొత్త AstAlwaysPostponed{flp, ifp});
                ఒకవేళ (v3Global.opt.timing().is_సమితి_True()) {
                    AstDelay* స్థిరరాశి delayp = కొత్త AstDelay{flp, skewp->unlink_Fr_వెనుక(), అబద్ధం};
                    delayp->timeunit(m_modp->timeunit());
                    assignp->timingControlp(delayp);
                } లేకపోతే ఒకవేళ (v3Global.opt.timing().is_సమితి_False()) {
                    nodep->v3warn(E_NOTIMING,
                                  "Clocking output skew greater than #0 requires --timing");
                } లేకపోతే {
                    nodep->v3warn(E_NEEDTIMINGOPT,
                                  "Use --timing or --no-timing to specify how "
                                  "clocking output skew greater than #0 should be handled");
                }
            }
        } లేకపోతే ఒకవేళ (nodep->direction() == VDirection::ఆదానం) {
            // Ref to the clockvar
            AstVarRef* స్థిరరాశి refp = కొత్త AstVarRef{flp, varp, VAccess::WRITE};
            refp->user1(నిజం);
            ఒకవేళ (skewp->అంకె().is1_అడుగు()) {
                // #1step means the value that is sampled is always the signal's last value
                // before the clock edge (IEEE 1800-2023 14.4)
                AstSampled* స్థిరరాశి sampledp
                    = కొత్త AstSampled{flp, exprp->clone_వృక్షం_శుద్ధ(అబద్ధం), exprp->dtypep(), నిజం};
                Ast_కేటాయించు* స్థిరరాశి assignp = కొత్త Ast_కేటాయించు{flp, refp, sampledp};
                m_clockingp->కూడు_తదుపరి_ఇక్కడ(కొత్త AstAlways{
                    flp, VAlwaysKwd::ALWAYS, m_clockingp->sentreep()->clone_వృక్షం(అబద్ధం), assignp});
            } లేకపోతే ఒకవేళ (skewp->isZero()) {
                // #0 means the var has to be sampled in Observed (IEEE 1800-2023 14.13)
                Ast_కేటాయించు* స్థిరరాశి assignp = కొత్త Ast_కేటాయించు{flp, refp, exprp->clone_వృక్షం_శుద్ధ(అబద్ధం)};
                m_clockingp->కూడు_తదుపరి_ఇక్కడ(కొత్త AstAlwaysObserved{
                    flp, m_clockingp->sentreep()->clone_వృక్షం(అబద్ధం), assignp});
            } లేకపోతే {
                // Create a queue where we'll store sampled values with timestamps
                Ast_Sample_వరుస_DType* స్థిరరాశి వరుస_Dtp
                    = కొత్త Ast_Sample_వరుస_DType{flp, exprp->dtypep()};
                m_netlistp->typeTablep()->కూడు_Typesp(వరుస_Dtp);
                AstVar* స్థిరరాశి వరుస_Varp
                    = కొత్త AstVar{flp, VVarType::MODULETEMP, "__Vqueue__" + varp->name(), వరుస_Dtp};
                వరుస_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_clockingp->కూడు_తదుపరి_ఇక్కడ(వరుస_Varp);
                // Create a process like this:
                //     always queue.push(<sampled var>);
                AstCMethodHard* స్థిరరాశి pushp = కొత్త AstCMethodHard{
                    flp, కొత్త AstVarRef{flp, వరుస_Varp, VAccess::WRITE}, VCMethod::DYN_నెట్టు,
                    కొత్త Ast_సమయం{nodep->fileline(), m_modp->timeunit()}};
                pushp->కూడు_Pinsp(exprp->clone_వృక్షం_శుద్ధ(అబద్ధం));
                pushp->dtype_సమితి_Void();
                m_clockingp->కూడు_తదుపరి_ఇక్కడ(
                    కొత్త AstAlways{flp, VAlwaysKwd::ALWAYS, శూన్యసూచిక, pushp->makeStmt()});
                // Create a process like this:
                //     always @<clocking event> queue.pop(<skew>, /*out*/<skewed var>);
                AstCMethodHard* స్థిరరాశి popp = కొత్త AstCMethodHard{
                    flp, కొత్త AstVarRef{flp, వరుస_Varp, VAccess::READWRITE}, VCMethod::DYN_తీయి,
                    కొత్త Ast_సమయం{nodep->fileline(), m_modp->timeunit()}};
                popp->కూడు_Pinsp(skewp->unlink_Fr_వెనుక());
                refp->access(VAccess::READWRITE);  // Only conditionally assigned
                popp->కూడు_Pinsp(refp);
                popp->dtype_సమితి_Void();
                m_clockingp->కూడు_తదుపరి_ఇక్కడ(కొత్త AstAlways{flp, VAlwaysKwd::ALWAYS,
                                                       m_clockingp->sentreep()->clone_వృక్షం(అబద్ధం),
                                                       popp->makeStmt()});
            }
        } లేకపోతే {
            nodep->v3fatalSrc("Invalid direction");
        }
    }
    శూన్య సందర్శించు(AstDelay* nodep) override {
        m_has_చక్రం_Delay |= nodep->is_చక్రం_Delay();
        // Only cycle delays are relevant in this stage; also only process once
        ఒకవేళ (!nodep->is_చక్రం_Delay()) {
            ఒకవేళ (m_లోపల_Synch_Drive) {
                nodep->v3error("Only cycle delays can be used in synchronous drives"
                               " (IEEE 1800-2023 14.16)");
            }
            iterateChildren(nodep);
            తిరిగి;
        }
        ఒకవేళ (m_లోపల_కేటాయించు && !m_లోపల_Synch_Drive) {
            nodep->v3error("Cycle delays not allowed as intra-assignment delays"
                           " (IEEE 1800-2023 14.11)");
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
            తిరిగి;
        }
        ఒకవేళ (nodep->stmtsp()) nodep->కూడు_తదుపరి_ఇక్కడ(nodep->stmtsp()->unlink_Fr_వెనుక_With_తదుపరి());
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        Ast_కణుపు_సమాసం* valuep = V3Const::constifyEdit(nodep->lhsp()->unlink_Fr_వెనుక());
        స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(valuep, Const);
        ఒకవేళ (!constp && m_pexprp) {
            // V3AssertNfa handles non-const delays in sequences before this pass and
            // replaces the property; this branch should never be reached.
            nodep->v3fatalSrc("Non-constant cycle delay in assertion: "
                              "should have been caught by V3AssertNfa");
        } లేకపోతే ఒకవేళ (constp && constp->isZero()) {
            VL_DO_DANGLING(నెట్టు_Deletep(valuep), valuep);
            ఒకవేళ (m_లోపల_Synch_Drive) {
                // ##0 has no effect in synchronous drives (IEEE 1800-2023 14.11)
                VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
                తిరిగి;
            }
            ఒకవేళ (m_pexprp) {
                // ##0 in sequence context = zero delay = same clock tick
                VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
                తిరిగి;
            }
            // Procedural ##0: synchronize with default clocking event (IEEE 1800-2023 14.11)
            ఒకవేళ (!m_defaultClockingp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
                తిరిగి;
            }
            nodep->replaceWith(new_చక్రం_Delay_Zerop(flp));
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి;
        }
        Ast_Sen_వృక్షం* sentreep = శూన్యసూచిక;
        ఒకవేళ (!m_drives.ఖాళీలు()) {
            ఒకవేళ (m_లోపల_Concat_కేటాయించు) {
                // Also the clockvars may be of different clocking blocks
                nodep->v3error("Cycle delays cannot be used in synchronous drives to a"
                               " concatenation (IEEE 1800-2023 14.16)");
                VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
                VL_DO_DANGLING(నెట్టు_Deletep(valuep), valuep);
                తిరిగి;
            }
            // Count the cycles of the driven clockvar's clocking block (IEEE 1800-2023 14.16)
            sentreep = newDriveSentreep(nodep, m_drives.front());
            ఒకవేళ (!sentreep) {
                VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
                VL_DO_DANGLING(నెట్టు_Deletep(valuep), valuep);
                తిరిగి;
            }
        } లేకపోతే ఒకవేళ (!m_defaultClockingp) {
            ఒకవేళ (!m_pexprp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
                VL_DO_DANGLING(నెట్టు_Deletep(valuep), valuep);
                తిరిగి;
            }
            sentreep = కొత్త Ast_Sen_వృక్షం{flp, m_senip->clone_వృక్షం(అబద్ధం)};
        } లేకపోతే {
            sentreep = m_defaultClockingp->sentreep()->clone_వృక్షం(అబద్ధం);
        }
        AstEventControl* స్థిరరాశి controlp
            = కొత్త AstEventControl{nodep->fileline(), sentreep, శూన్యసూచిక};
        స్థిరరాశి std::string delayName = m_చక్రం_Dly_పేర్లు.get(nodep);
        Ast_కణుపు_సమాసం* throughoutp
            = nodep->throughoutp() ? nodep->throughoutp()->unlink_Fr_వెనుక() : శూన్యసూచిక;

        AstVar* స్థిరరాశి cntVarp = కొత్త AstVar{flp, VVarType::BLOCKTEMP, delayName + "__counter",
                                           nodep->కనుగొను_Basic_DType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstBegin* స్థిరరాశి beginp = కొత్త AstBegin{flp, delayName + "__block", cntVarp, నిజం};
        // A non-constant delay is counted at runtime. The count is integral of 64 bits, as
        // V3Width makes delays, or real, which is rounded (IEEE 1800-2023 6.12.1)
        ఒకవేళ (valuep->isDouble()) {
            valuep = కొత్త AstRToIRoundS{flp, valuep};
            valuep->dtypeFrom(cntVarp);
        } లేకపోతే {
            valuep = కొత్త AstSel{flp, valuep, 0, cntVarp->వెడల్పు()};
        }
        beginp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, cntVarp, VAccess::WRITE}, valuep});
        // If the count is zero, a synchronous drive waits for none, like with a constant '##0',
        // and a procedural delay waits like a procedural '##0' (IEEE 1800-2023 14.11)
        ఒకవేళ (!constp && !m_లోపల_Synch_Drive) {
            beginp->కూడు_Stmtsp(కొత్త AstIf{
                flp,
                కొత్త AstEq{flp, కొత్త AstVarRef{flp, cntVarp, VAccess::READ}, కొత్త AstConst{flp, 0}},
                new_చక్రం_Delay_Zerop(flp)});
        }

        // Throughout: create flag tracking whether condition held every tick
        AstVar* throughoutOkp = శూన్యసూచిక;
        ఒకవేళ (throughoutp) {
            throughoutOkp = కొత్త AstVar{flp, VVarType::BLOCKTEMP, delayName + "__throughoutOk",
                                       nodep->కనుగొను_Basic_DType(VBasicDTypeKwd::LOGIC_IMPLICIT)};
            throughoutOkp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->కూడు_Stmtsp(throughoutOkp);
            beginp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            కొత్త AstConst{flp, AstConst::బిట్టు_True{}}});
            // Check condition at tick 0 (sequence start, before entering loop)
            beginp->కూడు_Stmtsp(
                కొత్త AstIf{flp, కొత్త AstLogNot{flp, throughoutp->clone_వృక్షం_శుద్ధ(అబద్ధం)},
                          కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                        కొత్త AstConst{flp, AstConst::బిట్టు_False{}}}});
        }

        {
            AstLoop* స్థిరరాశి loopp = కొత్త AstLoop{flp};
            // When throughout is present, exit loop early if condition fails
            Ast_కణుపు_సమాసం* loopCondp
                = కొత్త AstGt{flp, కొత్త AstVarRef{flp, cntVarp, VAccess::READ}, కొత్త AstConst{flp, 0}};
            ఒకవేళ (throughoutOkp) {
                loopCondp = కొత్త AstLogAnd{flp, loopCondp,
                                          కొత్త AstVarRef{flp, throughoutOkp, VAccess::READ}};
            }
            loopp->కూడు_Stmtsp(కొత్త AstLoopTest{flp, loopp, loopCondp});
            loopp->కూడు_Stmtsp(controlp);
            loopp->కూడు_Stmtsp(
                కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, cntVarp, VAccess::WRITE},
                              కొత్త AstSub{flp, కొత్త AstVarRef{flp, cntVarp, VAccess::READ},
                                         కొత్త AstConst{flp, 1}}});
            // Check throughout condition at each tick during delay (IEEE 1800-2023 16.9.9)
            ఒకవేళ (throughoutp) {
                loopp->కూడు_Stmtsp(
                    కొత్త AstIf{flp, కొత్త AstLogNot{flp, throughoutp},
                              కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            కొత్త AstConst{flp, AstConst::బిట్టు_False{}}}});
            }
            beginp->కూడు_Stmtsp(loopp);
        }
        // Compose wrappers on remaining sequence: throughout gate (inner), disable iff (outer)
        Ast_కణుపు* remainp = nodep->nextp() ? nodep->nextp()->unlink_Fr_వెనుక_With_తదుపరి() : శూన్యసూచిక;
        ఒకవేళ (throughoutOkp) {
            // If condition failed during delay, fail assertion
            remainp = కొత్త AstIf{flp, కొత్త AstVarRef{flp, throughoutOkp, VAccess::READ}, remainp,
                                కొత్త AstPExprClause{flp, /*pass=*/అబద్ధం}};
        }
        ఒకవేళ (m_disableSeqIfp && remainp) {
            AstIf* స్థిరరాశి disableSeqIfp = m_disableSeqIfp->clone_వృక్షం(అబద్ధం);
            disableSeqIfp->కూడు_Thensp(remainp);
            remainp = disableSeqIfp;
        }
        ఒకవేళ (remainp) {
            ఒకవేళ (throughoutOkp) {
                // throughoutOkp is declared in beginp scope -- check must be inside it
                beginp->కూడు_Stmtsp(remainp);
            } లేకపోతే {
                nodep->కూడు_తదుపరి_ఇక్కడ(remainp);
            }
        }
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
    }
    శూన్య సందర్శించు(Ast_Sen_వృక్షం* nodep) override {
        ఒకవేళ (m_లోపల_Synch_Drive) {
            nodep->v3error("Event controls cannot be used in "
                           "synchronous drives (IEEE 1800-2023 14.16)");
        }

        స్థిరరాశి AstSampled* sampledp;
        ఒకవేళ (nodep->exists([&sampledp](స్థిరరాశి AstSampled* స్థిరరాశి sp) {
                sampledp = sp;
                తిరిగి నిజం;
            })) {
            sampledp->v3warn(E_UNSUPPORTED, "Unsupported: $sampled inside sensitivity list");
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_Var_Ref* nodep) override {
        UINFO(8, " -varref:  " << nodep);
        UINFO(8, " -varref-var-back:  " << nodep->varp()->backp());
        UINFO(8, " -varref-var-user1:  " << nodep->varp()->user1p());
        ఒకవేళ (Ast_Clocking_అంశం* స్థిరరాశి itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->మొదటి_Abovep(),
                Clocking_అంశం)) {
            ఒకవేళ (nodep->user1()) తిరిగి;

            // ensure linking still works, this has to be done only once
            ఒకవేళ (AstVarXRef* xrefp = VN_CAST(nodep, VarXRef)) {
                UINFO(8, " -clockvarxref-in:  " << xrefp);
                string dotted = xrefp->dotted();
                స్థిరరాశి కొలత_t dotPos = dotted.rfind('.');
                dotted.erase(dotPos, string::npos);
                xrefp->dotted(dotted);
                UINFO(8, " -clockvarxref-out: " << xrefp);
                m_xrefsp.emplace_వెనుక(xrefp);
            }

            ఒకవేళ (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            ఒకవేళ (nodep->access().isWriteOrRW()) {
                ఒకవేళ (itemp->direction() == VDirection::OUTPUT) {
                    ఒకవేళ (!m_లోపల_కేటాయించు_Dly_Lhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    ఒకవేళ (m_లోపల_కేటాయించు) {
                        m_లోపల_Synch_Drive = నిజం;
                        m_drives.నెట్టు_వెనుక({itemp, nodep});
                    }
                } లేకపోతే ఒకవేళ (itemp->direction() == VDirection::ఆదానం) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
            nodep->user1(నిజం);
        }
    }
    శూన్య సందర్శించు(AstMemberSel* nodep) override {
        ఒకవేళ (Ast_Clocking_అంశం* స్థిరరాశి itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->మొదటి_Abovep(),
                Clocking_అంశం)) {
            ఒకవేళ (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            ఒకవేళ (nodep->access().isWriteOrRW()) {
                ఒకవేళ (itemp->direction() == VDirection::OUTPUT) {
                    ఒకవేళ (!m_లోపల_కేటాయించు_Dly_Lhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    ఒకవేళ (m_లోపల_కేటాయించు) {
                        m_లోపల_Synch_Drive = నిజం;
                        m_drives.నెట్టు_వెనుక({itemp, nodep});
                    }
                } లేకపోతే ఒకవేళ (itemp->direction() == VDirection::ఆదానం) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_కేటాయించు* nodep) override {
        ఒకవేళ (nodep->user1()) తిరిగి;
        VL_RESTORER(m_లోపల_కేటాయించు);
        VL_RESTORER(m_లోపల_Synch_Drive);
        VL_RESTORER(m_లోపల_Concat_కేటాయించు);
        VL_RESTORER_CLEAR(m_drives);
        m_లోపల_కేటాయించు = నిజం;
        m_లోపల_Synch_Drive = అబద్ధం;
        {
            // The braces of a concatenation are a replication, by one
            స్థిరరాశి Ast_కణుపు_సమాసం* lhsp = nodep->lhsp();
            ఒకవేళ (స్థిరరాశి AstReplicate* స్థిరరాశి repp = VN_CAST(lhsp, Replicate)) lhsp = repp->srcp();
            m_లోపల_Concat_కేటాయించు = VN_IS(lhsp, Concat);
        }
        {
            VL_RESTORER(m_లోపల_కేటాయించు_Dly_Lhs);
            m_లోపల_కేటాయించు_Dly_Lhs = VN_IS(nodep, కేటాయించు_Dly);
            iterate(nodep->lhsp());
        }
        iterate(nodep->rhsp());
        ఒకవేళ (nodep->timingControlp()) iterate(nodep->timingControlp());
        // A drive without a cycle delay, also after removing '##0', updates the clockvar now
        ఒకవేళ (m_లోపల_Synch_Drive && !nodep->timingControlp()) {
            Ast_కేటాయించు* స్థిరరాశి assignp = కొత్త Ast_కేటాయించు{
                nodep->fileline(), nodep->lhsp()->unlink_Fr_వెనుక(), nodep->rhsp()->unlink_Fr_వెనుక()};
            assignp->user1(నిజం);
            nodep->replaceWith(assignp);
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
            nodep = assignp;
        }
        // Note the drives when they take effect, after the cycle delay if any. Each clockvar
        // of the LHS is driven, e.g. in a concatenation which other simulators accept, although
        // IEEE 1800-2023 14.16 does not allow it
        Ast_కణుపు* setsp = శూన్యసూచిక;
        కొరకు (స్థిరరాశి SynchDrive& drive : m_drives) {
            setsp = Ast_కణుపు::కూడు_తదుపరి(setsp, newDrivenSetp(nodep->fileline(), drive));
        }
        ఒకవేళ (!setsp) తిరిగి;
        ఒకవేళ (AstBegin* స్థిరరాశి beginp = VN_CAST(nodep->timingControlp(), Begin)) {
            beginp->కూడు_Stmtsp(setsp);
        } లేకపోతే {
            nodep->కూడు_తదుపరి_ఇక్కడ(setsp);
        }
    }
    శూన్య సందర్శించు(AstAlways* nodep) override {
        iterate_And_తదుపరి_Null(nodep->sentreep());
        ఒకవేళ (nodep->sentreep()) m_seniAlwaysp = nodep->sentreep()->sensesp();
        iterate_And_తదుపరి_Null(nodep->stmtsp());
        m_seniAlwaysp = శూన్యసూచిక;
    }

    శూన్య సందర్శించు(Ast_కణుపు_Cover_Or_Assert* nodep) override {
        ఒకవేళ (nodep->sentreep()) తిరిగి;  // Already processed

        VL_RESTORER(m_senip);
        VL_RESTORER(m_disablep);
        m_senip = శూన్యసూచిక;
        m_disablep = శూన్యసూచిక;

        // Find Clocking's buried under nodep->exprsp
        iterateChildren(nodep);
        ఒకవేళ (!nodep->తక్షణ()) nodep->sentreep(new_Sen_వృక్షం(nodep, శూన్యసూచిక, nodep));
    }
    శూన్య సందర్శించు(AstFalling* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        Ast_కణుపు_సమాసం* exprp = nodep->exprp()->unlink_Fr_వెనుక();
        ఒకవేళ (exprp->వెడల్పు() > 1) exprp = కొత్త AstSel{fl, exprp, 0, 1};
        Ast_కణుపు_సమాసం* స్థిరరాశి futurep = కొత్త AstFuture{fl, exprp, new_Sen_వృక్షం(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = కొత్త AstAnd{fl, exprp->clone_వృక్షం_శుద్ధ(అబద్ధం), కొత్త AstNot{fl, futurep}};
        exprp->dtype_సమితి_బిట్టు();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య సందర్శించు(AstFell* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        Ast_కణుపు_సమాసం* exprp = nodep->exprp()->unlink_Fr_వెనుక();
        ఒకవేళ (exprp->వెడల్పు() > 1) exprp = కొత్త AstSel{fl, exprp, 0, 1};
        Ast_Sen_వృక్షం* sentreep = nodep->sentreep();
        ఒకవేళ (sentreep) sentreep->unlink_Fr_వెనుక();
        AstPast* స్థిరరాశి pastp = కొత్త AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_వృక్షం(nodep, sentreep));
        exprp = కొత్త AstAnd{fl, pastp, కొత్త AstNot{fl, exprp->clone_వృక్షం_శుద్ధ(అబద్ధం)}};
        exprp->dtype_సమితి_బిట్టు();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య సందర్శించు(AstFuture* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        Ast_Sen_వృక్షం* స్థిరరాశి sentreep = nodep->sentreep();
        ఒకవేళ (sentreep) VL_DO_DANGLING(నెట్టు_Deletep(sentreep->unlink_Fr_వెనుక()), sentreep);
        nodep->sentreep(new_Sen_వృక్షం(nodep));
    }
    శూన్య సందర్శించు(AstPast* nodep) override {
        checkSamplingFuncDType(nodep, nodep->exprp());
        ఒకవేళ (nodep->sentreep()) తిరిగి;  // Already processed
        iterateChildren(nodep);
        nodep->sentreep(new_Sen_వృక్షం(nodep));
    }
    శూన్య సందర్శించు(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- Lower standalone exact [*N] (N >= 2) via saturating counter.
        // Range/unbounded forms and SExpr-contained forms are lowered by V3AssertNfa.
        iterateChildren(nodep);
        // V3AssertNfa handles unbounded/ranged forms upstream, so this fast-path
        // is effectively unreachable when NFA is enabled.
        ఒకవేళ (nodep->unbounded() || nodep->maxCountp()) తిరిగి;  // LCOV_EXCL_LINE
        స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(nodep->countp(), Const);
        ఒకవేళ (VL_UNLIKELY(!constp || constp->గమ్యం_SInt() < 1)) {
            nodep->v3fatalSrc("Consecutive repetition count must be a positive constant"
                              " (should have been caught by V3Width)");
            తిరిగి;
        }
        స్థిరరాశి పూర్ణాంకం n = constp->గమ్యం_SInt();
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి exprp = nodep->exprp()->unlink_Fr_వెనుక();
        ఒకవేళ (n == 1) {
            nodep->replaceWith(exprp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి;
        }
        // Saturating counter: if (expr) cnt <= min(cnt+1, N); else cnt <= 0;
        AstVar* స్థిరరాశి cntVarp = కొత్త AstVar{flp, VVarType::MODULETEMP, m_cons_Rep_పేర్లు.get(""),
                                           nodep->కనుగొను_Basic_DType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->కూడు_Stmtsp(cntVarp);
        Ast_కణుపు_సమాసం* స్థిరరాశి సమాసం_Clonep = exprp->clone_వృక్షం_శుద్ధ(అబద్ధం);
        Ast_కణుపు_సమాసం* స్థిరరాశి saturatingIncrp = కొత్త AstCond{
            flp,
            కొత్త AstLt{flp, కొత్త AstVarRef{flp, cntVarp, VAccess::READ},
                      కొత్త AstConst{flp, static_cast<uint32_t>(n)}},
            కొత్త Ast_కూడు{flp, కొత్త AstVarRef{flp, cntVarp, VAccess::READ}, కొత్త AstConst{flp, 1u}},
            కొత్త AstConst{flp, static_cast<uint32_t>(n)}};
        Ast_కేటాయించు_Dly* స్థిరరాశి incrAssignp
            = కొత్త Ast_కేటాయించు_Dly{flp, కొత్త AstVarRef{flp, cntVarp, VAccess::WRITE}, saturatingIncrp};
        Ast_కేటాయించు_Dly* స్థిరరాశి resetAssignp = కొత్త Ast_కేటాయించు_Dly{
            flp, కొత్త AstVarRef{flp, cntVarp, VAccess::WRITE}, కొత్త AstConst{flp, 0u}};
        AstIf* స్థిరరాశి ifp = కొత్త AstIf{flp, సమాసం_Clonep, incrAssignp, resetAssignp};
        Ast_Sen_వృక్షం* స్థిరరాశి senTreep = new_Sen_వృక్షం(nodep);
        AstAlways* స్థిరరాశి alwaysp = కొత్త AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, ifp};
        cntVarp->కూడు_తదుపరి_ఇక్కడ(alwaysp);
        // Match: cnt >= N-1 (previous cycles via NBA) && expr (current cycle)
        Ast_కణుపు_సమాసం* స్థిరరాశి cntCheckp = కొత్త AstGte{flp, కొత్త AstVarRef{flp, cntVarp, VAccess::READ},
                                                  కొత్త AstConst{flp, static_cast<uint32_t>(n - 1)}};
        cntCheckp->dtype_సమితి_బిట్టు();
        Ast_కణుపు_సమాసం* స్థిరరాశి matchp = కొత్త AstAnd{flp, cntCheckp, exprp};
        matchp->dtype_సమితి_బిట్టు();
        nodep->replaceWith(matchp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య సందర్శించు(AstRising* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        Ast_కణుపు_సమాసం* exprp = nodep->exprp()->unlink_Fr_వెనుక();
        ఒకవేళ (exprp->వెడల్పు() > 1) exprp = కొత్త AstSel{fl, exprp, 0, 1};
        Ast_కణుపు_సమాసం* స్థిరరాశి futurep = కొత్త AstFuture{fl, exprp, new_Sen_వృక్షం(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = కొత్త AstAnd{fl, కొత్త AstNot{fl, exprp->clone_వృక్షం_శుద్ధ(అబద్ధం)}, futurep};
        exprp->dtype_సమితి_బిట్టు();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య సందర్శించు(AstRose* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        Ast_కణుపు_సమాసం* exprp = nodep->exprp()->unlink_Fr_వెనుక();
        ఒకవేళ (exprp->వెడల్పు() > 1) exprp = కొత్త AstSel{fl, exprp, 0, 1};
        Ast_Sen_వృక్షం* sentreep = nodep->sentreep();
        ఒకవేళ (sentreep) sentreep->unlink_Fr_వెనుక();
        AstPast* స్థిరరాశి pastp = కొత్త AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_వృక్షం(nodep, sentreep));
        exprp = కొత్త AstAnd{fl, కొత్త AstNot{fl, pastp}, exprp->clone_వృక్షం_శుద్ధ(అబద్ధం)};
        exprp->dtype_సమితి_బిట్టు();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    స్థిర AstAssocArrayDType* getProcessAssocArrayType(File_పంక్తి* స్థిరరాశి flp) {
        // Type of __VassertsActive___x[std::process]
        Ast_కణుపు_DType* valp
            = v3Global.rootp()->typeTablep()->కనుగొను_Basic_DType(flp, VBasicDTypeKwd::బిట్టు);
        AstClassRefDType* keyp
            = కొత్త AstClassRefDType{flp, v3Global.rootp()->stdPackageProcessp(), శూన్యసూచిక};
        keyp->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        v3Global.rootp()->typeTablep()->కూడు_Typesp(keyp);
        AstAssocArrayDType* స్థిరరాశి typep = కొత్త AstAssocArrayDType{flp, valp, keyp};
        typep->dtypep(typep);
        v3Global.rootp()->typeTablep()->కూడు_Typesp(typep);
        తిరిగి typep;
    }
    స్థిర Ast_Stmt_సమాసం* getProcessAssocArrayDelete(AstVarRef* స్థిరరాశి refp) {
        // Constructs refp.delete(std::process::self()) statement
        File_పంక్తి* స్థిరరాశి flp = refp->fileline();
        AstCMethodHard* స్థిరరాశి deletep = కొత్త AstCMethodHard{
            flp, refp, VCMethod::ASSOC_ERASE, v3Global.rootp()->stdPackageProcessSelfp(flp)};
        deletep->dtypep(refp->కనుగొను_Void_DType());
        తిరిగి కొత్త Ast_Stmt_సమాసం{flp, deletep};
    }
    స్థిర Ast_కణుపు_సమాసం* get_Process_Assoc_Array_కొలత(AstVarRef* స్థిరరాశి refp) {
        // Constructs refp.size() statement
        AstCMethodHard* స్థిరరాశి sizep
            = కొత్త AstCMethodHard{refp->fileline(), refp, VCMethod::ASSOC_కొలత};
        sizep->dtypep(refp->కనుగొను_Basic_DType(VBasicDTypeKwd::UINT32));
        తిరిగి sizep;
    }
    శూన్య సందర్శించు(AstSEventually* nodep) override {
        UASSERT(v3Global.rootp()->stdPackagep(), "Should be imported");

        Ast_Sen_వృక్షం* స్థిరరాశి sentreep = new_Sen_వృక్షం(nodep);
        ఒకవేళ (!sentreep->sensesp()) {
            VL_DO_DANGLING(నెట్టు_Deletep(sentreep), sentreep);
            nodep->replaceWith(కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_False{}});
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి;
        }

        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();

        // Track active assertions
        AstVar* స్థిరరాశి activep = కొత్త AstVar{flp, VVarType::MODULETEMP, m_చురుకైన_పేర్లు.get(""),
                                           getProcessAssocArrayType(flp)};
        activep->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->కూడు_Stmtsp(activep);

        // Assertion condition check
        AstLoop* స్థిరరాశి loopp = కొత్త AstLoop{flp};
        AstSampled* స్థిరరాశి condp
            = కొత్త AstSampled{flp, nodep->exprp()->unlink_Fr_వెనుక(), శూన్యసూచిక, నిజం};
        loopp->కూడు_Stmtsp(కొత్త AstLoopTest{flp, loopp, కొత్త AstLogNot{flp, condp}});
        loopp->కూడు_Stmtsp(కొత్త AstEventControl{flp, sentreep, శూన్యసూచిక});

        // Add assertion to the active set
        AstAssocSel* స్థిరరాశి selp = కొత్త AstAssocSel{flp, కొత్త AstVarRef{flp, activep, VAccess::WRITE},
                                                  v3Global.rootp()->stdPackageProcessSelfp(flp)};
        Ast_కేటాయించు* స్థిరరాశి incrementp = కొత్త Ast_కేటాయించు{flp, selp, కొత్త AstConst{flp, 1}};
        AstPExprClause* స్థిరరాశి clausep = కొత్త AstPExprClause{flp};
        Ast_Stmt_సమాసం* స్థిరరాశి deletep
            = getProcessAssocArrayDelete(కొత్త AstVarRef{flp, activep, VAccess::WRITE});

        // Main assertion block
        AstBegin* స్థిరరాశి bodyp = కొత్త AstBegin{flp, m_ఖండం_పేర్లు.get(""), శూన్యసూచిక, నిజం};
        bodyp->కూడు_Stmtsp(incrementp);
        bodyp->కూడు_Stmtsp(loopp);
        bodyp->కూడు_Stmtsp(clausep);
        bodyp->కూడు_Stmtsp(deletep);

        // Validate assertion condition for each active assert
        AstVar* స్థిరరాశి చురుకైన_Countp = కొత్త AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                nodep->కనుగొను_Basic_DType(VBasicDTypeKwd::UINT32)};
        చురుకైన_Countp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        Ast_కేటాయించు* స్థిరరాశి init_చురుకైన_Countp
            = కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, చురుకైన_Countp, VAccess::WRITE},
                            get_Process_Assoc_Array_కొలత(కొత్త AstVarRef{flp, activep, VAccess::READ})};
        AstLoop* స్థిరరాశి finalLoopp = కొత్త AstLoop{flp};
        AstIf* స్థిరరాశి finalBodypCondp
            = కొత్త AstIf{flp, condp->clone_వృక్షం_శుద్ధ(అబద్ధం), కొత్త AstPExprClause{flp},
                        కొత్త AstPExprClause{flp, అబద్ధం}};
        finalLoopp->కూడు_Stmtsp(
            కొత్త AstLoopTest{flp, finalLoopp,
                            కొత్త AstNeq{flp, కొత్త AstVarRef{flp, చురుకైన_Countp, VAccess::READ},
                                       కొత్త AstConst{flp, 0}}});
        finalLoopp->కూడు_Stmtsp(finalBodypCondp);
        finalLoopp->కూడు_Stmtsp(
            కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, చురుకైన_Countp, VAccess::WRITE},
                          కొత్త AstSub{flp, కొత్త AstVarRef{flp, చురుకైన_Countp, VAccess::READ},
                                     కొత్త AstConst{flp, 1}}});

        // Final assertion block
        AstBegin* స్థిరరాశి finalp = కొత్త AstBegin{flp, m_ఖండం_పేర్లు.get(""), శూన్యసూచిక, నిజం};
        finalp->కూడు_Stmtsp(చురుకైన_Countp);
        finalp->కూడు_Stmtsp(init_చురుకైన_Countp);
        finalp->కూడు_Stmtsp(finalLoopp);

        VL_RESTORER(m_pexprp);
        m_pexprp = కొత్త AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
        VL_RESTORER(m_has_చక్రం_Delay);
        m_has_చక్రం_Delay = అబద్ధం;
        iterate(bodyp);
        iterate(finalp);

        ఒకవేళ (m_has_చక్రం_Delay) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in s_eventually");
            nodep->replaceWith(కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_False{}});
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            VL_DO_DANGLING(m_pexprp->delete_వృక్షం(), m_pexprp);
            తిరిగి;
        }

        nodep->replaceWith(m_pexprp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య సందర్శించు(AstStable* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        Ast_కణుపు_సమాసం* exprp = nodep->exprp()->unlink_Fr_వెనుక();
        Ast_Sen_వృక్షం* sentreep = nodep->sentreep();
        ఒకవేళ (sentreep) sentreep->unlink_Fr_వెనుక();
        AstPast* స్థిరరాశి pastp = కొత్త AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_వృక్షం(nodep, sentreep));
        exprp = కొత్త AstEq{fl, pastp, exprp->clone_వృక్షం_శుద్ధ(అబద్ధం)};
        exprp->dtype_సమితి_బిట్టు();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య సందర్శించు(AstSteady* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        Ast_కణుపు_సమాసం* exprp = nodep->exprp()->unlink_Fr_వెనుక();
        ఒకవేళ (exprp->వెడల్పు() > 1) exprp = కొత్త AstSel{fl, exprp, 0, 1};
        Ast_కణుపు_సమాసం* స్థిరరాశి futurep = కొత్త AstFuture{fl, exprp, new_Sen_వృక్షం(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = కొత్త AstEq{fl, exprp->clone_వృక్షం_శుద్ధ(అబద్ధం), futurep};
        exprp->dtype_సమితి_బిట్టు();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య సందర్శించు(AstSampled* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (!nodep->అంతర్గత_2()) checkSamplingFuncDType(nodep, nodep->exprp());
    }

    // Validate repetition count: must be a non-negative elaboration-time constant.
    // Shared by goto [->N] and nonconsecutive [=N] repetition.
    // On error, replaces nodep with BitFalse placeholder and returns nullptr.
    స్థిరరాశి AstConst* validate_Rep_లెక్క(Ast_కణుపు* nodep, Ast_కణుపు_సమాసం*& countp) {
        countp = V3Const::constifyEdit(countp);
        స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(countp, Const);
        ఒకవేళ (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(నెట్టు_Deletep(countp), countp);
            nodep->replaceWith(కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_False_Erroring{}});
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి శూన్యసూచిక;
        }
        ఒకవేళ (constp->గమ్యం_SInt() < 0) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(నెట్టు_Deletep(countp), countp);
            nodep->replaceWith(కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_False_Erroring{}});
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి శూన్యసూచిక;
        }
        ఒకవేళ (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(నెట్టు_Deletep(countp), countp);
            nodep->replaceWith(కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_False{}});
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి శూన్యసూచిక;
        }
        తిరిగి constp;
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
    AstPExpr* createRepPExpr(File_పంక్తి* flp, Ast_కణుపు_సమాసం* exprp, Ast_కణుపు_సమాసం* countp,
                             Ast_కణుపు_సమాసం* rhsp, తార్కిక isOverlapped, తార్కిక isNonConsec) {
        Ast_Sen_అంశం* స్థిరరాశి sensesp = m_senip;
        UASSERT_OBJ(sensesp, exprp, "Repetition requires a clock");

        స్థిరరాశి std::string name
            = isNonConsec ? m_non_Cons_Rep_పేర్లు.get(exprp) : m_goto_Rep_పేర్లు.get(exprp);
        AstVar* స్థిరరాశి cntVarp = కొత్త AstVar{flp, VVarType::BLOCKTEMP, name + "__counter",
                                           exprp->కనుగొను_Basic_DType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstBegin* స్థిరరాశి beginp = కొత్త AstBegin{flp, name + "__block", cntVarp, నిజం};
        beginp->కూడు_Stmtsp(
            కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, cntVarp, VAccess::WRITE}, కొత్త AstConst{flp, 0}});

        // Counting loop: find N occurrences of expr (shared by goto and nonconsec)
        AstLoop* స్థిరరాశి loopp = కొత్త AstLoop{flp};
        loopp->కూడు_Stmtsp(కొత్త AstLoopTest{flp, loopp,
                                         కొత్త AstLt{flp, కొత్త AstVarRef{flp, cntVarp, VAccess::READ},
                                                   countp->clone_వృక్షం_శుద్ధ(అబద్ధం)}});
        // if (expr) cnt++ -- sampled is applied to whole property expr by V3Assert
        loopp->కూడు_Stmtsp(
            కొత్త AstIf{flp, exprp,
                      కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, cntVarp, VAccess::WRITE},
                                    కొత్త Ast_కూడు{flp, కొత్త AstVarRef{flp, cntVarp, VAccess::READ},
                                               కొత్త AstConst{flp, 1}}}});
        loopp->కూడు_Stmtsp(కొత్త AstIf{
            flp, కొత్త AstLt{flp, కొత్త AstVarRef{flp, cntVarp, VAccess::READ}, countp},
            కొత్త AstEventControl{flp, కొత్త Ast_Sen_వృక్షం{flp, sensesp->clone_వృక్షం(అబద్ధం)}, శూన్యసూచిక}});

        beginp->కూడు_Stmtsp(loopp);

        ఒకవేళ (isNonConsec && rhsp) {
            // IEEE 16.9.2: b[=N] = b[->N] ##1 !b[*0:$]
            // Trailing window: ##1 advance, then scan !expr cycles checking rhs.
            // Window closes when expr becomes true again (fail if rhs was never true).
            beginp->కూడు_Stmtsp(కొత్త AstEventControl{
                flp, కొత్త Ast_Sen_వృక్షం{flp, sensesp->clone_వృక్షం(అబద్ధం)}, శూన్యసూచిక});  // ##1
            ఒకవేళ (!isOverlapped) {
                beginp->కూడు_Stmtsp(కొత్త AstEventControl{
                    flp, కొత్త Ast_Sen_వృక్షం{flp, sensesp->clone_వృక్షం(అబద్ధం)}, శూన్యసూచిక});  // |=>
            }
            // Window loop: check rhs at each !expr cycle (done variable for termination)
            AstVar* స్థిరరాశి పూర్తి_Varp
                = కొత్త AstVar{flp, VVarType::BLOCKTEMP, name + "__done", exprp->కనుగొను_బిట్టు_DType()};
            పూర్తి_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->కూడు_Stmtsp(పూర్తి_Varp);
            beginp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, పూర్తి_Varp, VAccess::WRITE},
                                            కొత్త AstConst{flp, AstConst::బిట్టు_False{}}});
            స్వయంచాలక సమితి_పూర్తి = [&]() {
                తిరిగి కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, పూర్తి_Varp, VAccess::WRITE},
                                     కొత్త AstConst{flp, AstConst::బిట్టు_True{}}};
            };
            AstLoop* స్థిరరాశి windowp = కొత్త AstLoop{flp};
            // LoopTest: continue while !done
            windowp->కూడు_Stmtsp(కొత్త AstLoopTest{
                flp, windowp, కొత్త AstNot{flp, కొత్త AstVarRef{flp, పూర్తి_Varp, VAccess::READ}}});
            // if (expr) { fail; done = 1; } -- window closed, expr true again
            Ast_కణుపు* స్థిరరాశి failsp = కొత్త AstPExprClause{flp, అబద్ధం};
            failsp->కూడు_తదుపరి(సమితి_పూర్తి());
            windowp->కూడు_Stmtsp(కొత్త AstIf{flp, exprp->clone_వృక్షం_శుద్ధ(అబద్ధం), failsp});
            // if (rhs) { pass; done = 1; } -- consequent true at this !expr endpoint
            Ast_కణుపు* స్థిరరాశి passsp = కొత్త AstPExprClause{flp, నిజం};
            passsp->కూడు_తదుపరి(సమితి_పూర్తి());
            windowp->కూడు_Stmtsp(కొత్త AstIf{flp, rhsp, passsp});
            // @(clk) -- advance to next cycle in window
            windowp->కూడు_Stmtsp(
                కొత్త AstEventControl{flp, కొత్త Ast_Sen_వృక్షం{flp, sensesp->clone_వృక్షం(అబద్ధం)}, శూన్యసూచిక});
            beginp->కూడు_Stmtsp(windowp);
        } లేకపోతే ఒకవేళ (isNonConsec) {
            // Standalone nonconsec: ##1 into window, then pass
            beginp->కూడు_Stmtsp(
                కొత్త AstEventControl{flp, కొత్త Ast_Sen_వృక్షం{flp, sensesp->clone_వృక్షం(అబద్ధం)}, శూన్యసూచిక});
            beginp->కూడు_Stmtsp(కొత్త AstPExprClause{flp, నిజం});
        } లేకపోతే ఒకవేళ (rhsp) {
            // Goto rep: check consequent once at match endpoint
            ఒకవేళ (!isOverlapped) {
                beginp->కూడు_Stmtsp(కొత్త AstEventControl{
                    flp, కొత్త Ast_Sen_వృక్షం{flp, sensesp->clone_వృక్షం(అబద్ధం)}, శూన్యసూచిక});
            }
            beginp->కూడు_Stmtsp(కొత్త AstIf{flp, rhsp, కొత్త AstPExprClause{flp, నిజం},
                                        కొత్త AstPExprClause{flp, అబద్ధం}});
        } లేకపోతే {
            // Standalone goto: pass after counting
            beginp->కూడు_Stmtsp(కొత్త AstPExprClause{flp, నిజం});
        }

        తిరిగి కొత్త AstPExpr{flp, beginp, exprp->కనుగొను_బిట్టు_DType()};
    }

    // Reject [=M:N] nonconsecutive range that reached V3AssertPre.
    // [->M:N] is fully handled upstream (NFA path); only [=M:N] still lands
    // here because AstSNonConsRep lowering is not yet implemented there.
    // Replaces the node with BitFalse so lowering continues after the warning.
    తార్కిక rejectNonconsecRange(Ast_కణుపు* nodep, Ast_కణుపు_సమాసం* maxCountp) {
        ఒకవేళ (!maxCountp) తిరిగి అబద్ధం;
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: [=M:N] nonconsecutive range repetition"
                                     " (IEEE 1800-2023 16.9.2)");
        nodep->replaceWith(కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_False{}});
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        తిరిగి నిజం;
    }

    శూన్య సందర్శించు(AstSGotoRep* nodep) override {
        // Standalone goto rep (not inside implication antecedent)
        iterateChildren(nodep);
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        Ast_కణుపు_సమాసం* countp = nodep->countp()->unlink_Fr_వెనుక();
        ఒకవేళ (!validate_Rep_లెక్క(nodep, countp)) తిరిగి;
        Ast_కణుపు_సమాసం* స్థిరరాశి exprp = nodep->exprp()->unlink_Fr_వెనుక();
        AstPExpr* స్థిరరాశి pexprp = createRepPExpr(flp, exprp, countp, శూన్యసూచిక, నిజం, అబద్ధం);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }

    శూన్య సందర్శించు(AstSNonConsRep* nodep) override {
        // Standalone nonconsecutive rep (not inside implication antecedent)
        iterateChildren(nodep);
        ఒకవేళ (rejectNonconsecRange(nodep, nodep->maxCountp())) తిరిగి;
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        Ast_కణుపు_సమాసం* countp = nodep->countp()->unlink_Fr_వెనుక();
        ఒకవేళ (!validate_Rep_లెక్క(nodep, countp)) తిరిగి;
        Ast_కణుపు_సమాసం* స్థిరరాశి exprp = nodep->exprp()->unlink_Fr_వెనుక();
        AstPExpr* స్థిరరాశి pexprp = createRepPExpr(flp, exprp, countp, శూన్యసూచిక, నిజం, నిజం);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }

    శూన్య సందర్శించు(AstFuncRef* nodep) override {
        // IEEE 1800-2023 16.8: Inline sequence instances wherever they appear
        // in the expression tree (inside implications, boolean ops, nested refs, etc.)
        ఒకవేళ (AstSequence* స్థిరరాశి seqp = VN_CAST(nodep->taskp(), Sequence)) {
            substituteSequenceCall(nodep, seqp);
            // The FuncRef has been replaced; do not access nodep after this point.
            // The replacement node will be visited by the parent's iterateChildren.
            తిరిగి;
        }
        ఒకవేళ (స్థిరరాశి AstProperty* స్థిరరాశి propp = VN_CAST(nodep->taskp(), Property)) {
            substituteNestedPropertyCall(nodep, propp);
            తిరిగి;
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstImplication* nodep) override {
        ఒకవేళ (nodep->sentreep()) తిరిగి;  // Already processed

        // Top-level followed-by is claimed by V3AssertNfa; anything reaching here
        // is nested inside iff/implies/or. IEEE 1800-2023 16.12.9 permits the
        // composition, but silent lowering would drop non-vacuous-fail semantics.
        ఒకవేళ (nodep->isFollowedBy()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: followed-by (#-# / #=#) nested inside property operator"
                          " (iff/implies/or) (IEEE 1800-2023 16.12.9)");
            nodep->replaceWith(కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_False{}});
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి;
        }

        // Handle goto repetition as antecedent before iterateChildren,
        // so the standalone AstSGotoRep visitor doesn't process it
        ఒకవేళ (AstSGotoRep* స్థిరరాశి gotop = VN_CAST(nodep->lhsp(), SGotoRep)) {
            iterateChildren(gotop);
            iterate_And_తదుపరి_Null(nodep->rhsp());
            File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
            Ast_కణుపు_సమాసం* countp = gotop->countp()->unlink_Fr_వెనుక();
            ఒకవేళ (!validate_Rep_లెక్క(nodep, countp)) తిరిగి;
            Ast_కణుపు_సమాసం* స్థిరరాశి exprp = gotop->exprp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = nodep->rhsp()->unlink_Fr_వెనుక();
            AstPExpr* స్థిరరాశి pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), అబద్ధం);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి;
        }

        // Handle nonconsecutive repetition as antecedent before iterateChildren,
        // so the standalone AstSNonConsRep visitor doesn't process it
        ఒకవేళ (AstSNonConsRep* స్థిరరాశి ncrp = VN_CAST(nodep->lhsp(), SNonConsRep)) {
            iterateChildren(ncrp);
            iterate_And_తదుపరి_Null(nodep->rhsp());
            ఒకవేళ (rejectNonconsecRange(nodep, ncrp->maxCountp())) తిరిగి;
            File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
            Ast_కణుపు_సమాసం* countp = ncrp->countp()->unlink_Fr_వెనుక();
            ఒకవేళ (!validate_Rep_లెక్క(nodep, countp)) తిరిగి;
            Ast_కణుపు_సమాసం* స్థిరరాశి exprp = ncrp->exprp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = nodep->rhsp()->unlink_Fr_వెనుక();
            AstPExpr* స్థిరరాశి pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), నిజం);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి;
        }

        iterateChildren(nodep);

        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = nodep->rhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* lhsp = nodep->lhsp()->unlink_Fr_వెనుక();

        // Lower sequence match items (IEEE 16.11): (expr, var = val, ...) |-> / |=>
        ఒకవేళ (Ast_సమాసం_Stmt* స్థిరరాశి సమాసం_Stmtp = VN_CAST(lhsp, సమాసం_Stmt)) {
            Ast_కణుపు_సమాసం* స్థిరరాశి antExprp = సమాసం_Stmtp->resultp()->unlink_Fr_వెనుక();

            ఒకవేళ (nodep->isOverlapped()) {
                // |-> : assign to __Vpropvar via always_comb (continuous).
                // The assign evaluates RHS once; V3Sampled snapshots the
                // result so all consequent refs read the same value.
                Ast_కణుపు* సరిపోలిక_Assignsp = శూన్యసూచిక;
                కొరకు (Ast_కణుపు* stmtp = సమాసం_Stmtp->stmtsp(); stmtp;) {
                    Ast_కణుపు* స్థిరరాశి nextp = stmtp->nextp();
                    ఒకవేళ (Ast_కేటాయించు* స్థిరరాశి assignp = VN_CAST(stmtp, కేటాయించు)) {
                        assignp->unlink_Fr_వెనుక();
                        సరిపోలిక_Assignsp = Ast_కణుపు::కూడు_తదుపరి_Null(సరిపోలిక_Assignsp, assignp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(నెట్టు_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                ఒకవేళ (సరిపోలిక_Assignsp) {
                    AstAlways* స్థిరరాశి alwaysp
                        = కొత్త AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, శూన్యసూచిక, సరిపోలిక_Assignsp};
                    m_modp->కూడు_Stmtsp(alwaysp);
                }
            } లేకపోతే {
                // |=> : assign to __Vpropvar via NBA in a clocked always block.
                // The NBA commits before the next cycle's sampled snapshot,
                // so the consequent (which already references __Vpropvar)
                // sees the captured value.
                Ast_కణుపు* సరిపోలిక_Assignsp = శూన్యసూచిక;
                కొరకు (Ast_కణుపు* stmtp = సమాసం_Stmtp->stmtsp(); stmtp;) {
                    Ast_కణుపు* స్థిరరాశి nextp = stmtp->nextp();
                    ఒకవేళ (Ast_కేటాయించు* స్థిరరాశి assignp = VN_CAST(stmtp, కేటాయించు)) {
                        assignp->unlink_Fr_వెనుక();
                        Ast_కణుపు_సమాసం* స్థిరరాశి కేటాయించు_Lhsp = assignp->lhsp()->unlink_Fr_వెనుక();
                        Ast_కణుపు_సమాసం* స్థిరరాశి కేటాయించు_Rhsp = assignp->rhsp()->unlink_Fr_వెనుక();
                        Ast_కేటాయించు_Dly* స్థిరరాశి dlyp = కొత్త Ast_కేటాయించు_Dly{flp, కేటాయించు_Lhsp, కేటాయించు_Rhsp};
                        VL_DO_DANGLING(నెట్టు_Deletep(assignp), assignp);
                        సరిపోలిక_Assignsp = Ast_కణుపు::కూడు_తదుపరి_Null(సరిపోలిక_Assignsp, dlyp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(నెట్టు_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                ఒకవేళ (సరిపోలిక_Assignsp) {
                    AstIf* స్థిరరాశి condp
                        = కొత్త AstIf{flp, antExprp->clone_వృక్షం_శుద్ధ(అబద్ధం), సరిపోలిక_Assignsp};
                    AstAlways* స్థిరరాశి alwaysp
                        = కొత్త AstAlways{flp, VAlwaysKwd::ALWAYS, new_Sen_వృక్షం(nodep), condp};
                    m_modp->కూడు_Stmtsp(alwaysp);
                }
            }
        }

        ఒకవేళ (AstPExpr* స్థిరరాశి pexprp = VN_CAST(rhsp, PExpr)) {
            // Implication with sequence expression on RHS (IEEE 1800-2023 16.11, 16.12.7).
            // The PExpr was lowered from the property expression earlier in this pass.
            // Wrap the PExpr body with the antecedent check so the sequence only
            // starts when the antecedent holds.
            Ast_కణుపు_సమాసం* condp;
            ఒకవేళ (nodep->isOverlapped()) {
                // Overlapped implication (|->): check antecedent on same cycle.
                // disable iff is applied at the assertion level, not at the
                // antecedent gate, matching the existing non-PExpr overlapped path.
                condp = lhsp;
            } లేకపోతే {
                // Non-overlapped implication (|=>): check antecedent from previous cycle
                ఒకవేళ (m_disablep) {
                    lhsp
                        = కొత్త AstAnd{flp, కొత్త AstNot{flp, m_disablep->clone_వృక్షం_శుద్ధ(అబద్ధం)}, lhsp};
                }
                AstPast* స్థిరరాశి pastp
                    = కొత్త AstPast{flp, lhsp, శూన్యసూచిక, శూన్యసూచిక, /* propertyTiming */ నిజం};
                pastp->dtypeFrom(lhsp);
                pastp->sentreep(new_Sen_వృక్షం(nodep));
                condp = pastp;
            }
            // Wrap existing PExpr body: if (antecedent) { <original body> } else { /* vacuous pass
            // */ }
            // Only the statements are guarded; declarations stay directly in the block. After
            // V3Fork moves the block's statements into a task, V3Task only handles variables
            // that are direct statements of the task.
            AstBegin* స్థిరరాశి bodyp = pexprp->bodyp();
            AstIf* స్థిరరాశి guardp = కొత్త AstIf{flp, condp};
            కొరకు (Ast_కణుపు* stmtp = bodyp->stmtsp(); stmtp;) {
                Ast_కణుపు* స్థిరరాశి nextp = stmtp->nextp();
                ఒకవేళ (!VN_IS(stmtp, Var)) guardp->కూడు_Thensp(stmtp->unlink_Fr_వెనుక());
                stmtp = nextp;
            }
            bodyp->కూడు_Stmtsp(guardp);
            nodep->replaceWith(pexprp);
            // Don't iterate pexprp here -- it was already iterated when created
            // (in visit(AstSExpr*)), so delays and disable iff are already processed.
        } లేకపోతే ఒకవేళ (nodep->isOverlapped()) {
            Ast_కణుపు_సమాసం* స్థిరరాశి exprp
                = m_underCover ? static_cast<Ast_కణుపు_సమాసం*>(కొత్త AstLogAnd{flp, lhsp, rhsp})
                               : కొత్త AstLogOr{flp, కొత్త AstLogNot{flp, lhsp}, rhsp};
            nodep->replaceWith(exprp);
        } లేకపోతే {
            ఒకవేళ (m_disablep) {
                lhsp = కొత్త AstAnd{flp, కొత్త AstNot{flp, m_disablep->clone_వృక్షం_శుద్ధ(అబద్ధం)}, lhsp};
            }

            AstPast* స్థిరరాశి pastp
                = కొత్త AstPast{flp, lhsp, శూన్యసూచిక, శూన్యసూచిక, /* propertyTiming */ నిజం};
            pastp->dtypeFrom(lhsp);
            pastp->sentreep(new_Sen_వృక్షం(nodep));
            Ast_కణుపు_సమాసం* స్థిరరాశి exprp
                = m_underCover ? static_cast<Ast_కణుపు_సమాసం*>(కొత్త AstAnd{flp, pastp, rhsp})
                               : కొత్త AstOr{flp, కొత్త AstNot{flp, pastp}, rhsp};
            exprp->dtype_సమితి_బిట్టు();
            nodep->replaceWith(exprp);
        }
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య సందర్శించు(AstUntil* nodep) override {
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        UASSERT_OBJ(
            !m_pexprp, nodep,
            "'" << nodep->verilogKwd()
                << "' in complex property expression should have been rejected by V3AssertNfa");
        ఒకవేళ (nodep->isStrong()
            && (v3Global.opt.timing().is_సమితి_False() || !v3Global.opt.timing().is_సమితి_True())) {
            nodep->v3warn(E_NOTIMING, nodep->verilogKwd() << " requires --timing");
            nodep->replaceWith(కొత్త AstConst{flp, AstConst::బిట్టు_False{}});
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి;
        }
        ఒకవేళ (nodep->isStrong()) {
            // p s_until q / p s_until_with q: q must eventually be true. Until then, p must
            // be true on every sampled tick. For s_until, check q first: when q is true on
            // this tick, p is not required. For s_until_with, p must be true on the q tick too.
            Ast_కణుపు_సమాసం* స్థిరరాశి rawLhsp = nodep->lhsp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* స్థిరరాశి rawRhsp = nodep->rhsp()->unlink_Fr_వెనుక();
            AstSampled* స్థిరరాశి lhsp = కొత్త AstSampled{flp, rawLhsp, rawLhsp->dtypep(), నిజం};
            AstSampled* స్థిరరాశి rhsp = కొత్త AstSampled{flp, rawRhsp, rawRhsp->dtypep(), నిజం};
            Ast_కణుపు_సమాసం* finalCondp = rhsp->clone_వృక్షం_శుద్ధ(అబద్ధం);
            ఒకవేళ (nodep->isOverlapping()) {
                finalCondp = కొత్త AstLogAnd{flp, lhsp->clone_వృక్షం_శుద్ధ(అబద్ధం), finalCondp};
            }

            // Track active assertion attempts. Only the count is needed to emit final failures.
            AstVar* స్థిరరాశి activep = కొత్త AstVar{flp, VVarType::MODULETEMP, m_చురుకైన_పేర్లు.get(""),
                                               nodep->కనుగొను_Basic_DType(VBasicDTypeKwd::UINT32)};
            activep->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->కూడు_Stmtsp(activep);

            AstVar* స్థిరరాశి donep
                = కొత్త AstVar{flp, VVarType::BLOCKTEMP, "__VassertDone", nodep->కనుగొను_బిట్టు_DType()};
            donep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            స్వయంచాలక సమితి_పూర్తి = [&]() {
                తిరిగి కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, donep, VAccess::WRITE},
                                     కొత్త AstConst{flp, AstConst::బిట్టు_True{}}};
            };

            AstLoop* స్థిరరాశి loopp = కొత్త AstLoop{flp};
            Ast_కేటాయించు* స్థిరరాశి decrementVar = కొత్త Ast_కేటాయించు{
                flp, కొత్త AstVarRef{flp, activep, VAccess::WRITE},
                కొత్త AstSub{flp, కొత్త AstVarRef{flp, activep, VAccess::READ}, కొత్త AstConst{flp, 1}}};
            loopp->కూడు_Stmtsp(కొత్త AstLoopTest{
                flp, loopp, కొత్త AstLogNot{flp, కొత్త AstVarRef{flp, donep, VAccess::READ}}});
            {
                Ast_కణుపు* స్థిరరాశి passp = కొత్త AstPExprClause{flp};
                passp->కూడు_తదుపరి(decrementVar);
                passp->కూడు_తదుపరి(సమితి_పూర్తి());
                Ast_కణుపు_సమాసం* passCondp = rhsp;
                ఒకవేళ (nodep->isOverlapping()) {
                    passCondp = కొత్త AstLogAnd{flp, lhsp->clone_వృక్షం_శుద్ధ(అబద్ధం), passCondp};
                }
                loopp->కూడు_Stmtsp(కొత్త AstIf{flp, passCondp, passp});
            }
            {
                Ast_కణుపు* స్థిరరాశి failp = కొత్త AstPExprClause{flp, అబద్ధం};
                failp->కూడు_తదుపరి(decrementVar->clone_వృక్షం(అబద్ధం));
                failp->కూడు_తదుపరి(సమితి_పూర్తి());
                loopp->కూడు_Stmtsp(కొత్త AstIf{
                    flp,
                    కొత్త AstLogAnd{flp,
                                  కొత్త AstLogNot{flp, కొత్త AstVarRef{flp, donep, VAccess::READ}},
                                  కొత్త AstLogNot{flp, lhsp}},
                    failp});
            }
            AstDelay* స్థిరరాశి delayp = కొత్త AstDelay{flp, కొత్త AstConst{flp, 1}, అబద్ధం};
            delayp->timeunit(m_modp->timeunit());
            loopp->కూడు_Stmtsp(కొత్త AstIf{
                flp, కొత్త AstLogNot{flp, కొత్త AstVarRef{flp, donep, VAccess::READ}}, delayp});

            // Main assertion block
            AstBegin* స్థిరరాశి bodyp = కొత్త AstBegin{flp, m_ఖండం_పేర్లు.get(""), శూన్యసూచిక, నిజం};
            bodyp->కూడు_Stmtsp(donep);
            bodyp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, donep, VAccess::WRITE},
                                           కొత్త AstConst{flp, AstConst::బిట్టు_False{}}});
            File_పంక్తి* స్థిరరాశి flp = activep->fileline();
            bodyp->కూడు_Stmtsp(
                కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, activep, VAccess::WRITE},
                              కొత్త Ast_కూడు{flp, కొత్త AstVarRef{flp, activep, VAccess::READ},
                                         కొత్త AstConst{flp, 1}}});
            bodyp->కూడు_Stmtsp(loopp);

            // Validate assertion condition for each active assert
            AstVar* స్థిరరాశి చురుకైన_Countp = కొత్త AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                    nodep->కనుగొను_Basic_DType(VBasicDTypeKwd::UINT32)};
            చురుకైన_Countp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

            Ast_కేటాయించు* స్థిరరాశి init_చురుకైన_Countp
                = కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, చురుకైన_Countp, VAccess::WRITE},
                                కొత్త AstVarRef{flp, activep, VAccess::READ}};
            AstLoop* స్థిరరాశి finalLoopp = కొత్త AstLoop{flp};
            finalLoopp->కూడు_Stmtsp(
                కొత్త AstLoopTest{flp, finalLoopp,
                                కొత్త AstNeq{flp, కొత్త AstVarRef{flp, చురుకైన_Countp, VAccess::READ},
                                           కొత్త AstConst{flp, 0}}});
            finalLoopp->కూడు_Stmtsp(కొత్త AstIf{flp, finalCondp, కొత్త AstPExprClause{flp},
                                            కొత్త AstPExprClause{flp, అబద్ధం}});
            finalLoopp->కూడు_Stmtsp(
                కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, చురుకైన_Countp, VAccess::WRITE},
                              కొత్త AstSub{flp, కొత్త AstVarRef{flp, చురుకైన_Countp, VAccess::READ},
                                         కొత్త AstConst{flp, 1}}});

            // Final assertion block
            AstBegin* స్థిరరాశి finalp = కొత్త AstBegin{flp, m_ఖండం_పేర్లు.get(""), శూన్యసూచిక, నిజం};
            finalp->కూడు_Stmtsp(చురుకైన_Countp);
            finalp->కూడు_Stmtsp(init_చురుకైన_Countp);
            finalp->కూడు_Stmtsp(finalLoopp);

            AstPExpr* స్థిరరాశి pexprp = కొత్త AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
            VL_RESTORER(m_pexprp);
            VL_RESTORER(m_has_చక్రం_Delay);
            m_pexprp = pexprp;
            m_has_చక్రం_Delay = అబద్ధం;
            iterate(bodyp);
            iterate(finalp);

            UASSERT_OBJ(!m_has_చక్రం_Delay, nodep,
                        "s_until cycle delay should have been handled by V3AssertNfa");

            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి;
        }
        AstLoop* స్థిరరాశి loopp = కొత్త AstLoop{flp};
        Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = nodep->rhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = nodep->lhsp()->unlink_Fr_వెనుక();
        AstLogAnd* స్థిరరాశి loopCondp = కొత్త AstLogAnd{flp, lhsp, కొత్త AstLogNot{flp, rhsp}};
        loopp->కూడు_Stmtsp(కొత్త AstLoopTest{flp, loopp, loopCondp});
        loopp->కూడు_Stmtsp(కొత్త AstEventControl{flp, new_Sen_వృక్షం(nodep), శూన్యసూచిక});

        Ast_కణుపు_సమాసం* స్థిరరాశి rhsCopyp = rhsp->clone_వృక్షం_శుద్ధ(అబద్ధం);
        Ast_కణుపు_సమాసం* స్థిరరాశి passCondp
            = nodep->isOverlapping() ? కొత్త AstLogAnd{flp, lhsp->clone_వృక్షం_శుద్ధ(అబద్ధం), rhsCopyp}
                                     : rhsCopyp;
        AstBegin* స్థిరరాశి beginp = కొత్త AstBegin{flp, m_ఖండం_పేర్లు.get(""), loopp, నిజం};
        beginp->కూడు_Stmtsp(
            కొత్త AstIf{flp, passCondp, కొత్త AstPExprClause{flp}, కొత్త AstPExprClause{flp, అబద్ధం}});

        AstPExpr* స్థిరరాశి pexprp = కొత్త AstPExpr{flp, beginp, nodep->dtypep()};
        pexprp->user1(1);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }

    శూన్య సందర్శించు(AstDefaultDisable* nodep) override {
        VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
    }
    శూన్య సందర్శించు(AstInferredDisable* nodep) override {
        Ast_కణుపు* newp;
        ఒకవేళ (m_defaultDisablep) {
            newp = m_defaultDisablep->condp()->clone_వృక్షం_శుద్ధ(నిజం);
        } లేకపోతే {
            newp = కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_False{}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }

    శూన్య సందర్శించు(AstPropSpec* nodep) override {
        nodep = substitutePropertyCall(nodep);
        iterate_And_తదుపరి_Null(nodep->sensesp());
        ఒకవేళ (m_senip && m_senip != nodep->sensesp())
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Only one PSL clock allowed per assertion");
        స్థిరరాశి AstCover* స్థిరరాశి coverp = VN_CAST(nodep->backp(), Cover);
        స్థిరరాశి తార్కిక seqEvent = coverp && coverp->isSeqEvent();
        ఒకవేళ (!nodep->disablep() && m_defaultDisablep && !seqEvent) {
            nodep->disablep(m_defaultDisablep->condp()->clone_వృక్షం_శుద్ధ(నిజం));
        }
        m_disablep = nodep->disablep();
        // Unlink and just keep a pointer to it, convert to sentree as needed
        m_senip = nodep->sensesp();
        iterateNull(nodep->disablep());
        ఒకవేళ (!VN_AS(nodep->backp(), కణుపు_Cover_Or_Assert)->తక్షణ()) {
            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి propDtp = nodep->propp()->dtypep();
            nodep->propp(కొత్త AstSampled{nodep->fileline(), nodep->propp()->unlink_Fr_వెనుక(),
                                        propDtp->dtypep(), నిజం});
        }
        // cover counts non-vacuous matches only (IEEE 1800-2023 16.15.2), so an
        // implication antecedent must hold; assert passes vacuously instead.
        VL_RESTORER(m_underCover);
        m_underCover = VN_IS(nodep->backp(), Cover);
        iterate(nodep->propp());
        iterateNull(nodep->సరిపోలిక_Countp());
    }
    శూన్య సందర్శించు(AstPExpr* nodep) override {
        // V3AssertNfa handles multi-cycle property expressions before this pass,
        // so the following unsupported paths are defensive and typically unreached.
        ఒకవేళ (m_pexprp && m_pexprp->user1()) {  // LCOV_EXCL_START
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Complex property expression inside 'until''");
            nodep->replaceWith(కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_False{}});
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి;
        }  // LCOV_EXCL_STOP
        ఒకవేళ (AstLogNot* స్థిరరాశి notp = VN_CAST(nodep->backp(), LogNot)) {
            notp->replaceWith(nodep->unlink_Fr_వెనుక());
            VL_DO_DANGLING(నెట్టు_Deletep(notp), notp);
            iterate(nodep);
            తిరిగి;
        }
        // Sequence expression as antecedent of implication is not yet supported
        ఒకవేళ (AstImplication* స్థిరరాశి implp = VN_CAST(nodep->backp(), Implication)) {
            ఒకవేళ (implp->lhsp() == nodep) {  // LCOV_EXCL_START
                implp->v3warn(E_UNSUPPORTED,
                              "Unsupported: Implication with sequence expression as antecedent");
                nodep->replaceWith(కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_False{}});
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                తిరిగి;
            }  // LCOV_EXCL_STOP
        }
        VL_RESTORER(m_pexprp);
        VL_RESTORER(m_disableSeqIfp);
        m_pexprp = nodep;

        ఒకవేళ (m_disablep) {
            స్థిరరాశి AstSampled* sampledp = శూన్యసూచిక;
            ఒకవేళ (m_disablep->exists([&sampledp](స్థిరరాశి AstSampled* స్థిరరాశి sp) {
                    sampledp = sp;
                    తిరిగి నిజం;
                })) {
                sampledp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: $sampled inside disabled condition of a sequence");
                m_disablep = కొత్త AstConst{m_disablep->fileline(), AstConst::బిట్టు_False{}};
                // always a copy is used, so remove it now
                నెట్టు_Deletep(m_disablep);  // m_disablep used later
            }
            File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
            // Add counter which counts times the condition turned true
            AstVar* స్థిరరాశి disableCntp
                = కొత్త AstVar{flp, VVarType::MODULETEMP, m_disable_Cnt_పేర్లు.get(""),
                             nodep->కనుగొను_Basic_DType(VBasicDTypeKwd::UINT32)};
            disableCntp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->కూడు_Stmtsp(disableCntp);
            AstVarRef* స్థిరరాశి readCntRefp = కొత్త AstVarRef{flp, disableCntp, VAccess::READ};
            AstVarRef* స్థిరరాశి writeCntRefp = కొత్త AstVarRef{flp, disableCntp, VAccess::WRITE};
            Ast_కేటాయించు* స్థిరరాశి incrStmtp = కొత్త Ast_కేటాయించు{
                flp, writeCntRefp, కొత్త Ast_కూడు{flp, readCntRefp, కొత్త AstConst{flp, 1}}};
            AstAlways* స్థిరరాశి alwaysp
                = కొత్త AstAlways{flp, VAlwaysKwd::ALWAYS,
                                కొత్త Ast_Sen_వృక్షం{flp, కొత్త Ast_Sen_అంశం{flp, VEdgeType::ET_POSEDGE,
                                                                   m_disablep->clone_వృక్షం(అబద్ధం)}},
                                incrStmtp};
            disableCntp->కూడు_తదుపరి_ఇక్కడ(alwaysp);

            // Store value of that counter at the beginning of sequence evaluation
            AstBegin* స్థిరరాశి bodyp = nodep->bodyp();
            AstVar* స్థిరరాశి initialCntp = కొత్త AstVar{flp, VVarType::BLOCKTEMP, "__VinitialCnt",
                                                   nodep->కనుగొను_Basic_DType(VBasicDTypeKwd::UINT32)};
            initialCntp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            Ast_కేటాయించు* స్థిరరాశి assignp
                = కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, initialCntp, VAccess::WRITE},
                                readCntRefp->clone_వృక్షం(అబద్ధం)};
            // Prepend to the sequence body to keep statement list structure valid.
            UASSERT_OBJ(!bodyp->name().ఖాళీలు(), bodyp, "Sequence body should be named");
            Ast_కణుపు* స్థిరరాశి newStmtsp = Ast_కణుపు::కూడు_తదుపరి<Ast_కణుపు, Ast_కణుపు>(initialCntp, assignp);
            bodyp->prependStmtsp(newStmtsp);

            m_disableSeqIfp
                = కొత్త AstIf{flp, కొత్త AstEq{flp, కొత్త AstVarRef{flp, initialCntp, VAccess::READ},
                                           readCntRefp->clone_వృక్షం(అబద్ధం)}};
            // Delete it, because it is always copied before insetion to the AST
            నెట్టు_Deletep(m_disableSeqIfp);  // m_disableSeqIfp used later
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultClkEvtVarp);
        VL_RESTORER(m_defaultDisablep);
        VL_RESTORER(m_modp);
        ఒకవేళ (!VN_IS(nodep, Class)) {
            // A class nested in a module is in the scope of the module's default clocking
            m_defaultClockingp = శూన్యసూచిక;
            m_defaultClkEvtVarp = శూన్యసూచిక;
        }
        m_defaultDisablep = nodep->defaultDisablep();
        m_modp = nodep;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_Gen_ఖండం* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstProperty* nodep) override {
        // The body will be visited when will be substituted in place of property reference
        // (AstFuncRef)
        VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
    }
    శూన్య సందర్శించు(AstSequence* nodep) override {
        // Sequence declarations are not visited directly; their bodies are inlined
        // at call sites by visit(AstFuncRef*). Collect for post-traversal cleanup.
        m_seqs_గమ్యం_Cleanup.నెట్టు_వెనుక(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన AssertPreVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        { AssertPreClockingVisitor{nodep}; }
        // Process
        iterate(nodep);
        // Fix up varref names
        కొరకు (AstVarXRef* xrefp : m_xrefsp) xrefp->name(xrefp->varp()->name());
        // Clean up sequence declarations after inlining.
        // Referenced sequences that were inlined have isReferenced cleared.
        // Remaining referenced sequences are in unsupported contexts (e.g. @seq event).
        కొరకు (AstSequence* seqp : m_seqs_గమ్యం_Cleanup) {
            ఒకవేళ (seqp->isReferenced()) {
                seqp->v3warn(E_UNSUPPORTED,
                             "Unsupported: sequence referenced outside assertion property");
            } లేకపోతే {
                VL_DO_DANGLING(seqp->unlink_Fr_వెనుక()->delete_వృక్షం(), seqp);
            }
        }
    }
    ~AssertPreVisitor() override = అప్రమేయ;
};

//######################################################################
// Top Assert class

శూన్య V3AssertPre::assertPreAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { AssertPreVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("assertpre", 0, dump_వృక్షం_Either_Level() >= 3);
}
