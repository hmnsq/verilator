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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3AssertPre.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Assert.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Const.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Task.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Timing.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3UniqueNames.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clocking blocks of clocking items

ਵਰਗ AssertPreClockingVisitor final : ਜਨਤਕ VNVisitorConst {
    // Captures the clocking block of each clocking item before AssertPreVisitor, which needs it
    // at synchronous drives, often visited before the clocking block, e.g. in a parent module
    // NODE STATE
    // AstClockingItem::user3p()  // AstClocking*. Clocking block of the item, user3 claimed by
    //                            //               AssertPreVisitor

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstNetlist* nodep) override { iterate_And_ਅਗਲਾ_Const_Null(nodep->modulesp()); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override { iterate_And_ਅਗਲਾ_Const_Null(nodep->stmtsp()); }
    ਖਾਲੀ ਦੇਖੋ(Ast_Gen_ਖੰਡ* nodep) override { iterate_And_ਅਗਲਾ_Const_Null(nodep->itemsp()); }
    ਖਾਲੀ ਦੇਖੋ(AstClocking* nodep) override {
        ਲਈ (Ast_ਗੰਢ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            ਜੇ (Ast_Clocking_ਵਸਤੂ* ਅਚਲ citemp = VN_CAST(itemp, Clocking_ਵਸਤੂ)) {
                citemp->user3p(nodep);
            }
        }
    }
    // Clocking blocks are only in modules and their generate blocks
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ*) override {}

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ AssertPreClockingVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~AssertPreClockingVisitor() override = ਮੂਲ;
};

//######################################################################
// Assert class functions

