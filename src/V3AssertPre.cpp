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
    ಶೂನ್ಯ ಭೇಟಿ(AstNetlist* nodep) override { iterate_And_ಮುಂದಿನ_Const_Null(nodep->modulesp()); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override { iterate_And_ಮುಂದಿನ_Const_Null(nodep->stmtsp()); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Gen_ಖಂಡ* nodep) override { iterate_And_ಮುಂದಿನ_Const_Null(nodep->itemsp()); }
    ಶೂನ್ಯ ಭೇಟಿ(AstClocking* nodep) override {
        ಗಾಗಿ (Ast_ಗಂಟು* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            ಆದರೆ (Ast_Clocking_ಅಂಶ* ಸ್ಥಿರಾಂಕ citemp = VN_CAST(itemp, Clocking_ಅಂಶ)) {
                citemp->user3p(nodep);
            }
        }
    }
    // Clocking blocks are only in modules and their generate blocks
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು*) override {}

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ AssertPreClockingVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~AssertPreClockingVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Assert class functions

ವರ್ಗ AssertPreVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // Removes clocks and other pre-optimizations
    // Eventually inlines calls to sequences, properties, etc.
    // We're not parsing the tree, or anything more complicated.
ಖಾಸಗಿ:
    // TYPES
    ರಚನೆ SynchDrive final {
        Ast_Clocking_ಅಂಶ* itemp;  // Clocking item of the driven clockvar
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* refp;  // Reference to the driven clockvar
    };

    // NODE STATE
    // AstClockingItem::user1p()         // AstVar*.      varp() of ClockingItem after unlink
    // AstClockingItem::user2p()         // AstVar*.      Flag set by drives of output clockvar
    // AstClockingItem::user3p()         // AstClocking*. Clocking block of the item, captured
    //                                   //               by AssertPreClockingVisitor
    // AstPExpr::user1()                 // bool.         Created from AstUntil
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_inuser2;
    ಸ್ಥಿರಾಂಕ VNUser3_ಒಳ_Use m_inuser3;
    // STATE
    // Current context:
    AstNetlist* ಸ್ಥಿರಾಂಕ m_netlistp = ಶೂನ್ಯಸೂಚಕ;  // Current netlist
    Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Current module
    AstClocking* m_clockingp = ಶೂನ್ಯಸೂಚಕ;  // Current clocking block
    // Reset each module:
    AstClocking* m_defaultClockingp = ಶೂನ್ಯಸೂಚಕ;  // Default clocking for current module
    AstVar* m_defaultClkEvtVarp = ಶೂನ್ಯಸೂಚಕ;  // Event var for default clocking (for ##0)
    AstDefaultDisable* m_defaultDisablep = ಶೂನ್ಯಸೂಚಕ;  // Default disable for current module
    // Reset each assertion:
    Ast_Sen_ಅಂಶ* m_senip = ಶೂನ್ಯಸೂಚಕ;  // Last sensitivity
    // Reset each always:
    Ast_Sen_ಅಂಶ* m_seniAlwaysp = ಶೂನ್ಯಸೂಚಕ;  // Last sensitivity in always
    // Reset each assertion:
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_disablep = ಶೂನ್ಯಸೂಚಕ;  // Last disable
    AstIf* m_disableSeqIfp = ಶೂನ್ಯಸೂಚಕ;  // Used for handling disable iff in sequences
    AstPExpr* m_pexprp = ಶೂನ್ಯಸೂಚಕ;  // Last AstPExpr
    ತಾರ್ಕಿಕ m_underCover = ಸುಳ್ಳು;  // True if the enclosing assertion is a cover
    // Other:
    V3_Unique_ಹೆಸರುಗಳು m_ಚಕ್ರ_Dly_ಹೆಸರುಗಳು{"__VcycleDly"};  // Cycle delay counter name generator
    V3_Unique_ಹೆಸರುಗಳು m_cons_Rep_ಹೆಸರುಗಳು{"__VconsRep"};  // Consecutive repetition counter name generator
    V3_Unique_ಹೆಸರುಗಳು m_goto_Rep_ಹೆಸರುಗಳು{"__VgotoRep"};  // Goto repetition counter name generator
    V3_Unique_ಹೆಸರುಗಳು m_non_Cons_Rep_ಹೆಸರುಗಳು{"__VnonConsRep"};  // Nonconsecutive rep name generator
    V3_Unique_ಹೆಸರುಗಳು m_disable_Cnt_ಹೆಸರುಗಳು{"__VdisableCnt"};  // Disable condition counter name generator
    V3_Unique_ಹೆಸರುಗಳು m_ಖಂಡ_ಹೆಸರುಗಳು{"__VassertBlock"};  // Names of blocks with temporaries
    V3_Unique_ಹೆಸರುಗಳು m_prop_Var_ಹೆಸರುಗಳು{"__Vpropvar"};  // Property-local variable name generator
    V3_Unique_ಹೆಸರುಗಳು m_ಸಕ್ರಿಯ_ಹೆಸರುಗಳು{"__VassertsActive"};  // Active asserts map name generator
    V3_Unique_ಹೆಸರುಗಳು m_driven_ಹೆಸರುಗಳು{"__VclockingDriven"};  // Clockvar drive flag name generator
    ತಾರ್ಕಿಕ m_ಒಳ_ನಿಯೋಜಿಸು = ಸುಳ್ಳು;  // True if in an AssignNode
    ತಾರ್ಕಿಕ m_ಒಳ_ನಿಯೋಜಿಸು_Dly_Lhs = ಸುಳ್ಳು;  // True if in AssignDly's LHS
    ತಾರ್ಕಿಕ m_ಒಳ_Synch_Drive = ಸುಳ್ಳು;  // True if in synchronous drive
    ತಾರ್ಕಿಕ m_ಒಳ_Concat_ನಿಯೋಜಿಸು = ಸುಳ್ಳು;  // True if in assignment to a concatenation
    std::ಸದಿಶ<SynchDrive> m_drives;  // Clockvars written by the synchronous drive
    ತಾರ್ಕಿಕ m_has_ಚಕ್ರ_Delay = ಸುಳ್ಳು;  // True if node has cycle delay beneath
    std::ಸದಿಶ<AstVarXRef*> m_xrefsp;  // list of xrefs that need name fixup
    std::ಸದಿಶ<AstSequence*> m_seqs_ಗಮ್ಯ_Cleanup;  // Sequences to clean up after traversal

    // METHODS

    // Flag set by each drive of an output clockvar, added to the module with its clocking item
    AstVar* getCreateDrivenVarp(Ast_Clocking_ಅಂಶ* itemp) {
        ಆದರೆ (!itemp->user2p()) {
            AstVar* ಸ್ಥಿರಾಂಕ varp = ಹೊಸ AstVar{itemp->fileline(), VVarType::MODULETEMP,
                                            m_driven_ಹೆಸರುಗಳು.get(itemp), itemp->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            itemp->user2p(varp);
        }
        ಹಿಂತಿರುಗಿ VN_AS(itemp->user2p(), Var);
    }
    // Reference to a variable in the scope of a driven clockvar, made like the clockvar's
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newDriveRefp(File_ಸಾಲು* flp, ಸ್ಥಿರಾಂಕ SynchDrive& drive, AstVar* varp,
                                     ಸ್ಥಿರಾಂಕ VAccess& access) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarXRef* ಸ್ಥಿರಾಂಕ xrefp = VN_CAST(drive.refp, VarXRef)) {
            ಹಿಂತಿರುಗಿ ಹೊಸ AstVarXRef{flp, varp, xrefp->dotted(), access};
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstMemberSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(drive.refp, MemberSel)) {
            // The interface expression is evaluated again, so it must not have side effects;
            // cloneTreePure warns if it has any
            AstMemberSel* ಸ್ಥಿರಾಂಕ newSelp
                = ಹೊಸ AstMemberSel{flp, selp->fromp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), varp};
            newSelp->access(access);
            ಹಿಂತಿರುಗಿ newSelp;
        }
        ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{flp, varp, access};
    }
    // Assignment setting the drive flag of a driven clockvar
    Ast_ನಿಯೋಜಿಸು* newDrivenSetp(File_ಸಾಲು* flp, ಸ್ಥಿರಾಂಕ SynchDrive& drive) {
        AstVar* ಸ್ಥಿರಾಂಕ varp = getCreateDrivenVarp(drive.itemp);
        Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ setp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, newDriveRefp(flp, drive, varp, VAccess::WRITE),
                                              ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_True{}}};
        setp->user1(ನಿಜ);
        ಹಿಂತಿರುಗಿ setp;
    }
    // Clocking block of a clocking item
    ಸ್ಥಿರ AstClocking* clocking_ದ(ಸ್ಥಿರಾಂಕ Ast_Clocking_ಅಂಶ* itemp) {
        UASSERT_OBJ(itemp->user3p(), itemp, "Clocking item not captured in a clocking block");
        ಹಿಂತಿರುಗಿ VN_AS(itemp->user3p(), Clocking);
    }
    // Clocking event of a driven clockvar, referenced from the drive like the clockvar, or
    // nullptr if unsupported, which is reported on the drive's cycle delay
    Ast_Sen_ವೃಕ್ಷ* newDriveSentreep(AstDelay* delayp, ಸ್ಥಿರಾಂಕ SynchDrive& drive) {
        Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ origp = clocking_ದ(drive.itemp)->sentreep();
        ಆದರೆ (VN_IS(drive.refp, VarRef)) ಹಿಂತಿರುಗಿ origp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
        ಆದರೆ (origp->exists([](ಸ್ಥಿರಾಂಕ AstVarXRef*) { ಹಿಂತಿರುಗಿ ನಿಜ; })) {
            // The reference is relative to the clocking block
            delayp->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in synchronous drive to"
                                          " clockvar in another scope, whose clocking event"
                                          " has a hierarchical reference");
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }
        ಸ್ಥಿರಾಂಕ AstMemberSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(drive.refp, MemberSel);
        // Processes outside classes evaluate their events also when not waiting on them,
        // possibly before the interface reference is set, so the event is guarded against a
        // null reference, which needs an integral value; scheduling does not guard it
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ guardNull = selp && !VN_IS(m_modp, Class);
        ಆದರೆ (guardNull) {
            ಗಾಗಿ (Ast_Sen_ಅಂಶ* itemp = origp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Sen_ಅಂಶ)) {
                ಆದರೆ (!itemp->sensp()->dtypep()->skipRefp()->isIntegralOrPacked()) {
                    delayp->v3warn(E_UNSUPPORTED,
                                   "Unsupported: cycle delay in synchronous drive through"
                                   " an interface reference outside a class, to clocking"
                                   " block with non-integral clocking event");
                    ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
                }
            }
        }
        // Interface declaring the clocking block, if driven through an interface reference
        AstIface* ಸ್ಥಿರಾಂಕ ifacep
            = selp ? VN_AS(selp->fromp()->dtypep()->skipRefp(), IfaceRefDType)->iface_ಮೂಲಕ_Cellp()
                   : ಶೂನ್ಯಸೂಚಕ;
        Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ sentreep = origp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
        std::ಸದಿಶ<AstVarRef*> refps;
        sentreep->foreach([&](AstVarRef* refp) {
            ಆದರೆ (!refp->classOrPackagep()) refps.ತಳ್ಳು_ಹಿಂಭಾಗ(refp);
        });
        ಗಾಗಿ (AstVarRef* ಸ್ಥಿರಾಂಕ refp : refps) {
            AstVar* ಸ್ಥಿರಾಂಕ varp = refp->varp();
            ಆದರೆ (ifacep) {
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
        ಆದರೆ (guardNull) {
            ಗಾಗಿ (Ast_Sen_ಅಂಶ* itemp = sentreep->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Sen_ಅಂಶ)) {
                guardNullIface(itemp, selp->fromp());
            }
        }
        ಹಿಂತಿರುಗಿ sentreep;
    }
    // Make an integral clocking event through an interface reference false while the reference
    // is null
    ಸ್ಥಿರ ಶೂನ್ಯ guardNullIface(Ast_Sen_ಅಂಶ* sensesp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = sensesp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ sensp = sensesp->sensp()->unlink_Fr_ಹಿಂಭಾಗ();
        sensesp->sensp(ಹೊಸ AstCond{flp, newNotNullp(flp, fromp), sensp,
                                   ಹೊಸ AstConst{flp, AstConst::DTyped{}, sensp->dtypep()}});
        ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp = sensesp->condp()) {
            condp->unlink_Fr_ಹಿಂಭಾಗ();
            sensesp->condp(ಹೊಸ AstLogAnd{flp, newNotNullp(flp, fromp), condp});
        }
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newNotNullp(File_ಸಾಲು* flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp) {
        ಹಿಂತಿರುಗಿ ಹೊಸ AstNeq{flp, fromp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), ಹೊಸ AstConst{flp, AstConst::Null{}}};
    }
    // Procedural '##0': if the default clocking event has not yet occurred in this time step,
    // wait for it; otherwise continue without suspension (IEEE 1800-2023 14.11)
    AstIf* new_ಚಕ್ರ_Delay_Zerop(File_ಸಾಲು* flp) ಸ್ಥಿರಾಂಕ {
        AstVar* ಸ್ಥಿರಾಂಕ evtVarp = m_defaultClkEvtVarp;
        UASSERT_OBJ(evtVarp, flp, "Default clocking event var not pre-created");
        AstCMethodHard* ಸ್ಥಿರಾಂಕ isTriggeredp = ಹೊಸ AstCMethodHard{
            flp, ಹೊಸ AstVarRef{flp, evtVarp, VAccess::READ}, VCMethod::EVENT_IS_TRIGGERED};
        isTriggeredp->dtype_ಗಣ_ಬಿಟ್();
        AstEventControl* ಸ್ಥಿರಾಂಕ waitp = ಹೊಸ AstEventControl{
            flp,
            ಹೊಸ Ast_Sen_ವೃಕ್ಷ{flp, ಹೊಸ Ast_Sen_ಅಂಶ{flp, VEdgeType::ET_EVENT,
                                               ಹೊಸ AstVarRef{flp, evtVarp, VAccess::READ}}},
            ಶೂನ್ಯಸೂಚಕ};
        ಹಿಂತಿರುಗಿ ಹೊಸ AstIf{flp, ಹೊಸ AstNot{flp, isTriggeredp}, waitp};
    }

    ಸ್ಥಿರ ಶೂನ್ಯ checkSamplingFuncDType(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* exprp) {
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = exprp->dtypep()->skipRefp();
        ಆದರೆ (!dtypep->isIntegralOrPacked()) {
            nodep->v3error("Expected numeric type, but got a " << dtypep->prettyDTypeNameQ()
                                                               << " data type");
        }
    }

    Ast_Sen_ವೃಕ್ಷ* new_Sen_ವೃಕ್ಷ(Ast_ಗಂಟು* nodep, Ast_Sen_ವೃಕ್ಷ* useTreep = ಶೂನ್ಯಸೂಚಕ,
                           Ast_ಗಂಟು_Cover_Or_Assert* cassertp = ಶೂನ್ಯಸೂಚಕ) {
        // Create sentree based on clocked or default clock
        // Return nullptr for always
        ಆದರೆ (useTreep) ಹಿಂತಿರುಗಿ useTreep;
        Ast_Sen_ವೃಕ್ಷ* newp = ಶೂನ್ಯಸೂಚಕ;
        Ast_Sen_ಅಂಶ* senip = m_senip;
        ತಾರ್ಕಿಕ fromAlways = ಸುಳ್ಳು;
        ಆದರೆ (!senip && m_defaultClockingp) senip = m_defaultClockingp->sentreep()->sensesp();
        ಆದರೆ (!senip) {
            senip = m_seniAlwaysp;
            fromAlways = ನಿಜ;
        }
        ಆದರೆ (!senip) {
            nodep->v3error("Concurrent assertion has no clock (IEEE 1800-2023 16.16)\n"
                           << nodep->warn_ಇನ್ನಷ್ಟು()
                           << "... Suggest provide a clocking event, a default"
                              " clocking, or a clocked procedural context");
            newp = ಹೊಸ Ast_Sen_ವೃಕ್ಷ{nodep->fileline(), ಶೂನ್ಯಸೂಚಕ};
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (cassertp && fromAlways) cassertp->senFromAlways(ನಿಜ);
            newp = ಹೊಸ Ast_Sen_ವೃಕ್ಷ{nodep->fileline(), senip->clone_ವೃಕ್ಷ(ನಿಜ)};
        }
        ಹಿಂತಿರುಗಿ newp;
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* getSequenceBodyExprp(ಸ್ಥಿರಾಂಕ AstSequence* ಸ್ಥಿರಾಂಕ seqp) ಸ್ಥಿರಾಂಕ {
        // The statements in AstSequence are optional AstVar (ports) followed by body expr.
        Ast_ಗಂಟು* bodyp = seqp->stmtsp();
        ಆಗ (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        ಹಿಂತಿರುಗಿ VN_CAST(bodyp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
    }
    AstPropSpec* getPropertyExprp(ಸ್ಥಿರಾಂಕ AstProperty* ಸ್ಥಿರಾಂಕ propp) {
        // Statements in AstProperty: AstVar (ports/local vars),
        // AstInitialStaticStmt/AstInitialAutomaticStmt (var init), AstPropSpec (body).
        Ast_ಗಂಟು* propExprp = propp->stmtsp();
        ಆಗ (propExprp
               && (VN_IS(propExprp, Var) || VN_IS(propExprp, InitialStaticStmt)
                   || VN_IS(propExprp, InitialAutomaticStmt))) {
            propExprp = propExprp->nextp();
        }
        ಹಿಂತಿರುಗಿ VN_CAST(propExprp, PropSpec);
    }
    ಶೂನ್ಯ substituteSequenceCall(AstFuncRef* funcrefp, AstSequence* seqp) {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Inline the sequence body at the call site, replacing the FuncRef
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        // Clone the body expression since the sequence may be referenced multiple times
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* clonedp = bodyExprp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
        // Build substitution map, then do a single traversal to replace all formals
        // (textual substitution per IEEE 16.8.2).
        ಸ್ಥಿರಾಂಕ V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<ಸ್ಥಿರಾಂಕ AstVar*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> ಪೋರ್ಟ್_Map;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& tconnect : tconnects) {
            ಪೋರ್ಟ್_Map[tconnect.ಮೊದಲ] = tconnect.ಎರಡನೇ->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = ಪೋರ್ಟ್_Map.ಕಂಡುಹಿಡಿ(refp->varp());
            ಆದರೆ (it != ಪೋರ್ಟ್_Map.ಕೊನೆ()) {
                refp->replaceWith(it->ಎರಡನೇ->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp), refp);
            }
        });
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& tconnect : tconnects) {
            ತಳ್ಳು_Deletep(tconnect.ಎರಡನೇ->exprp()->unlink_Fr_ಹಿಂಭಾಗ());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        // Replace the FuncRef with the inlined body
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(funcrefp), funcrefp);
        // Clear referenced flag; sequences with isReferenced==false are deleted in assertPreAll
        seqp->isReferenced(ಸುಳ್ಳು);
    }
    AstPropSpec* clonePropertyBody(AstFuncRef* funcrefp, ಸ್ಥಿರಾಂಕ AstProperty* propp) {
        AstPropSpec* propExprp = getPropertyExprp(propp);
        // Substitute inner property call before copying in order to not doing the same for
        // each call of outer property call.
        propExprp = substitutePropertyCall(propExprp);
        // Clone subtree after substitution. It is needed, because property might be called
        // multiple times with different arguments.
        propExprp = propExprp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
        // Build substitution maps for formal arguments and property-local
        // variables, then perform a single foreach to apply all replacements.
        // Map port vars to their actual argument expressions
        ಸ್ಥಿರಾಂಕ V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propp->stmtsp());
        std::unordered_map<ಸ್ಥಿರಾಂಕ AstVar*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> ಪೋರ್ಟ್_Map;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& tconnect : tconnects) {
            ಪೋರ್ಟ್_Map[tconnect.ಮೊದಲ] = tconnect.ಎರಡನೇ->exprp();
        }

        // Promote property-local variables (non-port vars, IEEE 16.10) to
        // module-level __Vpropvar temps. Cross-cycle persistence is handled
        // by the match item lowering in visit(AstImplication*).
        std::unordered_map<ಸ್ಥಿರಾಂಕ AstVar*, AstVar*> localVarMap;
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = propp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(stmtp, Var)) {
                ಆದರೆ (!varp->isIO()) {
                    ಸ್ಥಿರಾಂಕ string newName = m_prop_Var_ಹೆಸರುಗಳು.get(varp);
                    AstVar* ಸ್ಥಿರಾಂಕ newVarp = ಹೊಸ AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->ಕೂಡಿಸು_Stmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        // Single traversal: substitute ports and update local var references
        propExprp->foreach([&](AstVarRef* refp) {
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಪೋರ್ಟ್_It = ಪೋರ್ಟ್_Map.ಕಂಡುಹಿಡಿ(refp->varp());
            ಆದರೆ (ಪೋರ್ಟ್_It != ಪೋರ್ಟ್_Map.ಕೊನೆ()) {
                refp->replaceWith(ಪೋರ್ಟ್_It->ಎರಡನೇ->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp), refp);
                ಹಿಂತಿರುಗಿ;
            }
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ localIt = localVarMap.ಕಂಡುಹಿಡಿ(refp->varp());
            ಆದರೆ (localIt != localVarMap.ಕೊನೆ()) refp->varp(localIt->ಎರಡನೇ);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        // Clean up argument expressions
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& tconnect : tconnects) {
            ತಳ್ಳು_Deletep(tconnect.ಎರಡನೇ->exprp()->unlink_Fr_ಹಿಂಭಾಗ());
        }
        V3AssertCommon::checkSvaInstanceDelays(propExprp, funcrefp);
        ಹಿಂತಿರುಗಿ propExprp;
    }
    AstPropSpec* substitutePropertyCall(AstPropSpec* nodep) {
        ಆದರೆ (AstFuncRef* ಸ್ಥಿರಾಂಕ funcrefp = VN_CAST(nodep->propp(), FuncRef)) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstProperty* ಸ್ಥಿರಾಂಕ propp = VN_CAST(funcrefp->taskp(), Property)) {
                AstPropSpec* ಸ್ಥಿರಾಂಕ propExprp = clonePropertyBody(funcrefp, propp);

                // Handle case with 2 disable iff statement (IEEE 1800-2023 16.12.1)
                ಆದರೆ (nodep->disablep() && propExprp->disablep()) {
                    nodep->v3error("disable iff expression before property call and in its "
                                   "body is not legal");
                    ತಳ್ಳು_Deletep(propExprp->disablep()->unlink_Fr_ಹಿಂಭಾಗ());
                }
                // If disable iff is in outer property, move it to inner
                ಆದರೆ (nodep->disablep()) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ disablep = nodep->disablep()->unlink_Fr_ಹಿಂಭಾಗ();
                    propExprp->disablep(disablep);
                }

                ಆದರೆ (nodep->sensesp() && propExprp->sensesp()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Clock event before property call and in its body");
                    ತಳ್ಳು_Deletep(propExprp->sensesp()->unlink_Fr_ಹಿಂಭಾಗ());
                }
                // If clock event is in outer property, move it to inner
                ಆದರೆ (nodep->sensesp()) {
                    Ast_Sen_ಅಂಶ* ಸ್ಥಿರಾಂಕ sensesp = nodep->sensesp();
                    sensesp->unlink_Fr_ಹಿಂಭಾಗ();
                    propExprp->sensesp(sensesp);
                }

                // Now substitute property reference with property body
                nodep->replaceWith(propExprp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಹಿಂತಿರುಗಿ propExprp;
            }
        }
        ಹಿಂತಿರುಗಿ nodep;
    }

    ಶೂನ್ಯ substituteNestedPropertyCall(AstFuncRef* funcrefp, ಸ್ಥಿರಾಂಕ AstProperty* propp) {
        AstPropSpec* ಸ್ಥಿರಾಂಕ specp = clonePropertyBody(funcrefp, propp);
        Ast_ಗಂಟು* replp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (specp->disablep()) {
            funcrefp->v3error("Illegal 'disable iff' in a nested property instance"
                              " (IEEE 1800-2023 16.12.1)");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (specp->sensesp()) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: clock event in a nested property instance");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (specp->propp()->exists(
                       [](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* np) { ಹಿಂತಿರುಗಿ np->is_Multi_ಚಕ್ರ_Sva(); })) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: multi-cycle body in a nested property instance");
        } ಇಲ್ಲದಿದ್ದರೆ {
            replp = specp->propp()->unlink_Fr_ಹಿಂಭಾಗ();
        }
        ಆದರೆ (!replp) replp = ಹೊಸ AstConst{funcrefp->fileline(), AstConst::ಬಿಟ್_False{}};
        funcrefp->replaceWith(replp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(funcrefp), funcrefp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(specp), specp);
    }

    // VISITORS
    //========== Statements
    ಶೂನ್ಯ ಭೇಟಿ(AstClocking* ಸ್ಥಿರಾಂಕ nodep) override {
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        UINFO(8, "   CLOCKING" << nodep);
        ಆದರೆ (nodep->isDefault()) {
            ಆದರೆ (m_defaultClockingp) {
                nodep->v3error("Only one default clocking block allowed per module"
                               " (IEEE 1800-2023 14.12)");
            } ಇಲ್ಲದಿದ್ದರೆ {
                m_defaultClockingp = nodep;
                m_defaultClkEvtVarp = nodep->ensureEventp();
            }
        }
        iterateChildren(nodep);
        ಆದರೆ (nodep->eventp()) nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(nodep->eventp()->unlink_Fr_ಹಿಂಭಾಗ());
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstModportClockingRef* ಸ್ಥಿರಾಂಕ nodep) override {
        // It has to be converted to a list of ModportClockingVarRefs,
        // because clocking blocks are removed in this pass
        ಗಾಗಿ (Ast_ಗಂಟು* itemp = nodep->clockingp()->itemsp(); itemp; itemp = itemp->nextp()) {
            ಆದರೆ (Ast_Clocking_ಅಂಶ* ಸ್ಥಿರಾಂಕ citemp = VN_CAST(itemp, Clocking_ಅಂಶ)) {
                ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp
                    = citemp->varp() ? citemp->varp() : VN_AS(citemp->user1p(), Var)) {
                    AstModportVarRef* ಸ್ಥಿರಾಂಕ modVarp = ಹೊಸ AstModportVarRef{
                        nodep->fileline(), varp->name(), citemp->direction()};
                    modVarp->varp(varp);
                    nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(modVarp);
                    ಆದರೆ (citemp->direction().isOutput()) {
                        // Drives through the modport set the drive flag
                        AstVar* ಸ್ಥಿರಾಂಕ drivenVarp = getCreateDrivenVarp(citemp);
                        AstModportVarRef* ಸ್ಥಿರಾಂಕ drivenRefp = ಹೊಸ AstModportVarRef{
                            nodep->fileline(), drivenVarp->name(), VDirection::OUTPUT};
                        drivenRefp->varp(drivenVarp);
                        nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(drivenRefp);
                    }
                }
            }
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Clocking_ಅಂಶ* ಸ್ಥಿರಾಂಕ nodep) override {
        // Get a ref to the sampled/driven variable
        AstVar* ಸ್ಥಿರಾಂಕ varp = nodep->varp();
        ಆದರೆ (!varp) {
            // Unused item
            ಹಿಂತಿರುಗಿ;
        }
        // Flag set by drives of an output clockvar, possibly visited before this item
        ಆದರೆ (nodep->direction().isOutput()) m_modp->ಕೂಡಿಸು_Stmtsp(getCreateDrivenVarp(nodep));
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        V3Const::constifyEdit(nodep->skewp());
        ಆದರೆ (!VN_IS(nodep->skewp(), Const)) {
            nodep->skewp()->v3error("Skew must be constant (IEEE 1800-2023 14.4)");
            ಹಿಂತಿರುಗಿ;
        }
        AstConst* ಸ್ಥಿರಾಂಕ skewp = VN_AS(nodep->skewp(), Const);
        ಆದರೆ (skewp->ಅಂಕೆ().isNegative()) skewp->v3error("Skew cannot be negative");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = nodep->exprp();
        varp->name(m_clockingp->name() + "__DOT__" + varp->name());
        m_clockingp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(varp->unlink_Fr_ಹಿಂಭಾಗ());
        nodep->user1p(varp);
        varp->user1p(nodep);
        ಆದರೆ (nodep->direction() == VDirection::OUTPUT) {
            exprp->foreach([](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* varrefp) {
                // Prevent confusing BLKANDNBLK warnings on clockvars due to generated assignments
                varrefp->fileline()->warnOff(V3_Error_ಸಂಕೇತ::BLKANDNBLK, ನಿಜ);
                // Mark the driven signal so later passes (e.g. V3Inst port
                // connection) know the drive flows out from inside the cell.
                // Item expressions are plain signal references, so only the
                // driven signal is marked, never index selects.
                varrefp->varp()->isClockingDriven(ನಿಜ);
            });
            AstVarRef* ಸ್ಥಿರಾಂಕ skewedReadRefp = ಹೊಸ AstVarRef{flp, varp, VAccess::READ};
            skewedReadRefp->user1(ನಿಜ);
            // Initialize the clockvar
            AstVarRef* ಸ್ಥಿರಾಂಕ skewedWriteRefp = ಹೊಸ AstVarRef{flp, varp, VAccess::WRITE};
            skewedWriteRefp->user1(ನಿಜ);
            AstInitialStatic* ಸ್ಥಿರಾಂಕ initClockvarp = ಹೊಸ AstInitialStatic{
                flp, ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, skewedWriteRefp, exprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)}};
            m_modp->ಕೂಡಿಸು_Stmtsp(initClockvarp);
            // Each drive sets a flag, so the signal is also assigned when driven with the same
            // value again, e.g. after another assignment to the signal (IEEE 1800-2023 14.16.2)
            AstVar* ಸ್ಥಿರಾಂಕ drivenVarp = getCreateDrivenVarp(nodep);
            m_modp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstInitialStatic{
                flp, ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                   ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_False{}}}});
            // Assign the clockvar to the actual var if it was driven
            Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp
                = ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, exprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), skewedReadRefp};
            AstIf* ಸ್ಥಿರಾಂಕ ifp
                = ಹೊಸ AstIf{flp, ಹೊಸ AstVarRef{flp, drivenVarp, VAccess::READ},
                            ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                          ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_False{}}}};
            ifp->ಕೂಡಿಸು_Thensp(assignp);
            // Only in the time step of the clocking event
            AstCMethodHard* ಸ್ಥಿರಾಂಕ trigp = ಹೊಸ AstCMethodHard{
                nodep->fileline(), ಹೊಸ AstVarRef{flp, m_clockingp->ensureEventp(), VAccess::READ},
                VCMethod::EVENT_IS_TRIGGERED};
            trigp->dtype_ಗಣ_ಬಿಟ್();
            ifp->condp(ಹೊಸ AstLogAnd{flp, ifp->condp()->unlink_Fr_ಹಿಂಭಾಗ(), trigp});
            // A skew rounding to no time precision ticks is a zero skew
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ticksp
                = V3Timing::delay_ಗಮ್ಯ_Ticks(skewp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), m_modp->timeunit());
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ zeroSkew = ticksp->isZero();
            VL_DO_DANGLING(ticksp->delete_ವೃಕ್ಷ(), ticksp);
            ಆದರೆ (zeroSkew || !skewp->fileline()->timingOn()) {
                // Drive the var in Re-NBA (IEEE 1800-2023 14.16), ignoring skews where timing is
                // turned off
                Ast_Sen_ವೃಕ್ಷ* senTreep = m_clockingp->sentreep()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                senTreep->ಕೂಡಿಸು_Sensesp(ಹೊಸ Ast_Sen_ಅಂಶ{
                    flp, VEdgeType::ET_ಬದಲಾಯಿತು, ಹೊಸ AstVarRef{flp, drivenVarp, VAccess::READ}});
                m_clockingp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(ಹೊಸ AstAlwaysReactive{flp, senTreep, ifp});
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Fork only the assignment, so this AlwaysPostponed can be retriggered
                // before the assignment happens
                AstFork* ಸ್ಥಿರಾಂಕ forkp = ಹೊಸ AstFork{flp, VJoinType::JOIN_NONE};
                forkp->ತಕ್ಷಣ_ಆರಂಭ(ನಿಜ);
                forkp->ಕೂಡಿಸು_Forksp(ಹೊಸ AstBegin{flp, "", assignp->unlink_Fr_ಹಿಂಭಾಗ(), ನಿಜ});
                ifp->ಕೂಡಿಸು_Thensp(forkp);
                // Read the clockvar in Postponed, so a drive late in the time step is applied
                // at this event plus the skew (IEEE 1800-2023 14.16)
                m_clockingp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(ಹೊಸ AstAlwaysPostponed{flp, ifp});
                ಆದರೆ (v3Global.opt.timing().is_ಗಣ_True()) {
                    AstDelay* ಸ್ಥಿರಾಂಕ delayp = ಹೊಸ AstDelay{flp, skewp->unlink_Fr_ಹಿಂಭಾಗ(), ಸುಳ್ಳು};
                    delayp->timeunit(m_modp->timeunit());
                    assignp->timingControlp(delayp);
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (v3Global.opt.timing().is_ಗಣ_False()) {
                    nodep->v3warn(E_NOTIMING,
                                  "Clocking output skew greater than #0 requires --timing");
                } ಇಲ್ಲದಿದ್ದರೆ {
                    nodep->v3warn(E_NEEDTIMINGOPT,
                                  "Use --timing or --no-timing to specify how "
                                  "clocking output skew greater than #0 should be handled");
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->direction() == VDirection::ಒಳಹರಿವು) {
            // Ref to the clockvar
            AstVarRef* ಸ್ಥಿರಾಂಕ refp = ಹೊಸ AstVarRef{flp, varp, VAccess::WRITE};
            refp->user1(ನಿಜ);
            ಆದರೆ (skewp->ಅಂಕೆ().is1_ಹೆಜ್ಜೆ()) {
                // #1step means the value that is sampled is always the signal's last value
                // before the clock edge (IEEE 1800-2023 14.4)
                AstSampled* ಸ್ಥಿರಾಂಕ sampledp
                    = ಹೊಸ AstSampled{flp, exprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), exprp->dtypep(), ನಿಜ};
                Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, refp, sampledp};
                m_clockingp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(ಹೊಸ AstAlways{
                    flp, VAlwaysKwd::ALWAYS, m_clockingp->sentreep()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), assignp});
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (skewp->isZero()) {
                // #0 means the var has to be sampled in Observed (IEEE 1800-2023 14.13)
                Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, refp, exprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
                m_clockingp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(ಹೊಸ AstAlwaysObserved{
                    flp, m_clockingp->sentreep()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), assignp});
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Create a queue where we'll store sampled values with timestamps
                Ast_Sample_ಸರತಿ_DType* ಸ್ಥಿರಾಂಕ ಸರತಿ_Dtp
                    = ಹೊಸ Ast_Sample_ಸರತಿ_DType{flp, exprp->dtypep()};
                m_netlistp->typeTablep()->ಕೂಡಿಸು_Typesp(ಸರತಿ_Dtp);
                AstVar* ಸ್ಥಿರಾಂಕ ಸರತಿ_Varp
                    = ಹೊಸ AstVar{flp, VVarType::MODULETEMP, "__Vqueue__" + varp->name(), ಸರತಿ_Dtp};
                ಸರತಿ_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_clockingp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(ಸರತಿ_Varp);
                // Create a process like this:
                //     always queue.push(<sampled var>);
                AstCMethodHard* ಸ್ಥಿರಾಂಕ pushp = ಹೊಸ AstCMethodHard{
                    flp, ಹೊಸ AstVarRef{flp, ಸರತಿ_Varp, VAccess::WRITE}, VCMethod::DYN_ತಳ್ಳು,
                    ಹೊಸ Ast_ಸಮಯ{nodep->fileline(), m_modp->timeunit()}};
                pushp->ಕೂಡಿಸು_Pinsp(exprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು));
                pushp->dtype_ಗಣ_Void();
                m_clockingp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(
                    ಹೊಸ AstAlways{flp, VAlwaysKwd::ALWAYS, ಶೂನ್ಯಸೂಚಕ, pushp->makeStmt()});
                // Create a process like this:
                //     always @<clocking event> queue.pop(<skew>, /*out*/<skewed var>);
                AstCMethodHard* ಸ್ಥಿರಾಂಕ popp = ಹೊಸ AstCMethodHard{
                    flp, ಹೊಸ AstVarRef{flp, ಸರತಿ_Varp, VAccess::READWRITE}, VCMethod::DYN_ತೆಗೆ,
                    ಹೊಸ Ast_ಸಮಯ{nodep->fileline(), m_modp->timeunit()}};
                popp->ಕೂಡಿಸು_Pinsp(skewp->unlink_Fr_ಹಿಂಭಾಗ());
                refp->access(VAccess::READWRITE);  // Only conditionally assigned
                popp->ಕೂಡಿಸು_Pinsp(refp);
                popp->dtype_ಗಣ_Void();
                m_clockingp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(ಹೊಸ AstAlways{flp, VAlwaysKwd::ALWAYS,
                                                       m_clockingp->sentreep()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು),
                                                       popp->makeStmt()});
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3fatalSrc("Invalid direction");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDelay* nodep) override {
        m_has_ಚಕ್ರ_Delay |= nodep->is_ಚಕ್ರ_Delay();
        // Only cycle delays are relevant in this stage; also only process once
        ಆದರೆ (!nodep->is_ಚಕ್ರ_Delay()) {
            ಆದರೆ (m_ಒಳ_Synch_Drive) {
                nodep->v3error("Only cycle delays can be used in synchronous drives"
                               " (IEEE 1800-2023 14.16)");
            }
            iterateChildren(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (m_ಒಳ_ನಿಯೋಜಿಸು && !m_ಒಳ_Synch_Drive) {
            nodep->v3error("Cycle delays not allowed as intra-assignment delays"
                           " (IEEE 1800-2023 14.11)");
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (nodep->stmtsp()) nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(nodep->stmtsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = V3Const::constifyEdit(nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ());
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(valuep, Const);
        ಆದರೆ (!constp && m_pexprp) {
            // V3AssertNfa handles non-const delays in sequences before this pass and
            // replaces the property; this branch should never be reached.
            nodep->v3fatalSrc("Non-constant cycle delay in assertion: "
                              "should have been caught by V3AssertNfa");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (constp && constp->isZero()) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(valuep), valuep);
            ಆದರೆ (m_ಒಳ_Synch_Drive) {
                // ##0 has no effect in synchronous drives (IEEE 1800-2023 14.11)
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
                ಹಿಂತಿರುಗಿ;
            }
            ಆದರೆ (m_pexprp) {
                // ##0 in sequence context = zero delay = same clock tick
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
                ಹಿಂತಿರುಗಿ;
            }
            // Procedural ##0: synchronize with default clocking event (IEEE 1800-2023 14.11)
            ಆದರೆ (!m_defaultClockingp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
                ಹಿಂತಿರುಗಿ;
            }
            nodep->replaceWith(new_ಚಕ್ರ_Delay_Zerop(flp));
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        Ast_Sen_ವೃಕ್ಷ* sentreep = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (!m_drives.ಖಾಲಿಗಳು()) {
            ಆದರೆ (m_ಒಳ_Concat_ನಿಯೋಜಿಸು) {
                // Also the clockvars may be of different clocking blocks
                nodep->v3error("Cycle delays cannot be used in synchronous drives to a"
                               " concatenation (IEEE 1800-2023 14.16)");
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(valuep), valuep);
                ಹಿಂತಿರುಗಿ;
            }
            // Count the cycles of the driven clockvar's clocking block (IEEE 1800-2023 14.16)
            sentreep = newDriveSentreep(nodep, m_drives.front());
            ಆದರೆ (!sentreep) {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(valuep), valuep);
                ಹಿಂತಿರುಗಿ;
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!m_defaultClockingp) {
            ಆದರೆ (!m_pexprp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(valuep), valuep);
                ಹಿಂತಿರುಗಿ;
            }
            sentreep = ಹೊಸ Ast_Sen_ವೃಕ್ಷ{flp, m_senip->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)};
        } ಇಲ್ಲದಿದ್ದರೆ {
            sentreep = m_defaultClockingp->sentreep()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
        }
        AstEventControl* ಸ್ಥಿರಾಂಕ controlp
            = ಹೊಸ AstEventControl{nodep->fileline(), sentreep, ಶೂನ್ಯಸೂಚಕ};
        ಸ್ಥಿರಾಂಕ std::string delayName = m_ಚಕ್ರ_Dly_ಹೆಸರುಗಳು.get(nodep);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* throughoutp
            = nodep->throughoutp() ? nodep->throughoutp()->unlink_Fr_ಹಿಂಭಾಗ() : ಶೂನ್ಯಸೂಚಕ;

        AstVar* ಸ್ಥಿರಾಂಕ cntVarp = ಹೊಸ AstVar{flp, VVarType::BLOCKTEMP, delayName + "__counter",
                                           nodep->ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstBegin* ಸ್ಥಿರಾಂಕ beginp = ಹೊಸ AstBegin{flp, delayName + "__block", cntVarp, ನಿಜ};
        // A non-constant delay is counted at runtime. The count is integral of 64 bits, as
        // V3Width makes delays, or real, which is rounded (IEEE 1800-2023 6.12.1)
        ಆದರೆ (valuep->isDouble()) {
            valuep = ಹೊಸ AstRToIRoundS{flp, valuep};
            valuep->dtypeFrom(cntVarp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            valuep = ಹೊಸ AstSel{flp, valuep, 0, cntVarp->ಅಗಲ()};
        }
        beginp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, cntVarp, VAccess::WRITE}, valuep});
        // If the count is zero, a synchronous drive waits for none, like with a constant '##0',
        // and a procedural delay waits like a procedural '##0' (IEEE 1800-2023 14.11)
        ಆದರೆ (!constp && !m_ಒಳ_Synch_Drive) {
            beginp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstIf{
                flp,
                ಹೊಸ AstEq{flp, ಹೊಸ AstVarRef{flp, cntVarp, VAccess::READ}, ಹೊಸ AstConst{flp, 0}},
                new_ಚಕ್ರ_Delay_Zerop(flp)});
        }

        // Throughout: create flag tracking whether condition held every tick
        AstVar* throughoutOkp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (throughoutp) {
            throughoutOkp = ಹೊಸ AstVar{flp, VVarType::BLOCKTEMP, delayName + "__throughoutOk",
                                       nodep->ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::LOGIC_IMPLICIT)};
            throughoutOkp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->ಕೂಡಿಸು_Stmtsp(throughoutOkp);
            beginp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_True{}}});
            // Check condition at tick 0 (sequence start, before entering loop)
            beginp->ಕೂಡಿಸು_Stmtsp(
                ಹೊಸ AstIf{flp, ಹೊಸ AstLogNot{flp, throughoutp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)},
                          ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                        ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_False{}}}});
        }

        {
            AstLoop* ಸ್ಥಿರಾಂಕ loopp = ಹೊಸ AstLoop{flp};
            // When throughout is present, exit loop early if condition fails
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* loopCondp
                = ಹೊಸ AstGt{flp, ಹೊಸ AstVarRef{flp, cntVarp, VAccess::READ}, ಹೊಸ AstConst{flp, 0}};
            ಆದರೆ (throughoutOkp) {
                loopCondp = ಹೊಸ AstLogAnd{flp, loopCondp,
                                          ಹೊಸ AstVarRef{flp, throughoutOkp, VAccess::READ}};
            }
            loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstLoopTest{flp, loopp, loopCondp});
            loopp->ಕೂಡಿಸು_Stmtsp(controlp);
            loopp->ಕೂಡಿಸು_Stmtsp(
                ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, cntVarp, VAccess::WRITE},
                              ಹೊಸ AstSub{flp, ಹೊಸ AstVarRef{flp, cntVarp, VAccess::READ},
                                         ಹೊಸ AstConst{flp, 1}}});
            // Check throughout condition at each tick during delay (IEEE 1800-2023 16.9.9)
            ಆದರೆ (throughoutp) {
                loopp->ಕೂಡಿಸು_Stmtsp(
                    ಹೊಸ AstIf{flp, ಹೊಸ AstLogNot{flp, throughoutp},
                              ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_False{}}}});
            }
            beginp->ಕೂಡಿಸು_Stmtsp(loopp);
        }
        // Compose wrappers on remaining sequence: throughout gate (inner), disable iff (outer)
        Ast_ಗಂಟು* remainp = nodep->nextp() ? nodep->nextp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ() : ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (throughoutOkp) {
            // If condition failed during delay, fail assertion
            remainp = ಹೊಸ AstIf{flp, ಹೊಸ AstVarRef{flp, throughoutOkp, VAccess::READ}, remainp,
                                ಹೊಸ AstPExprClause{flp, /*pass=*/ಸುಳ್ಳು}};
        }
        ಆದರೆ (m_disableSeqIfp && remainp) {
            AstIf* ಸ್ಥಿರಾಂಕ disableSeqIfp = m_disableSeqIfp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
            disableSeqIfp->ಕೂಡಿಸು_Thensp(remainp);
            remainp = disableSeqIfp;
        }
        ಆದರೆ (remainp) {
            ಆದರೆ (throughoutOkp) {
                // throughoutOkp is declared in beginp scope -- check must be inside it
                beginp->ಕೂಡಿಸು_Stmtsp(remainp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(remainp);
            }
        }
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Sen_ವೃಕ್ಷ* nodep) override {
        ಆದರೆ (m_ಒಳ_Synch_Drive) {
            nodep->v3error("Event controls cannot be used in "
                           "synchronous drives (IEEE 1800-2023 14.16)");
        }

        ಸ್ಥಿರಾಂಕ AstSampled* sampledp;
        ಆದರೆ (nodep->exists([&sampledp](ಸ್ಥಿರಾಂಕ AstSampled* ಸ್ಥಿರಾಂಕ sp) {
                sampledp = sp;
                ಹಿಂತಿರುಗಿ ನಿಜ;
            })) {
            sampledp->v3warn(E_UNSUPPORTED, "Unsupported: $sampled inside sensitivity list");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Var_Ref* nodep) override {
        UINFO(8, " -varref:  " << nodep);
        UINFO(8, " -varref-var-back:  " << nodep->varp()->backp());
        UINFO(8, " -varref-var-user1:  " << nodep->varp()->user1p());
        ಆದರೆ (Ast_Clocking_ಅಂಶ* ಸ್ಥಿರಾಂಕ itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->ಮೊದಲ_Abovep(),
                Clocking_ಅಂಶ)) {
            ಆದರೆ (nodep->user1()) ಹಿಂತಿರುಗಿ;

            // ensure linking still works, this has to be done only once
            ಆದರೆ (AstVarXRef* xrefp = VN_CAST(nodep, VarXRef)) {
                UINFO(8, " -clockvarxref-in:  " << xrefp);
                string dotted = xrefp->dotted();
                ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t dotPos = dotted.rfind('.');
                dotted.erase(dotPos, string::npos);
                xrefp->dotted(dotted);
                UINFO(8, " -clockvarxref-out: " << xrefp);
                m_xrefsp.emplace_ಹಿಂಭಾಗ(xrefp);
            }

            ಆದರೆ (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            ಆದರೆ (nodep->access().isWriteOrRW()) {
                ಆದರೆ (itemp->direction() == VDirection::OUTPUT) {
                    ಆದರೆ (!m_ಒಳ_ನಿಯೋಜಿಸು_Dly_Lhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    ಆದರೆ (m_ಒಳ_ನಿಯೋಜಿಸು) {
                        m_ಒಳ_Synch_Drive = ನಿಜ;
                        m_drives.ತಳ್ಳು_ಹಿಂಭಾಗ({itemp, nodep});
                    }
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (itemp->direction() == VDirection::ಒಳಹರಿವು) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
            nodep->user1(ನಿಜ);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstMemberSel* nodep) override {
        ಆದರೆ (Ast_Clocking_ಅಂಶ* ಸ್ಥಿರಾಂಕ itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->ಮೊದಲ_Abovep(),
                Clocking_ಅಂಶ)) {
            ಆದರೆ (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            ಆದರೆ (nodep->access().isWriteOrRW()) {
                ಆದರೆ (itemp->direction() == VDirection::OUTPUT) {
                    ಆದರೆ (!m_ಒಳ_ನಿಯೋಜಿಸು_Dly_Lhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    ಆದರೆ (m_ಒಳ_ನಿಯೋಜಿಸು) {
                        m_ಒಳ_Synch_Drive = ನಿಜ;
                        m_drives.ತಳ್ಳು_ಹಿಂಭಾಗ({itemp, nodep});
                    }
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (itemp->direction() == VDirection::ಒಳಹರಿವು) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) override {
        ಆದರೆ (nodep->user1()) ಹಿಂತಿರುಗಿ;
        VL_RESTORER(m_ಒಳ_ನಿಯೋಜಿಸು);
        VL_RESTORER(m_ಒಳ_Synch_Drive);
        VL_RESTORER(m_ಒಳ_Concat_ನಿಯೋಜಿಸು);
        VL_RESTORER_CLEAR(m_drives);
        m_ಒಳ_ನಿಯೋಜಿಸು = ನಿಜ;
        m_ಒಳ_Synch_Drive = ಸುಳ್ಳು;
        {
            // The braces of a concatenation are a replication, by one
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp = nodep->lhsp();
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstReplicate* ಸ್ಥಿರಾಂಕ repp = VN_CAST(lhsp, Replicate)) lhsp = repp->srcp();
            m_ಒಳ_Concat_ನಿಯೋಜಿಸು = VN_IS(lhsp, Concat);
        }
        {
            VL_RESTORER(m_ಒಳ_ನಿಯೋಜಿಸು_Dly_Lhs);
            m_ಒಳ_ನಿಯೋಜಿಸು_Dly_Lhs = VN_IS(nodep, ನಿಯೋಜಿಸು_Dly);
            iterate(nodep->lhsp());
        }
        iterate(nodep->rhsp());
        ಆದರೆ (nodep->timingControlp()) iterate(nodep->timingControlp());
        // A drive without a cycle delay, also after removing '##0', updates the clockvar now
        ಆದರೆ (m_ಒಳ_Synch_Drive && !nodep->timingControlp()) {
            Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                nodep->fileline(), nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ(), nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ()};
            assignp->user1(ನಿಜ);
            nodep->replaceWith(assignp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            nodep = assignp;
        }
        // Note the drives when they take effect, after the cycle delay if any. Each clockvar
        // of the LHS is driven, e.g. in a concatenation which other simulators accept, although
        // IEEE 1800-2023 14.16 does not allow it
        Ast_ಗಂಟು* setsp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ SynchDrive& drive : m_drives) {
            setsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(setsp, newDrivenSetp(nodep->fileline(), drive));
        }
        ಆದರೆ (!setsp) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (AstBegin* ಸ್ಥಿರಾಂಕ beginp = VN_CAST(nodep->timingControlp(), Begin)) {
            beginp->ಕೂಡಿಸು_Stmtsp(setsp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(setsp);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAlways* nodep) override {
        iterate_And_ಮುಂದಿನ_Null(nodep->sentreep());
        ಆದರೆ (nodep->sentreep()) m_seniAlwaysp = nodep->sentreep()->sensesp();
        iterate_And_ಮುಂದಿನ_Null(nodep->stmtsp());
        m_seniAlwaysp = ಶೂನ್ಯಸೂಚಕ;
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Cover_Or_Assert* nodep) override {
        ಆದರೆ (nodep->sentreep()) ಹಿಂತಿರುಗಿ;  // Already processed

        VL_RESTORER(m_senip);
        VL_RESTORER(m_disablep);
        m_senip = ಶೂನ್ಯಸೂಚಕ;
        m_disablep = ಶೂನ್ಯಸೂಚಕ;

        // Find Clocking's buried under nodep->exprsp
        iterateChildren(nodep);
        ಆದರೆ (!nodep->ತಕ್ಷಣ()) nodep->sentreep(new_Sen_ವೃಕ್ಷ(nodep, ಶೂನ್ಯಸೂಚಕ, nodep));
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFalling* nodep) override {
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = nodep->exprp()->unlink_Fr_ಹಿಂಭಾಗ();
        ಆದರೆ (exprp->ಅಗಲ() > 1) exprp = ಹೊಸ AstSel{fl, exprp, 0, 1};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ futurep = ಹೊಸ AstFuture{fl, exprp, new_Sen_ವೃಕ್ಷ(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = ಹೊಸ AstAnd{fl, exprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), ಹೊಸ AstNot{fl, futurep}};
        exprp->dtype_ಗಣ_ಬಿಟ್();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFell* nodep) override {
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = nodep->exprp()->unlink_Fr_ಹಿಂಭಾಗ();
        ಆದರೆ (exprp->ಅಗಲ() > 1) exprp = ಹೊಸ AstSel{fl, exprp, 0, 1};
        Ast_Sen_ವೃಕ್ಷ* sentreep = nodep->sentreep();
        ಆದರೆ (sentreep) sentreep->unlink_Fr_ಹಿಂಭಾಗ();
        AstPast* ಸ್ಥಿರಾಂಕ pastp = ಹೊಸ AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_ವೃಕ್ಷ(nodep, sentreep));
        exprp = ಹೊಸ AstAnd{fl, pastp, ಹೊಸ AstNot{fl, exprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)}};
        exprp->dtype_ಗಣ_ಬಿಟ್();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFuture* nodep) override {
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ sentreep = nodep->sentreep();
        ಆದರೆ (sentreep) VL_DO_DANGLING(ತಳ್ಳು_Deletep(sentreep->unlink_Fr_ಹಿಂಭಾಗ()), sentreep);
        nodep->sentreep(new_Sen_ವೃಕ್ಷ(nodep));
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPast* nodep) override {
        checkSamplingFuncDType(nodep, nodep->exprp());
        ಆದರೆ (nodep->sentreep()) ಹಿಂತಿರುಗಿ;  // Already processed
        iterateChildren(nodep);
        nodep->sentreep(new_Sen_ವೃಕ್ಷ(nodep));
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- Lower standalone exact [*N] (N >= 2) via saturating counter.
        // Range/unbounded forms and SExpr-contained forms are lowered by V3AssertNfa.
        iterateChildren(nodep);
        // V3AssertNfa handles unbounded/ranged forms upstream, so this fast-path
        // is effectively unreachable when NFA is enabled.
        ಆದರೆ (nodep->unbounded() || nodep->maxCountp()) ಹಿಂತಿರುಗಿ;  // LCOV_EXCL_LINE
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep->countp(), Const);
        ಆದರೆ (VL_UNLIKELY(!constp || constp->ಗಮ್ಯ_SInt() < 1)) {
            nodep->v3fatalSrc("Consecutive repetition count must be a positive constant"
                              " (should have been caught by V3Width)");
            ಹಿಂತಿರುಗಿ;
        }
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ n = constp->ಗಮ್ಯ_SInt();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = nodep->exprp()->unlink_Fr_ಹಿಂಭಾಗ();
        ಆದರೆ (n == 1) {
            nodep->replaceWith(exprp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        // Saturating counter: if (expr) cnt <= min(cnt+1, N); else cnt <= 0;
        AstVar* ಸ್ಥಿರಾಂಕ cntVarp = ಹೊಸ AstVar{flp, VVarType::MODULETEMP, m_cons_Rep_ಹೆಸರುಗಳು.get(""),
                                           nodep->ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->ಕೂಡಿಸು_Stmtsp(cntVarp);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಅಭಿವ್ಯಕ್ತಿ_Clonep = exprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ saturatingIncrp = ಹೊಸ AstCond{
            flp,
            ಹೊಸ AstLt{flp, ಹೊಸ AstVarRef{flp, cntVarp, VAccess::READ},
                      ಹೊಸ AstConst{flp, static_cast<uint32_t>(n)}},
            ಹೊಸ Ast_ಕೂಡಿಸು{flp, ಹೊಸ AstVarRef{flp, cntVarp, VAccess::READ}, ಹೊಸ AstConst{flp, 1u}},
            ಹೊಸ AstConst{flp, static_cast<uint32_t>(n)}};
        Ast_ನಿಯೋಜಿಸು_Dly* ಸ್ಥಿರಾಂಕ incrAssignp
            = ಹೊಸ Ast_ನಿಯೋಜಿಸು_Dly{flp, ಹೊಸ AstVarRef{flp, cntVarp, VAccess::WRITE}, saturatingIncrp};
        Ast_ನಿಯೋಜಿಸು_Dly* ಸ್ಥಿರಾಂಕ resetAssignp = ಹೊಸ Ast_ನಿಯೋಜಿಸು_Dly{
            flp, ಹೊಸ AstVarRef{flp, cntVarp, VAccess::WRITE}, ಹೊಸ AstConst{flp, 0u}};
        AstIf* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{flp, ಅಭಿವ್ಯಕ್ತಿ_Clonep, incrAssignp, resetAssignp};
        Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ senTreep = new_Sen_ವೃಕ್ಷ(nodep);
        AstAlways* ಸ್ಥಿರಾಂಕ alwaysp = ಹೊಸ AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, ifp};
        cntVarp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(alwaysp);
        // Match: cnt >= N-1 (previous cycles via NBA) && expr (current cycle)
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ cntCheckp = ಹೊಸ AstGte{flp, ಹೊಸ AstVarRef{flp, cntVarp, VAccess::READ},
                                                  ಹೊಸ AstConst{flp, static_cast<uint32_t>(n - 1)}};
        cntCheckp->dtype_ಗಣ_ಬಿಟ್();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ matchp = ಹೊಸ AstAnd{flp, cntCheckp, exprp};
        matchp->dtype_ಗಣ_ಬಿಟ್();
        nodep->replaceWith(matchp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRising* nodep) override {
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = nodep->exprp()->unlink_Fr_ಹಿಂಭಾಗ();
        ಆದರೆ (exprp->ಅಗಲ() > 1) exprp = ಹೊಸ AstSel{fl, exprp, 0, 1};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ futurep = ಹೊಸ AstFuture{fl, exprp, new_Sen_ವೃಕ್ಷ(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = ಹೊಸ AstAnd{fl, ಹೊಸ AstNot{fl, exprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)}, futurep};
        exprp->dtype_ಗಣ_ಬಿಟ್();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRose* nodep) override {
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = nodep->exprp()->unlink_Fr_ಹಿಂಭಾಗ();
        ಆದರೆ (exprp->ಅಗಲ() > 1) exprp = ಹೊಸ AstSel{fl, exprp, 0, 1};
        Ast_Sen_ವೃಕ್ಷ* sentreep = nodep->sentreep();
        ಆದರೆ (sentreep) sentreep->unlink_Fr_ಹಿಂಭಾಗ();
        AstPast* ಸ್ಥಿರಾಂಕ pastp = ಹೊಸ AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_ವೃಕ್ಷ(nodep, sentreep));
        exprp = ಹೊಸ AstAnd{fl, ಹೊಸ AstNot{fl, pastp}, exprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
        exprp->dtype_ಗಣ_ಬಿಟ್();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಸ್ಥಿರ AstAssocArrayDType* getProcessAssocArrayType(File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp) {
        // Type of __VassertsActive___x[std::process]
        Ast_ಗಂಟು_DType* valp
            = v3Global.rootp()->typeTablep()->ಕಂಡುಹಿಡಿ_Basic_DType(flp, VBasicDTypeKwd::ಬಿಟ್);
        AstClassRefDType* keyp
            = ಹೊಸ AstClassRefDType{flp, v3Global.rootp()->stdPackageProcessp(), ಶೂನ್ಯಸೂಚಕ};
        keyp->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        v3Global.rootp()->typeTablep()->ಕೂಡಿಸು_Typesp(keyp);
        AstAssocArrayDType* ಸ್ಥಿರಾಂಕ typep = ಹೊಸ AstAssocArrayDType{flp, valp, keyp};
        typep->dtypep(typep);
        v3Global.rootp()->typeTablep()->ಕೂಡಿಸು_Typesp(typep);
        ಹಿಂತಿರುಗಿ typep;
    }
    ಸ್ಥಿರ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* getProcessAssocArrayDelete(AstVarRef* ಸ್ಥಿರಾಂಕ refp) {
        // Constructs refp.delete(std::process::self()) statement
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = refp->fileline();
        AstCMethodHard* ಸ್ಥಿರಾಂಕ deletep = ಹೊಸ AstCMethodHard{
            flp, refp, VCMethod::ASSOC_ERASE, v3Global.rootp()->stdPackageProcessSelfp(flp)};
        deletep->dtypep(refp->ಕಂಡುಹಿಡಿ_Void_DType());
        ಹಿಂತಿರುಗಿ ಹೊಸ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ{flp, deletep};
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* get_Process_Assoc_Array_ಗಾತ್ರ(AstVarRef* ಸ್ಥಿರಾಂಕ refp) {
        // Constructs refp.size() statement
        AstCMethodHard* ಸ್ಥಿರಾಂಕ sizep
            = ಹೊಸ AstCMethodHard{refp->fileline(), refp, VCMethod::ASSOC_ಗಾತ್ರ};
        sizep->dtypep(refp->ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::UINT32));
        ಹಿಂತಿರುಗಿ sizep;
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSEventually* nodep) override {
        UASSERT(v3Global.rootp()->stdPackagep(), "Should be imported");

        Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ sentreep = new_Sen_ವೃಕ್ಷ(nodep);
        ಆದರೆ (!sentreep->sensesp()) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(sentreep), sentreep);
            nodep->replaceWith(ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False{}});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ;
        }

        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();

        // Track active assertions
        AstVar* ಸ್ಥಿರಾಂಕ activep = ಹೊಸ AstVar{flp, VVarType::MODULETEMP, m_ಸಕ್ರಿಯ_ಹೆಸರುಗಳು.get(""),
                                           getProcessAssocArrayType(flp)};
        activep->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->ಕೂಡಿಸು_Stmtsp(activep);

        // Assertion condition check
        AstLoop* ಸ್ಥಿರಾಂಕ loopp = ಹೊಸ AstLoop{flp};
        AstSampled* ಸ್ಥಿರಾಂಕ condp
            = ಹೊಸ AstSampled{flp, nodep->exprp()->unlink_Fr_ಹಿಂಭಾಗ(), ಶೂನ್ಯಸೂಚಕ, ನಿಜ};
        loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstLoopTest{flp, loopp, ಹೊಸ AstLogNot{flp, condp}});
        loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstEventControl{flp, sentreep, ಶೂನ್ಯಸೂಚಕ});

        // Add assertion to the active set
        AstAssocSel* ಸ್ಥಿರಾಂಕ selp = ಹೊಸ AstAssocSel{flp, ಹೊಸ AstVarRef{flp, activep, VAccess::WRITE},
                                                  v3Global.rootp()->stdPackageProcessSelfp(flp)};
        Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ incrementp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, selp, ಹೊಸ AstConst{flp, 1}};
        AstPExprClause* ಸ್ಥಿರಾಂಕ clausep = ಹೊಸ AstPExprClause{flp};
        Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ deletep
            = getProcessAssocArrayDelete(ಹೊಸ AstVarRef{flp, activep, VAccess::WRITE});

        // Main assertion block
        AstBegin* ಸ್ಥಿರಾಂಕ bodyp = ಹೊಸ AstBegin{flp, m_ಖಂಡ_ಹೆಸರುಗಳು.get(""), ಶೂನ್ಯಸೂಚಕ, ನಿಜ};
        bodyp->ಕೂಡಿಸು_Stmtsp(incrementp);
        bodyp->ಕೂಡಿಸು_Stmtsp(loopp);
        bodyp->ಕೂಡಿಸು_Stmtsp(clausep);
        bodyp->ಕೂಡಿಸು_Stmtsp(deletep);

        // Validate assertion condition for each active assert
        AstVar* ಸ್ಥಿರಾಂಕ ಸಕ್ರಿಯ_Countp = ಹೊಸ AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                nodep->ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::UINT32)};
        ಸಕ್ರಿಯ_Countp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ init_ಸಕ್ರಿಯ_Countp
            = ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, ಸಕ್ರಿಯ_Countp, VAccess::WRITE},
                            get_Process_Assoc_Array_ಗಾತ್ರ(ಹೊಸ AstVarRef{flp, activep, VAccess::READ})};
        AstLoop* ಸ್ಥಿರಾಂಕ finalLoopp = ಹೊಸ AstLoop{flp};
        AstIf* ಸ್ಥಿರಾಂಕ finalBodypCondp
            = ಹೊಸ AstIf{flp, condp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), ಹೊಸ AstPExprClause{flp},
                        ಹೊಸ AstPExprClause{flp, ಸುಳ್ಳು}};
        finalLoopp->ಕೂಡಿಸು_Stmtsp(
            ಹೊಸ AstLoopTest{flp, finalLoopp,
                            ಹೊಸ AstNeq{flp, ಹೊಸ AstVarRef{flp, ಸಕ್ರಿಯ_Countp, VAccess::READ},
                                       ಹೊಸ AstConst{flp, 0}}});
        finalLoopp->ಕೂಡಿಸು_Stmtsp(finalBodypCondp);
        finalLoopp->ಕೂಡಿಸು_Stmtsp(
            ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, ಸಕ್ರಿಯ_Countp, VAccess::WRITE},
                          ಹೊಸ AstSub{flp, ಹೊಸ AstVarRef{flp, ಸಕ್ರಿಯ_Countp, VAccess::READ},
                                     ಹೊಸ AstConst{flp, 1}}});

        // Final assertion block
        AstBegin* ಸ್ಥಿರಾಂಕ finalp = ಹೊಸ AstBegin{flp, m_ಖಂಡ_ಹೆಸರುಗಳು.get(""), ಶೂನ್ಯಸೂಚಕ, ನಿಜ};
        finalp->ಕೂಡಿಸು_Stmtsp(ಸಕ್ರಿಯ_Countp);
        finalp->ಕೂಡಿಸು_Stmtsp(init_ಸಕ್ರಿಯ_Countp);
        finalp->ಕೂಡಿಸು_Stmtsp(finalLoopp);

        VL_RESTORER(m_pexprp);
        m_pexprp = ಹೊಸ AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
        VL_RESTORER(m_has_ಚಕ್ರ_Delay);
        m_has_ಚಕ್ರ_Delay = ಸುಳ್ಳು;
        iterate(bodyp);
        iterate(finalp);

        ಆದರೆ (m_has_ಚಕ್ರ_Delay) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in s_eventually");
            nodep->replaceWith(ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False{}});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            VL_DO_DANGLING(m_pexprp->delete_ವೃಕ್ಷ(), m_pexprp);
            ಹಿಂತಿರುಗಿ;
        }

        nodep->replaceWith(m_pexprp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstStable* nodep) override {
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = nodep->exprp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_Sen_ವೃಕ್ಷ* sentreep = nodep->sentreep();
        ಆದರೆ (sentreep) sentreep->unlink_Fr_ಹಿಂಭಾಗ();
        AstPast* ಸ್ಥಿರಾಂಕ pastp = ಹೊಸ AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_ವೃಕ್ಷ(nodep, sentreep));
        exprp = ಹೊಸ AstEq{fl, pastp, exprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
        exprp->dtype_ಗಣ_ಬಿಟ್();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSteady* nodep) override {
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = nodep->exprp()->unlink_Fr_ಹಿಂಭಾಗ();
        ಆದರೆ (exprp->ಅಗಲ() > 1) exprp = ಹೊಸ AstSel{fl, exprp, 0, 1};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ futurep = ಹೊಸ AstFuture{fl, exprp, new_Sen_ವೃಕ್ಷ(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = ಹೊಸ AstEq{fl, exprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), futurep};
        exprp->dtype_ಗಣ_ಬಿಟ್();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSampled* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (!nodep->ಆಂತರಿಕ_2()) checkSamplingFuncDType(nodep, nodep->exprp());
    }

    // Validate repetition count: must be a non-negative elaboration-time constant.
    // Shared by goto [->N] and nonconsecutive [=N] repetition.
    // On error, replaces nodep with BitFalse placeholder and returns nullptr.
    ಸ್ಥಿರಾಂಕ AstConst* validate_Rep_ಎಣಿಕೆ(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*& countp) {
        countp = V3Const::constifyEdit(countp);
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(countp, Const);
        ಆದರೆ (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(countp), countp);
            nodep->replaceWith(ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False_Erroring{}});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }
        ಆದರೆ (constp->ಗಮ್ಯ_SInt() < 0) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(countp), countp);
            nodep->replaceWith(ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False_Erroring{}});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }
        ಆದರೆ (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(countp), countp);
            nodep->replaceWith(ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False{}});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }
        ಹಿಂತಿರುಗಿ constp;
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
        Ast_Sen_ಅಂಶ* ಸ್ಥಿರಾಂಕ sensesp = m_senip;
        UASSERT_OBJ(sensesp, exprp, "Repetition requires a clock");

        ಸ್ಥಿರಾಂಕ std::string name
            = isNonConsec ? m_non_Cons_Rep_ಹೆಸರುಗಳು.get(exprp) : m_goto_Rep_ಹೆಸರುಗಳು.get(exprp);
        AstVar* ಸ್ಥಿರಾಂಕ cntVarp = ಹೊಸ AstVar{flp, VVarType::BLOCKTEMP, name + "__counter",
                                           exprp->ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstBegin* ಸ್ಥಿರಾಂಕ beginp = ಹೊಸ AstBegin{flp, name + "__block", cntVarp, ನಿಜ};
        beginp->ಕೂಡಿಸು_Stmtsp(
            ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, cntVarp, VAccess::WRITE}, ಹೊಸ AstConst{flp, 0}});

        // Counting loop: find N occurrences of expr (shared by goto and nonconsec)
        AstLoop* ಸ್ಥಿರಾಂಕ loopp = ಹೊಸ AstLoop{flp};
        loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstLoopTest{flp, loopp,
                                         ಹೊಸ AstLt{flp, ಹೊಸ AstVarRef{flp, cntVarp, VAccess::READ},
                                                   countp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)}});
        // if (expr) cnt++ -- sampled is applied to whole property expr by V3Assert
        loopp->ಕೂಡಿಸು_Stmtsp(
            ಹೊಸ AstIf{flp, exprp,
                      ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, cntVarp, VAccess::WRITE},
                                    ಹೊಸ Ast_ಕೂಡಿಸು{flp, ಹೊಸ AstVarRef{flp, cntVarp, VAccess::READ},
                                               ಹೊಸ AstConst{flp, 1}}}});
        loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstIf{
            flp, ಹೊಸ AstLt{flp, ಹೊಸ AstVarRef{flp, cntVarp, VAccess::READ}, countp},
            ಹೊಸ AstEventControl{flp, ಹೊಸ Ast_Sen_ವೃಕ್ಷ{flp, sensesp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)}, ಶೂನ್ಯಸೂಚಕ}});

        beginp->ಕೂಡಿಸು_Stmtsp(loopp);

        ಆದರೆ (isNonConsec && rhsp) {
            // IEEE 16.9.2: b[=N] = b[->N] ##1 !b[*0:$]
            // Trailing window: ##1 advance, then scan !expr cycles checking rhs.
            // Window closes when expr becomes true again (fail if rhs was never true).
            beginp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstEventControl{
                flp, ಹೊಸ Ast_Sen_ವೃಕ್ಷ{flp, sensesp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)}, ಶೂನ್ಯಸೂಚಕ});  // ##1
            ಆದರೆ (!isOverlapped) {
                beginp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstEventControl{
                    flp, ಹೊಸ Ast_Sen_ವೃಕ್ಷ{flp, sensesp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)}, ಶೂನ್ಯಸೂಚಕ});  // |=>
            }
            // Window loop: check rhs at each !expr cycle (done variable for termination)
            AstVar* ಸ್ಥಿರಾಂಕ ಮುಗಿದ_Varp
                = ಹೊಸ AstVar{flp, VVarType::BLOCKTEMP, name + "__done", exprp->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType()};
            ಮುಗಿದ_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->ಕೂಡಿಸು_Stmtsp(ಮುಗಿದ_Varp);
            beginp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, ಮುಗಿದ_Varp, VAccess::WRITE},
                                            ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_False{}}});
            ಸ್ವಯಂಚಾಲಿತ ಗಣ_ಮುಗಿದ = [&]() {
                ಹಿಂತಿರುಗಿ ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, ಮುಗಿದ_Varp, VAccess::WRITE},
                                     ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_True{}}};
            };
            AstLoop* ಸ್ಥಿರಾಂಕ windowp = ಹೊಸ AstLoop{flp};
            // LoopTest: continue while !done
            windowp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstLoopTest{
                flp, windowp, ಹೊಸ AstNot{flp, ಹೊಸ AstVarRef{flp, ಮುಗಿದ_Varp, VAccess::READ}}});
            // if (expr) { fail; done = 1; } -- window closed, expr true again
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ failsp = ಹೊಸ AstPExprClause{flp, ಸುಳ್ಳು};
            failsp->ಕೂಡಿಸು_ಮುಂದಿನ(ಗಣ_ಮುಗಿದ());
            windowp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstIf{flp, exprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), failsp});
            // if (rhs) { pass; done = 1; } -- consequent true at this !expr endpoint
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ passsp = ಹೊಸ AstPExprClause{flp, ನಿಜ};
            passsp->ಕೂಡಿಸು_ಮುಂದಿನ(ಗಣ_ಮುಗಿದ());
            windowp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstIf{flp, rhsp, passsp});
            // @(clk) -- advance to next cycle in window
            windowp->ಕೂಡಿಸು_Stmtsp(
                ಹೊಸ AstEventControl{flp, ಹೊಸ Ast_Sen_ವೃಕ್ಷ{flp, sensesp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)}, ಶೂನ್ಯಸೂಚಕ});
            beginp->ಕೂಡಿಸು_Stmtsp(windowp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (isNonConsec) {
            // Standalone nonconsec: ##1 into window, then pass
            beginp->ಕೂಡಿಸು_Stmtsp(
                ಹೊಸ AstEventControl{flp, ಹೊಸ Ast_Sen_ವೃಕ್ಷ{flp, sensesp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)}, ಶೂನ್ಯಸೂಚಕ});
            beginp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstPExprClause{flp, ನಿಜ});
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (rhsp) {
            // Goto rep: check consequent once at match endpoint
            ಆದರೆ (!isOverlapped) {
                beginp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstEventControl{
                    flp, ಹೊಸ Ast_Sen_ವೃಕ್ಷ{flp, sensesp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)}, ಶೂನ್ಯಸೂಚಕ});
            }
            beginp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstIf{flp, rhsp, ಹೊಸ AstPExprClause{flp, ನಿಜ},
                                        ಹೊಸ AstPExprClause{flp, ಸುಳ್ಳು}});
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Standalone goto: pass after counting
            beginp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstPExprClause{flp, ನಿಜ});
        }

        ಹಿಂತಿರುಗಿ ಹೊಸ AstPExpr{flp, beginp, exprp->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType()};
    }

    // Reject [=M:N] nonconsecutive range that reached V3AssertPre.
    // [->M:N] is fully handled upstream (NFA path); only [=M:N] still lands
    // here because AstSNonConsRep lowering is not yet implemented there.
    // Replaces the node with BitFalse so lowering continues after the warning.
    ತಾರ್ಕಿಕ rejectNonconsecRange(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* maxCountp) {
        ಆದರೆ (!maxCountp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: [=M:N] nonconsecutive range repetition"
                                     " (IEEE 1800-2023 16.9.2)");
        nodep->replaceWith(ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False{}});
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstSGotoRep* nodep) override {
        // Standalone goto rep (not inside implication antecedent)
        iterateChildren(nodep);
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* countp = nodep->countp()->unlink_Fr_ಹಿಂಭಾಗ();
        ಆದರೆ (!validate_Rep_ಎಣಿಕೆ(nodep, countp)) ಹಿಂತಿರುಗಿ;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = nodep->exprp()->unlink_Fr_ಹಿಂಭಾಗ();
        AstPExpr* ಸ್ಥಿರಾಂಕ pexprp = createRepPExpr(flp, exprp, countp, ಶೂನ್ಯಸೂಚಕ, ನಿಜ, ಸುಳ್ಳು);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstSNonConsRep* nodep) override {
        // Standalone nonconsecutive rep (not inside implication antecedent)
        iterateChildren(nodep);
        ಆದರೆ (rejectNonconsecRange(nodep, nodep->maxCountp())) ಹಿಂತಿರುಗಿ;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* countp = nodep->countp()->unlink_Fr_ಹಿಂಭಾಗ();
        ಆದರೆ (!validate_Rep_ಎಣಿಕೆ(nodep, countp)) ಹಿಂತಿರುಗಿ;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = nodep->exprp()->unlink_Fr_ಹಿಂಭಾಗ();
        AstPExpr* ಸ್ಥಿರಾಂಕ pexprp = createRepPExpr(flp, exprp, countp, ಶೂನ್ಯಸೂಚಕ, ನಿಜ, ನಿಜ);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstFuncRef* nodep) override {
        // IEEE 1800-2023 16.8: Inline sequence instances wherever they appear
        // in the expression tree (inside implications, boolean ops, nested refs, etc.)
        ಆದರೆ (AstSequence* ಸ್ಥಿರಾಂಕ seqp = VN_CAST(nodep->taskp(), Sequence)) {
            substituteSequenceCall(nodep, seqp);
            // The FuncRef has been replaced; do not access nodep after this point.
            // The replacement node will be visited by the parent's iterateChildren.
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstProperty* ಸ್ಥಿರಾಂಕ propp = VN_CAST(nodep->taskp(), Property)) {
            substituteNestedPropertyCall(nodep, propp);
            ಹಿಂತಿರುಗಿ;
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstImplication* nodep) override {
        ಆದರೆ (nodep->sentreep()) ಹಿಂತಿರುಗಿ;  // Already processed

        // Top-level followed-by is claimed by V3AssertNfa; anything reaching here
        // is nested inside iff/implies/or. IEEE 1800-2023 16.12.9 permits the
        // composition, but silent lowering would drop non-vacuous-fail semantics.
        ಆದರೆ (nodep->isFollowedBy()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: followed-by (#-# / #=#) nested inside property operator"
                          " (iff/implies/or) (IEEE 1800-2023 16.12.9)");
            nodep->replaceWith(ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False{}});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ;
        }

        // Handle goto repetition as antecedent before iterateChildren,
        // so the standalone AstSGotoRep visitor doesn't process it
        ಆದರೆ (AstSGotoRep* ಸ್ಥಿರಾಂಕ gotop = VN_CAST(nodep->lhsp(), SGotoRep)) {
            iterateChildren(gotop);
            iterate_And_ಮುಂದಿನ_Null(nodep->rhsp());
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* countp = gotop->countp()->unlink_Fr_ಹಿಂಭಾಗ();
            ಆದರೆ (!validate_Rep_ಎಣಿಕೆ(nodep, countp)) ಹಿಂತಿರುಗಿ;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = gotop->exprp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            AstPExpr* ಸ್ಥಿರಾಂಕ pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), ಸುಳ್ಳು);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ;
        }

        // Handle nonconsecutive repetition as antecedent before iterateChildren,
        // so the standalone AstSNonConsRep visitor doesn't process it
        ಆದರೆ (AstSNonConsRep* ಸ್ಥಿರಾಂಕ ncrp = VN_CAST(nodep->lhsp(), SNonConsRep)) {
            iterateChildren(ncrp);
            iterate_And_ಮುಂದಿನ_Null(nodep->rhsp());
            ಆದರೆ (rejectNonconsecRange(nodep, ncrp->maxCountp())) ಹಿಂತಿರುಗಿ;
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* countp = ncrp->countp()->unlink_Fr_ಹಿಂಭಾಗ();
            ಆದರೆ (!validate_Rep_ಎಣಿಕೆ(nodep, countp)) ಹಿಂತಿರುಗಿ;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = ncrp->exprp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            AstPExpr* ಸ್ಥಿರಾಂಕ pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), ನಿಜ);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ;
        }

        iterateChildren(nodep);

        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();

        // Lower sequence match items (IEEE 16.11): (expr, var = val, ...) |-> / |=>
        ಆದರೆ (Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* ಸ್ಥಿರಾಂಕ ಅಭಿವ್ಯಕ್ತಿ_Stmtp = VN_CAST(lhsp, ಅಭಿವ್ಯಕ್ತಿ_Stmt)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ antExprp = ಅಭಿವ್ಯಕ್ತಿ_Stmtp->resultp()->unlink_Fr_ಹಿಂಭಾಗ();

            ಆದರೆ (nodep->isOverlapped()) {
                // |-> : assign to __Vpropvar via always_comb (continuous).
                // The assign evaluates RHS once; V3Sampled snapshots the
                // result so all consequent refs read the same value.
                Ast_ಗಂಟು* ಹೊಂದಿಕೆ_Assignsp = ಶೂನ್ಯಸೂಚಕ;
                ಗಾಗಿ (Ast_ಗಂಟು* stmtp = ಅಭಿವ್ಯಕ್ತಿ_Stmtp->stmtsp(); stmtp;) {
                    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nextp = stmtp->nextp();
                    ಆದರೆ (Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp = VN_CAST(stmtp, ನಿಯೋಜಿಸು)) {
                        assignp->unlink_Fr_ಹಿಂಭಾಗ();
                        ಹೊಂದಿಕೆ_Assignsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ_Null(ಹೊಂದಿಕೆ_Assignsp, assignp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                ಆದರೆ (ಹೊಂದಿಕೆ_Assignsp) {
                    AstAlways* ಸ್ಥಿರಾಂಕ alwaysp
                        = ಹೊಸ AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, ಶೂನ್ಯಸೂಚಕ, ಹೊಂದಿಕೆ_Assignsp};
                    m_modp->ಕೂಡಿಸು_Stmtsp(alwaysp);
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                // |=> : assign to __Vpropvar via NBA in a clocked always block.
                // The NBA commits before the next cycle's sampled snapshot,
                // so the consequent (which already references __Vpropvar)
                // sees the captured value.
                Ast_ಗಂಟು* ಹೊಂದಿಕೆ_Assignsp = ಶೂನ್ಯಸೂಚಕ;
                ಗಾಗಿ (Ast_ಗಂಟು* stmtp = ಅಭಿವ್ಯಕ್ತಿ_Stmtp->stmtsp(); stmtp;) {
                    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nextp = stmtp->nextp();
                    ಆದರೆ (Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp = VN_CAST(stmtp, ನಿಯೋಜಿಸು)) {
                        assignp->unlink_Fr_ಹಿಂಭಾಗ();
                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ನಿಯೋಜಿಸು_Lhsp = assignp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ನಿಯೋಜಿಸು_Rhsp = assignp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
                        Ast_ನಿಯೋಜಿಸು_Dly* ಸ್ಥಿರಾಂಕ dlyp = ಹೊಸ Ast_ನಿಯೋಜಿಸು_Dly{flp, ನಿಯೋಜಿಸು_Lhsp, ನಿಯೋಜಿಸು_Rhsp};
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(assignp), assignp);
                        ಹೊಂದಿಕೆ_Assignsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ_Null(ಹೊಂದಿಕೆ_Assignsp, dlyp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                ಆದರೆ (ಹೊಂದಿಕೆ_Assignsp) {
                    AstIf* ಸ್ಥಿರಾಂಕ condp
                        = ಹೊಸ AstIf{flp, antExprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), ಹೊಂದಿಕೆ_Assignsp};
                    AstAlways* ಸ್ಥಿರಾಂಕ alwaysp
                        = ಹೊಸ AstAlways{flp, VAlwaysKwd::ALWAYS, new_Sen_ವೃಕ್ಷ(nodep), condp};
                    m_modp->ಕೂಡಿಸು_Stmtsp(alwaysp);
                }
            }
        }

        ಆದರೆ (AstPExpr* ಸ್ಥಿರಾಂಕ pexprp = VN_CAST(rhsp, PExpr)) {
            // Implication with sequence expression on RHS (IEEE 1800-2023 16.11, 16.12.7).
            // The PExpr was lowered from the property expression earlier in this pass.
            // Wrap the PExpr body with the antecedent check so the sequence only
            // starts when the antecedent holds.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp;
            ಆದರೆ (nodep->isOverlapped()) {
                // Overlapped implication (|->): check antecedent on same cycle.
                // disable iff is applied at the assertion level, not at the
                // antecedent gate, matching the existing non-PExpr overlapped path.
                condp = lhsp;
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Non-overlapped implication (|=>): check antecedent from previous cycle
                ಆದರೆ (m_disablep) {
                    lhsp
                        = ಹೊಸ AstAnd{flp, ಹೊಸ AstNot{flp, m_disablep->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)}, lhsp};
                }
                AstPast* ಸ್ಥಿರಾಂಕ pastp
                    = ಹೊಸ AstPast{flp, lhsp, ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ, /* propertyTiming */ ನಿಜ};
                pastp->dtypeFrom(lhsp);
                pastp->sentreep(new_Sen_ವೃಕ್ಷ(nodep));
                condp = pastp;
            }
            // Wrap existing PExpr body: if (antecedent) { <original body> } else { /* vacuous pass
            // */ }
            // Only the statements are guarded; declarations stay directly in the block. After
            // V3Fork moves the block's statements into a task, V3Task only handles variables
            // that are direct statements of the task.
            AstBegin* ಸ್ಥಿರಾಂಕ bodyp = pexprp->bodyp();
            AstIf* ಸ್ಥಿರಾಂಕ guardp = ಹೊಸ AstIf{flp, condp};
            ಗಾಗಿ (Ast_ಗಂಟು* stmtp = bodyp->stmtsp(); stmtp;) {
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nextp = stmtp->nextp();
                ಆದರೆ (!VN_IS(stmtp, Var)) guardp->ಕೂಡಿಸು_Thensp(stmtp->unlink_Fr_ಹಿಂಭಾಗ());
                stmtp = nextp;
            }
            bodyp->ಕೂಡಿಸು_Stmtsp(guardp);
            nodep->replaceWith(pexprp);
            // Don't iterate pexprp here -- it was already iterated when created
            // (in visit(AstSExpr*)), so delays and disable iff are already processed.
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->isOverlapped()) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp
                = m_underCover ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ಹೊಸ AstLogAnd{flp, lhsp, rhsp})
                               : ಹೊಸ AstLogOr{flp, ಹೊಸ AstLogNot{flp, lhsp}, rhsp};
            nodep->replaceWith(exprp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (m_disablep) {
                lhsp = ಹೊಸ AstAnd{flp, ಹೊಸ AstNot{flp, m_disablep->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)}, lhsp};
            }

            AstPast* ಸ್ಥಿರಾಂಕ pastp
                = ಹೊಸ AstPast{flp, lhsp, ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ, /* propertyTiming */ ನಿಜ};
            pastp->dtypeFrom(lhsp);
            pastp->sentreep(new_Sen_ವೃಕ್ಷ(nodep));
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp
                = m_underCover ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ಹೊಸ AstAnd{flp, pastp, rhsp})
                               : ಹೊಸ AstOr{flp, ಹೊಸ AstNot{flp, pastp}, rhsp};
            exprp->dtype_ಗಣ_ಬಿಟ್();
            nodep->replaceWith(exprp);
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstUntil* nodep) override {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        UASSERT_OBJ(
            !m_pexprp, nodep,
            "'" << nodep->verilogKwd()
                << "' in complex property expression should have been rejected by V3AssertNfa");
        ಆದರೆ (nodep->isStrong()
            && (v3Global.opt.timing().is_ಗಣ_False() || !v3Global.opt.timing().is_ಗಣ_True())) {
            nodep->v3warn(E_NOTIMING, nodep->verilogKwd() << " requires --timing");
            nodep->replaceWith(ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_False{}});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (nodep->isStrong()) {
            // p s_until q / p s_until_with q: q must eventually be true. Until then, p must
            // be true on every sampled tick. For s_until, check q first: when q is true on
            // this tick, p is not required. For s_until_with, p must be true on the q tick too.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rawLhsp = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rawRhsp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            AstSampled* ಸ್ಥಿರಾಂಕ lhsp = ಹೊಸ AstSampled{flp, rawLhsp, rawLhsp->dtypep(), ನಿಜ};
            AstSampled* ಸ್ಥಿರಾಂಕ rhsp = ಹೊಸ AstSampled{flp, rawRhsp, rawRhsp->dtypep(), ನಿಜ};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* finalCondp = rhsp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
            ಆದರೆ (nodep->isOverlapping()) {
                finalCondp = ಹೊಸ AstLogAnd{flp, lhsp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), finalCondp};
            }

            // Track active assertion attempts. Only the count is needed to emit final failures.
            AstVar* ಸ್ಥಿರಾಂಕ activep = ಹೊಸ AstVar{flp, VVarType::MODULETEMP, m_ಸಕ್ರಿಯ_ಹೆಸರುಗಳು.get(""),
                                               nodep->ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::UINT32)};
            activep->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->ಕೂಡಿಸು_Stmtsp(activep);

            AstVar* ಸ್ಥಿರಾಂಕ donep
                = ಹೊಸ AstVar{flp, VVarType::BLOCKTEMP, "__VassertDone", nodep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType()};
            donep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            ಸ್ವಯಂಚಾಲಿತ ಗಣ_ಮುಗಿದ = [&]() {
                ಹಿಂತಿರುಗಿ ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, donep, VAccess::WRITE},
                                     ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_True{}}};
            };

            AstLoop* ಸ್ಥಿರಾಂಕ loopp = ಹೊಸ AstLoop{flp};
            Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ decrementVar = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                flp, ಹೊಸ AstVarRef{flp, activep, VAccess::WRITE},
                ಹೊಸ AstSub{flp, ಹೊಸ AstVarRef{flp, activep, VAccess::READ}, ಹೊಸ AstConst{flp, 1}}};
            loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstLoopTest{
                flp, loopp, ಹೊಸ AstLogNot{flp, ಹೊಸ AstVarRef{flp, donep, VAccess::READ}}});
            {
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ passp = ಹೊಸ AstPExprClause{flp};
                passp->ಕೂಡಿಸು_ಮುಂದಿನ(decrementVar);
                passp->ಕೂಡಿಸು_ಮುಂದಿನ(ಗಣ_ಮುಗಿದ());
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* passCondp = rhsp;
                ಆದರೆ (nodep->isOverlapping()) {
                    passCondp = ಹೊಸ AstLogAnd{flp, lhsp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), passCondp};
                }
                loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstIf{flp, passCondp, passp});
            }
            {
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ failp = ಹೊಸ AstPExprClause{flp, ಸುಳ್ಳು};
                failp->ಕೂಡಿಸು_ಮುಂದಿನ(decrementVar->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
                failp->ಕೂಡಿಸು_ಮುಂದಿನ(ಗಣ_ಮುಗಿದ());
                loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstIf{
                    flp,
                    ಹೊಸ AstLogAnd{flp,
                                  ಹೊಸ AstLogNot{flp, ಹೊಸ AstVarRef{flp, donep, VAccess::READ}},
                                  ಹೊಸ AstLogNot{flp, lhsp}},
                    failp});
            }
            AstDelay* ಸ್ಥಿರಾಂಕ delayp = ಹೊಸ AstDelay{flp, ಹೊಸ AstConst{flp, 1}, ಸುಳ್ಳು};
            delayp->timeunit(m_modp->timeunit());
            loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstIf{
                flp, ಹೊಸ AstLogNot{flp, ಹೊಸ AstVarRef{flp, donep, VAccess::READ}}, delayp});

            // Main assertion block
            AstBegin* ಸ್ಥಿರಾಂಕ bodyp = ಹೊಸ AstBegin{flp, m_ಖಂಡ_ಹೆಸರುಗಳು.get(""), ಶೂನ್ಯಸೂಚಕ, ನಿಜ};
            bodyp->ಕೂಡಿಸು_Stmtsp(donep);
            bodyp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, donep, VAccess::WRITE},
                                           ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_False{}}});
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = activep->fileline();
            bodyp->ಕೂಡಿಸು_Stmtsp(
                ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, activep, VAccess::WRITE},
                              ಹೊಸ Ast_ಕೂಡಿಸು{flp, ಹೊಸ AstVarRef{flp, activep, VAccess::READ},
                                         ಹೊಸ AstConst{flp, 1}}});
            bodyp->ಕೂಡಿಸು_Stmtsp(loopp);

            // Validate assertion condition for each active assert
            AstVar* ಸ್ಥಿರಾಂಕ ಸಕ್ರಿಯ_Countp = ಹೊಸ AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                    nodep->ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::UINT32)};
            ಸಕ್ರಿಯ_Countp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

            Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ init_ಸಕ್ರಿಯ_Countp
                = ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, ಸಕ್ರಿಯ_Countp, VAccess::WRITE},
                                ಹೊಸ AstVarRef{flp, activep, VAccess::READ}};
            AstLoop* ಸ್ಥಿರಾಂಕ finalLoopp = ಹೊಸ AstLoop{flp};
            finalLoopp->ಕೂಡಿಸು_Stmtsp(
                ಹೊಸ AstLoopTest{flp, finalLoopp,
                                ಹೊಸ AstNeq{flp, ಹೊಸ AstVarRef{flp, ಸಕ್ರಿಯ_Countp, VAccess::READ},
                                           ಹೊಸ AstConst{flp, 0}}});
            finalLoopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstIf{flp, finalCondp, ಹೊಸ AstPExprClause{flp},
                                            ಹೊಸ AstPExprClause{flp, ಸುಳ್ಳು}});
            finalLoopp->ಕೂಡಿಸು_Stmtsp(
                ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, ಸಕ್ರಿಯ_Countp, VAccess::WRITE},
                              ಹೊಸ AstSub{flp, ಹೊಸ AstVarRef{flp, ಸಕ್ರಿಯ_Countp, VAccess::READ},
                                         ಹೊಸ AstConst{flp, 1}}});

            // Final assertion block
            AstBegin* ಸ್ಥಿರಾಂಕ finalp = ಹೊಸ AstBegin{flp, m_ಖಂಡ_ಹೆಸರುಗಳು.get(""), ಶೂನ್ಯಸೂಚಕ, ನಿಜ};
            finalp->ಕೂಡಿಸು_Stmtsp(ಸಕ್ರಿಯ_Countp);
            finalp->ಕೂಡಿಸು_Stmtsp(init_ಸಕ್ರಿಯ_Countp);
            finalp->ಕೂಡಿಸು_Stmtsp(finalLoopp);

            AstPExpr* ಸ್ಥಿರಾಂಕ pexprp = ಹೊಸ AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
            VL_RESTORER(m_pexprp);
            VL_RESTORER(m_has_ಚಕ್ರ_Delay);
            m_pexprp = pexprp;
            m_has_ಚಕ್ರ_Delay = ಸುಳ್ಳು;
            iterate(bodyp);
            iterate(finalp);

            UASSERT_OBJ(!m_has_ಚಕ್ರ_Delay, nodep,
                        "s_until cycle delay should have been handled by V3AssertNfa");

            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        AstLoop* ಸ್ಥಿರಾಂಕ loopp = ಹೊಸ AstLoop{flp};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        AstLogAnd* ಸ್ಥಿರಾಂಕ loopCondp = ಹೊಸ AstLogAnd{flp, lhsp, ಹೊಸ AstLogNot{flp, rhsp}};
        loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstLoopTest{flp, loopp, loopCondp});
        loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstEventControl{flp, new_Sen_ವೃಕ್ಷ(nodep), ಶೂನ್ಯಸೂಚಕ});

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsCopyp = rhsp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ passCondp
            = nodep->isOverlapping() ? ಹೊಸ AstLogAnd{flp, lhsp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), rhsCopyp}
                                     : rhsCopyp;
        AstBegin* ಸ್ಥಿರಾಂಕ beginp = ಹೊಸ AstBegin{flp, m_ಖಂಡ_ಹೆಸರುಗಳು.get(""), loopp, ನಿಜ};
        beginp->ಕೂಡಿಸು_Stmtsp(
            ಹೊಸ AstIf{flp, passCondp, ಹೊಸ AstPExprClause{flp}, ಹೊಸ AstPExprClause{flp, ಸುಳ್ಳು}});

        AstPExpr* ಸ್ಥಿರಾಂಕ pexprp = ಹೊಸ AstPExpr{flp, beginp, nodep->dtypep()};
        pexprp->user1(1);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstDefaultDisable* nodep) override {
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstInferredDisable* nodep) override {
        Ast_ಗಂಟು* newp;
        ಆದರೆ (m_defaultDisablep) {
            newp = m_defaultDisablep->condp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ);
        } ಇಲ್ಲದಿದ್ದರೆ {
            newp = ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False{}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstPropSpec* nodep) override {
        nodep = substitutePropertyCall(nodep);
        iterate_And_ಮುಂದಿನ_Null(nodep->sensesp());
        ಆದರೆ (m_senip && m_senip != nodep->sensesp())
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Only one PSL clock allowed per assertion");
        ಸ್ಥಿರಾಂಕ AstCover* ಸ್ಥಿರಾಂಕ coverp = VN_CAST(nodep->backp(), Cover);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ seqEvent = coverp && coverp->isSeqEvent();
        ಆದರೆ (!nodep->disablep() && m_defaultDisablep && !seqEvent) {
            nodep->disablep(m_defaultDisablep->condp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ));
        }
        m_disablep = nodep->disablep();
        // Unlink and just keep a pointer to it, convert to sentree as needed
        m_senip = nodep->sensesp();
        iterateNull(nodep->disablep());
        ಆದರೆ (!VN_AS(nodep->backp(), ಗಂಟು_Cover_Or_Assert)->ತಕ್ಷಣ()) {
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ propDtp = nodep->propp()->dtypep();
            nodep->propp(ಹೊಸ AstSampled{nodep->fileline(), nodep->propp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                        propDtp->dtypep(), ನಿಜ});
        }
        // cover counts non-vacuous matches only (IEEE 1800-2023 16.15.2), so an
        // implication antecedent must hold; assert passes vacuously instead.
        VL_RESTORER(m_underCover);
        m_underCover = VN_IS(nodep->backp(), Cover);
        iterate(nodep->propp());
        iterateNull(nodep->ಹೊಂದಿಕೆ_Countp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPExpr* nodep) override {
        // V3AssertNfa handles multi-cycle property expressions before this pass,
        // so the following unsupported paths are defensive and typically unreached.
        ಆದರೆ (m_pexprp && m_pexprp->user1()) {  // LCOV_EXCL_START
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Complex property expression inside 'until''");
            nodep->replaceWith(ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False{}});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ;
        }  // LCOV_EXCL_STOP
        ಆದರೆ (AstLogNot* ಸ್ಥಿರಾಂಕ notp = VN_CAST(nodep->backp(), LogNot)) {
            notp->replaceWith(nodep->unlink_Fr_ಹಿಂಭಾಗ());
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(notp), notp);
            iterate(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        // Sequence expression as antecedent of implication is not yet supported
        ಆದರೆ (AstImplication* ಸ್ಥಿರಾಂಕ implp = VN_CAST(nodep->backp(), Implication)) {
            ಆದರೆ (implp->lhsp() == nodep) {  // LCOV_EXCL_START
                implp->v3warn(E_UNSUPPORTED,
                              "Unsupported: Implication with sequence expression as antecedent");
                nodep->replaceWith(ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False{}});
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಹಿಂತಿರುಗಿ;
            }  // LCOV_EXCL_STOP
        }
        VL_RESTORER(m_pexprp);
        VL_RESTORER(m_disableSeqIfp);
        m_pexprp = nodep;

        ಆದರೆ (m_disablep) {
            ಸ್ಥಿರಾಂಕ AstSampled* sampledp = ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (m_disablep->exists([&sampledp](ಸ್ಥಿರಾಂಕ AstSampled* ಸ್ಥಿರಾಂಕ sp) {
                    sampledp = sp;
                    ಹಿಂತಿರುಗಿ ನಿಜ;
                })) {
                sampledp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: $sampled inside disabled condition of a sequence");
                m_disablep = ಹೊಸ AstConst{m_disablep->fileline(), AstConst::ಬಿಟ್_False{}};
                // always a copy is used, so remove it now
                ತಳ್ಳು_Deletep(m_disablep);  // m_disablep used later
            }
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
            // Add counter which counts times the condition turned true
            AstVar* ಸ್ಥಿರಾಂಕ disableCntp
                = ಹೊಸ AstVar{flp, VVarType::MODULETEMP, m_disable_Cnt_ಹೆಸರುಗಳು.get(""),
                             nodep->ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::UINT32)};
            disableCntp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->ಕೂಡಿಸು_Stmtsp(disableCntp);
            AstVarRef* ಸ್ಥಿರಾಂಕ readCntRefp = ಹೊಸ AstVarRef{flp, disableCntp, VAccess::READ};
            AstVarRef* ಸ್ಥಿರಾಂಕ writeCntRefp = ಹೊಸ AstVarRef{flp, disableCntp, VAccess::WRITE};
            Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ incrStmtp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                flp, writeCntRefp, ಹೊಸ Ast_ಕೂಡಿಸು{flp, readCntRefp, ಹೊಸ AstConst{flp, 1}}};
            AstAlways* ಸ್ಥಿರಾಂಕ alwaysp
                = ಹೊಸ AstAlways{flp, VAlwaysKwd::ALWAYS,
                                ಹೊಸ Ast_Sen_ವೃಕ್ಷ{flp, ಹೊಸ Ast_Sen_ಅಂಶ{flp, VEdgeType::ET_POSEDGE,
                                                                   m_disablep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)}},
                                incrStmtp};
            disableCntp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(alwaysp);

            // Store value of that counter at the beginning of sequence evaluation
            AstBegin* ಸ್ಥಿರಾಂಕ bodyp = nodep->bodyp();
            AstVar* ಸ್ಥಿರಾಂಕ initialCntp = ಹೊಸ AstVar{flp, VVarType::BLOCKTEMP, "__VinitialCnt",
                                                   nodep->ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::UINT32)};
            initialCntp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp
                = ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, initialCntp, VAccess::WRITE},
                                readCntRefp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)};
            // Prepend to the sequence body to keep statement list structure valid.
            UASSERT_OBJ(!bodyp->name().ಖಾಲಿಗಳು(), bodyp, "Sequence body should be named");
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newStmtsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ<Ast_ಗಂಟು, Ast_ಗಂಟು>(initialCntp, assignp);
            bodyp->prependStmtsp(newStmtsp);

            m_disableSeqIfp
                = ಹೊಸ AstIf{flp, ಹೊಸ AstEq{flp, ಹೊಸ AstVarRef{flp, initialCntp, VAccess::READ},
                                           readCntRefp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)}};
            // Delete it, because it is always copied before insetion to the AST
            ತಳ್ಳು_Deletep(m_disableSeqIfp);  // m_disableSeqIfp used later
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultClkEvtVarp);
        VL_RESTORER(m_defaultDisablep);
        VL_RESTORER(m_modp);
        ಆದರೆ (!VN_IS(nodep, Class)) {
            // A class nested in a module is in the scope of the module's default clocking
            m_defaultClockingp = ಶೂನ್ಯಸೂಚಕ;
            m_defaultClkEvtVarp = ಶೂನ್ಯಸೂಚಕ;
        }
        m_defaultDisablep = nodep->defaultDisablep();
        m_modp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Gen_ಖಂಡ* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstProperty* nodep) override {
        // The body will be visited when will be substituted in place of property reference
        // (AstFuncRef)
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSequence* nodep) override {
        // Sequence declarations are not visited directly; their bodies are inlined
        // at call sites by visit(AstFuncRef*). Collect for post-traversal cleanup.
        m_seqs_ಗಮ್ಯ_Cleanup.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ AssertPreVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        { AssertPreClockingVisitor{nodep}; }
        // Process
        iterate(nodep);
        // Fix up varref names
        ಗಾಗಿ (AstVarXRef* xrefp : m_xrefsp) xrefp->name(xrefp->varp()->name());
        // Clean up sequence declarations after inlining.
        // Referenced sequences that were inlined have isReferenced cleared.
        // Remaining referenced sequences are in unsupported contexts (e.g. @seq event).
        ಗಾಗಿ (AstSequence* seqp : m_seqs_ಗಮ್ಯ_Cleanup) {
            ಆದರೆ (seqp->isReferenced()) {
                seqp->v3warn(E_UNSUPPORTED,
                             "Unsupported: sequence referenced outside assertion property");
            } ಇಲ್ಲದಿದ್ದರೆ {
                VL_DO_DANGLING(seqp->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), seqp);
            }
        }
    }
    ~AssertPreVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Top Assert class

ಶೂನ್ಯ V3AssertPre::assertPreAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { AssertPreVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("assertpre", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