ਵਰਗ AssertPreVisitor final : ਜਨਤਕ VNVisitor {
    // Removes clocks and other pre-optimizations
    // Eventually inlines calls to sequences, properties, etc.
    // We're not parsing the tree, or anything more complicated.
ਨਿਜੀ:
    // TYPES
    ਬਣਤਰ SynchDrive final {
        Ast_Clocking_ਵਸਤੂ* itemp;  // Clocking item of the driven clockvar
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* refp;  // Reference to the driven clockvar
    };

    // NODE STATE
    // AstClockingItem::user1p()         // AstVar*.      varp() of ClockingItem after unlink
    // AstClockingItem::user2p()         // AstVar*.      Flag set by drives of output clockvar
    // AstClockingItem::user3p()         // AstClocking*. Clocking block of the item, captured
    //                                   //               by AssertPreClockingVisitor
    // AstPExpr::user1()                 // bool.         Created from AstUntil
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_inuser2;
    ਅਚਲ VNUser3_ਵਿੱਚ_Use m_inuser3;
    // STATE
    // Current context:
    AstNetlist* ਅਚਲ m_netlistp = ਖਾਲੀ_ਸੂਚਕ;  // Current netlist
    Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Current module
    AstClocking* m_clockingp = ਖਾਲੀ_ਸੂਚਕ;  // Current clocking block
    // Reset each module:
    AstClocking* m_defaultClockingp = ਖਾਲੀ_ਸੂਚਕ;  // Default clocking for current module
    AstVar* m_defaultClkEvtVarp = ਖਾਲੀ_ਸੂਚਕ;  // Event var for default clocking (for ##0)
    AstDefaultDisable* m_defaultDisablep = ਖਾਲੀ_ਸੂਚਕ;  // Default disable for current module
    // Reset each assertion:
    Ast_Sen_ਵਸਤੂ* m_senip = ਖਾਲੀ_ਸੂਚਕ;  // Last sensitivity
    // Reset each always:
    Ast_Sen_ਵਸਤੂ* m_seniAlwaysp = ਖਾਲੀ_ਸੂਚਕ;  // Last sensitivity in always
    // Reset each assertion:
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_disablep = ਖਾਲੀ_ਸੂਚਕ;  // Last disable
    AstIf* m_disableSeqIfp = ਖਾਲੀ_ਸੂਚਕ;  // Used for handling disable iff in sequences
    AstPExpr* m_pexprp = ਖਾਲੀ_ਸੂਚਕ;  // Last AstPExpr
    ਤਾਰਕਿਕ m_underCover = ਝੂਠ;  // True if the enclosing assertion is a cover
    // Other:
    V3_Unique_ਨਾਂ m_ਚੱਕਰ_Dly_ਨਾਂ{"__VcycleDly"};  // Cycle delay counter name generator
    V3_Unique_ਨਾਂ m_cons_Rep_ਨਾਂ{"__VconsRep"};  // Consecutive repetition counter name generator
    V3_Unique_ਨਾਂ m_goto_Rep_ਨਾਂ{"__VgotoRep"};  // Goto repetition counter name generator
    V3_Unique_ਨਾਂ m_non_Cons_Rep_ਨਾਂ{"__VnonConsRep"};  // Nonconsecutive rep name generator
    V3_Unique_ਨਾਂ m_disable_Cnt_ਨਾਂ{"__VdisableCnt"};  // Disable condition counter name generator
    V3_Unique_ਨਾਂ m_ਖੰਡ_ਨਾਂ{"__VassertBlock"};  // Names of blocks with temporaries
    V3_Unique_ਨਾਂ m_prop_Var_ਨਾਂ{"__Vpropvar"};  // Property-local variable name generator
    V3_Unique_ਨਾਂ m_ਸਰਗਰਮ_ਨਾਂ{"__VassertsActive"};  // Active asserts map name generator
    V3_Unique_ਨਾਂ m_driven_ਨਾਂ{"__VclockingDriven"};  // Clockvar drive flag name generator
    ਤਾਰਕਿਕ m_ਵਿੱਚ_ਸੌਂਪੋ = ਝੂਠ;  // True if in an AssignNode
    ਤਾਰਕਿਕ m_ਵਿੱਚ_ਸੌਂਪੋ_Dly_Lhs = ਝੂਠ;  // True if in AssignDly's LHS
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Synch_Drive = ਝੂਠ;  // True if in synchronous drive
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Concat_ਸੌਂਪੋ = ਝੂਠ;  // True if in assignment to a concatenation
    std::ਸਦਿਸ਼<SynchDrive> m_drives;  // Clockvars written by the synchronous drive
    ਤਾਰਕਿਕ m_has_ਚੱਕਰ_Delay = ਝੂਠ;  // True if node has cycle delay beneath
    std::ਸਦਿਸ਼<AstVarXRef*> m_xrefsp;  // list of xrefs that need name fixup
    std::ਸਦਿਸ਼<AstSequence*> m_seqs_ਮੰਜ਼ਿਲ_Cleanup;  // Sequences to clean up after traversal

    // METHODS

    // Flag set by each drive of an output clockvar, added to the module with its clocking item
    AstVar* getCreateDrivenVarp(Ast_Clocking_ਵਸਤੂ* itemp) {
        ਜੇ (!itemp->user2p()) {
            AstVar* ਅਚਲ varp = ਨਵਾਂ AstVar{itemp->fileline(), VVarType::MODULETEMP,
                                            m_driven_ਨਾਂ.get(itemp), itemp->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            itemp->user2p(varp);
        }
        ਵਾਪਸ VN_AS(itemp->user2p(), Var);
    }
    // Reference to a variable in the scope of a driven clockvar, made like the clockvar's
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* newDriveRefp(File_ਸਤਰ* flp, ਅਚਲ SynchDrive& drive, AstVar* varp,
                                     ਅਚਲ VAccess& access) {
        ਜੇ (ਅਚਲ AstVarXRef* ਅਚਲ xrefp = VN_CAST(drive.refp, VarXRef)) {
            ਵਾਪਸ ਨਵਾਂ AstVarXRef{flp, varp, xrefp->dotted(), access};
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstMemberSel* ਅਚਲ selp = VN_CAST(drive.refp, MemberSel)) {
            // The interface expression is evaluated again, so it must not have side effects;
            // cloneTreePure warns if it has any
            AstMemberSel* ਅਚਲ newSelp
                = ਨਵਾਂ AstMemberSel{flp, selp->fromp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), varp};
            newSelp->access(access);
            ਵਾਪਸ newSelp;
        }
        ਵਾਪਸ ਨਵਾਂ AstVarRef{flp, varp, access};
    }
    // Assignment setting the drive flag of a driven clockvar
    Ast_ਸੌਂਪੋ* newDrivenSetp(File_ਸਤਰ* flp, ਅਚਲ SynchDrive& drive) {
        AstVar* ਅਚਲ varp = getCreateDrivenVarp(drive.itemp);
        Ast_ਸੌਂਪੋ* ਅਚਲ setp = ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, newDriveRefp(flp, drive, varp, VAccess::WRITE),
                                              ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}}};
        setp->user1(ਸੱਚ);
        ਵਾਪਸ setp;
    }
    // Clocking block of a clocking item
    ਸਥਿਰ AstClocking* clocking_ਦਾ(ਅਚਲ Ast_Clocking_ਵਸਤੂ* itemp) {
        UASSERT_OBJ(itemp->user3p(), itemp, "Clocking item not captured in a clocking block");
        ਵਾਪਸ VN_AS(itemp->user3p(), Clocking);
    }
    // Clocking event of a driven clockvar, referenced from the drive like the clockvar, or
    // nullptr if unsupported, which is reported on the drive's cycle delay
    Ast_Sen_ਰੁੱਖ* newDriveSentreep(AstDelay* delayp, ਅਚਲ SynchDrive& drive) {
        Ast_Sen_ਰੁੱਖ* ਅਚਲ origp = clocking_ਦਾ(drive.itemp)->sentreep();
        ਜੇ (VN_IS(drive.refp, VarRef)) ਵਾਪਸ origp->clone_ਰੁੱਖ(ਝੂਠ);
        ਜੇ (origp->exists([](ਅਚਲ AstVarXRef*) { ਵਾਪਸ ਸੱਚ; })) {
            // The reference is relative to the clocking block
            delayp->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in synchronous drive to"
                                          " clockvar in another scope, whose clocking event"
                                          " has a hierarchical reference");
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }
        ਅਚਲ AstMemberSel* ਅਚਲ selp = VN_CAST(drive.refp, MemberSel);
        // Processes outside classes evaluate their events also when not waiting on them,
        // possibly before the interface reference is set, so the event is guarded against a
        // null reference, which needs an integral value; scheduling does not guard it
        ਅਚਲ ਤਾਰਕਿਕ guardNull = selp && !VN_IS(m_modp, Class);
        ਜੇ (guardNull) {
            ਲਈ (Ast_Sen_ਵਸਤੂ* itemp = origp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Sen_ਵਸਤੂ)) {
                ਜੇ (!itemp->sensp()->dtypep()->skipRefp()->isIntegralOrPacked()) {
                    delayp->v3warn(E_UNSUPPORTED,
                                   "Unsupported: cycle delay in synchronous drive through"
                                   " an interface reference outside a class, to clocking"
                                   " block with non-integral clocking event");
                    ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
                }
            }
        }
        // Interface declaring the clocking block, if driven through an interface reference
        AstIface* ਅਚਲ ifacep
            = selp ? VN_AS(selp->fromp()->dtypep()->skipRefp(), IfaceRefDType)->iface_ਰਾਹੀਂ_Cellp()
                   : ਖਾਲੀ_ਸੂਚਕ;
        Ast_Sen_ਰੁੱਖ* ਅਚਲ sentreep = origp->clone_ਰੁੱਖ(ਝੂਠ);
        std::ਸਦਿਸ਼<AstVarRef*> refps;
        sentreep->foreach([&](AstVarRef* refp) {
            ਜੇ (!refp->classOrPackagep()) refps.ਧੱਕੋ_ਪਿੱਠ(refp);
        });
        ਲਈ (AstVarRef* ਅਚਲ refp : refps) {
            AstVar* ਅਚਲ varp = refp->varp();
            ਜੇ (ifacep) {
                // Mark the variable as read through an interface reference, as V3Width does for
                // a member select. The mark is the interface declaring the variable, whose
                // instances all share it, so drives through references to any instances agree.
                UASSERT_OBJ(!varp->sensIfacep() || varp->sensIfacep() == ifacep, refp,
                            "Variable read through references to different interfaces");
                varp->sensIfacep(ifacep);
            }
            refp->replaceWith(newDriveRefp(refp->fileline(), drive, varp, refp->access()));
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(refp), refp);
        }
        ਜੇ (guardNull) {
            ਲਈ (Ast_Sen_ਵਸਤੂ* itemp = sentreep->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Sen_ਵਸਤੂ)) {
                guardNullIface(itemp, selp->fromp());
            }
        }
        ਵਾਪਸ sentreep;
    }
    // Make an integral clocking event through an interface reference false while the reference
    // is null
    ਸਥਿਰ ਖਾਲੀ guardNullIface(Ast_Sen_ਵਸਤੂ* sensesp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp) {
        File_ਸਤਰ* ਅਚਲ flp = sensesp->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ sensp = sensesp->sensp()->unlink_Fr_ਪਿੱਠ();
        sensesp->sensp(ਨਵਾਂ AstCond{flp, newNotNullp(flp, fromp), sensp,
                                   ਨਵਾਂ AstConst{flp, AstConst::DTyped{}, sensp->dtypep()}});
        ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp = sensesp->condp()) {
            condp->unlink_Fr_ਪਿੱਠ();
            sensesp->condp(ਨਵਾਂ AstLogAnd{flp, newNotNullp(flp, fromp), condp});
        }
    }
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* newNotNullp(File_ਸਤਰ* flp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp) {
        ਵਾਪਸ ਨਵਾਂ AstNeq{flp, fromp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), ਨਵਾਂ AstConst{flp, AstConst::Null{}}};
    }
    // Procedural '##0': if the default clocking event has not yet occurred in this time step,
    // wait for it; otherwise continue without suspension (IEEE 1800-2023 14.11)
    AstIf* new_ਚੱਕਰ_Delay_Zerop(File_ਸਤਰ* flp) ਅਚਲ {
        AstVar* ਅਚਲ evtVarp = m_defaultClkEvtVarp;
        UASSERT_OBJ(evtVarp, flp, "Default clocking event var not pre-created");
        AstCMethodHard* ਅਚਲ isTriggeredp = ਨਵਾਂ AstCMethodHard{
            flp, ਨਵਾਂ AstVarRef{flp, evtVarp, VAccess::READ}, VCMethod::EVENT_IS_TRIGGERED};
        isTriggeredp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        AstEventControl* ਅਚਲ waitp = ਨਵਾਂ AstEventControl{
            flp,
            ਨਵਾਂ Ast_Sen_ਰੁੱਖ{flp, ਨਵਾਂ Ast_Sen_ਵਸਤੂ{flp, VEdgeType::ET_EVENT,
                                               ਨਵਾਂ AstVarRef{flp, evtVarp, VAccess::READ}}},
            ਖਾਲੀ_ਸੂਚਕ};
        ਵਾਪਸ ਨਵਾਂ AstIf{flp, ਨਵਾਂ AstNot{flp, isTriggeredp}, waitp};
    }

    ਸਥਿਰ ਖਾਲੀ checkSamplingFuncDType(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep, ਅਚਲ Ast_ਗੰਢ* exprp) {
        ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep = exprp->dtypep()->skipRefp();
        ਜੇ (!dtypep->isIntegralOrPacked()) {
            nodep->v3error("Expected numeric type, but got a " << dtypep->prettyDTypeNameQ()
                                                               << " data type");
        }
    }

    Ast_Sen_ਰੁੱਖ* new_Sen_ਰੁੱਖ(Ast_ਗੰਢ* nodep, Ast_Sen_ਰੁੱਖ* useTreep = ਖਾਲੀ_ਸੂਚਕ,
                           Ast_ਗੰਢ_Cover_Or_Assert* cassertp = ਖਾਲੀ_ਸੂਚਕ) {
        // Create sentree based on clocked or default clock
        // Return nullptr for always
        ਜੇ (useTreep) ਵਾਪਸ useTreep;
        Ast_Sen_ਰੁੱਖ* newp = ਖਾਲੀ_ਸੂਚਕ;
        Ast_Sen_ਵਸਤੂ* senip = m_senip;
        ਤਾਰਕਿਕ fromAlways = ਝੂਠ;
        ਜੇ (!senip && m_defaultClockingp) senip = m_defaultClockingp->sentreep()->sensesp();
        ਜੇ (!senip) {
            senip = m_seniAlwaysp;
            fromAlways = ਸੱਚ;
        }
        ਜੇ (!senip) {
            nodep->v3error("Concurrent assertion has no clock (IEEE 1800-2023 16.16)\n"
                           << nodep->warn_ਹੋਰ()
                           << "... Suggest provide a clocking event, a default"
                              " clocking, or a clocked procedural context");
            newp = ਨਵਾਂ Ast_Sen_ਰੁੱਖ{nodep->fileline(), ਖਾਲੀ_ਸੂਚਕ};
        } ਨਹੀਂਤਾਂ {
            ਜੇ (cassertp && fromAlways) cassertp->senFromAlways(ਸੱਚ);
            newp = ਨਵਾਂ Ast_Sen_ਰੁੱਖ{nodep->fileline(), senip->clone_ਰੁੱਖ(ਸੱਚ)};
        }
        ਵਾਪਸ newp;
    }
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* getSequenceBodyExprp(ਅਚਲ AstSequence* ਅਚਲ seqp) ਅਚਲ {
        // The statements in AstSequence are optional AstVar (ports) followed by body expr.
        Ast_ਗੰਢ* bodyp = seqp->stmtsp();
        ਜਦੋਂ (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        ਵਾਪਸ VN_CAST(bodyp, ਗੰਢ_ਵਿਅੰਜਕ);
    }
    AstPropSpec* getPropertyExprp(ਅਚਲ AstProperty* ਅਚਲ propp) {
        // Statements in AstProperty: AstVar (ports/local vars),
        // AstInitialStaticStmt/AstInitialAutomaticStmt (var init), AstPropSpec (body).
        Ast_ਗੰਢ* propExprp = propp->stmtsp();
        ਜਦੋਂ (propExprp
               && (VN_IS(propExprp, Var) || VN_IS(propExprp, InitialStaticStmt)
                   || VN_IS(propExprp, InitialAutomaticStmt))) {
            propExprp = propExprp->nextp();
        }
        ਵਾਪਸ VN_CAST(propExprp, PropSpec);
    }
    ਖਾਲੀ substituteSequenceCall(AstFuncRef* funcrefp, AstSequence* seqp) {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Inline the sequence body at the call site, replacing the FuncRef
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        // Clone the body expression since the sequence may be referenced multiple times
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* clonedp = bodyExprp->clone_ਰੁੱਖ(ਝੂਠ);
        // Build substitution map, then do a single traversal to replace all formals
        // (textual substitution per IEEE 16.8.2).
        ਅਚਲ V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<ਅਚਲ AstVar*, Ast_ਗੰਢ_ਵਿਅੰਜਕ*> ਪੋਰਟ_Map;
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& tconnect : tconnects) {
            ਪੋਰਟ_Map[tconnect.ਪਹਿਲਾ] = tconnect.ਦੂਜਾ->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            ਅਚਲ ਸਵੈਚਾਲਿਤ it = ਪੋਰਟ_Map.ਲੱਭੋ(refp->varp());
            ਜੇ (it != ਪੋਰਟ_Map.ਸਿਰਾ()) {
                refp->replaceWith(it->ਦੂਜਾ->clone_ਰੁੱਖ(ਝੂਠ));
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(refp), refp);
            }
        });
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& tconnect : tconnects) {
            ਧੱਕੋ_Deletep(tconnect.ਦੂਜਾ->exprp()->unlink_Fr_ਪਿੱਠ());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        // Replace the FuncRef with the inlined body
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(funcrefp), funcrefp);
        // Clear referenced flag; sequences with isReferenced==false are deleted in assertPreAll
        seqp->isReferenced(ਝੂਠ);
    }
    AstPropSpec* clonePropertyBody(AstFuncRef* funcrefp, ਅਚਲ AstProperty* propp) {
        AstPropSpec* propExprp = getPropertyExprp(propp);
        // Substitute inner property call before copying in order to not doing the same for
        // each call of outer property call.
        propExprp = substitutePropertyCall(propExprp);
        // Clone subtree after substitution. It is needed, because property might be called
        // multiple times with different arguments.
        propExprp = propExprp->clone_ਰੁੱਖ(ਝੂਠ);
        // Build substitution maps for formal arguments and property-local
        // variables, then perform a single foreach to apply all replacements.
        // Map port vars to their actual argument expressions
        ਅਚਲ V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propp->stmtsp());
        std::unordered_map<ਅਚਲ AstVar*, Ast_ਗੰਢ_ਵਿਅੰਜਕ*> ਪੋਰਟ_Map;
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& tconnect : tconnects) {
            ਪੋਰਟ_Map[tconnect.ਪਹਿਲਾ] = tconnect.ਦੂਜਾ->exprp();
        }

        // Promote property-local variables (non-port vars, IEEE 16.10) to
        // module-level __Vpropvar temps. Cross-cycle persistence is handled
        // by the match item lowering in visit(AstImplication*).
        std::unordered_map<ਅਚਲ AstVar*, AstVar*> localVarMap;
        ਲਈ (Ast_ਗੰਢ* stmtp = propp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(stmtp, Var)) {
                ਜੇ (!varp->isIO()) {
                    ਅਚਲ string newName = m_prop_Var_ਨਾਂ.get(varp);
                    AstVar* ਅਚਲ newVarp = ਨਵਾਂ AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->ਜੋੜੋ_Stmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        // Single traversal: substitute ports and update local var references
        propExprp->foreach([&](AstVarRef* refp) {
            ਅਚਲ ਸਵੈਚਾਲਿਤ ਪੋਰਟ_It = ਪੋਰਟ_Map.ਲੱਭੋ(refp->varp());
            ਜੇ (ਪੋਰਟ_It != ਪੋਰਟ_Map.ਸਿਰਾ()) {
                refp->replaceWith(ਪੋਰਟ_It->ਦੂਜਾ->clone_ਰੁੱਖ(ਝੂਠ));
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(refp), refp);
                ਵਾਪਸ;
            }
            ਅਚਲ ਸਵੈਚਾਲਿਤ localIt = localVarMap.ਲੱਭੋ(refp->varp());
            ਜੇ (localIt != localVarMap.ਸਿਰਾ()) refp->varp(localIt->ਦੂਜਾ);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        // Clean up argument expressions
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& tconnect : tconnects) {
            ਧੱਕੋ_Deletep(tconnect.ਦੂਜਾ->exprp()->unlink_Fr_ਪਿੱਠ());
        }
        V3AssertCommon::checkSvaInstanceDelays(propExprp, funcrefp);
        ਵਾਪਸ propExprp;
    }
    AstPropSpec* substitutePropertyCall(AstPropSpec* nodep) {
        ਜੇ (AstFuncRef* ਅਚਲ funcrefp = VN_CAST(nodep->propp(), FuncRef)) {
            ਜੇ (ਅਚਲ AstProperty* ਅਚਲ propp = VN_CAST(funcrefp->taskp(), Property)) {
                AstPropSpec* ਅਚਲ propExprp = clonePropertyBody(funcrefp, propp);

                // Handle case with 2 disable iff statement (IEEE 1800-2023 16.12.1)
                ਜੇ (nodep->disablep() && propExprp->disablep()) {
                    nodep->v3error("disable iff expression before property call and in its "
                                   "body is not legal");
                    ਧੱਕੋ_Deletep(propExprp->disablep()->unlink_Fr_ਪਿੱਠ());
                }
                // If disable iff is in outer property, move it to inner
                ਜੇ (nodep->disablep()) {
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ disablep = nodep->disablep()->unlink_Fr_ਪਿੱਠ();
                    propExprp->disablep(disablep);
                }

                ਜੇ (nodep->sensesp() && propExprp->sensesp()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Clock event before property call and in its body");
                    ਧੱਕੋ_Deletep(propExprp->sensesp()->unlink_Fr_ਪਿੱਠ());
                }
                // If clock event is in outer property, move it to inner
                ਜੇ (nodep->sensesp()) {
                    Ast_Sen_ਵਸਤੂ* ਅਚਲ sensesp = nodep->sensesp();
                    sensesp->unlink_Fr_ਪਿੱਠ();
                    propExprp->sensesp(sensesp);
                }

                // Now substitute property reference with property body
                nodep->replaceWith(propExprp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                ਵਾਪਸ propExprp;
            }
        }
        ਵਾਪਸ nodep;
    }

    ਖਾਲੀ substituteNestedPropertyCall(AstFuncRef* funcrefp, ਅਚਲ AstProperty* propp) {
        AstPropSpec* ਅਚਲ specp = clonePropertyBody(funcrefp, propp);
        Ast_ਗੰਢ* replp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (specp->disablep()) {
            funcrefp->v3error("Illegal 'disable iff' in a nested property instance"
                              " (IEEE 1800-2023 16.12.1)");
        } ਨਹੀਂਤਾਂ ਜੇ (specp->sensesp()) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: clock event in a nested property instance");
        } ਨਹੀਂਤਾਂ ਜੇ (specp->propp()->exists(
                       [](ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* np) { ਵਾਪਸ np->is_Multi_ਚੱਕਰ_Sva(); })) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: multi-cycle body in a nested property instance");
        } ਨਹੀਂਤਾਂ {
            replp = specp->propp()->unlink_Fr_ਪਿੱਠ();
        }
        ਜੇ (!replp) replp = ਨਵਾਂ AstConst{funcrefp->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}};
        funcrefp->replaceWith(replp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(funcrefp), funcrefp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(specp), specp);
    }

    // VISITORS
    //========== Statements
    ਖਾਲੀ ਦੇਖੋ(AstClocking* ਅਚਲ nodep) override {
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        UINFO(8, "   CLOCKING" << nodep);
        ਜੇ (nodep->isDefault()) {
            ਜੇ (m_defaultClockingp) {
                nodep->v3error("Only one default clocking block allowed per module"
                               " (IEEE 1800-2023 14.12)");
            } ਨਹੀਂਤਾਂ {
                m_defaultClockingp = nodep;
                m_defaultClkEvtVarp = nodep->ensureEventp();
            }
        }
        iterateChildren(nodep);
        ਜੇ (nodep->eventp()) nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(nodep->eventp()->unlink_Fr_ਪਿੱਠ());
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstModportClockingRef* ਅਚਲ nodep) override {
        // It has to be converted to a list of ModportClockingVarRefs,
        // because clocking blocks are removed in this pass
        ਲਈ (Ast_ਗੰਢ* itemp = nodep->clockingp()->itemsp(); itemp; itemp = itemp->nextp()) {
            ਜੇ (Ast_Clocking_ਵਸਤੂ* ਅਚਲ citemp = VN_CAST(itemp, Clocking_ਵਸਤੂ)) {
                ਜੇ (AstVar* ਅਚਲ varp
                    = citemp->varp() ? citemp->varp() : VN_AS(citemp->user1p(), Var)) {
                    AstModportVarRef* ਅਚਲ modVarp = ਨਵਾਂ AstModportVarRef{
                        nodep->fileline(), varp->name(), citemp->direction()};
                    modVarp->varp(varp);
                    nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(modVarp);
                    ਜੇ (citemp->direction().isOutput()) {
                        // Drives through the modport set the drive flag
                        AstVar* ਅਚਲ drivenVarp = getCreateDrivenVarp(citemp);
                        AstModportVarRef* ਅਚਲ drivenRefp = ਨਵਾਂ AstModportVarRef{
                            nodep->fileline(), drivenVarp->name(), VDirection::OUTPUT};
                        drivenRefp->varp(drivenVarp);
                        nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(drivenRefp);
                    }
                }
            }
        }
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Clocking_ਵਸਤੂ* ਅਚਲ nodep) override {
        // Get a ref to the sampled/driven variable
        AstVar* ਅਚਲ varp = nodep->varp();
        ਜੇ (!varp) {
            // Unused item
            ਵਾਪਸ;
        }
        // Flag set by drives of an output clockvar, possibly visited before this item
        ਜੇ (nodep->direction().isOutput()) m_modp->ਜੋੜੋ_Stmtsp(getCreateDrivenVarp(nodep));
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        V3Const::constifyEdit(nodep->skewp());
        ਜੇ (!VN_IS(nodep->skewp(), Const)) {
            nodep->skewp()->v3error("Skew must be constant (IEEE 1800-2023 14.4)");
            ਵਾਪਸ;
        }
        AstConst* ਅਚਲ skewp = VN_AS(nodep->skewp(), Const);
        ਜੇ (skewp->ਅੰਕ().isNegative()) skewp->v3error("Skew cannot be negative");
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = nodep->exprp();
        varp->name(m_clockingp->name() + "__DOT__" + varp->name());
        m_clockingp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(varp->unlink_Fr_ਪਿੱਠ());
        nodep->user1p(varp);
        varp->user1p(nodep);
        ਜੇ (nodep->direction() == VDirection::OUTPUT) {
            exprp->foreach([](ਅਚਲ Ast_ਗੰਢ_Var_Ref* varrefp) {
                // Prevent confusing BLKANDNBLK warnings on clockvars due to generated assignments
                varrefp->fileline()->warnOff(V3_Error_ਕੋਡ::BLKANDNBLK, ਸੱਚ);
                // Mark the driven signal so later passes (e.g. V3Inst port
                // connection) know the drive flows out from inside the cell.
                // Item expressions are plain signal references, so only the
                // driven signal is marked, never index selects.
                varrefp->varp()->isClockingDriven(ਸੱਚ);
            });
            AstVarRef* ਅਚਲ skewedReadRefp = ਨਵਾਂ AstVarRef{flp, varp, VAccess::READ};
            skewedReadRefp->user1(ਸੱਚ);
            // Initialize the clockvar
            AstVarRef* ਅਚਲ skewedWriteRefp = ਨਵਾਂ AstVarRef{flp, varp, VAccess::WRITE};
            skewedWriteRefp->user1(ਸੱਚ);
            AstInitialStatic* ਅਚਲ initClockvarp = ਨਵਾਂ AstInitialStatic{
                flp, ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, skewedWriteRefp, exprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)}};
            m_modp->ਜੋੜੋ_Stmtsp(initClockvarp);
            // Each drive sets a flag, so the signal is also assigned when driven with the same
            // value again, e.g. after another assignment to the signal (IEEE 1800-2023 14.16.2)
            AstVar* ਅਚਲ drivenVarp = getCreateDrivenVarp(nodep);
            m_modp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstInitialStatic{
                flp, ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                   ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}}}});
            // Assign the clockvar to the actual var if it was driven
            Ast_ਸੌਂਪੋ* ਅਚਲ assignp
                = ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, exprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), skewedReadRefp};
            AstIf* ਅਚਲ ifp
                = ਨਵਾਂ AstIf{flp, ਨਵਾਂ AstVarRef{flp, drivenVarp, VAccess::READ},
                            ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                          ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}}}};
            ifp->ਜੋੜੋ_Thensp(assignp);
            // Only in the time step of the clocking event
            AstCMethodHard* ਅਚਲ trigp = ਨਵਾਂ AstCMethodHard{
                nodep->fileline(), ਨਵਾਂ AstVarRef{flp, m_clockingp->ensureEventp(), VAccess::READ},
                VCMethod::EVENT_IS_TRIGGERED};
            trigp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
            ifp->condp(ਨਵਾਂ AstLogAnd{flp, ifp->condp()->unlink_Fr_ਪਿੱਠ(), trigp});
            // A skew rounding to no time precision ticks is a zero skew
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ticksp
                = V3Timing::delay_ਮੰਜ਼ਿਲ_Ticks(skewp->clone_ਰੁੱਖ(ਝੂਠ), m_modp->timeunit());
            ਅਚਲ ਤਾਰਕਿਕ zeroSkew = ticksp->isZero();
            VL_DO_DANGLING(ticksp->delete_ਰੁੱਖ(), ticksp);
            ਜੇ (zeroSkew || !skewp->fileline()->timingOn()) {
                // Drive the var in Re-NBA (IEEE 1800-2023 14.16), ignoring skews where timing is
                // turned off
                Ast_Sen_ਰੁੱਖ* senTreep = m_clockingp->sentreep()->clone_ਰੁੱਖ(ਝੂਠ);
                senTreep->ਜੋੜੋ_Sensesp(ਨਵਾਂ Ast_Sen_ਵਸਤੂ{
                    flp, VEdgeType::ET_ਬਦਲਿਆ, ਨਵਾਂ AstVarRef{flp, drivenVarp, VAccess::READ}});
                m_clockingp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(ਨਵਾਂ AstAlwaysReactive{flp, senTreep, ifp});
            } ਨਹੀਂਤਾਂ {
                // Fork only the assignment, so this AlwaysPostponed can be retriggered
                // before the assignment happens
                AstFork* ਅਚਲ forkp = ਨਵਾਂ AstFork{flp, VJoinType::JOIN_NONE};
                forkp->ਤੁਰੰਤ_ਸ਼ੁਰੂ(ਸੱਚ);
                forkp->ਜੋੜੋ_Forksp(ਨਵਾਂ AstBegin{flp, "", assignp->unlink_Fr_ਪਿੱਠ(), ਸੱਚ});
                ifp->ਜੋੜੋ_Thensp(forkp);
                // Read the clockvar in Postponed, so a drive late in the time step is applied
                // at this event plus the skew (IEEE 1800-2023 14.16)
                m_clockingp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(ਨਵਾਂ AstAlwaysPostponed{flp, ifp});
                ਜੇ (v3Global.opt.timing().is_ਸੈੱਟ_True()) {
                    AstDelay* ਅਚਲ delayp = ਨਵਾਂ AstDelay{flp, skewp->unlink_Fr_ਪਿੱਠ(), ਝੂਠ};
                    delayp->timeunit(m_modp->timeunit());
                    assignp->timingControlp(delayp);
                } ਨਹੀਂਤਾਂ ਜੇ (v3Global.opt.timing().is_ਸੈੱਟ_False()) {
                    nodep->v3warn(E_NOTIMING,
                                  "Clocking output skew greater than #0 requires --timing");
                } ਨਹੀਂਤਾਂ {
                    nodep->v3warn(E_NEEDTIMINGOPT,
                                  "Use --timing or --no-timing to specify how "
                                  "clocking output skew greater than #0 should be handled");
                }
            }
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->direction() == VDirection::ਨਿਵੇਸ਼) {
            // Ref to the clockvar
            AstVarRef* ਅਚਲ refp = ਨਵਾਂ AstVarRef{flp, varp, VAccess::WRITE};
            refp->user1(ਸੱਚ);
            ਜੇ (skewp->ਅੰਕ().is1_ਕਦਮ()) {
                // #1step means the value that is sampled is always the signal's last value
                // before the clock edge (IEEE 1800-2023 14.4)
                AstSampled* ਅਚਲ sampledp
                    = ਨਵਾਂ AstSampled{flp, exprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), exprp->dtypep(), ਸੱਚ};
                Ast_ਸੌਂਪੋ* ਅਚਲ assignp = ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, refp, sampledp};
                m_clockingp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(ਨਵਾਂ AstAlways{
                    flp, VAlwaysKwd::ALWAYS, m_clockingp->sentreep()->clone_ਰੁੱਖ(ਝੂਠ), assignp});
            } ਨਹੀਂਤਾਂ ਜੇ (skewp->isZero()) {
                // #0 means the var has to be sampled in Observed (IEEE 1800-2023 14.13)
                Ast_ਸੌਂਪੋ* ਅਚਲ assignp = ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, refp, exprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
                m_clockingp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(ਨਵਾਂ AstAlwaysObserved{
                    flp, m_clockingp->sentreep()->clone_ਰੁੱਖ(ਝੂਠ), assignp});
            } ਨਹੀਂਤਾਂ {
                // Create a queue where we'll store sampled values with timestamps
                Ast_Sample_ਕਤਾਰ_DType* ਅਚਲ ਕਤਾਰ_Dtp
                    = ਨਵਾਂ Ast_Sample_ਕਤਾਰ_DType{flp, exprp->dtypep()};
                m_netlistp->typeTablep()->ਜੋੜੋ_Typesp(ਕਤਾਰ_Dtp);
                AstVar* ਅਚਲ ਕਤਾਰ_Varp
                    = ਨਵਾਂ AstVar{flp, VVarType::MODULETEMP, "__Vqueue__" + varp->name(), ਕਤਾਰ_Dtp};
                ਕਤਾਰ_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_clockingp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(ਕਤਾਰ_Varp);
                // Create a process like this:
                //     always queue.push(<sampled var>);
                AstCMethodHard* ਅਚਲ pushp = ਨਵਾਂ AstCMethodHard{
                    flp, ਨਵਾਂ AstVarRef{flp, ਕਤਾਰ_Varp, VAccess::WRITE}, VCMethod::DYN_ਧੱਕੋ,
                    ਨਵਾਂ Ast_ਸਮਾਂ{nodep->fileline(), m_modp->timeunit()}};
                pushp->ਜੋੜੋ_Pinsp(exprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ));
                pushp->dtype_ਸੈੱਟ_Void();
                m_clockingp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(
                    ਨਵਾਂ AstAlways{flp, VAlwaysKwd::ALWAYS, ਖਾਲੀ_ਸੂਚਕ, pushp->makeStmt()});
                // Create a process like this:
                //     always @<clocking event> queue.pop(<skew>, /*out*/<skewed var>);
                AstCMethodHard* ਅਚਲ popp = ਨਵਾਂ AstCMethodHard{
                    flp, ਨਵਾਂ AstVarRef{flp, ਕਤਾਰ_Varp, VAccess::READWRITE}, VCMethod::DYN_ਕੱਢੋ,
                    ਨਵਾਂ Ast_ਸਮਾਂ{nodep->fileline(), m_modp->timeunit()}};
                popp->ਜੋੜੋ_Pinsp(skewp->unlink_Fr_ਪਿੱਠ());
                refp->access(VAccess::READWRITE);  // Only conditionally assigned
                popp->ਜੋੜੋ_Pinsp(refp);
                popp->dtype_ਸੈੱਟ_Void();
                m_clockingp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(ਨਵਾਂ AstAlways{flp, VAlwaysKwd::ALWAYS,
                                                       m_clockingp->sentreep()->clone_ਰੁੱਖ(ਝੂਠ),
                                                       popp->makeStmt()});
            }
        } ਨਹੀਂਤਾਂ {
            nodep->v3fatalSrc("Invalid direction");
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstDelay* nodep) override {
        m_has_ਚੱਕਰ_Delay |= nodep->is_ਚੱਕਰ_Delay();
        // Only cycle delays are relevant in this stage; also only process once
        ਜੇ (!nodep->is_ਚੱਕਰ_Delay()) {
            ਜੇ (m_ਵਿੱਚ_Synch_Drive) {
                nodep->v3error("Only cycle delays can be used in synchronous drives"
                               " (IEEE 1800-2023 14.16)");
            }
            iterateChildren(nodep);
            ਵਾਪਸ;
        }
        ਜੇ (m_ਵਿੱਚ_ਸੌਂਪੋ && !m_ਵਿੱਚ_Synch_Drive) {
            nodep->v3error("Cycle delays not allowed as intra-assignment delays"
                           " (IEEE 1800-2023 14.11)");
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
            ਵਾਪਸ;
        }
        ਜੇ (nodep->stmtsp()) nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(nodep->stmtsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep = V3Const::constifyEdit(nodep->lhsp()->unlink_Fr_ਪਿੱਠ());
        ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(valuep, Const);
        ਜੇ (!constp && m_pexprp) {
            // V3AssertNfa handles non-const delays in sequences before this pass and
            // replaces the property; this branch should never be reached.
            nodep->v3fatalSrc("Non-constant cycle delay in assertion: "
                              "should have been caught by V3AssertNfa");
        } ਨਹੀਂਤਾਂ ਜੇ (constp && constp->isZero()) {
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(valuep), valuep);
            ਜੇ (m_ਵਿੱਚ_Synch_Drive) {
                // ##0 has no effect in synchronous drives (IEEE 1800-2023 14.11)
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
                ਵਾਪਸ;
            }
            ਜੇ (m_pexprp) {
                // ##0 in sequence context = zero delay = same clock tick
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
                ਵਾਪਸ;
            }
            // Procedural ##0: synchronize with default clocking event (IEEE 1800-2023 14.11)
            ਜੇ (!m_defaultClockingp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
                ਵਾਪਸ;
            }
            nodep->replaceWith(new_ਚੱਕਰ_Delay_Zerop(flp));
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ;
        }
        Ast_Sen_ਰੁੱਖ* sentreep = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (!m_drives.ਖਾਲੀ_ਥਾਵਾਂ()) {
            ਜੇ (m_ਵਿੱਚ_Concat_ਸੌਂਪੋ) {
                // Also the clockvars may be of different clocking blocks
                nodep->v3error("Cycle delays cannot be used in synchronous drives to a"
                               " concatenation (IEEE 1800-2023 14.16)");
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(valuep), valuep);
                ਵਾਪਸ;
            }
            // Count the cycles of the driven clockvar's clocking block (IEEE 1800-2023 14.16)
            sentreep = newDriveSentreep(nodep, m_drives.front());
            ਜੇ (!sentreep) {
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(valuep), valuep);
                ਵਾਪਸ;
            }
        } ਨਹੀਂਤਾਂ ਜੇ (!m_defaultClockingp) {
            ਜੇ (!m_pexprp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(valuep), valuep);
                ਵਾਪਸ;
            }
            sentreep = ਨਵਾਂ Ast_Sen_ਰੁੱਖ{flp, m_senip->clone_ਰੁੱਖ(ਝੂਠ)};
        } ਨਹੀਂਤਾਂ {
            sentreep = m_defaultClockingp->sentreep()->clone_ਰੁੱਖ(ਝੂਠ);
        }
        AstEventControl* ਅਚਲ controlp
            = ਨਵਾਂ AstEventControl{nodep->fileline(), sentreep, ਖਾਲੀ_ਸੂਚਕ};
        ਅਚਲ std::string delayName = m_ਚੱਕਰ_Dly_ਨਾਂ.get(nodep);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* throughoutp
            = nodep->throughoutp() ? nodep->throughoutp()->unlink_Fr_ਪਿੱਠ() : ਖਾਲੀ_ਸੂਚਕ;

        AstVar* ਅਚਲ cntVarp = ਨਵਾਂ AstVar{flp, VVarType::BLOCKTEMP, delayName + "__counter",
                                           nodep->ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstBegin* ਅਚਲ beginp = ਨਵਾਂ AstBegin{flp, delayName + "__block", cntVarp, ਸੱਚ};
        // A non-constant delay is counted at runtime. The count is integral of 64 bits, as
        // V3Width makes delays, or real, which is rounded (IEEE 1800-2023 6.12.1)
        ਜੇ (valuep->isDouble()) {
            valuep = ਨਵਾਂ AstRToIRoundS{flp, valuep};
            valuep->dtypeFrom(cntVarp);
        } ਨਹੀਂਤਾਂ {
            valuep = ਨਵਾਂ AstSel{flp, valuep, 0, cntVarp->ਚੌੜਾਈ()};
        }
        beginp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, cntVarp, VAccess::WRITE}, valuep});
        // If the count is zero, a synchronous drive waits for none, like with a constant '##0',
        // and a procedural delay waits like a procedural '##0' (IEEE 1800-2023 14.11)
        ਜੇ (!constp && !m_ਵਿੱਚ_Synch_Drive) {
            beginp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstIf{
                flp,
                ਨਵਾਂ AstEq{flp, ਨਵਾਂ AstVarRef{flp, cntVarp, VAccess::READ}, ਨਵਾਂ AstConst{flp, 0}},
                new_ਚੱਕਰ_Delay_Zerop(flp)});
        }

        // Throughout: create flag tracking whether condition held every tick
        AstVar* throughoutOkp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (throughoutp) {
            throughoutOkp = ਨਵਾਂ AstVar{flp, VVarType::BLOCKTEMP, delayName + "__throughoutOk",
                                       nodep->ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::LOGIC_IMPLICIT)};
            throughoutOkp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->ਜੋੜੋ_Stmtsp(throughoutOkp);
            beginp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}}});
            // Check condition at tick 0 (sequence start, before entering loop)
            beginp->ਜੋੜੋ_Stmtsp(
                ਨਵਾਂ AstIf{flp, ਨਵਾਂ AstLogNot{flp, throughoutp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)},
                          ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                        ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}}}});
        }

        {
            AstLoop* ਅਚਲ loopp = ਨਵਾਂ AstLoop{flp};
            // When throughout is present, exit loop early if condition fails
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* loopCondp
                = ਨਵਾਂ AstGt{flp, ਨਵਾਂ AstVarRef{flp, cntVarp, VAccess::READ}, ਨਵਾਂ AstConst{flp, 0}};
            ਜੇ (throughoutOkp) {
                loopCondp = ਨਵਾਂ AstLogAnd{flp, loopCondp,
                                          ਨਵਾਂ AstVarRef{flp, throughoutOkp, VAccess::READ}};
            }
            loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstLoopTest{flp, loopp, loopCondp});
            loopp->ਜੋੜੋ_Stmtsp(controlp);
            loopp->ਜੋੜੋ_Stmtsp(
                ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, cntVarp, VAccess::WRITE},
                              ਨਵਾਂ AstSub{flp, ਨਵਾਂ AstVarRef{flp, cntVarp, VAccess::READ},
                                         ਨਵਾਂ AstConst{flp, 1}}});
            // Check throughout condition at each tick during delay (IEEE 1800-2023 16.9.9)
            ਜੇ (throughoutp) {
                loopp->ਜੋੜੋ_Stmtsp(
                    ਨਵਾਂ AstIf{flp, ਨਵਾਂ AstLogNot{flp, throughoutp},
                              ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}}}});
            }
            beginp->ਜੋੜੋ_Stmtsp(loopp);
        }
        // Compose wrappers on remaining sequence: throughout gate (inner), disable iff (outer)
        Ast_ਗੰਢ* remainp = nodep->nextp() ? nodep->nextp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ() : ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (throughoutOkp) {
            // If condition failed during delay, fail assertion
            remainp = ਨਵਾਂ AstIf{flp, ਨਵਾਂ AstVarRef{flp, throughoutOkp, VAccess::READ}, remainp,
                                ਨਵਾਂ AstPExprClause{flp, /*pass=*/ਝੂਠ}};
        }
        ਜੇ (m_disableSeqIfp && remainp) {
            AstIf* ਅਚਲ disableSeqIfp = m_disableSeqIfp->clone_ਰੁੱਖ(ਝੂਠ);
            disableSeqIfp->ਜੋੜੋ_Thensp(remainp);
            remainp = disableSeqIfp;
        }
        ਜੇ (remainp) {
            ਜੇ (throughoutOkp) {
                // throughoutOkp is declared in beginp scope -- check must be inside it
                beginp->ਜੋੜੋ_Stmtsp(remainp);
            } ਨਹੀਂਤਾਂ {
                nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(remainp);
            }
        }
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Sen_ਰੁੱਖ* nodep) override {
        ਜੇ (m_ਵਿੱਚ_Synch_Drive) {
            nodep->v3error("Event controls cannot be used in "
                           "synchronous drives (IEEE 1800-2023 14.16)");
        }

        ਅਚਲ AstSampled* sampledp;
        ਜੇ (nodep->exists([&sampledp](ਅਚਲ AstSampled* ਅਚਲ sp) {
                sampledp = sp;
                ਵਾਪਸ ਸੱਚ;
            })) {
            sampledp->v3warn(E_UNSUPPORTED, "Unsupported: $sampled inside sensitivity list");
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Var_Ref* nodep) override {
        UINFO(8, " -varref:  " << nodep);
        UINFO(8, " -varref-var-back:  " << nodep->varp()->backp());
        UINFO(8, " -varref-var-user1:  " << nodep->varp()->user1p());
        ਜੇ (Ast_Clocking_ਵਸਤੂ* ਅਚਲ itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->ਪਹਿਲਾ_Abovep(),
                Clocking_ਵਸਤੂ)) {
            ਜੇ (nodep->user1()) ਵਾਪਸ;

            // ensure linking still works, this has to be done only once
            ਜੇ (AstVarXRef* xrefp = VN_CAST(nodep, VarXRef)) {
                UINFO(8, " -clockvarxref-in:  " << xrefp);
                string dotted = xrefp->dotted();
                ਅਚਲ ਮਾਪ_t dotPos = dotted.rfind('.');
                dotted.erase(dotPos, string::npos);
                xrefp->dotted(dotted);
                UINFO(8, " -clockvarxref-out: " << xrefp);
                m_xrefsp.emplace_ਪਿੱਠ(xrefp);
            }

            ਜੇ (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            ਜੇ (nodep->access().isWriteOrRW()) {
                ਜੇ (itemp->direction() == VDirection::OUTPUT) {
                    ਜੇ (!m_ਵਿੱਚ_ਸੌਂਪੋ_Dly_Lhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    ਜੇ (m_ਵਿੱਚ_ਸੌਂਪੋ) {
                        m_ਵਿੱਚ_Synch_Drive = ਸੱਚ;
                        m_drives.ਧੱਕੋ_ਪਿੱਠ({itemp, nodep});
                    }
                } ਨਹੀਂਤਾਂ ਜੇ (itemp->direction() == VDirection::ਨਿਵੇਸ਼) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
            nodep->user1(ਸੱਚ);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstMemberSel* nodep) override {
        ਜੇ (Ast_Clocking_ਵਸਤੂ* ਅਚਲ itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->ਪਹਿਲਾ_Abovep(),
                Clocking_ਵਸਤੂ)) {
            ਜੇ (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            ਜੇ (nodep->access().isWriteOrRW()) {
                ਜੇ (itemp->direction() == VDirection::OUTPUT) {
                    ਜੇ (!m_ਵਿੱਚ_ਸੌਂਪੋ_Dly_Lhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    ਜੇ (m_ਵਿੱਚ_ਸੌਂਪੋ) {
                        m_ਵਿੱਚ_Synch_Drive = ਸੱਚ;
                        m_drives.ਧੱਕੋ_ਪਿੱਠ({itemp, nodep});
                    }
                } ਨਹੀਂਤਾਂ ਜੇ (itemp->direction() == VDirection::ਨਿਵੇਸ਼) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) override {
        ਜੇ (nodep->user1()) ਵਾਪਸ;
        VL_RESTORER(m_ਵਿੱਚ_ਸੌਂਪੋ);
        VL_RESTORER(m_ਵਿੱਚ_Synch_Drive);
        VL_RESTORER(m_ਵਿੱਚ_Concat_ਸੌਂਪੋ);
        VL_RESTORER_CLEAR(m_drives);
        m_ਵਿੱਚ_ਸੌਂਪੋ = ਸੱਚ;
        m_ਵਿੱਚ_Synch_Drive = ਝੂਠ;
        {
            // The braces of a concatenation are a replication, by one
            ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp = nodep->lhsp();
            ਜੇ (ਅਚਲ AstReplicate* ਅਚਲ repp = VN_CAST(lhsp, Replicate)) lhsp = repp->srcp();
            m_ਵਿੱਚ_Concat_ਸੌਂਪੋ = VN_IS(lhsp, Concat);
        }
        {
            VL_RESTORER(m_ਵਿੱਚ_ਸੌਂਪੋ_Dly_Lhs);
            m_ਵਿੱਚ_ਸੌਂਪੋ_Dly_Lhs = VN_IS(nodep, ਸੌਂਪੋ_Dly);
            iterate(nodep->lhsp());
        }
        iterate(nodep->rhsp());
        ਜੇ (nodep->timingControlp()) iterate(nodep->timingControlp());
        // A drive without a cycle delay, also after removing '##0', updates the clockvar now
        ਜੇ (m_ਵਿੱਚ_Synch_Drive && !nodep->timingControlp()) {
            Ast_ਸੌਂਪੋ* ਅਚਲ assignp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
                nodep->fileline(), nodep->lhsp()->unlink_Fr_ਪਿੱਠ(), nodep->rhsp()->unlink_Fr_ਪਿੱਠ()};
            assignp->user1(ਸੱਚ);
            nodep->replaceWith(assignp);
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
            nodep = assignp;
        }
        // Note the drives when they take effect, after the cycle delay if any. Each clockvar
        // of the LHS is driven, e.g. in a concatenation which other simulators accept, although
        // IEEE 1800-2023 14.16 does not allow it
        Ast_ਗੰਢ* setsp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (ਅਚਲ SynchDrive& drive : m_drives) {
            setsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(setsp, newDrivenSetp(nodep->fileline(), drive));
        }
        ਜੇ (!setsp) ਵਾਪਸ;
        ਜੇ (AstBegin* ਅਚਲ beginp = VN_CAST(nodep->timingControlp(), Begin)) {
            beginp->ਜੋੜੋ_Stmtsp(setsp);
        } ਨਹੀਂਤਾਂ {
            nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(setsp);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstAlways* nodep) override {
        iterate_And_ਅਗਲਾ_Null(nodep->sentreep());
        ਜੇ (nodep->sentreep()) m_seniAlwaysp = nodep->sentreep()->sensesp();
        iterate_And_ਅਗਲਾ_Null(nodep->stmtsp());
        m_seniAlwaysp = ਖਾਲੀ_ਸੂਚਕ;
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Cover_Or_Assert* nodep) override {
        ਜੇ (nodep->sentreep()) ਵਾਪਸ;  // Already processed

        VL_RESTORER(m_senip);
        VL_RESTORER(m_disablep);
        m_senip = ਖਾਲੀ_ਸੂਚਕ;
        m_disablep = ਖਾਲੀ_ਸੂਚਕ;

        // Find Clocking's buried under nodep->exprsp
        iterateChildren(nodep);
        ਜੇ (!nodep->ਤੁਰੰਤ()) nodep->sentreep(new_Sen_ਰੁੱਖ(nodep, ਖਾਲੀ_ਸੂਚਕ, nodep));
    }
    ਖਾਲੀ ਦੇਖੋ(AstFalling* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp = nodep->exprp()->unlink_Fr_ਪਿੱਠ();
        ਜੇ (exprp->ਚੌੜਾਈ() > 1) exprp = ਨਵਾਂ AstSel{fl, exprp, 0, 1};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ futurep = ਨਵਾਂ AstFuture{fl, exprp, new_Sen_ਰੁੱਖ(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = ਨਵਾਂ AstAnd{fl, exprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), ਨਵਾਂ AstNot{fl, futurep}};
        exprp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstFell* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp = nodep->exprp()->unlink_Fr_ਪਿੱਠ();
        ਜੇ (exprp->ਚੌੜਾਈ() > 1) exprp = ਨਵਾਂ AstSel{fl, exprp, 0, 1};
        Ast_Sen_ਰੁੱਖ* sentreep = nodep->sentreep();
        ਜੇ (sentreep) sentreep->unlink_Fr_ਪਿੱਠ();
        AstPast* ਅਚਲ pastp = ਨਵਾਂ AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_ਰੁੱਖ(nodep, sentreep));
        exprp = ਨਵਾਂ AstAnd{fl, pastp, ਨਵਾਂ AstNot{fl, exprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)}};
        exprp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstFuture* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        Ast_Sen_ਰੁੱਖ* ਅਚਲ sentreep = nodep->sentreep();
        ਜੇ (sentreep) VL_DO_DANGLING(ਧੱਕੋ_Deletep(sentreep->unlink_Fr_ਪਿੱਠ()), sentreep);
        nodep->sentreep(new_Sen_ਰੁੱਖ(nodep));
    }
    ਖਾਲੀ ਦੇਖੋ(AstPast* nodep) override {
        checkSamplingFuncDType(nodep, nodep->exprp());
        ਜੇ (nodep->sentreep()) ਵਾਪਸ;  // Already processed
        iterateChildren(nodep);
        nodep->sentreep(new_Sen_ਰੁੱਖ(nodep));
    }
    ਖਾਲੀ ਦੇਖੋ(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- Lower standalone exact [*N] (N >= 2) via saturating counter.
        // Range/unbounded forms and SExpr-contained forms are lowered by V3AssertNfa.
        iterateChildren(nodep);
        // V3AssertNfa handles unbounded/ranged forms upstream, so this fast-path
        // is effectively unreachable when NFA is enabled.
        ਜੇ (nodep->unbounded() || nodep->maxCountp()) ਵਾਪਸ;  // LCOV_EXCL_LINE
        ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(nodep->countp(), Const);
        ਜੇ (VL_UNLIKELY(!constp || constp->ਮੰਜ਼ਿਲ_SInt() < 1)) {
            nodep->v3fatalSrc("Consecutive repetition count must be a positive constant"
                              " (should have been caught by V3Width)");
            ਵਾਪਸ;
        }
        ਅਚਲ ਪੂਰਨ n = constp->ਮੰਜ਼ਿਲ_SInt();
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = nodep->exprp()->unlink_Fr_ਪਿੱਠ();
        ਜੇ (n == 1) {
            nodep->replaceWith(exprp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ;
        }
        // Saturating counter: if (expr) cnt <= min(cnt+1, N); else cnt <= 0;
        AstVar* ਅਚਲ cntVarp = ਨਵਾਂ AstVar{flp, VVarType::MODULETEMP, m_cons_Rep_ਨਾਂ.get(""),
                                           nodep->ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->ਜੋੜੋ_Stmtsp(cntVarp);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਵਿਅੰਜਕ_Clonep = exprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ saturatingIncrp = ਨਵਾਂ AstCond{
            flp,
            ਨਵਾਂ AstLt{flp, ਨਵਾਂ AstVarRef{flp, cntVarp, VAccess::READ},
                      ਨਵਾਂ AstConst{flp, static_cast<uint32_t>(n)}},
            ਨਵਾਂ Ast_ਜੋੜੋ{flp, ਨਵਾਂ AstVarRef{flp, cntVarp, VAccess::READ}, ਨਵਾਂ AstConst{flp, 1u}},
            ਨਵਾਂ AstConst{flp, static_cast<uint32_t>(n)}};
        Ast_ਸੌਂਪੋ_Dly* ਅਚਲ incrAssignp
            = ਨਵਾਂ Ast_ਸੌਂਪੋ_Dly{flp, ਨਵਾਂ AstVarRef{flp, cntVarp, VAccess::WRITE}, saturatingIncrp};
        Ast_ਸੌਂਪੋ_Dly* ਅਚਲ resetAssignp = ਨਵਾਂ Ast_ਸੌਂਪੋ_Dly{
            flp, ਨਵਾਂ AstVarRef{flp, cntVarp, VAccess::WRITE}, ਨਵਾਂ AstConst{flp, 0u}};
        AstIf* ਅਚਲ ifp = ਨਵਾਂ AstIf{flp, ਵਿਅੰਜਕ_Clonep, incrAssignp, resetAssignp};
        Ast_Sen_ਰੁੱਖ* ਅਚਲ senTreep = new_Sen_ਰੁੱਖ(nodep);
        AstAlways* ਅਚਲ alwaysp = ਨਵਾਂ AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, ifp};
        cntVarp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(alwaysp);
        // Match: cnt >= N-1 (previous cycles via NBA) && expr (current cycle)
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ cntCheckp = ਨਵਾਂ AstGte{flp, ਨਵਾਂ AstVarRef{flp, cntVarp, VAccess::READ},
                                                  ਨਵਾਂ AstConst{flp, static_cast<uint32_t>(n - 1)}};
        cntCheckp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ matchp = ਨਵਾਂ AstAnd{flp, cntCheckp, exprp};
        matchp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        nodep->replaceWith(matchp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstRising* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp = nodep->exprp()->unlink_Fr_ਪਿੱਠ();
        ਜੇ (exprp->ਚੌੜਾਈ() > 1) exprp = ਨਵਾਂ AstSel{fl, exprp, 0, 1};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ futurep = ਨਵਾਂ AstFuture{fl, exprp, new_Sen_ਰੁੱਖ(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = ਨਵਾਂ AstAnd{fl, ਨਵਾਂ AstNot{fl, exprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)}, futurep};
        exprp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstRose* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp = nodep->exprp()->unlink_Fr_ਪਿੱਠ();
        ਜੇ (exprp->ਚੌੜਾਈ() > 1) exprp = ਨਵਾਂ AstSel{fl, exprp, 0, 1};
        Ast_Sen_ਰੁੱਖ* sentreep = nodep->sentreep();
        ਜੇ (sentreep) sentreep->unlink_Fr_ਪਿੱਠ();
        AstPast* ਅਚਲ pastp = ਨਵਾਂ AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_ਰੁੱਖ(nodep, sentreep));
        exprp = ਨਵਾਂ AstAnd{fl, ਨਵਾਂ AstNot{fl, pastp}, exprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
        exprp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਸਥਿਰ AstAssocArrayDType* getProcessAssocArrayType(File_ਸਤਰ* ਅਚਲ flp) {
        // Type of __VassertsActive___x[std::process]
        Ast_ਗੰਢ_DType* valp
            = v3Global.rootp()->typeTablep()->ਲੱਭੋ_Basic_DType(flp, VBasicDTypeKwd::ਦੋਆਧਾਰੀ_ਅੰਕ);
        AstClassRefDType* keyp
            = ਨਵਾਂ AstClassRefDType{flp, v3Global.rootp()->stdPackageProcessp(), ਖਾਲੀ_ਸੂਚਕ};
        keyp->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        v3Global.rootp()->typeTablep()->ਜੋੜੋ_Typesp(keyp);
        AstAssocArrayDType* ਅਚਲ typep = ਨਵਾਂ AstAssocArrayDType{flp, valp, keyp};
        typep->dtypep(typep);
        v3Global.rootp()->typeTablep()->ਜੋੜੋ_Typesp(typep);
        ਵਾਪਸ typep;
    }
    ਸਥਿਰ Ast_Stmt_ਵਿਅੰਜਕ* getProcessAssocArrayDelete(AstVarRef* ਅਚਲ refp) {
        // Constructs refp.delete(std::process::self()) statement
        File_ਸਤਰ* ਅਚਲ flp = refp->fileline();
        AstCMethodHard* ਅਚਲ deletep = ਨਵਾਂ AstCMethodHard{
            flp, refp, VCMethod::ASSOC_ERASE, v3Global.rootp()->stdPackageProcessSelfp(flp)};
        deletep->dtypep(refp->ਲੱਭੋ_Void_DType());
        ਵਾਪਸ ਨਵਾਂ Ast_Stmt_ਵਿਅੰਜਕ{flp, deletep};
    }
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* get_Process_Assoc_Array_ਮਾਪ(AstVarRef* ਅਚਲ refp) {
        // Constructs refp.size() statement
        AstCMethodHard* ਅਚਲ sizep
            = ਨਵਾਂ AstCMethodHard{refp->fileline(), refp, VCMethod::ASSOC_ਮਾਪ};
        sizep->dtypep(refp->ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::UINT32));
        ਵਾਪਸ sizep;
    }
    ਖਾਲੀ ਦੇਖੋ(AstSEventually* nodep) override {
        UASSERT(v3Global.rootp()->stdPackagep(), "Should be imported");

        Ast_Sen_ਰੁੱਖ* ਅਚਲ sentreep = new_Sen_ਰੁੱਖ(nodep);
        ਜੇ (!sentreep->sensesp()) {
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(sentreep), sentreep);
            nodep->replaceWith(ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}});
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ;
        }

        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();

        // Track active assertions
        AstVar* ਅਚਲ activep = ਨਵਾਂ AstVar{flp, VVarType::MODULETEMP, m_ਸਰਗਰਮ_ਨਾਂ.get(""),
                                           getProcessAssocArrayType(flp)};
        activep->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->ਜੋੜੋ_Stmtsp(activep);

        // Assertion condition check
        AstLoop* ਅਚਲ loopp = ਨਵਾਂ AstLoop{flp};
        AstSampled* ਅਚਲ condp
            = ਨਵਾਂ AstSampled{flp, nodep->exprp()->unlink_Fr_ਪਿੱਠ(), ਖਾਲੀ_ਸੂਚਕ, ਸੱਚ};
        loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstLoopTest{flp, loopp, ਨਵਾਂ AstLogNot{flp, condp}});
        loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstEventControl{flp, sentreep, ਖਾਲੀ_ਸੂਚਕ});

        // Add assertion to the active set
        AstAssocSel* ਅਚਲ selp = ਨਵਾਂ AstAssocSel{flp, ਨਵਾਂ AstVarRef{flp, activep, VAccess::WRITE},
                                                  v3Global.rootp()->stdPackageProcessSelfp(flp)};
        Ast_ਸੌਂਪੋ* ਅਚਲ incrementp = ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, selp, ਨਵਾਂ AstConst{flp, 1}};
        AstPExprClause* ਅਚਲ clausep = ਨਵਾਂ AstPExprClause{flp};
        Ast_Stmt_ਵਿਅੰਜਕ* ਅਚਲ deletep
            = getProcessAssocArrayDelete(ਨਵਾਂ AstVarRef{flp, activep, VAccess::WRITE});

        // Main assertion block
        AstBegin* ਅਚਲ bodyp = ਨਵਾਂ AstBegin{flp, m_ਖੰਡ_ਨਾਂ.get(""), ਖਾਲੀ_ਸੂਚਕ, ਸੱਚ};
        bodyp->ਜੋੜੋ_Stmtsp(incrementp);
        bodyp->ਜੋੜੋ_Stmtsp(loopp);
        bodyp->ਜੋੜੋ_Stmtsp(clausep);
        bodyp->ਜੋੜੋ_Stmtsp(deletep);

        // Validate assertion condition for each active assert
        AstVar* ਅਚਲ ਸਰਗਰਮ_Countp = ਨਵਾਂ AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                nodep->ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::UINT32)};
        ਸਰਗਰਮ_Countp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        Ast_ਸੌਂਪੋ* ਅਚਲ init_ਸਰਗਰਮ_Countp
            = ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, ਸਰਗਰਮ_Countp, VAccess::WRITE},
                            get_Process_Assoc_Array_ਮਾਪ(ਨਵਾਂ AstVarRef{flp, activep, VAccess::READ})};
        AstLoop* ਅਚਲ finalLoopp = ਨਵਾਂ AstLoop{flp};
        AstIf* ਅਚਲ finalBodypCondp
            = ਨਵਾਂ AstIf{flp, condp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), ਨਵਾਂ AstPExprClause{flp},
                        ਨਵਾਂ AstPExprClause{flp, ਝੂਠ}};
        finalLoopp->ਜੋੜੋ_Stmtsp(
            ਨਵਾਂ AstLoopTest{flp, finalLoopp,
                            ਨਵਾਂ AstNeq{flp, ਨਵਾਂ AstVarRef{flp, ਸਰਗਰਮ_Countp, VAccess::READ},
                                       ਨਵਾਂ AstConst{flp, 0}}});
        finalLoopp->ਜੋੜੋ_Stmtsp(finalBodypCondp);
        finalLoopp->ਜੋੜੋ_Stmtsp(
            ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, ਸਰਗਰਮ_Countp, VAccess::WRITE},
                          ਨਵਾਂ AstSub{flp, ਨਵਾਂ AstVarRef{flp, ਸਰਗਰਮ_Countp, VAccess::READ},
                                     ਨਵਾਂ AstConst{flp, 1}}});

        // Final assertion block
        AstBegin* ਅਚਲ finalp = ਨਵਾਂ AstBegin{flp, m_ਖੰਡ_ਨਾਂ.get(""), ਖਾਲੀ_ਸੂਚਕ, ਸੱਚ};
        finalp->ਜੋੜੋ_Stmtsp(ਸਰਗਰਮ_Countp);
        finalp->ਜੋੜੋ_Stmtsp(init_ਸਰਗਰਮ_Countp);
        finalp->ਜੋੜੋ_Stmtsp(finalLoopp);

        VL_RESTORER(m_pexprp);
        m_pexprp = ਨਵਾਂ AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
        VL_RESTORER(m_has_ਚੱਕਰ_Delay);
        m_has_ਚੱਕਰ_Delay = ਝੂਠ;
        iterate(bodyp);
        iterate(finalp);

        ਜੇ (m_has_ਚੱਕਰ_Delay) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in s_eventually");
            nodep->replaceWith(ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}});
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            VL_DO_DANGLING(m_pexprp->delete_ਰੁੱਖ(), m_pexprp);
            ਵਾਪਸ;
        }

        nodep->replaceWith(m_pexprp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstStable* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp = nodep->exprp()->unlink_Fr_ਪਿੱਠ();
        Ast_Sen_ਰੁੱਖ* sentreep = nodep->sentreep();
        ਜੇ (sentreep) sentreep->unlink_Fr_ਪਿੱਠ();
        AstPast* ਅਚਲ pastp = ਨਵਾਂ AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_ਰੁੱਖ(nodep, sentreep));
        exprp = ਨਵਾਂ AstEq{fl, pastp, exprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
        exprp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstSteady* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp = nodep->exprp()->unlink_Fr_ਪਿੱਠ();
        ਜੇ (exprp->ਚੌੜਾਈ() > 1) exprp = ਨਵਾਂ AstSel{fl, exprp, 0, 1};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ futurep = ਨਵਾਂ AstFuture{fl, exprp, new_Sen_ਰੁੱਖ(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = ਨਵਾਂ AstEq{fl, exprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), futurep};
        exprp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstSampled* nodep) override {
        iterateChildren(nodep);
        ਜੇ (!nodep->ਅੰਦਰੂਨੀ_2()) checkSamplingFuncDType(nodep, nodep->exprp());
    }

    // Validate repetition count: must be a non-negative elaboration-time constant.
    // Shared by goto [->N] and nonconsecutive [=N] repetition.
    // On error, replaces nodep with BitFalse placeholder and returns nullptr.
    ਅਚਲ AstConst* validate_Rep_ਗਿਣਤੀ(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ_ਵਿਅੰਜਕ*& countp) {
        countp = V3Const::constifyEdit(countp);
        ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(countp, Const);
        ਜੇ (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(countp), countp);
            nodep->replaceWith(ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False_Erroring{}});
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }
        ਜੇ (constp->ਮੰਜ਼ਿਲ_SInt() < 0) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(countp), countp);
            nodep->replaceWith(ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False_Erroring{}});
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }
        ਜੇ (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(countp), countp);
            nodep->replaceWith(ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}});
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }
        ਵਾਪਸ constp;
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
    AstPExpr* createRepPExpr(File_ਸਤਰ* flp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* countp,
                             Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp, ਤਾਰਕਿਕ isOverlapped, ਤਾਰਕਿਕ isNonConsec) {
        Ast_Sen_ਵਸਤੂ* ਅਚਲ sensesp = m_senip;
        UASSERT_OBJ(sensesp, exprp, "Repetition requires a clock");

        ਅਚਲ std::string name
            = isNonConsec ? m_non_Cons_Rep_ਨਾਂ.get(exprp) : m_goto_Rep_ਨਾਂ.get(exprp);
        AstVar* ਅਚਲ cntVarp = ਨਵਾਂ AstVar{flp, VVarType::BLOCKTEMP, name + "__counter",
                                           exprp->ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstBegin* ਅਚਲ beginp = ਨਵਾਂ AstBegin{flp, name + "__block", cntVarp, ਸੱਚ};
        beginp->ਜੋੜੋ_Stmtsp(
            ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, cntVarp, VAccess::WRITE}, ਨਵਾਂ AstConst{flp, 0}});

        // Counting loop: find N occurrences of expr (shared by goto and nonconsec)
        AstLoop* ਅਚਲ loopp = ਨਵਾਂ AstLoop{flp};
        loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstLoopTest{flp, loopp,
                                         ਨਵਾਂ AstLt{flp, ਨਵਾਂ AstVarRef{flp, cntVarp, VAccess::READ},
                                                   countp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)}});
        // if (expr) cnt++ -- sampled is applied to whole property expr by V3Assert
        loopp->ਜੋੜੋ_Stmtsp(
            ਨਵਾਂ AstIf{flp, exprp,
                      ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, cntVarp, VAccess::WRITE},
                                    ਨਵਾਂ Ast_ਜੋੜੋ{flp, ਨਵਾਂ AstVarRef{flp, cntVarp, VAccess::READ},
                                               ਨਵਾਂ AstConst{flp, 1}}}});
        loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstIf{
            flp, ਨਵਾਂ AstLt{flp, ਨਵਾਂ AstVarRef{flp, cntVarp, VAccess::READ}, countp},
            ਨਵਾਂ AstEventControl{flp, ਨਵਾਂ Ast_Sen_ਰੁੱਖ{flp, sensesp->clone_ਰੁੱਖ(ਝੂਠ)}, ਖਾਲੀ_ਸੂਚਕ}});

        beginp->ਜੋੜੋ_Stmtsp(loopp);

        ਜੇ (isNonConsec && rhsp) {
            // IEEE 16.9.2: b[=N] = b[->N] ##1 !b[*0:$]
            // Trailing window: ##1 advance, then scan !expr cycles checking rhs.
            // Window closes when expr becomes true again (fail if rhs was never true).
            beginp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstEventControl{
                flp, ਨਵਾਂ Ast_Sen_ਰੁੱਖ{flp, sensesp->clone_ਰੁੱਖ(ਝੂਠ)}, ਖਾਲੀ_ਸੂਚਕ});  // ##1
            ਜੇ (!isOverlapped) {
                beginp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstEventControl{
                    flp, ਨਵਾਂ Ast_Sen_ਰੁੱਖ{flp, sensesp->clone_ਰੁੱਖ(ਝੂਠ)}, ਖਾਲੀ_ਸੂਚਕ});  // |=>
            }
            // Window loop: check rhs at each !expr cycle (done variable for termination)
            AstVar* ਅਚਲ ਪੂਰਾ_Varp
                = ਨਵਾਂ AstVar{flp, VVarType::BLOCKTEMP, name + "__done", exprp->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType()};
            ਪੂਰਾ_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->ਜੋੜੋ_Stmtsp(ਪੂਰਾ_Varp);
            beginp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, ਪੂਰਾ_Varp, VAccess::WRITE},
                                            ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}}});
            ਸਵੈਚਾਲਿਤ ਸੈੱਟ_ਪੂਰਾ = [&]() {
                ਵਾਪਸ ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, ਪੂਰਾ_Varp, VAccess::WRITE},
                                     ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}}};
            };
            AstLoop* ਅਚਲ windowp = ਨਵਾਂ AstLoop{flp};
            // LoopTest: continue while !done
            windowp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstLoopTest{
                flp, windowp, ਨਵਾਂ AstNot{flp, ਨਵਾਂ AstVarRef{flp, ਪੂਰਾ_Varp, VAccess::READ}}});
            // if (expr) { fail; done = 1; } -- window closed, expr true again
            Ast_ਗੰਢ* ਅਚਲ failsp = ਨਵਾਂ AstPExprClause{flp, ਝੂਠ};
            failsp->ਜੋੜੋ_ਅਗਲਾ(ਸੈੱਟ_ਪੂਰਾ());
            windowp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstIf{flp, exprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), failsp});
            // if (rhs) { pass; done = 1; } -- consequent true at this !expr endpoint
            Ast_ਗੰਢ* ਅਚਲ passsp = ਨਵਾਂ AstPExprClause{flp, ਸੱਚ};
            passsp->ਜੋੜੋ_ਅਗਲਾ(ਸੈੱਟ_ਪੂਰਾ());
            windowp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstIf{flp, rhsp, passsp});
            // @(clk) -- advance to next cycle in window
            windowp->ਜੋੜੋ_Stmtsp(
                ਨਵਾਂ AstEventControl{flp, ਨਵਾਂ Ast_Sen_ਰੁੱਖ{flp, sensesp->clone_ਰੁੱਖ(ਝੂਠ)}, ਖਾਲੀ_ਸੂਚਕ});
            beginp->ਜੋੜੋ_Stmtsp(windowp);
        } ਨਹੀਂਤਾਂ ਜੇ (isNonConsec) {
            // Standalone nonconsec: ##1 into window, then pass
            beginp->ਜੋੜੋ_Stmtsp(
                ਨਵਾਂ AstEventControl{flp, ਨਵਾਂ Ast_Sen_ਰੁੱਖ{flp, sensesp->clone_ਰੁੱਖ(ਝੂਠ)}, ਖਾਲੀ_ਸੂਚਕ});
            beginp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstPExprClause{flp, ਸੱਚ});
        } ਨਹੀਂਤਾਂ ਜੇ (rhsp) {
            // Goto rep: check consequent once at match endpoint
            ਜੇ (!isOverlapped) {
                beginp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstEventControl{
                    flp, ਨਵਾਂ Ast_Sen_ਰੁੱਖ{flp, sensesp->clone_ਰੁੱਖ(ਝੂਠ)}, ਖਾਲੀ_ਸੂਚਕ});
            }
            beginp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstIf{flp, rhsp, ਨਵਾਂ AstPExprClause{flp, ਸੱਚ},
                                        ਨਵਾਂ AstPExprClause{flp, ਝੂਠ}});
        } ਨਹੀਂਤਾਂ {
            // Standalone goto: pass after counting
            beginp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstPExprClause{flp, ਸੱਚ});
        }

        ਵਾਪਸ ਨਵਾਂ AstPExpr{flp, beginp, exprp->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType()};
    }

    // Reject [=M:N] nonconsecutive range that reached V3AssertPre.
    // [->M:N] is fully handled upstream (NFA path); only [=M:N] still lands
    // here because AstSNonConsRep lowering is not yet implemented there.
    // Replaces the node with BitFalse so lowering continues after the warning.
    ਤਾਰਕਿਕ rejectNonconsecRange(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* maxCountp) {
        ਜੇ (!maxCountp) ਵਾਪਸ ਝੂਠ;
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: [=M:N] nonconsecutive range repetition"
                                     " (IEEE 1800-2023 16.9.2)");
        nodep->replaceWith(ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}});
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        ਵਾਪਸ ਸੱਚ;
    }

    ਖਾਲੀ ਦੇਖੋ(AstSGotoRep* nodep) override {
        // Standalone goto rep (not inside implication antecedent)
        iterateChildren(nodep);
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* countp = nodep->countp()->unlink_Fr_ਪਿੱਠ();
        ਜੇ (!validate_Rep_ਗਿਣਤੀ(nodep, countp)) ਵਾਪਸ;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = nodep->exprp()->unlink_Fr_ਪਿੱਠ();
        AstPExpr* ਅਚਲ pexprp = createRepPExpr(flp, exprp, countp, ਖਾਲੀ_ਸੂਚਕ, ਸੱਚ, ਝੂਠ);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstSNonConsRep* nodep) override {
        // Standalone nonconsecutive rep (not inside implication antecedent)
        iterateChildren(nodep);
        ਜੇ (rejectNonconsecRange(nodep, nodep->maxCountp())) ਵਾਪਸ;
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* countp = nodep->countp()->unlink_Fr_ਪਿੱਠ();
        ਜੇ (!validate_Rep_ਗਿਣਤੀ(nodep, countp)) ਵਾਪਸ;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = nodep->exprp()->unlink_Fr_ਪਿੱਠ();
        AstPExpr* ਅਚਲ pexprp = createRepPExpr(flp, exprp, countp, ਖਾਲੀ_ਸੂਚਕ, ਸੱਚ, ਸੱਚ);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstFuncRef* nodep) override {
        // IEEE 1800-2023 16.8: Inline sequence instances wherever they appear
        // in the expression tree (inside implications, boolean ops, nested refs, etc.)
        ਜੇ (AstSequence* ਅਚਲ seqp = VN_CAST(nodep->taskp(), Sequence)) {
            substituteSequenceCall(nodep, seqp);
            // The FuncRef has been replaced; do not access nodep after this point.
            // The replacement node will be visited by the parent's iterateChildren.
            ਵਾਪਸ;
        }
        ਜੇ (ਅਚਲ AstProperty* ਅਚਲ propp = VN_CAST(nodep->taskp(), Property)) {
            substituteNestedPropertyCall(nodep, propp);
            ਵਾਪਸ;
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstImplication* nodep) override {
        ਜੇ (nodep->sentreep()) ਵਾਪਸ;  // Already processed

        // Top-level followed-by is claimed by V3AssertNfa; anything reaching here
        // is nested inside iff/implies/or. IEEE 1800-2023 16.12.9 permits the
        // composition, but silent lowering would drop non-vacuous-fail semantics.
        ਜੇ (nodep->isFollowedBy()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: followed-by (#-# / #=#) nested inside property operator"
                          " (iff/implies/or) (IEEE 1800-2023 16.12.9)");
            nodep->replaceWith(ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}});
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ;
        }

        // Handle goto repetition as antecedent before iterateChildren,
        // so the standalone AstSGotoRep visitor doesn't process it
        ਜੇ (AstSGotoRep* ਅਚਲ gotop = VN_CAST(nodep->lhsp(), SGotoRep)) {
            iterateChildren(gotop);
            iterate_And_ਅਗਲਾ_Null(nodep->rhsp());
            File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* countp = gotop->countp()->unlink_Fr_ਪਿੱਠ();
            ਜੇ (!validate_Rep_ਗਿਣਤੀ(nodep, countp)) ਵਾਪਸ;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = gotop->exprp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
            AstPExpr* ਅਚਲ pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), ਝੂਠ);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ;
        }

        // Handle nonconsecutive repetition as antecedent before iterateChildren,
        // so the standalone AstSNonConsRep visitor doesn't process it
        ਜੇ (AstSNonConsRep* ਅਚਲ ncrp = VN_CAST(nodep->lhsp(), SNonConsRep)) {
            iterateChildren(ncrp);
            iterate_And_ਅਗਲਾ_Null(nodep->rhsp());
            ਜੇ (rejectNonconsecRange(nodep, ncrp->maxCountp())) ਵਾਪਸ;
            File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* countp = ncrp->countp()->unlink_Fr_ਪਿੱਠ();
            ਜੇ (!validate_Rep_ਗਿਣਤੀ(nodep, countp)) ਵਾਪਸ;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = ncrp->exprp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
            AstPExpr* ਅਚਲ pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), ਸੱਚ);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ;
        }

        iterateChildren(nodep);

        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();

        // Lower sequence match items (IEEE 16.11): (expr, var = val, ...) |-> / |=>
        ਜੇ (Ast_ਵਿਅੰਜਕ_Stmt* ਅਚਲ ਵਿਅੰਜਕ_Stmtp = VN_CAST(lhsp, ਵਿਅੰਜਕ_Stmt)) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ antExprp = ਵਿਅੰਜਕ_Stmtp->resultp()->unlink_Fr_ਪਿੱਠ();

            ਜੇ (nodep->isOverlapped()) {
                // |-> : assign to __Vpropvar via always_comb (continuous).
                // The assign evaluates RHS once; V3Sampled snapshots the
                // result so all consequent refs read the same value.
                Ast_ਗੰਢ* ਮੇਲ_Assignsp = ਖਾਲੀ_ਸੂਚਕ;
                ਲਈ (Ast_ਗੰਢ* stmtp = ਵਿਅੰਜਕ_Stmtp->stmtsp(); stmtp;) {
                    Ast_ਗੰਢ* ਅਚਲ nextp = stmtp->nextp();
                    ਜੇ (Ast_ਸੌਂਪੋ* ਅਚਲ assignp = VN_CAST(stmtp, ਸੌਂਪੋ)) {
                        assignp->unlink_Fr_ਪਿੱਠ();
                        ਮੇਲ_Assignsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ_Null(ਮੇਲ_Assignsp, assignp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                ਜੇ (ਮੇਲ_Assignsp) {
                    AstAlways* ਅਚਲ alwaysp
                        = ਨਵਾਂ AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, ਖਾਲੀ_ਸੂਚਕ, ਮੇਲ_Assignsp};
                    m_modp->ਜੋੜੋ_Stmtsp(alwaysp);
                }
            } ਨਹੀਂਤਾਂ {
                // |=> : assign to __Vpropvar via NBA in a clocked always block.
                // The NBA commits before the next cycle's sampled snapshot,
                // so the consequent (which already references __Vpropvar)
                // sees the captured value.
                Ast_ਗੰਢ* ਮੇਲ_Assignsp = ਖਾਲੀ_ਸੂਚਕ;
                ਲਈ (Ast_ਗੰਢ* stmtp = ਵਿਅੰਜਕ_Stmtp->stmtsp(); stmtp;) {
                    Ast_ਗੰਢ* ਅਚਲ nextp = stmtp->nextp();
                    ਜੇ (Ast_ਸੌਂਪੋ* ਅਚਲ assignp = VN_CAST(stmtp, ਸੌਂਪੋ)) {
                        assignp->unlink_Fr_ਪਿੱਠ();
                        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਸੌਂਪੋ_Lhsp = assignp->lhsp()->unlink_Fr_ਪਿੱਠ();
                        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਸੌਂਪੋ_Rhsp = assignp->rhsp()->unlink_Fr_ਪਿੱਠ();
                        Ast_ਸੌਂਪੋ_Dly* ਅਚਲ dlyp = ਨਵਾਂ Ast_ਸੌਂਪੋ_Dly{flp, ਸੌਂਪੋ_Lhsp, ਸੌਂਪੋ_Rhsp};
                        VL_DO_DANGLING(ਧੱਕੋ_Deletep(assignp), assignp);
                        ਮੇਲ_Assignsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ_Null(ਮੇਲ_Assignsp, dlyp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                ਜੇ (ਮੇਲ_Assignsp) {
                    AstIf* ਅਚਲ condp
                        = ਨਵਾਂ AstIf{flp, antExprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), ਮੇਲ_Assignsp};
                    AstAlways* ਅਚਲ alwaysp
                        = ਨਵਾਂ AstAlways{flp, VAlwaysKwd::ALWAYS, new_Sen_ਰੁੱਖ(nodep), condp};
                    m_modp->ਜੋੜੋ_Stmtsp(alwaysp);
                }
            }
        }

        ਜੇ (AstPExpr* ਅਚਲ pexprp = VN_CAST(rhsp, PExpr)) {
            // Implication with sequence expression on RHS (IEEE 1800-2023 16.11, 16.12.7).
            // The PExpr was lowered from the property expression earlier in this pass.
            // Wrap the PExpr body with the antecedent check so the sequence only
            // starts when the antecedent holds.
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp;
            ਜੇ (nodep->isOverlapped()) {
                // Overlapped implication (|->): check antecedent on same cycle.
                // disable iff is applied at the assertion level, not at the
                // antecedent gate, matching the existing non-PExpr overlapped path.
                condp = lhsp;
            } ਨਹੀਂਤਾਂ {
                // Non-overlapped implication (|=>): check antecedent from previous cycle
                ਜੇ (m_disablep) {
                    lhsp
                        = ਨਵਾਂ AstAnd{flp, ਨਵਾਂ AstNot{flp, m_disablep->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)}, lhsp};
                }
                AstPast* ਅਚਲ pastp
                    = ਨਵਾਂ AstPast{flp, lhsp, ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ, /* propertyTiming */ ਸੱਚ};
                pastp->dtypeFrom(lhsp);
                pastp->sentreep(new_Sen_ਰੁੱਖ(nodep));
                condp = pastp;
            }
            // Wrap existing PExpr body: if (antecedent) { <original body> } else { /* vacuous pass
            // */ }
            // Only the statements are guarded; declarations stay directly in the block. After
            // V3Fork moves the block's statements into a task, V3Task only handles variables
            // that are direct statements of the task.
            AstBegin* ਅਚਲ bodyp = pexprp->bodyp();
            AstIf* ਅਚਲ guardp = ਨਵਾਂ AstIf{flp, condp};
            ਲਈ (Ast_ਗੰਢ* stmtp = bodyp->stmtsp(); stmtp;) {
                Ast_ਗੰਢ* ਅਚਲ nextp = stmtp->nextp();
                ਜੇ (!VN_IS(stmtp, Var)) guardp->ਜੋੜੋ_Thensp(stmtp->unlink_Fr_ਪਿੱਠ());
                stmtp = nextp;
            }
            bodyp->ਜੋੜੋ_Stmtsp(guardp);
            nodep->replaceWith(pexprp);
            // Don't iterate pexprp here -- it was already iterated when created
            // (in visit(AstSExpr*)), so delays and disable iff are already processed.
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->isOverlapped()) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp
                = m_underCover ? static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(ਨਵਾਂ AstLogAnd{flp, lhsp, rhsp})
                               : ਨਵਾਂ AstLogOr{flp, ਨਵਾਂ AstLogNot{flp, lhsp}, rhsp};
            nodep->replaceWith(exprp);
        } ਨਹੀਂਤਾਂ {
            ਜੇ (m_disablep) {
                lhsp = ਨਵਾਂ AstAnd{flp, ਨਵਾਂ AstNot{flp, m_disablep->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)}, lhsp};
            }

            AstPast* ਅਚਲ pastp
                = ਨਵਾਂ AstPast{flp, lhsp, ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ, /* propertyTiming */ ਸੱਚ};
            pastp->dtypeFrom(lhsp);
            pastp->sentreep(new_Sen_ਰੁੱਖ(nodep));
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp
                = m_underCover ? static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(ਨਵਾਂ AstAnd{flp, pastp, rhsp})
                               : ਨਵਾਂ AstOr{flp, ਨਵਾਂ AstNot{flp, pastp}, rhsp};
            exprp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
            nodep->replaceWith(exprp);
        }
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstUntil* nodep) override {
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        UASSERT_OBJ(
            !m_pexprp, nodep,
            "'" << nodep->verilogKwd()
                << "' in complex property expression should have been rejected by V3AssertNfa");
        ਜੇ (nodep->isStrong()
            && (v3Global.opt.timing().is_ਸੈੱਟ_False() || !v3Global.opt.timing().is_ਸੈੱਟ_True())) {
            nodep->v3warn(E_NOTIMING, nodep->verilogKwd() << " requires --timing");
            nodep->replaceWith(ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}});
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ;
        }
        ਜੇ (nodep->isStrong()) {
            // p s_until q / p s_until_with q: q must eventually be true. Until then, p must
            // be true on every sampled tick. For s_until, check q first: when q is true on
            // this tick, p is not required. For s_until_with, p must be true on the q tick too.
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rawLhsp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rawRhsp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
            AstSampled* ਅਚਲ lhsp = ਨਵਾਂ AstSampled{flp, rawLhsp, rawLhsp->dtypep(), ਸੱਚ};
            AstSampled* ਅਚਲ rhsp = ਨਵਾਂ AstSampled{flp, rawRhsp, rawRhsp->dtypep(), ਸੱਚ};
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* finalCondp = rhsp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
            ਜੇ (nodep->isOverlapping()) {
                finalCondp = ਨਵਾਂ AstLogAnd{flp, lhsp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), finalCondp};
            }

            // Track active assertion attempts. Only the count is needed to emit final failures.
            AstVar* ਅਚਲ activep = ਨਵਾਂ AstVar{flp, VVarType::MODULETEMP, m_ਸਰਗਰਮ_ਨਾਂ.get(""),
                                               nodep->ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::UINT32)};
            activep->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->ਜੋੜੋ_Stmtsp(activep);

            AstVar* ਅਚਲ donep
                = ਨਵਾਂ AstVar{flp, VVarType::BLOCKTEMP, "__VassertDone", nodep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType()};
            donep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            ਸਵੈਚਾਲਿਤ ਸੈੱਟ_ਪੂਰਾ = [&]() {
                ਵਾਪਸ ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, donep, VAccess::WRITE},
                                     ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}}};
            };

            AstLoop* ਅਚਲ loopp = ਨਵਾਂ AstLoop{flp};
            Ast_ਸੌਂਪੋ* ਅਚਲ decrementVar = ਨਵਾਂ Ast_ਸੌਂਪੋ{
                flp, ਨਵਾਂ AstVarRef{flp, activep, VAccess::WRITE},
                ਨਵਾਂ AstSub{flp, ਨਵਾਂ AstVarRef{flp, activep, VAccess::READ}, ਨਵਾਂ AstConst{flp, 1}}};
            loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstLoopTest{
                flp, loopp, ਨਵਾਂ AstLogNot{flp, ਨਵਾਂ AstVarRef{flp, donep, VAccess::READ}}});
            {
                Ast_ਗੰਢ* ਅਚਲ passp = ਨਵਾਂ AstPExprClause{flp};
                passp->ਜੋੜੋ_ਅਗਲਾ(decrementVar);
                passp->ਜੋੜੋ_ਅਗਲਾ(ਸੈੱਟ_ਪੂਰਾ());
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* passCondp = rhsp;
                ਜੇ (nodep->isOverlapping()) {
                    passCondp = ਨਵਾਂ AstLogAnd{flp, lhsp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), passCondp};
                }
                loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstIf{flp, passCondp, passp});
            }
            {
                Ast_ਗੰਢ* ਅਚਲ failp = ਨਵਾਂ AstPExprClause{flp, ਝੂਠ};
                failp->ਜੋੜੋ_ਅਗਲਾ(decrementVar->clone_ਰੁੱਖ(ਝੂਠ));
                failp->ਜੋੜੋ_ਅਗਲਾ(ਸੈੱਟ_ਪੂਰਾ());
                loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstIf{
                    flp,
                    ਨਵਾਂ AstLogAnd{flp,
                                  ਨਵਾਂ AstLogNot{flp, ਨਵਾਂ AstVarRef{flp, donep, VAccess::READ}},
                                  ਨਵਾਂ AstLogNot{flp, lhsp}},
                    failp});
            }
            AstDelay* ਅਚਲ delayp = ਨਵਾਂ AstDelay{flp, ਨਵਾਂ AstConst{flp, 1}, ਝੂਠ};
            delayp->timeunit(m_modp->timeunit());
            loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstIf{
                flp, ਨਵਾਂ AstLogNot{flp, ਨਵਾਂ AstVarRef{flp, donep, VAccess::READ}}, delayp});

            // Main assertion block
            AstBegin* ਅਚਲ bodyp = ਨਵਾਂ AstBegin{flp, m_ਖੰਡ_ਨਾਂ.get(""), ਖਾਲੀ_ਸੂਚਕ, ਸੱਚ};
            bodyp->ਜੋੜੋ_Stmtsp(donep);
            bodyp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, donep, VAccess::WRITE},
                                           ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}}});
            File_ਸਤਰ* ਅਚਲ flp = activep->fileline();
            bodyp->ਜੋੜੋ_Stmtsp(
                ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, activep, VAccess::WRITE},
                              ਨਵਾਂ Ast_ਜੋੜੋ{flp, ਨਵਾਂ AstVarRef{flp, activep, VAccess::READ},
                                         ਨਵਾਂ AstConst{flp, 1}}});
            bodyp->ਜੋੜੋ_Stmtsp(loopp);

            // Validate assertion condition for each active assert
            AstVar* ਅਚਲ ਸਰਗਰਮ_Countp = ਨਵਾਂ AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                    nodep->ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::UINT32)};
            ਸਰਗਰਮ_Countp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

            Ast_ਸੌਂਪੋ* ਅਚਲ init_ਸਰਗਰਮ_Countp
                = ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, ਸਰਗਰਮ_Countp, VAccess::WRITE},
                                ਨਵਾਂ AstVarRef{flp, activep, VAccess::READ}};
            AstLoop* ਅਚਲ finalLoopp = ਨਵਾਂ AstLoop{flp};
            finalLoopp->ਜੋੜੋ_Stmtsp(
                ਨਵਾਂ AstLoopTest{flp, finalLoopp,
                                ਨਵਾਂ AstNeq{flp, ਨਵਾਂ AstVarRef{flp, ਸਰਗਰਮ_Countp, VAccess::READ},
                                           ਨਵਾਂ AstConst{flp, 0}}});
            finalLoopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstIf{flp, finalCondp, ਨਵਾਂ AstPExprClause{flp},
                                            ਨਵਾਂ AstPExprClause{flp, ਝੂਠ}});
            finalLoopp->ਜੋੜੋ_Stmtsp(
                ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, ਸਰਗਰਮ_Countp, VAccess::WRITE},
                              ਨਵਾਂ AstSub{flp, ਨਵਾਂ AstVarRef{flp, ਸਰਗਰਮ_Countp, VAccess::READ},
                                         ਨਵਾਂ AstConst{flp, 1}}});

            // Final assertion block
            AstBegin* ਅਚਲ finalp = ਨਵਾਂ AstBegin{flp, m_ਖੰਡ_ਨਾਂ.get(""), ਖਾਲੀ_ਸੂਚਕ, ਸੱਚ};
            finalp->ਜੋੜੋ_Stmtsp(ਸਰਗਰਮ_Countp);
            finalp->ਜੋੜੋ_Stmtsp(init_ਸਰਗਰਮ_Countp);
            finalp->ਜੋੜੋ_Stmtsp(finalLoopp);

            AstPExpr* ਅਚਲ pexprp = ਨਵਾਂ AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
            VL_RESTORER(m_pexprp);
            VL_RESTORER(m_has_ਚੱਕਰ_Delay);
            m_pexprp = pexprp;
            m_has_ਚੱਕਰ_Delay = ਝੂਠ;
            iterate(bodyp);
            iterate(finalp);

            UASSERT_OBJ(!m_has_ਚੱਕਰ_Delay, nodep,
                        "s_until cycle delay should have been handled by V3AssertNfa");

            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ;
        }
        AstLoop* ਅਚਲ loopp = ਨਵਾਂ AstLoop{flp};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
        AstLogAnd* ਅਚਲ loopCondp = ਨਵਾਂ AstLogAnd{flp, lhsp, ਨਵਾਂ AstLogNot{flp, rhsp}};
        loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstLoopTest{flp, loopp, loopCondp});
        loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstEventControl{flp, new_Sen_ਰੁੱਖ(nodep), ਖਾਲੀ_ਸੂਚਕ});

        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsCopyp = rhsp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ passCondp
            = nodep->isOverlapping() ? ਨਵਾਂ AstLogAnd{flp, lhsp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), rhsCopyp}
                                     : rhsCopyp;
        AstBegin* ਅਚਲ beginp = ਨਵਾਂ AstBegin{flp, m_ਖੰਡ_ਨਾਂ.get(""), loopp, ਸੱਚ};
        beginp->ਜੋੜੋ_Stmtsp(
            ਨਵਾਂ AstIf{flp, passCondp, ਨਵਾਂ AstPExprClause{flp}, ਨਵਾਂ AstPExprClause{flp, ਝੂਠ}});

        AstPExpr* ਅਚਲ pexprp = ਨਵਾਂ AstPExpr{flp, beginp, nodep->dtypep()};
        pexprp->user1(1);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstDefaultDisable* nodep) override {
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstInferredDisable* nodep) override {
        Ast_ਗੰਢ* newp;
        ਜੇ (m_defaultDisablep) {
            newp = m_defaultDisablep->condp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ);
        } ਨਹੀਂਤਾਂ {
            newp = ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstPropSpec* nodep) override {
        nodep = substitutePropertyCall(nodep);
        iterate_And_ਅਗਲਾ_Null(nodep->sensesp());
        ਜੇ (m_senip && m_senip != nodep->sensesp())
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Only one PSL clock allowed per assertion");
        ਅਚਲ AstCover* ਅਚਲ coverp = VN_CAST(nodep->backp(), Cover);
        ਅਚਲ ਤਾਰਕਿਕ seqEvent = coverp && coverp->isSeqEvent();
        ਜੇ (!nodep->disablep() && m_defaultDisablep && !seqEvent) {
            nodep->disablep(m_defaultDisablep->condp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ));
        }
        m_disablep = nodep->disablep();
        // Unlink and just keep a pointer to it, convert to sentree as needed
        m_senip = nodep->sensesp();
        iterateNull(nodep->disablep());
        ਜੇ (!VN_AS(nodep->backp(), ਗੰਢ_Cover_Or_Assert)->ਤੁਰੰਤ()) {
            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ propDtp = nodep->propp()->dtypep();
            nodep->propp(ਨਵਾਂ AstSampled{nodep->fileline(), nodep->propp()->unlink_Fr_ਪਿੱਠ(),
                                        propDtp->dtypep(), ਸੱਚ});
        }
        // cover counts non-vacuous matches only (IEEE 1800-2023 16.15.2), so an
        // implication antecedent must hold; assert passes vacuously instead.
        VL_RESTORER(m_underCover);
        m_underCover = VN_IS(nodep->backp(), Cover);
        iterate(nodep->propp());
        iterateNull(nodep->ਮੇਲ_Countp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstPExpr* nodep) override {
        // V3AssertNfa handles multi-cycle property expressions before this pass,
        // so the following unsupported paths are defensive and typically unreached.
        ਜੇ (m_pexprp && m_pexprp->user1()) {  // LCOV_EXCL_START
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Complex property expression inside 'until''");
            nodep->replaceWith(ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}});
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ;
        }  // LCOV_EXCL_STOP
        ਜੇ (AstLogNot* ਅਚਲ notp = VN_CAST(nodep->backp(), LogNot)) {
            notp->replaceWith(nodep->unlink_Fr_ਪਿੱਠ());
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(notp), notp);
            iterate(nodep);
            ਵਾਪਸ;
        }
        // Sequence expression as antecedent of implication is not yet supported
        ਜੇ (AstImplication* ਅਚਲ implp = VN_CAST(nodep->backp(), Implication)) {
            ਜੇ (implp->lhsp() == nodep) {  // LCOV_EXCL_START
                implp->v3warn(E_UNSUPPORTED,
                              "Unsupported: Implication with sequence expression as antecedent");
                nodep->replaceWith(ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}});
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                ਵਾਪਸ;
            }  // LCOV_EXCL_STOP
        }
        VL_RESTORER(m_pexprp);
        VL_RESTORER(m_disableSeqIfp);
        m_pexprp = nodep;

        ਜੇ (m_disablep) {
            ਅਚਲ AstSampled* sampledp = ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (m_disablep->exists([&sampledp](ਅਚਲ AstSampled* ਅਚਲ sp) {
                    sampledp = sp;
                    ਵਾਪਸ ਸੱਚ;
                })) {
                sampledp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: $sampled inside disabled condition of a sequence");
                m_disablep = ਨਵਾਂ AstConst{m_disablep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}};
                // always a copy is used, so remove it now
                ਧੱਕੋ_Deletep(m_disablep);  // m_disablep used later
            }
            File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
            // Add counter which counts times the condition turned true
            AstVar* ਅਚਲ disableCntp
                = ਨਵਾਂ AstVar{flp, VVarType::MODULETEMP, m_disable_Cnt_ਨਾਂ.get(""),
                             nodep->ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::UINT32)};
            disableCntp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->ਜੋੜੋ_Stmtsp(disableCntp);
            AstVarRef* ਅਚਲ readCntRefp = ਨਵਾਂ AstVarRef{flp, disableCntp, VAccess::READ};
            AstVarRef* ਅਚਲ writeCntRefp = ਨਵਾਂ AstVarRef{flp, disableCntp, VAccess::WRITE};
            Ast_ਸੌਂਪੋ* ਅਚਲ incrStmtp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
                flp, writeCntRefp, ਨਵਾਂ Ast_ਜੋੜੋ{flp, readCntRefp, ਨਵਾਂ AstConst{flp, 1}}};
            AstAlways* ਅਚਲ alwaysp
                = ਨਵਾਂ AstAlways{flp, VAlwaysKwd::ALWAYS,
                                ਨਵਾਂ Ast_Sen_ਰੁੱਖ{flp, ਨਵਾਂ Ast_Sen_ਵਸਤੂ{flp, VEdgeType::ET_POSEDGE,
                                                                   m_disablep->clone_ਰੁੱਖ(ਝੂਠ)}},
                                incrStmtp};
            disableCntp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(alwaysp);

            // Store value of that counter at the beginning of sequence evaluation
            AstBegin* ਅਚਲ bodyp = nodep->bodyp();
            AstVar* ਅਚਲ initialCntp = ਨਵਾਂ AstVar{flp, VVarType::BLOCKTEMP, "__VinitialCnt",
                                                   nodep->ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::UINT32)};
            initialCntp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            Ast_ਸੌਂਪੋ* ਅਚਲ assignp
                = ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, initialCntp, VAccess::WRITE},
                                readCntRefp->clone_ਰੁੱਖ(ਝੂਠ)};
            // Prepend to the sequence body to keep statement list structure valid.
            UASSERT_OBJ(!bodyp->name().ਖਾਲੀ_ਥਾਵਾਂ(), bodyp, "Sequence body should be named");
            Ast_ਗੰਢ* ਅਚਲ newStmtsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ<Ast_ਗੰਢ, Ast_ਗੰਢ>(initialCntp, assignp);
            bodyp->prependStmtsp(newStmtsp);

            m_disableSeqIfp
                = ਨਵਾਂ AstIf{flp, ਨਵਾਂ AstEq{flp, ਨਵਾਂ AstVarRef{flp, initialCntp, VAccess::READ},
                                           readCntRefp->clone_ਰੁੱਖ(ਝੂਠ)}};
            // Delete it, because it is always copied before insetion to the AST
            ਧੱਕੋ_Deletep(m_disableSeqIfp);  // m_disableSeqIfp used later
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultClkEvtVarp);
        VL_RESTORER(m_defaultDisablep);
        VL_RESTORER(m_modp);
        ਜੇ (!VN_IS(nodep, Class)) {
            // A class nested in a module is in the scope of the module's default clocking
            m_defaultClockingp = ਖਾਲੀ_ਸੂਚਕ;
            m_defaultClkEvtVarp = ਖਾਲੀ_ਸੂਚਕ;
        }
        m_defaultDisablep = nodep->defaultDisablep();
        m_modp = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Gen_ਖੰਡ* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstProperty* nodep) override {
        // The body will be visited when will be substituted in place of property reference
        // (AstFuncRef)
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstSequence* nodep) override {
        // Sequence declarations are not visited directly; their bodies are inlined
        // at call sites by visit(AstFuncRef*). Collect for post-traversal cleanup.
        m_seqs_ਮੰਜ਼ਿਲ_Cleanup.ਧੱਕੋ_ਪਿੱਠ(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ AssertPreVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        { AssertPreClockingVisitor{nodep}; }
        // Process
        iterate(nodep);
        // Fix up varref names
        ਲਈ (AstVarXRef* xrefp : m_xrefsp) xrefp->name(xrefp->varp()->name());
        // Clean up sequence declarations after inlining.
        // Referenced sequences that were inlined have isReferenced cleared.
        // Remaining referenced sequences are in unsupported contexts (e.g. @seq event).
        ਲਈ (AstSequence* seqp : m_seqs_ਮੰਜ਼ਿਲ_Cleanup) {
            ਜੇ (seqp->isReferenced()) {
                seqp->v3warn(E_UNSUPPORTED,
                             "Unsupported: sequence referenced outside assertion property");
            } ਨਹੀਂਤਾਂ {
                VL_DO_DANGLING(seqp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), seqp);
            }
        }
    }
    ~AssertPreVisitor() override = ਮੂਲ;
};

//######################################################################
// Top Assert class

ਖਾਲੀ V3AssertPre::assertPreAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { AssertPreVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("assertpre", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
