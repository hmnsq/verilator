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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3AssertPre.h"

#ضمن "V3Assert.h"
#ضمن "V3Const.h"
#ضمن "V3Task.h"
#ضمن "V3Timing.h"
#ضمن "V3UniqueNames.h"

#ضمن <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clocking blocks of clocking items

صنف AssertPreClockingVisitor final : عام VNVisitorConst {
    // Captures the clocking block of each clocking item before AssertPreVisitor, which needs it
    // at synchronous drives, often visited before the clocking block, e.g. in a parent module
    // NODE STATE
    // AstClockingItem::user3p()  // AstClocking*. Clocking block of the item, user3 claimed by
    //                            //               AssertPreVisitor

    // VISITORS
    فراغ لیدنه(AstNetlist* nodep) override { iterate_And_بل_Const_Null(nodep->modulesp()); }
    فراغ لیدنه(Ast_غوټه_Module* nodep) override { iterate_And_بل_Const_Null(nodep->stmtsp()); }
    فراغ لیدنه(Ast_Gen_بلاک* nodep) override { iterate_And_بل_Const_Null(nodep->itemsp()); }
    فراغ لیدنه(AstClocking* nodep) override {
        لكل (Ast_غوټه* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            إذا (Ast_Clocking_توکی* ثابت citemp = VN_CAST(itemp, Clocking_توکی)) {
                citemp->user3p(nodep);
            }
        }
    }
    // Clocking blocks are only in modules and their generate blocks
    فراغ لیدنه(Ast_غوټه*) override {}

عام:
    // CONSTRUCTORS
    څرګند AssertPreClockingVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~AssertPreClockingVisitor() override = افتراضي;
};

//######################################################################
// Assert class functions

صنف AssertPreVisitor final : عام VNVisitor {
    // Removes clocks and other pre-optimizations
    // Eventually inlines calls to sequences, properties, etc.
    // We're not parsing the tree, or anything more complicated.
خاص:
    // TYPES
    بنية SynchDrive final {
        Ast_Clocking_توکی* itemp;  // Clocking item of the driven clockvar
        Ast_غوټه_بیان* refp;  // Reference to the driven clockvar
    };

    // NODE STATE
    // AstClockingItem::user1p()         // AstVar*.      varp() of ClockingItem after unlink
    // AstClockingItem::user2p()         // AstVar*.      Flag set by drives of output clockvar
    // AstClockingItem::user3p()         // AstClocking*. Clocking block of the item, captured
    //                                   //               by AssertPreClockingVisitor
    // AstPExpr::user1()                 // bool.         Created from AstUntil
    ثابت VNUser1_دننه_Use m_inuser1;
    ثابت VNUser2_دننه_Use m_inuser2;
    ثابت VNUser3_دننه_Use m_inuser3;
    // STATE
    // Current context:
    AstNetlist* ثابت m_netlistp = تش_اشاره;  // Current netlist
    Ast_غوټه_Module* m_modp = تش_اشاره;  // Current module
    AstClocking* m_clockingp = تش_اشاره;  // Current clocking block
    // Reset each module:
    AstClocking* m_defaultClockingp = تش_اشاره;  // Default clocking for current module
    AstVar* m_defaultClkEvtVarp = تش_اشاره;  // Event var for default clocking (for ##0)
    AstDefaultDisable* m_defaultDisablep = تش_اشاره;  // Default disable for current module
    // Reset each assertion:
    Ast_Sen_توکی* m_senip = تش_اشاره;  // Last sensitivity
    // Reset each always:
    Ast_Sen_توکی* m_seniAlwaysp = تش_اشاره;  // Last sensitivity in always
    // Reset each assertion:
    Ast_غوټه_بیان* m_disablep = تش_اشاره;  // Last disable
    AstIf* m_disableSeqIfp = تش_اشاره;  // Used for handling disable iff in sequences
    AstPExpr* m_pexprp = تش_اشاره;  // Last AstPExpr
    منطقي m_underCover = خطأ;  // True if the enclosing assertion is a cover
    // Other:
    V3_Unique_نومونه m_دوره_Dly_نومونه{"__VcycleDly"};  // Cycle delay counter name generator
    V3_Unique_نومونه m_cons_Rep_نومونه{"__VconsRep"};  // Consecutive repetition counter name generator
    V3_Unique_نومونه m_goto_Rep_نومونه{"__VgotoRep"};  // Goto repetition counter name generator
    V3_Unique_نومونه m_non_Cons_Rep_نومونه{"__VnonConsRep"};  // Nonconsecutive rep name generator
    V3_Unique_نومونه m_disable_Cnt_نومونه{"__VdisableCnt"};  // Disable condition counter name generator
    V3_Unique_نومونه m_بلاک_نومونه{"__VassertBlock"};  // Names of blocks with temporaries
    V3_Unique_نومونه m_prop_Var_نومونه{"__Vpropvar"};  // Property-local variable name generator
    V3_Unique_نومونه m_فعال_نومونه{"__VassertsActive"};  // Active asserts map name generator
    V3_Unique_نومونه m_driven_نومونه{"__VclockingDriven"};  // Clockvar drive flag name generator
    منطقي m_دننه_ټاکل = خطأ;  // True if in an AssignNode
    منطقي m_دننه_ټاکل_Dly_Lhs = خطأ;  // True if in AssignDly's LHS
    منطقي m_دننه_Synch_Drive = خطأ;  // True if in synchronous drive
    منطقي m_دننه_Concat_ټاکل = خطأ;  // True if in assignment to a concatenation
    std::ویکټور<SynchDrive> m_drives;  // Clockvars written by the synchronous drive
    منطقي m_has_دوره_Delay = خطأ;  // True if node has cycle delay beneath
    std::ویکټور<AstVarXRef*> m_xrefsp;  // list of xrefs that need name fixup
    std::ویکټور<AstSequence*> m_seqs_منزل_Cleanup;  // Sequences to clean up after traversal

    // METHODS

    // Flag set by each drive of an output clockvar, added to the module with its clocking item
    AstVar* getCreateDrivenVarp(Ast_Clocking_توکی* itemp) {
        إذا (!itemp->user2p()) {
            AstVar* ثابت varp = جديد AstVar{itemp->fileline(), VVarType::MODULETEMP,
                                            m_driven_نومونه.get(itemp), itemp->پیدا_کړه_بټ_DType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            itemp->user2p(varp);
        }
        أرجع VN_AS(itemp->user2p(), Var);
    }
    // Reference to a variable in the scope of a driven clockvar, made like the clockvar's
    ساكن Ast_غوټه_بیان* newDriveRefp(File_کرښه* flp, ثابت SynchDrive& drive, AstVar* varp,
                                     ثابت VAccess& access) {
        إذا (ثابت AstVarXRef* ثابت xrefp = VN_CAST(drive.refp, VarXRef)) {
            أرجع جديد AstVarXRef{flp, varp, xrefp->dotted(), access};
        } وإلا إذا (ثابت AstMemberSel* ثابت selp = VN_CAST(drive.refp, MemberSel)) {
            // The interface expression is evaluated again, so it must not have side effects;
            // cloneTreePure warns if it has any
            AstMemberSel* ثابت newSelp
                = جديد AstMemberSel{flp, selp->fromp()->clone_ونه_خالص(خطأ), varp};
            newSelp->access(access);
            أرجع newSelp;
        }
        أرجع جديد AstVarRef{flp, varp, access};
    }
    // Assignment setting the drive flag of a driven clockvar
    Ast_ټاکل* newDrivenSetp(File_کرښه* flp, ثابت SynchDrive& drive) {
        AstVar* ثابت varp = getCreateDrivenVarp(drive.itemp);
        Ast_ټاکل* ثابت setp = جديد Ast_ټاکل{flp, newDriveRefp(flp, drive, varp, VAccess::WRITE),
                                              جديد AstConst{flp, AstConst::بټ_True{}}};
        setp->user1(ریښتیا);
        أرجع setp;
    }
    // Clocking block of a clocking item
    ساكن AstClocking* clocking_د(ثابت Ast_Clocking_توکی* itemp) {
        UASSERT_OBJ(itemp->user3p(), itemp, "Clocking item not captured in a clocking block");
        أرجع VN_AS(itemp->user3p(), Clocking);
    }
    // Clocking event of a driven clockvar, referenced from the drive like the clockvar, or
    // nullptr if unsupported, which is reported on the drive's cycle delay
    Ast_Sen_ونه* newDriveSentreep(AstDelay* delayp, ثابت SynchDrive& drive) {
        Ast_Sen_ونه* ثابت origp = clocking_د(drive.itemp)->sentreep();
        إذا (VN_IS(drive.refp, VarRef)) أرجع origp->clone_ونه(خطأ);
        إذا (origp->exists([](ثابت AstVarXRef*) { أرجع ریښتیا; })) {
            // The reference is relative to the clocking block
            delayp->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in synchronous drive to"
                                          " clockvar in another scope, whose clocking event"
                                          " has a hierarchical reference");
            أرجع تش_اشاره;
        }
        ثابت AstMemberSel* ثابت selp = VN_CAST(drive.refp, MemberSel);
        // Processes outside classes evaluate their events also when not waiting on them,
        // possibly before the interface reference is set, so the event is guarded against a
        // null reference, which needs an integral value; scheduling does not guard it
        ثابت منطقي guardNull = selp && !VN_IS(m_modp, Class);
        إذا (guardNull) {
            لكل (Ast_Sen_توکی* itemp = origp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Sen_توکی)) {
                إذا (!itemp->sensp()->dtypep()->skipRefp()->isIntegralOrPacked()) {
                    delayp->v3warn(E_UNSUPPORTED,
                                   "Unsupported: cycle delay in synchronous drive through"
                                   " an interface reference outside a class, to clocking"
                                   " block with non-integral clocking event");
                    أرجع تش_اشاره;
                }
            }
        }
        // Interface declaring the clocking block, if driven through an interface reference
        AstIface* ثابت ifacep
            = selp ? VN_AS(selp->fromp()->dtypep()->skipRefp(), IfaceRefDType)->iface_له_لارې_Cellp()
                   : تش_اشاره;
        Ast_Sen_ونه* ثابت sentreep = origp->clone_ونه(خطأ);
        std::ویکټور<AstVarRef*> refps;
        sentreep->foreach([&](AstVarRef* refp) {
            إذا (!refp->classOrPackagep()) refps.ټېل_شا(refp);
        });
        لكل (AstVarRef* ثابت refp : refps) {
            AstVar* ثابت varp = refp->varp();
            إذا (ifacep) {
                // Mark the variable as read through an interface reference, as V3Width does for
                // a member select. The mark is the interface declaring the variable, whose
                // instances all share it, so drives through references to any instances agree.
                UASSERT_OBJ(!varp->sensIfacep() || varp->sensIfacep() == ifacep, refp,
                            "Variable read through references to different interfaces");
                varp->sensIfacep(ifacep);
            }
            refp->replaceWith(newDriveRefp(refp->fileline(), drive, varp, refp->access()));
            VL_DO_DANGLING(ټېل_Deletep(refp), refp);
        }
        إذا (guardNull) {
            لكل (Ast_Sen_توکی* itemp = sentreep->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Sen_توکی)) {
                guardNullIface(itemp, selp->fromp());
            }
        }
        أرجع sentreep;
    }
    // Make an integral clocking event through an interface reference false while the reference
    // is null
    ساكن فراغ guardNullIface(Ast_Sen_توکی* sensesp, Ast_غوټه_بیان* fromp) {
        File_کرښه* ثابت flp = sensesp->fileline();
        Ast_غوټه_بیان* ثابت sensp = sensesp->sensp()->unlink_Fr_شا();
        sensesp->sensp(جديد AstCond{flp, newNotNullp(flp, fromp), sensp,
                                   جديد AstConst{flp, AstConst::DTyped{}, sensp->dtypep()}});
        إذا (Ast_غوټه_بیان* ثابت condp = sensesp->condp()) {
            condp->unlink_Fr_شا();
            sensesp->condp(جديد AstLogAnd{flp, newNotNullp(flp, fromp), condp});
        }
    }
    ساكن Ast_غوټه_بیان* newNotNullp(File_کرښه* flp, Ast_غوټه_بیان* fromp) {
        أرجع جديد AstNeq{flp, fromp->clone_ونه_خالص(خطأ), جديد AstConst{flp, AstConst::Null{}}};
    }
    // Procedural '##0': if the default clocking event has not yet occurred in this time step,
    // wait for it; otherwise continue without suspension (IEEE 1800-2023 14.11)
    AstIf* new_دوره_Delay_Zerop(File_کرښه* flp) ثابت {
        AstVar* ثابت evtVarp = m_defaultClkEvtVarp;
        UASSERT_OBJ(evtVarp, flp, "Default clocking event var not pre-created");
        AstCMethodHard* ثابت isTriggeredp = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, evtVarp, VAccess::READ}, VCMethod::EVENT_IS_TRIGGERED};
        isTriggeredp->dtype_سېټ_بټ();
        AstEventControl* ثابت waitp = جديد AstEventControl{
            flp,
            جديد Ast_Sen_ونه{flp, جديد Ast_Sen_توکی{flp, VEdgeType::ET_EVENT,
                                               جديد AstVarRef{flp, evtVarp, VAccess::READ}}},
            تش_اشاره};
        أرجع جديد AstIf{flp, جديد AstNot{flp, isTriggeredp}, waitp};
    }

    ساكن فراغ checkSamplingFuncDType(Ast_غوټه_بیان* nodep, ثابت Ast_غوټه* exprp) {
        ثابت Ast_غوټه_DType* ثابت dtypep = exprp->dtypep()->skipRefp();
        إذا (!dtypep->isIntegralOrPacked()) {
            nodep->v3error("Expected numeric type, but got a " << dtypep->prettyDTypeNameQ()
                                                               << " data type");
        }
    }

    Ast_Sen_ونه* new_Sen_ونه(Ast_غوټه* nodep, Ast_Sen_ونه* useTreep = تش_اشاره,
                           Ast_غوټه_Cover_Or_Assert* cassertp = تش_اشاره) {
        // Create sentree based on clocked or default clock
        // Return nullptr for always
        إذا (useTreep) أرجع useTreep;
        Ast_Sen_ونه* newp = تش_اشاره;
        Ast_Sen_توکی* senip = m_senip;
        منطقي fromAlways = خطأ;
        إذا (!senip && m_defaultClockingp) senip = m_defaultClockingp->sentreep()->sensesp();
        إذا (!senip) {
            senip = m_seniAlwaysp;
            fromAlways = ریښتیا;
        }
        إذا (!senip) {
            nodep->v3error("Concurrent assertion has no clock (IEEE 1800-2023 16.16)\n"
                           << nodep->warn_نور()
                           << "... Suggest provide a clocking event, a default"
                              " clocking, or a clocked procedural context");
            newp = جديد Ast_Sen_ونه{nodep->fileline(), تش_اشاره};
        } وإلا {
            إذا (cassertp && fromAlways) cassertp->senFromAlways(ریښتیا);
            newp = جديد Ast_Sen_ونه{nodep->fileline(), senip->clone_ونه(ریښتیا)};
        }
        أرجع newp;
    }
    Ast_غوټه_بیان* getSequenceBodyExprp(ثابت AstSequence* ثابت seqp) ثابت {
        // The statements in AstSequence are optional AstVar (ports) followed by body expr.
        Ast_غوټه* bodyp = seqp->stmtsp();
        طالما (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        أرجع VN_CAST(bodyp, غوټه_بیان);
    }
    AstPropSpec* getPropertyExprp(ثابت AstProperty* ثابت propp) {
        // Statements in AstProperty: AstVar (ports/local vars),
        // AstInitialStaticStmt/AstInitialAutomaticStmt (var init), AstPropSpec (body).
        Ast_غوټه* propExprp = propp->stmtsp();
        طالما (propExprp
               && (VN_IS(propExprp, Var) || VN_IS(propExprp, InitialStaticStmt)
                   || VN_IS(propExprp, InitialAutomaticStmt))) {
            propExprp = propExprp->nextp();
        }
        أرجع VN_CAST(propExprp, PropSpec);
    }
    فراغ substituteSequenceCall(AstFuncRef* funcrefp, AstSequence* seqp) {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Inline the sequence body at the call site, replacing the FuncRef
        Ast_غوټه_بیان* bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        // Clone the body expression since the sequence may be referenced multiple times
        Ast_غوټه_بیان* clonedp = bodyExprp->clone_ونه(خطأ);
        // Build substitution map, then do a single traversal to replace all formals
        // (textual substitution per IEEE 16.8.2).
        ثابت V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<ثابت AstVar*, Ast_غوټه_بیان*> پورټ_Map;
        لكل (ثابت خپلکاری& tconnect : tconnects) {
            پورټ_Map[tconnect.لومړی] = tconnect.دوهم->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            ثابت خپلکاری it = پورټ_Map.پیدا_کړه(refp->varp());
            إذا (it != پورټ_Map.پای_ټکی()) {
                refp->replaceWith(it->دوهم->clone_ونه(خطأ));
                VL_DO_DANGLING(ټېل_Deletep(refp), refp);
            }
        });
        لكل (ثابت خپلکاری& tconnect : tconnects) {
            ټېل_Deletep(tconnect.دوهم->exprp()->unlink_Fr_شا());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        // Replace the FuncRef with the inlined body
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(ټېل_Deletep(funcrefp), funcrefp);
        // Clear referenced flag; sequences with isReferenced==false are deleted in assertPreAll
        seqp->isReferenced(خطأ);
    }
    AstPropSpec* clonePropertyBody(AstFuncRef* funcrefp, ثابت AstProperty* propp) {
        AstPropSpec* propExprp = getPropertyExprp(propp);
        // Substitute inner property call before copying in order to not doing the same for
        // each call of outer property call.
        propExprp = substitutePropertyCall(propExprp);
        // Clone subtree after substitution. It is needed, because property might be called
        // multiple times with different arguments.
        propExprp = propExprp->clone_ونه(خطأ);
        // Build substitution maps for formal arguments and property-local
        // variables, then perform a single foreach to apply all replacements.
        // Map port vars to their actual argument expressions
        ثابت V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propp->stmtsp());
        std::unordered_map<ثابت AstVar*, Ast_غوټه_بیان*> پورټ_Map;
        لكل (ثابت خپلکاری& tconnect : tconnects) {
            پورټ_Map[tconnect.لومړی] = tconnect.دوهم->exprp();
        }

        // Promote property-local variables (non-port vars, IEEE 16.10) to
        // module-level __Vpropvar temps. Cross-cycle persistence is handled
        // by the match item lowering in visit(AstImplication*).
        std::unordered_map<ثابت AstVar*, AstVar*> localVarMap;
        لكل (Ast_غوټه* stmtp = propp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (!varp->isIO()) {
                    ثابت string newName = m_prop_Var_نومونه.get(varp);
                    AstVar* ثابت newVarp = جديد AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->جمع_کړه_Stmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        // Single traversal: substitute ports and update local var references
        propExprp->foreach([&](AstVarRef* refp) {
            ثابت خپلکاری پورټ_It = پورټ_Map.پیدا_کړه(refp->varp());
            إذا (پورټ_It != پورټ_Map.پای_ټکی()) {
                refp->replaceWith(پورټ_It->دوهم->clone_ونه(خطأ));
                VL_DO_DANGLING(ټېل_Deletep(refp), refp);
                أرجع;
            }
            ثابت خپلکاری localIt = localVarMap.پیدا_کړه(refp->varp());
            إذا (localIt != localVarMap.پای_ټکی()) refp->varp(localIt->دوهم);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        // Clean up argument expressions
        لكل (ثابت خپلکاری& tconnect : tconnects) {
            ټېل_Deletep(tconnect.دوهم->exprp()->unlink_Fr_شا());
        }
        V3AssertCommon::checkSvaInstanceDelays(propExprp, funcrefp);
        أرجع propExprp;
    }
    AstPropSpec* substitutePropertyCall(AstPropSpec* nodep) {
        إذا (AstFuncRef* ثابت funcrefp = VN_CAST(nodep->propp(), FuncRef)) {
            إذا (ثابت AstProperty* ثابت propp = VN_CAST(funcrefp->taskp(), Property)) {
                AstPropSpec* ثابت propExprp = clonePropertyBody(funcrefp, propp);

                // Handle case with 2 disable iff statement (IEEE 1800-2023 16.12.1)
                إذا (nodep->disablep() && propExprp->disablep()) {
                    nodep->v3error("disable iff expression before property call and in its "
                                   "body is not legal");
                    ټېل_Deletep(propExprp->disablep()->unlink_Fr_شا());
                }
                // If disable iff is in outer property, move it to inner
                إذا (nodep->disablep()) {
                    Ast_غوټه_بیان* ثابت disablep = nodep->disablep()->unlink_Fr_شا();
                    propExprp->disablep(disablep);
                }

                إذا (nodep->sensesp() && propExprp->sensesp()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Clock event before property call and in its body");
                    ټېل_Deletep(propExprp->sensesp()->unlink_Fr_شا());
                }
                // If clock event is in outer property, move it to inner
                إذا (nodep->sensesp()) {
                    Ast_Sen_توکی* ثابت sensesp = nodep->sensesp();
                    sensesp->unlink_Fr_شا();
                    propExprp->sensesp(sensesp);
                }

                // Now substitute property reference with property body
                nodep->replaceWith(propExprp);
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                أرجع propExprp;
            }
        }
        أرجع nodep;
    }

    فراغ substituteNestedPropertyCall(AstFuncRef* funcrefp, ثابت AstProperty* propp) {
        AstPropSpec* ثابت specp = clonePropertyBody(funcrefp, propp);
        Ast_غوټه* replp = تش_اشاره;
        إذا (specp->disablep()) {
            funcrefp->v3error("Illegal 'disable iff' in a nested property instance"
                              " (IEEE 1800-2023 16.12.1)");
        } وإلا إذا (specp->sensesp()) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: clock event in a nested property instance");
        } وإلا إذا (specp->propp()->exists(
                       [](ثابت Ast_غوټه_بیان* np) { أرجع np->is_Multi_دوره_Sva(); })) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: multi-cycle body in a nested property instance");
        } وإلا {
            replp = specp->propp()->unlink_Fr_شا();
        }
        إذا (!replp) replp = جديد AstConst{funcrefp->fileline(), AstConst::بټ_False{}};
        funcrefp->replaceWith(replp);
        VL_DO_DANGLING(ټېل_Deletep(funcrefp), funcrefp);
        VL_DO_DANGLING(ټېل_Deletep(specp), specp);
    }

    // VISITORS
    //========== Statements
    فراغ لیدنه(AstClocking* ثابت nodep) override {
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        UINFO(8, "   CLOCKING" << nodep);
        إذا (nodep->isDefault()) {
            إذا (m_defaultClockingp) {
                nodep->v3error("Only one default clocking block allowed per module"
                               " (IEEE 1800-2023 14.12)");
            } وإلا {
                m_defaultClockingp = nodep;
                m_defaultClkEvtVarp = nodep->ensureEventp();
            }
        }
        iterateChildren(nodep);
        إذا (nodep->eventp()) nodep->جمع_کړه_بل_دلته(nodep->eventp()->unlink_Fr_شا());
        VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
    }
    فراغ لیدنه(AstModportClockingRef* ثابت nodep) override {
        // It has to be converted to a list of ModportClockingVarRefs,
        // because clocking blocks are removed in this pass
        لكل (Ast_غوټه* itemp = nodep->clockingp()->itemsp(); itemp; itemp = itemp->nextp()) {
            إذا (Ast_Clocking_توکی* ثابت citemp = VN_CAST(itemp, Clocking_توکی)) {
                إذا (AstVar* ثابت varp
                    = citemp->varp() ? citemp->varp() : VN_AS(citemp->user1p(), Var)) {
                    AstModportVarRef* ثابت modVarp = جديد AstModportVarRef{
                        nodep->fileline(), varp->name(), citemp->direction()};
                    modVarp->varp(varp);
                    nodep->جمع_کړه_بل_دلته(modVarp);
                    إذا (citemp->direction().isOutput()) {
                        // Drives through the modport set the drive flag
                        AstVar* ثابت drivenVarp = getCreateDrivenVarp(citemp);
                        AstModportVarRef* ثابت drivenRefp = جديد AstModportVarRef{
                            nodep->fileline(), drivenVarp->name(), VDirection::OUTPUT};
                        drivenRefp->varp(drivenVarp);
                        nodep->جمع_کړه_بل_دلته(drivenRefp);
                    }
                }
            }
        }
        VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
    }
    فراغ لیدنه(Ast_Clocking_توکی* ثابت nodep) override {
        // Get a ref to the sampled/driven variable
        AstVar* ثابت varp = nodep->varp();
        إذا (!varp) {
            // Unused item
            أرجع;
        }
        // Flag set by drives of an output clockvar, possibly visited before this item
        إذا (nodep->direction().isOutput()) m_modp->جمع_کړه_Stmtsp(getCreateDrivenVarp(nodep));
        File_کرښه* ثابت flp = nodep->fileline();
        V3Const::constifyEdit(nodep->skewp());
        إذا (!VN_IS(nodep->skewp(), Const)) {
            nodep->skewp()->v3error("Skew must be constant (IEEE 1800-2023 14.4)");
            أرجع;
        }
        AstConst* ثابت skewp = VN_AS(nodep->skewp(), Const);
        إذا (skewp->عدد().isNegative()) skewp->v3error("Skew cannot be negative");
        Ast_غوټه_بیان* ثابت exprp = nodep->exprp();
        varp->name(m_clockingp->name() + "__DOT__" + varp->name());
        m_clockingp->جمع_کړه_بل_دلته(varp->unlink_Fr_شا());
        nodep->user1p(varp);
        varp->user1p(nodep);
        إذا (nodep->direction() == VDirection::OUTPUT) {
            exprp->foreach([](ثابت Ast_غوټه_Var_Ref* varrefp) {
                // Prevent confusing BLKANDNBLK warnings on clockvars due to generated assignments
                varrefp->fileline()->warnOff(V3_Error_کوډ::BLKANDNBLK, ریښتیا);
                // Mark the driven signal so later passes (e.g. V3Inst port
                // connection) know the drive flows out from inside the cell.
                // Item expressions are plain signal references, so only the
                // driven signal is marked, never index selects.
                varrefp->varp()->isClockingDriven(ریښتیا);
            });
            AstVarRef* ثابت skewedReadRefp = جديد AstVarRef{flp, varp, VAccess::READ};
            skewedReadRefp->user1(ریښتیا);
            // Initialize the clockvar
            AstVarRef* ثابت skewedWriteRefp = جديد AstVarRef{flp, varp, VAccess::WRITE};
            skewedWriteRefp->user1(ریښتیا);
            AstInitialStatic* ثابت initClockvarp = جديد AstInitialStatic{
                flp, جديد Ast_ټاکل{flp, skewedWriteRefp, exprp->clone_ونه_خالص(خطأ)}};
            m_modp->جمع_کړه_Stmtsp(initClockvarp);
            // Each drive sets a flag, so the signal is also assigned when driven with the same
            // value again, e.g. after another assignment to the signal (IEEE 1800-2023 14.16.2)
            AstVar* ثابت drivenVarp = getCreateDrivenVarp(nodep);
            m_modp->جمع_کړه_Stmtsp(جديد AstInitialStatic{
                flp, جديد Ast_ټاکل{flp, جديد AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                   جديد AstConst{flp, AstConst::بټ_False{}}}});
            // Assign the clockvar to the actual var if it was driven
            Ast_ټاکل* ثابت assignp
                = جديد Ast_ټاکل{flp, exprp->clone_ونه_خالص(خطأ), skewedReadRefp};
            AstIf* ثابت ifp
                = جديد AstIf{flp, جديد AstVarRef{flp, drivenVarp, VAccess::READ},
                            جديد Ast_ټاکل{flp, جديد AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                          جديد AstConst{flp, AstConst::بټ_False{}}}};
            ifp->جمع_کړه_Thensp(assignp);
            // Only in the time step of the clocking event
            AstCMethodHard* ثابت trigp = جديد AstCMethodHard{
                nodep->fileline(), جديد AstVarRef{flp, m_clockingp->ensureEventp(), VAccess::READ},
                VCMethod::EVENT_IS_TRIGGERED};
            trigp->dtype_سېټ_بټ();
            ifp->condp(جديد AstLogAnd{flp, ifp->condp()->unlink_Fr_شا(), trigp});
            // A skew rounding to no time precision ticks is a zero skew
            Ast_غوټه_بیان* ثابت ticksp
                = V3Timing::delay_منزل_Ticks(skewp->clone_ونه(خطأ), m_modp->timeunit());
            ثابت منطقي zeroSkew = ticksp->isZero();
            VL_DO_DANGLING(ticksp->delete_ونه(), ticksp);
            إذا (zeroSkew || !skewp->fileline()->timingOn()) {
                // Drive the var in Re-NBA (IEEE 1800-2023 14.16), ignoring skews where timing is
                // turned off
                Ast_Sen_ونه* senTreep = m_clockingp->sentreep()->clone_ونه(خطأ);
                senTreep->جمع_کړه_Sensesp(جديد Ast_Sen_توکی{
                    flp, VEdgeType::ET_بدل_شوی, جديد AstVarRef{flp, drivenVarp, VAccess::READ}});
                m_clockingp->جمع_کړه_بل_دلته(جديد AstAlwaysReactive{flp, senTreep, ifp});
            } وإلا {
                // Fork only the assignment, so this AlwaysPostponed can be retriggered
                // before the assignment happens
                AstFork* ثابت forkp = جديد AstFork{flp, VJoinType::JOIN_NONE};
                forkp->سمدستي_پیل(ریښتیا);
                forkp->جمع_کړه_Forksp(جديد AstBegin{flp, "", assignp->unlink_Fr_شا(), ریښتیا});
                ifp->جمع_کړه_Thensp(forkp);
                // Read the clockvar in Postponed, so a drive late in the time step is applied
                // at this event plus the skew (IEEE 1800-2023 14.16)
                m_clockingp->جمع_کړه_بل_دلته(جديد AstAlwaysPostponed{flp, ifp});
                إذا (v3Global.opt.timing().is_سېټ_True()) {
                    AstDelay* ثابت delayp = جديد AstDelay{flp, skewp->unlink_Fr_شا(), خطأ};
                    delayp->timeunit(m_modp->timeunit());
                    assignp->timingControlp(delayp);
                } وإلا إذا (v3Global.opt.timing().is_سېټ_False()) {
                    nodep->v3warn(E_NOTIMING,
                                  "Clocking output skew greater than #0 requires --timing");
                } وإلا {
                    nodep->v3warn(E_NEEDTIMINGOPT,
                                  "Use --timing or --no-timing to specify how "
                                  "clocking output skew greater than #0 should be handled");
                }
            }
        } وإلا إذا (nodep->direction() == VDirection::ننوت) {
            // Ref to the clockvar
            AstVarRef* ثابت refp = جديد AstVarRef{flp, varp, VAccess::WRITE};
            refp->user1(ریښتیا);
            إذا (skewp->عدد().is1_ګام()) {
                // #1step means the value that is sampled is always the signal's last value
                // before the clock edge (IEEE 1800-2023 14.4)
                AstSampled* ثابت sampledp
                    = جديد AstSampled{flp, exprp->clone_ونه_خالص(خطأ), exprp->dtypep(), ریښتیا};
                Ast_ټاکل* ثابت assignp = جديد Ast_ټاکل{flp, refp, sampledp};
                m_clockingp->جمع_کړه_بل_دلته(جديد AstAlways{
                    flp, VAlwaysKwd::ALWAYS, m_clockingp->sentreep()->clone_ونه(خطأ), assignp});
            } وإلا إذا (skewp->isZero()) {
                // #0 means the var has to be sampled in Observed (IEEE 1800-2023 14.13)
                Ast_ټاکل* ثابت assignp = جديد Ast_ټاکل{flp, refp, exprp->clone_ونه_خالص(خطأ)};
                m_clockingp->جمع_کړه_بل_دلته(جديد AstAlwaysObserved{
                    flp, m_clockingp->sentreep()->clone_ونه(خطأ), assignp});
            } وإلا {
                // Create a queue where we'll store sampled values with timestamps
                Ast_Sample_کتار_DType* ثابت کتار_Dtp
                    = جديد Ast_Sample_کتار_DType{flp, exprp->dtypep()};
                m_netlistp->typeTablep()->جمع_کړه_Typesp(کتار_Dtp);
                AstVar* ثابت کتار_Varp
                    = جديد AstVar{flp, VVarType::MODULETEMP, "__Vqueue__" + varp->name(), کتار_Dtp};
                کتار_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_clockingp->جمع_کړه_بل_دلته(کتار_Varp);
                // Create a process like this:
                //     always queue.push(<sampled var>);
                AstCMethodHard* ثابت pushp = جديد AstCMethodHard{
                    flp, جديد AstVarRef{flp, کتار_Varp, VAccess::WRITE}, VCMethod::DYN_ټېل,
                    جديد Ast_وخت{nodep->fileline(), m_modp->timeunit()}};
                pushp->جمع_کړه_Pinsp(exprp->clone_ونه_خالص(خطأ));
                pushp->dtype_سېټ_Void();
                m_clockingp->جمع_کړه_بل_دلته(
                    جديد AstAlways{flp, VAlwaysKwd::ALWAYS, تش_اشاره, pushp->makeStmt()});
                // Create a process like this:
                //     always @<clocking event> queue.pop(<skew>, /*out*/<skewed var>);
                AstCMethodHard* ثابت popp = جديد AstCMethodHard{
                    flp, جديد AstVarRef{flp, کتار_Varp, VAccess::READWRITE}, VCMethod::DYN_راوباسه,
                    جديد Ast_وخت{nodep->fileline(), m_modp->timeunit()}};
                popp->جمع_کړه_Pinsp(skewp->unlink_Fr_شا());
                refp->access(VAccess::READWRITE);  // Only conditionally assigned
                popp->جمع_کړه_Pinsp(refp);
                popp->dtype_سېټ_Void();
                m_clockingp->جمع_کړه_بل_دلته(جديد AstAlways{flp, VAlwaysKwd::ALWAYS,
                                                       m_clockingp->sentreep()->clone_ونه(خطأ),
                                                       popp->makeStmt()});
            }
        } وإلا {
            nodep->v3fatalSrc("Invalid direction");
        }
    }
    فراغ لیدنه(AstDelay* nodep) override {
        m_has_دوره_Delay |= nodep->is_دوره_Delay();
        // Only cycle delays are relevant in this stage; also only process once
        إذا (!nodep->is_دوره_Delay()) {
            إذا (m_دننه_Synch_Drive) {
                nodep->v3error("Only cycle delays can be used in synchronous drives"
                               " (IEEE 1800-2023 14.16)");
            }
            iterateChildren(nodep);
            أرجع;
        }
        إذا (m_دننه_ټاکل && !m_دننه_Synch_Drive) {
            nodep->v3error("Cycle delays not allowed as intra-assignment delays"
                           " (IEEE 1800-2023 14.11)");
            VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
            أرجع;
        }
        إذا (nodep->stmtsp()) nodep->جمع_کړه_بل_دلته(nodep->stmtsp()->unlink_Fr_شا_With_بل());
        File_کرښه* ثابت flp = nodep->fileline();
        Ast_غوټه_بیان* valuep = V3Const::constifyEdit(nodep->lhsp()->unlink_Fr_شا());
        ثابت AstConst* ثابت constp = VN_CAST(valuep, Const);
        إذا (!constp && m_pexprp) {
            // V3AssertNfa handles non-const delays in sequences before this pass and
            // replaces the property; this branch should never be reached.
            nodep->v3fatalSrc("Non-constant cycle delay in assertion: "
                              "should have been caught by V3AssertNfa");
        } وإلا إذا (constp && constp->isZero()) {
            VL_DO_DANGLING(ټېل_Deletep(valuep), valuep);
            إذا (m_دننه_Synch_Drive) {
                // ##0 has no effect in synchronous drives (IEEE 1800-2023 14.11)
                VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
                أرجع;
            }
            إذا (m_pexprp) {
                // ##0 in sequence context = zero delay = same clock tick
                VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
                أرجع;
            }
            // Procedural ##0: synchronize with default clocking event (IEEE 1800-2023 14.11)
            إذا (!m_defaultClockingp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
                أرجع;
            }
            nodep->replaceWith(new_دوره_Delay_Zerop(flp));
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع;
        }
        Ast_Sen_ونه* sentreep = تش_اشاره;
        إذا (!m_drives.تشه()) {
            إذا (m_دننه_Concat_ټاکل) {
                // Also the clockvars may be of different clocking blocks
                nodep->v3error("Cycle delays cannot be used in synchronous drives to a"
                               " concatenation (IEEE 1800-2023 14.16)");
                VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
                VL_DO_DANGLING(ټېل_Deletep(valuep), valuep);
                أرجع;
            }
            // Count the cycles of the driven clockvar's clocking block (IEEE 1800-2023 14.16)
            sentreep = newDriveSentreep(nodep, m_drives.front());
            إذا (!sentreep) {
                VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
                VL_DO_DANGLING(ټېل_Deletep(valuep), valuep);
                أرجع;
            }
        } وإلا إذا (!m_defaultClockingp) {
            إذا (!m_pexprp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
                VL_DO_DANGLING(ټېل_Deletep(valuep), valuep);
                أرجع;
            }
            sentreep = جديد Ast_Sen_ونه{flp, m_senip->clone_ونه(خطأ)};
        } وإلا {
            sentreep = m_defaultClockingp->sentreep()->clone_ونه(خطأ);
        }
        AstEventControl* ثابت controlp
            = جديد AstEventControl{nodep->fileline(), sentreep, تش_اشاره};
        ثابت std::string delayName = m_دوره_Dly_نومونه.get(nodep);
        Ast_غوټه_بیان* throughoutp
            = nodep->throughoutp() ? nodep->throughoutp()->unlink_Fr_شا() : تش_اشاره;

        AstVar* ثابت cntVarp = جديد AstVar{flp, VVarType::BLOCKTEMP, delayName + "__counter",
                                           nodep->پیدا_کړه_Basic_DType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstBegin* ثابت beginp = جديد AstBegin{flp, delayName + "__block", cntVarp, ریښتیا};
        // A non-constant delay is counted at runtime. The count is integral of 64 bits, as
        // V3Width makes delays, or real, which is rounded (IEEE 1800-2023 6.12.1)
        إذا (valuep->isDouble()) {
            valuep = جديد AstRToIRoundS{flp, valuep};
            valuep->dtypeFrom(cntVarp);
        } وإلا {
            valuep = جديد AstSel{flp, valuep, 0, cntVarp->پلنوالی()};
        }
        beginp->جمع_کړه_Stmtsp(جديد Ast_ټاکل{flp, جديد AstVarRef{flp, cntVarp, VAccess::WRITE}, valuep});
        // If the count is zero, a synchronous drive waits for none, like with a constant '##0',
        // and a procedural delay waits like a procedural '##0' (IEEE 1800-2023 14.11)
        إذا (!constp && !m_دننه_Synch_Drive) {
            beginp->جمع_کړه_Stmtsp(جديد AstIf{
                flp,
                جديد AstEq{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ}, جديد AstConst{flp, 0}},
                new_دوره_Delay_Zerop(flp)});
        }

        // Throughout: create flag tracking whether condition held every tick
        AstVar* throughoutOkp = تش_اشاره;
        إذا (throughoutp) {
            throughoutOkp = جديد AstVar{flp, VVarType::BLOCKTEMP, delayName + "__throughoutOk",
                                       nodep->پیدا_کړه_Basic_DType(VBasicDTypeKwd::LOGIC_IMPLICIT)};
            throughoutOkp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->جمع_کړه_Stmtsp(throughoutOkp);
            beginp->جمع_کړه_Stmtsp(جديد Ast_ټاکل{flp, جديد AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            جديد AstConst{flp, AstConst::بټ_True{}}});
            // Check condition at tick 0 (sequence start, before entering loop)
            beginp->جمع_کړه_Stmtsp(
                جديد AstIf{flp, جديد AstLogNot{flp, throughoutp->clone_ونه_خالص(خطأ)},
                          جديد Ast_ټاکل{flp, جديد AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                        جديد AstConst{flp, AstConst::بټ_False{}}}});
        }

        {
            AstLoop* ثابت loopp = جديد AstLoop{flp};
            // When throughout is present, exit loop early if condition fails
            Ast_غوټه_بیان* loopCondp
                = جديد AstGt{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ}, جديد AstConst{flp, 0}};
            إذا (throughoutOkp) {
                loopCondp = جديد AstLogAnd{flp, loopCondp,
                                          جديد AstVarRef{flp, throughoutOkp, VAccess::READ}};
            }
            loopp->جمع_کړه_Stmtsp(جديد AstLoopTest{flp, loopp, loopCondp});
            loopp->جمع_کړه_Stmtsp(controlp);
            loopp->جمع_کړه_Stmtsp(
                جديد Ast_ټاکل{flp, جديد AstVarRef{flp, cntVarp, VAccess::WRITE},
                              جديد AstSub{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ},
                                         جديد AstConst{flp, 1}}});
            // Check throughout condition at each tick during delay (IEEE 1800-2023 16.9.9)
            إذا (throughoutp) {
                loopp->جمع_کړه_Stmtsp(
                    جديد AstIf{flp, جديد AstLogNot{flp, throughoutp},
                              جديد Ast_ټاکل{flp, جديد AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            جديد AstConst{flp, AstConst::بټ_False{}}}});
            }
            beginp->جمع_کړه_Stmtsp(loopp);
        }
        // Compose wrappers on remaining sequence: throughout gate (inner), disable iff (outer)
        Ast_غوټه* remainp = nodep->nextp() ? nodep->nextp()->unlink_Fr_شا_With_بل() : تش_اشاره;
        إذا (throughoutOkp) {
            // If condition failed during delay, fail assertion
            remainp = جديد AstIf{flp, جديد AstVarRef{flp, throughoutOkp, VAccess::READ}, remainp,
                                جديد AstPExprClause{flp, /*pass=*/خطأ}};
        }
        إذا (m_disableSeqIfp && remainp) {
            AstIf* ثابت disableSeqIfp = m_disableSeqIfp->clone_ونه(خطأ);
            disableSeqIfp->جمع_کړه_Thensp(remainp);
            remainp = disableSeqIfp;
        }
        إذا (remainp) {
            إذا (throughoutOkp) {
                // throughoutOkp is declared in beginp scope -- check must be inside it
                beginp->جمع_کړه_Stmtsp(remainp);
            } وإلا {
                nodep->جمع_کړه_بل_دلته(remainp);
            }
        }
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
    }
    فراغ لیدنه(Ast_Sen_ونه* nodep) override {
        إذا (m_دننه_Synch_Drive) {
            nodep->v3error("Event controls cannot be used in "
                           "synchronous drives (IEEE 1800-2023 14.16)");
        }

        ثابت AstSampled* sampledp;
        إذا (nodep->exists([&sampledp](ثابت AstSampled* ثابت sp) {
                sampledp = sp;
                أرجع ریښتیا;
            })) {
            sampledp->v3warn(E_UNSUPPORTED, "Unsupported: $sampled inside sensitivity list");
        }
    }
    فراغ لیدنه(Ast_غوټه_Var_Ref* nodep) override {
        UINFO(8, " -varref:  " << nodep);
        UINFO(8, " -varref-var-back:  " << nodep->varp()->backp());
        UINFO(8, " -varref-var-user1:  " << nodep->varp()->user1p());
        إذا (Ast_Clocking_توکی* ثابت itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->لومړی_Abovep(),
                Clocking_توکی)) {
            إذا (nodep->user1()) أرجع;

            // ensure linking still works, this has to be done only once
            إذا (AstVarXRef* xrefp = VN_CAST(nodep, VarXRef)) {
                UINFO(8, " -clockvarxref-in:  " << xrefp);
                string dotted = xrefp->dotted();
                ثابت اندازه_t dotPos = dotted.rfind('.');
                dotted.erase(dotPos, string::npos);
                xrefp->dotted(dotted);
                UINFO(8, " -clockvarxref-out: " << xrefp);
                m_xrefsp.emplace_شا(xrefp);
            }

            إذا (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            إذا (nodep->access().isWriteOrRW()) {
                إذا (itemp->direction() == VDirection::OUTPUT) {
                    إذا (!m_دننه_ټاکل_Dly_Lhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    إذا (m_دننه_ټاکل) {
                        m_دننه_Synch_Drive = ریښتیا;
                        m_drives.ټېل_شا({itemp, nodep});
                    }
                } وإلا إذا (itemp->direction() == VDirection::ننوت) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
            nodep->user1(ریښتیا);
        }
    }
    فراغ لیدنه(AstMemberSel* nodep) override {
        إذا (Ast_Clocking_توکی* ثابت itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->لومړی_Abovep(),
                Clocking_توکی)) {
            إذا (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            إذا (nodep->access().isWriteOrRW()) {
                إذا (itemp->direction() == VDirection::OUTPUT) {
                    إذا (!m_دننه_ټاکل_Dly_Lhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    إذا (m_دننه_ټاکل) {
                        m_دننه_Synch_Drive = ریښتیا;
                        m_drives.ټېل_شا({itemp, nodep});
                    }
                } وإلا إذا (itemp->direction() == VDirection::ننوت) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
        }
    }
    فراغ لیدنه(Ast_غوټه_ټاکل* nodep) override {
        إذا (nodep->user1()) أرجع;
        VL_RESTORER(m_دننه_ټاکل);
        VL_RESTORER(m_دننه_Synch_Drive);
        VL_RESTORER(m_دننه_Concat_ټاکل);
        VL_RESTORER_CLEAR(m_drives);
        m_دننه_ټاکل = ریښتیا;
        m_دننه_Synch_Drive = خطأ;
        {
            // The braces of a concatenation are a replication, by one
            ثابت Ast_غوټه_بیان* lhsp = nodep->lhsp();
            إذا (ثابت AstReplicate* ثابت repp = VN_CAST(lhsp, Replicate)) lhsp = repp->srcp();
            m_دننه_Concat_ټاکل = VN_IS(lhsp, Concat);
        }
        {
            VL_RESTORER(m_دننه_ټاکل_Dly_Lhs);
            m_دننه_ټاکل_Dly_Lhs = VN_IS(nodep, ټاکل_Dly);
            iterate(nodep->lhsp());
        }
        iterate(nodep->rhsp());
        إذا (nodep->timingControlp()) iterate(nodep->timingControlp());
        // A drive without a cycle delay, also after removing '##0', updates the clockvar now
        إذا (m_دننه_Synch_Drive && !nodep->timingControlp()) {
            Ast_ټاکل* ثابت assignp = جديد Ast_ټاکل{
                nodep->fileline(), nodep->lhsp()->unlink_Fr_شا(), nodep->rhsp()->unlink_Fr_شا()};
            assignp->user1(ریښتیا);
            nodep->replaceWith(assignp);
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
            nodep = assignp;
        }
        // Note the drives when they take effect, after the cycle delay if any. Each clockvar
        // of the LHS is driven, e.g. in a concatenation which other simulators accept, although
        // IEEE 1800-2023 14.16 does not allow it
        Ast_غوټه* setsp = تش_اشاره;
        لكل (ثابت SynchDrive& drive : m_drives) {
            setsp = Ast_غوټه::جمع_کړه_بل(setsp, newDrivenSetp(nodep->fileline(), drive));
        }
        إذا (!setsp) أرجع;
        إذا (AstBegin* ثابت beginp = VN_CAST(nodep->timingControlp(), Begin)) {
            beginp->جمع_کړه_Stmtsp(setsp);
        } وإلا {
            nodep->جمع_کړه_بل_دلته(setsp);
        }
    }
    فراغ لیدنه(AstAlways* nodep) override {
        iterate_And_بل_Null(nodep->sentreep());
        إذا (nodep->sentreep()) m_seniAlwaysp = nodep->sentreep()->sensesp();
        iterate_And_بل_Null(nodep->stmtsp());
        m_seniAlwaysp = تش_اشاره;
    }

    فراغ لیدنه(Ast_غوټه_Cover_Or_Assert* nodep) override {
        إذا (nodep->sentreep()) أرجع;  // Already processed

        VL_RESTORER(m_senip);
        VL_RESTORER(m_disablep);
        m_senip = تش_اشاره;
        m_disablep = تش_اشاره;

        // Find Clocking's buried under nodep->exprsp
        iterateChildren(nodep);
        إذا (!nodep->سمدستي()) nodep->sentreep(new_Sen_ونه(nodep, تش_اشاره, nodep));
    }
    فراغ لیدنه(AstFalling* nodep) override {
        إذا (nodep->user1_سېټ_Once()) أرجع;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_کرښه* ثابت fl = nodep->fileline();
        Ast_غوټه_بیان* exprp = nodep->exprp()->unlink_Fr_شا();
        إذا (exprp->پلنوالی() > 1) exprp = جديد AstSel{fl, exprp, 0, 1};
        Ast_غوټه_بیان* ثابت futurep = جديد AstFuture{fl, exprp, new_Sen_ونه(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = جديد AstAnd{fl, exprp->clone_ونه_خالص(خطأ), جديد AstNot{fl, futurep}};
        exprp->dtype_سېټ_بټ();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(AstFell* nodep) override {
        إذا (nodep->user1_سېټ_Once()) أرجع;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_کرښه* ثابت fl = nodep->fileline();
        Ast_غوټه_بیان* exprp = nodep->exprp()->unlink_Fr_شا();
        إذا (exprp->پلنوالی() > 1) exprp = جديد AstSel{fl, exprp, 0, 1};
        Ast_Sen_ونه* sentreep = nodep->sentreep();
        إذا (sentreep) sentreep->unlink_Fr_شا();
        AstPast* ثابت pastp = جديد AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_ونه(nodep, sentreep));
        exprp = جديد AstAnd{fl, pastp, جديد AstNot{fl, exprp->clone_ونه_خالص(خطأ)}};
        exprp->dtype_سېټ_بټ();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(AstFuture* nodep) override {
        إذا (nodep->user1_سېټ_Once()) أرجع;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        Ast_Sen_ونه* ثابت sentreep = nodep->sentreep();
        إذا (sentreep) VL_DO_DANGLING(ټېل_Deletep(sentreep->unlink_Fr_شا()), sentreep);
        nodep->sentreep(new_Sen_ونه(nodep));
    }
    فراغ لیدنه(AstPast* nodep) override {
        checkSamplingFuncDType(nodep, nodep->exprp());
        إذا (nodep->sentreep()) أرجع;  // Already processed
        iterateChildren(nodep);
        nodep->sentreep(new_Sen_ونه(nodep));
    }
    فراغ لیدنه(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- Lower standalone exact [*N] (N >= 2) via saturating counter.
        // Range/unbounded forms and SExpr-contained forms are lowered by V3AssertNfa.
        iterateChildren(nodep);
        // V3AssertNfa handles unbounded/ranged forms upstream, so this fast-path
        // is effectively unreachable when NFA is enabled.
        إذا (nodep->unbounded() || nodep->maxCountp()) أرجع;  // LCOV_EXCL_LINE
        ثابت AstConst* ثابت constp = VN_CAST(nodep->countp(), Const);
        إذا (VL_UNLIKELY(!constp || constp->منزل_SInt() < 1)) {
            nodep->v3fatalSrc("Consecutive repetition count must be a positive constant"
                              " (should have been caught by V3Width)");
            أرجع;
        }
        ثابت صحيح n = constp->منزل_SInt();
        File_کرښه* ثابت flp = nodep->fileline();
        Ast_غوټه_بیان* ثابت exprp = nodep->exprp()->unlink_Fr_شا();
        إذا (n == 1) {
            nodep->replaceWith(exprp);
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع;
        }
        // Saturating counter: if (expr) cnt <= min(cnt+1, N); else cnt <= 0;
        AstVar* ثابت cntVarp = جديد AstVar{flp, VVarType::MODULETEMP, m_cons_Rep_نومونه.get(""),
                                           nodep->پیدا_کړه_Basic_DType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->جمع_کړه_Stmtsp(cntVarp);
        Ast_غوټه_بیان* ثابت بیان_Clonep = exprp->clone_ونه_خالص(خطأ);
        Ast_غوټه_بیان* ثابت saturatingIncrp = جديد AstCond{
            flp,
            جديد AstLt{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ},
                      جديد AstConst{flp, static_cast<uint32_t>(n)}},
            جديد Ast_جمع_کړه{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ}, جديد AstConst{flp, 1u}},
            جديد AstConst{flp, static_cast<uint32_t>(n)}};
        Ast_ټاکل_Dly* ثابت incrAssignp
            = جديد Ast_ټاکل_Dly{flp, جديد AstVarRef{flp, cntVarp, VAccess::WRITE}, saturatingIncrp};
        Ast_ټاکل_Dly* ثابت resetAssignp = جديد Ast_ټاکل_Dly{
            flp, جديد AstVarRef{flp, cntVarp, VAccess::WRITE}, جديد AstConst{flp, 0u}};
        AstIf* ثابت ifp = جديد AstIf{flp, بیان_Clonep, incrAssignp, resetAssignp};
        Ast_Sen_ونه* ثابت senTreep = new_Sen_ونه(nodep);
        AstAlways* ثابت alwaysp = جديد AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, ifp};
        cntVarp->جمع_کړه_بل_دلته(alwaysp);
        // Match: cnt >= N-1 (previous cycles via NBA) && expr (current cycle)
        Ast_غوټه_بیان* ثابت cntCheckp = جديد AstGte{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ},
                                                  جديد AstConst{flp, static_cast<uint32_t>(n - 1)}};
        cntCheckp->dtype_سېټ_بټ();
        Ast_غوټه_بیان* ثابت matchp = جديد AstAnd{flp, cntCheckp, exprp};
        matchp->dtype_سېټ_بټ();
        nodep->replaceWith(matchp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(AstRising* nodep) override {
        إذا (nodep->user1_سېټ_Once()) أرجع;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_کرښه* ثابت fl = nodep->fileline();
        Ast_غوټه_بیان* exprp = nodep->exprp()->unlink_Fr_شا();
        إذا (exprp->پلنوالی() > 1) exprp = جديد AstSel{fl, exprp, 0, 1};
        Ast_غوټه_بیان* ثابت futurep = جديد AstFuture{fl, exprp, new_Sen_ونه(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = جديد AstAnd{fl, جديد AstNot{fl, exprp->clone_ونه_خالص(خطأ)}, futurep};
        exprp->dtype_سېټ_بټ();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(AstRose* nodep) override {
        إذا (nodep->user1_سېټ_Once()) أرجع;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_کرښه* ثابت fl = nodep->fileline();
        Ast_غوټه_بیان* exprp = nodep->exprp()->unlink_Fr_شا();
        إذا (exprp->پلنوالی() > 1) exprp = جديد AstSel{fl, exprp, 0, 1};
        Ast_Sen_ونه* sentreep = nodep->sentreep();
        إذا (sentreep) sentreep->unlink_Fr_شا();
        AstPast* ثابت pastp = جديد AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_ونه(nodep, sentreep));
        exprp = جديد AstAnd{fl, جديد AstNot{fl, pastp}, exprp->clone_ونه_خالص(خطأ)};
        exprp->dtype_سېټ_بټ();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    ساكن AstAssocArrayDType* getProcessAssocArrayType(File_کرښه* ثابت flp) {
        // Type of __VassertsActive___x[std::process]
        Ast_غوټه_DType* valp
            = v3Global.rootp()->typeTablep()->پیدا_کړه_Basic_DType(flp, VBasicDTypeKwd::بټ);
        AstClassRefDType* keyp
            = جديد AstClassRefDType{flp, v3Global.rootp()->stdPackageProcessp(), تش_اشاره};
        keyp->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        v3Global.rootp()->typeTablep()->جمع_کړه_Typesp(keyp);
        AstAssocArrayDType* ثابت typep = جديد AstAssocArrayDType{flp, valp, keyp};
        typep->dtypep(typep);
        v3Global.rootp()->typeTablep()->جمع_کړه_Typesp(typep);
        أرجع typep;
    }
    ساكن Ast_Stmt_بیان* getProcessAssocArrayDelete(AstVarRef* ثابت refp) {
        // Constructs refp.delete(std::process::self()) statement
        File_کرښه* ثابت flp = refp->fileline();
        AstCMethodHard* ثابت deletep = جديد AstCMethodHard{
            flp, refp, VCMethod::ASSOC_ERASE, v3Global.rootp()->stdPackageProcessSelfp(flp)};
        deletep->dtypep(refp->پیدا_کړه_Void_DType());
        أرجع جديد Ast_Stmt_بیان{flp, deletep};
    }
    ساكن Ast_غوټه_بیان* get_Process_Assoc_Array_اندازه(AstVarRef* ثابت refp) {
        // Constructs refp.size() statement
        AstCMethodHard* ثابت sizep
            = جديد AstCMethodHard{refp->fileline(), refp, VCMethod::ASSOC_اندازه};
        sizep->dtypep(refp->پیدا_کړه_Basic_DType(VBasicDTypeKwd::UINT32));
        أرجع sizep;
    }
    فراغ لیدنه(AstSEventually* nodep) override {
        UASSERT(v3Global.rootp()->stdPackagep(), "Should be imported");

        Ast_Sen_ونه* ثابت sentreep = new_Sen_ونه(nodep);
        إذا (!sentreep->sensesp()) {
            VL_DO_DANGLING(ټېل_Deletep(sentreep), sentreep);
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::بټ_False{}});
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع;
        }

        File_کرښه* ثابت flp = nodep->fileline();

        // Track active assertions
        AstVar* ثابت activep = جديد AstVar{flp, VVarType::MODULETEMP, m_فعال_نومونه.get(""),
                                           getProcessAssocArrayType(flp)};
        activep->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->جمع_کړه_Stmtsp(activep);

        // Assertion condition check
        AstLoop* ثابت loopp = جديد AstLoop{flp};
        AstSampled* ثابت condp
            = جديد AstSampled{flp, nodep->exprp()->unlink_Fr_شا(), تش_اشاره, ریښتیا};
        loopp->جمع_کړه_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLogNot{flp, condp}});
        loopp->جمع_کړه_Stmtsp(جديد AstEventControl{flp, sentreep, تش_اشاره});

        // Add assertion to the active set
        AstAssocSel* ثابت selp = جديد AstAssocSel{flp, جديد AstVarRef{flp, activep, VAccess::WRITE},
                                                  v3Global.rootp()->stdPackageProcessSelfp(flp)};
        Ast_ټاکل* ثابت incrementp = جديد Ast_ټاکل{flp, selp, جديد AstConst{flp, 1}};
        AstPExprClause* ثابت clausep = جديد AstPExprClause{flp};
        Ast_Stmt_بیان* ثابت deletep
            = getProcessAssocArrayDelete(جديد AstVarRef{flp, activep, VAccess::WRITE});

        // Main assertion block
        AstBegin* ثابت bodyp = جديد AstBegin{flp, m_بلاک_نومونه.get(""), تش_اشاره, ریښتیا};
        bodyp->جمع_کړه_Stmtsp(incrementp);
        bodyp->جمع_کړه_Stmtsp(loopp);
        bodyp->جمع_کړه_Stmtsp(clausep);
        bodyp->جمع_کړه_Stmtsp(deletep);

        // Validate assertion condition for each active assert
        AstVar* ثابت فعال_Countp = جديد AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                nodep->پیدا_کړه_Basic_DType(VBasicDTypeKwd::UINT32)};
        فعال_Countp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        Ast_ټاکل* ثابت init_فعال_Countp
            = جديد Ast_ټاکل{flp, جديد AstVarRef{flp, فعال_Countp, VAccess::WRITE},
                            get_Process_Assoc_Array_اندازه(جديد AstVarRef{flp, activep, VAccess::READ})};
        AstLoop* ثابت finalLoopp = جديد AstLoop{flp};
        AstIf* ثابت finalBodypCondp
            = جديد AstIf{flp, condp->clone_ونه_خالص(خطأ), جديد AstPExprClause{flp},
                        جديد AstPExprClause{flp, خطأ}};
        finalLoopp->جمع_کړه_Stmtsp(
            جديد AstLoopTest{flp, finalLoopp,
                            جديد AstNeq{flp, جديد AstVarRef{flp, فعال_Countp, VAccess::READ},
                                       جديد AstConst{flp, 0}}});
        finalLoopp->جمع_کړه_Stmtsp(finalBodypCondp);
        finalLoopp->جمع_کړه_Stmtsp(
            جديد Ast_ټاکل{flp, جديد AstVarRef{flp, فعال_Countp, VAccess::WRITE},
                          جديد AstSub{flp, جديد AstVarRef{flp, فعال_Countp, VAccess::READ},
                                     جديد AstConst{flp, 1}}});

        // Final assertion block
        AstBegin* ثابت finalp = جديد AstBegin{flp, m_بلاک_نومونه.get(""), تش_اشاره, ریښتیا};
        finalp->جمع_کړه_Stmtsp(فعال_Countp);
        finalp->جمع_کړه_Stmtsp(init_فعال_Countp);
        finalp->جمع_کړه_Stmtsp(finalLoopp);

        VL_RESTORER(m_pexprp);
        m_pexprp = جديد AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
        VL_RESTORER(m_has_دوره_Delay);
        m_has_دوره_Delay = خطأ;
        iterate(bodyp);
        iterate(finalp);

        إذا (m_has_دوره_Delay) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in s_eventually");
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::بټ_False{}});
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            VL_DO_DANGLING(m_pexprp->delete_ونه(), m_pexprp);
            أرجع;
        }

        nodep->replaceWith(m_pexprp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(AstStable* nodep) override {
        إذا (nodep->user1_سېټ_Once()) أرجع;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_کرښه* ثابت fl = nodep->fileline();
        Ast_غوټه_بیان* exprp = nodep->exprp()->unlink_Fr_شا();
        Ast_Sen_ونه* sentreep = nodep->sentreep();
        إذا (sentreep) sentreep->unlink_Fr_شا();
        AstPast* ثابت pastp = جديد AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_ونه(nodep, sentreep));
        exprp = جديد AstEq{fl, pastp, exprp->clone_ونه_خالص(خطأ)};
        exprp->dtype_سېټ_بټ();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(AstSteady* nodep) override {
        إذا (nodep->user1_سېټ_Once()) أرجع;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_کرښه* ثابت fl = nodep->fileline();
        Ast_غوټه_بیان* exprp = nodep->exprp()->unlink_Fr_شا();
        إذا (exprp->پلنوالی() > 1) exprp = جديد AstSel{fl, exprp, 0, 1};
        Ast_غوټه_بیان* ثابت futurep = جديد AstFuture{fl, exprp, new_Sen_ونه(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = جديد AstEq{fl, exprp->clone_ونه_خالص(خطأ), futurep};
        exprp->dtype_سېټ_بټ();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(AstSampled* nodep) override {
        iterateChildren(nodep);
        إذا (!nodep->دننی_2()) checkSamplingFuncDType(nodep, nodep->exprp());
    }

    // Validate repetition count: must be a non-negative elaboration-time constant.
    // Shared by goto [->N] and nonconsecutive [=N] repetition.
    // On error, replaces nodep with BitFalse placeholder and returns nullptr.
    ثابت AstConst* validate_Rep_شمېر(Ast_غوټه* nodep, Ast_غوټه_بیان*& countp) {
        countp = V3Const::constifyEdit(countp);
        ثابت AstConst* ثابت constp = VN_CAST(countp, Const);
        إذا (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ټېل_Deletep(countp), countp);
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::بټ_False_Erroring{}});
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع تش_اشاره;
        }
        إذا (constp->منزل_SInt() < 0) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ټېل_Deletep(countp), countp);
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::بټ_False_Erroring{}});
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع تش_اشاره;
        }
        إذا (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ټېل_Deletep(countp), countp);
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::بټ_False{}});
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع تش_اشاره;
        }
        أرجع constp;
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
    AstPExpr* createRepPExpr(File_کرښه* flp, Ast_غوټه_بیان* exprp, Ast_غوټه_بیان* countp,
                             Ast_غوټه_بیان* rhsp, منطقي isOverlapped, منطقي isNonConsec) {
        Ast_Sen_توکی* ثابت sensesp = m_senip;
        UASSERT_OBJ(sensesp, exprp, "Repetition requires a clock");

        ثابت std::string name
            = isNonConsec ? m_non_Cons_Rep_نومونه.get(exprp) : m_goto_Rep_نومونه.get(exprp);
        AstVar* ثابت cntVarp = جديد AstVar{flp, VVarType::BLOCKTEMP, name + "__counter",
                                           exprp->پیدا_کړه_Basic_DType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstBegin* ثابت beginp = جديد AstBegin{flp, name + "__block", cntVarp, ریښتیا};
        beginp->جمع_کړه_Stmtsp(
            جديد Ast_ټاکل{flp, جديد AstVarRef{flp, cntVarp, VAccess::WRITE}, جديد AstConst{flp, 0}});

        // Counting loop: find N occurrences of expr (shared by goto and nonconsec)
        AstLoop* ثابت loopp = جديد AstLoop{flp};
        loopp->جمع_کړه_Stmtsp(جديد AstLoopTest{flp, loopp,
                                         جديد AstLt{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ},
                                                   countp->clone_ونه_خالص(خطأ)}});
        // if (expr) cnt++ -- sampled is applied to whole property expr by V3Assert
        loopp->جمع_کړه_Stmtsp(
            جديد AstIf{flp, exprp,
                      جديد Ast_ټاکل{flp, جديد AstVarRef{flp, cntVarp, VAccess::WRITE},
                                    جديد Ast_جمع_کړه{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ},
                                               جديد AstConst{flp, 1}}}});
        loopp->جمع_کړه_Stmtsp(جديد AstIf{
            flp, جديد AstLt{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ}, countp},
            جديد AstEventControl{flp, جديد Ast_Sen_ونه{flp, sensesp->clone_ونه(خطأ)}, تش_اشاره}});

        beginp->جمع_کړه_Stmtsp(loopp);

        إذا (isNonConsec && rhsp) {
            // IEEE 16.9.2: b[=N] = b[->N] ##1 !b[*0:$]
            // Trailing window: ##1 advance, then scan !expr cycles checking rhs.
            // Window closes when expr becomes true again (fail if rhs was never true).
            beginp->جمع_کړه_Stmtsp(جديد AstEventControl{
                flp, جديد Ast_Sen_ونه{flp, sensesp->clone_ونه(خطأ)}, تش_اشاره});  // ##1
            إذا (!isOverlapped) {
                beginp->جمع_کړه_Stmtsp(جديد AstEventControl{
                    flp, جديد Ast_Sen_ونه{flp, sensesp->clone_ونه(خطأ)}, تش_اشاره});  // |=>
            }
            // Window loop: check rhs at each !expr cycle (done variable for termination)
            AstVar* ثابت بشپړ_Varp
                = جديد AstVar{flp, VVarType::BLOCKTEMP, name + "__done", exprp->پیدا_کړه_بټ_DType()};
            بشپړ_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->جمع_کړه_Stmtsp(بشپړ_Varp);
            beginp->جمع_کړه_Stmtsp(جديد Ast_ټاکل{flp, جديد AstVarRef{flp, بشپړ_Varp, VAccess::WRITE},
                                            جديد AstConst{flp, AstConst::بټ_False{}}});
            خپلکاری سېټ_بشپړ = [&]() {
                أرجع جديد Ast_ټاکل{flp, جديد AstVarRef{flp, بشپړ_Varp, VAccess::WRITE},
                                     جديد AstConst{flp, AstConst::بټ_True{}}};
            };
            AstLoop* ثابت windowp = جديد AstLoop{flp};
            // LoopTest: continue while !done
            windowp->جمع_کړه_Stmtsp(جديد AstLoopTest{
                flp, windowp, جديد AstNot{flp, جديد AstVarRef{flp, بشپړ_Varp, VAccess::READ}}});
            // if (expr) { fail; done = 1; } -- window closed, expr true again
            Ast_غوټه* ثابت failsp = جديد AstPExprClause{flp, خطأ};
            failsp->جمع_کړه_بل(سېټ_بشپړ());
            windowp->جمع_کړه_Stmtsp(جديد AstIf{flp, exprp->clone_ونه_خالص(خطأ), failsp});
            // if (rhs) { pass; done = 1; } -- consequent true at this !expr endpoint
            Ast_غوټه* ثابت passsp = جديد AstPExprClause{flp, ریښتیا};
            passsp->جمع_کړه_بل(سېټ_بشپړ());
            windowp->جمع_کړه_Stmtsp(جديد AstIf{flp, rhsp, passsp});
            // @(clk) -- advance to next cycle in window
            windowp->جمع_کړه_Stmtsp(
                جديد AstEventControl{flp, جديد Ast_Sen_ونه{flp, sensesp->clone_ونه(خطأ)}, تش_اشاره});
            beginp->جمع_کړه_Stmtsp(windowp);
        } وإلا إذا (isNonConsec) {
            // Standalone nonconsec: ##1 into window, then pass
            beginp->جمع_کړه_Stmtsp(
                جديد AstEventControl{flp, جديد Ast_Sen_ونه{flp, sensesp->clone_ونه(خطأ)}, تش_اشاره});
            beginp->جمع_کړه_Stmtsp(جديد AstPExprClause{flp, ریښتیا});
        } وإلا إذا (rhsp) {
            // Goto rep: check consequent once at match endpoint
            إذا (!isOverlapped) {
                beginp->جمع_کړه_Stmtsp(جديد AstEventControl{
                    flp, جديد Ast_Sen_ونه{flp, sensesp->clone_ونه(خطأ)}, تش_اشاره});
            }
            beginp->جمع_کړه_Stmtsp(جديد AstIf{flp, rhsp, جديد AstPExprClause{flp, ریښتیا},
                                        جديد AstPExprClause{flp, خطأ}});
        } وإلا {
            // Standalone goto: pass after counting
            beginp->جمع_کړه_Stmtsp(جديد AstPExprClause{flp, ریښتیا});
        }

        أرجع جديد AstPExpr{flp, beginp, exprp->پیدا_کړه_بټ_DType()};
    }

    // Reject [=M:N] nonconsecutive range that reached V3AssertPre.
    // [->M:N] is fully handled upstream (NFA path); only [=M:N] still lands
    // here because AstSNonConsRep lowering is not yet implemented there.
    // Replaces the node with BitFalse so lowering continues after the warning.
    منطقي rejectNonconsecRange(Ast_غوټه* nodep, Ast_غوټه_بیان* maxCountp) {
        إذا (!maxCountp) أرجع خطأ;
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: [=M:N] nonconsecutive range repetition"
                                     " (IEEE 1800-2023 16.9.2)");
        nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::بټ_False{}});
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        أرجع ریښتیا;
    }

    فراغ لیدنه(AstSGotoRep* nodep) override {
        // Standalone goto rep (not inside implication antecedent)
        iterateChildren(nodep);
        File_کرښه* ثابت flp = nodep->fileline();
        Ast_غوټه_بیان* countp = nodep->countp()->unlink_Fr_شا();
        إذا (!validate_Rep_شمېر(nodep, countp)) أرجع;
        Ast_غوټه_بیان* ثابت exprp = nodep->exprp()->unlink_Fr_شا();
        AstPExpr* ثابت pexprp = createRepPExpr(flp, exprp, countp, تش_اشاره, ریښتیا, خطأ);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }

    فراغ لیدنه(AstSNonConsRep* nodep) override {
        // Standalone nonconsecutive rep (not inside implication antecedent)
        iterateChildren(nodep);
        إذا (rejectNonconsecRange(nodep, nodep->maxCountp())) أرجع;
        File_کرښه* ثابت flp = nodep->fileline();
        Ast_غوټه_بیان* countp = nodep->countp()->unlink_Fr_شا();
        إذا (!validate_Rep_شمېر(nodep, countp)) أرجع;
        Ast_غوټه_بیان* ثابت exprp = nodep->exprp()->unlink_Fr_شا();
        AstPExpr* ثابت pexprp = createRepPExpr(flp, exprp, countp, تش_اشاره, ریښتیا, ریښتیا);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }

    فراغ لیدنه(AstFuncRef* nodep) override {
        // IEEE 1800-2023 16.8: Inline sequence instances wherever they appear
        // in the expression tree (inside implications, boolean ops, nested refs, etc.)
        إذا (AstSequence* ثابت seqp = VN_CAST(nodep->taskp(), Sequence)) {
            substituteSequenceCall(nodep, seqp);
            // The FuncRef has been replaced; do not access nodep after this point.
            // The replacement node will be visited by the parent's iterateChildren.
            أرجع;
        }
        إذا (ثابت AstProperty* ثابت propp = VN_CAST(nodep->taskp(), Property)) {
            substituteNestedPropertyCall(nodep, propp);
            أرجع;
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstImplication* nodep) override {
        إذا (nodep->sentreep()) أرجع;  // Already processed

        // Top-level followed-by is claimed by V3AssertNfa; anything reaching here
        // is nested inside iff/implies/or. IEEE 1800-2023 16.12.9 permits the
        // composition, but silent lowering would drop non-vacuous-fail semantics.
        إذا (nodep->isFollowedBy()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: followed-by (#-# / #=#) nested inside property operator"
                          " (iff/implies/or) (IEEE 1800-2023 16.12.9)");
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::بټ_False{}});
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع;
        }

        // Handle goto repetition as antecedent before iterateChildren,
        // so the standalone AstSGotoRep visitor doesn't process it
        إذا (AstSGotoRep* ثابت gotop = VN_CAST(nodep->lhsp(), SGotoRep)) {
            iterateChildren(gotop);
            iterate_And_بل_Null(nodep->rhsp());
            File_کرښه* ثابت flp = nodep->fileline();
            Ast_غوټه_بیان* countp = gotop->countp()->unlink_Fr_شا();
            إذا (!validate_Rep_شمېر(nodep, countp)) أرجع;
            Ast_غوټه_بیان* ثابت exprp = gotop->exprp()->unlink_Fr_شا();
            Ast_غوټه_بیان* ثابت rhsp = nodep->rhsp()->unlink_Fr_شا();
            AstPExpr* ثابت pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), خطأ);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع;
        }

        // Handle nonconsecutive repetition as antecedent before iterateChildren,
        // so the standalone AstSNonConsRep visitor doesn't process it
        إذا (AstSNonConsRep* ثابت ncrp = VN_CAST(nodep->lhsp(), SNonConsRep)) {
            iterateChildren(ncrp);
            iterate_And_بل_Null(nodep->rhsp());
            إذا (rejectNonconsecRange(nodep, ncrp->maxCountp())) أرجع;
            File_کرښه* ثابت flp = nodep->fileline();
            Ast_غوټه_بیان* countp = ncrp->countp()->unlink_Fr_شا();
            إذا (!validate_Rep_شمېر(nodep, countp)) أرجع;
            Ast_غوټه_بیان* ثابت exprp = ncrp->exprp()->unlink_Fr_شا();
            Ast_غوټه_بیان* ثابت rhsp = nodep->rhsp()->unlink_Fr_شا();
            AstPExpr* ثابت pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), ریښتیا);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع;
        }

        iterateChildren(nodep);

        File_کرښه* ثابت flp = nodep->fileline();
        Ast_غوټه_بیان* ثابت rhsp = nodep->rhsp()->unlink_Fr_شا();
        Ast_غوټه_بیان* lhsp = nodep->lhsp()->unlink_Fr_شا();

        // Lower sequence match items (IEEE 16.11): (expr, var = val, ...) |-> / |=>
        إذا (Ast_بیان_Stmt* ثابت بیان_Stmtp = VN_CAST(lhsp, بیان_Stmt)) {
            Ast_غوټه_بیان* ثابت antExprp = بیان_Stmtp->resultp()->unlink_Fr_شا();

            إذا (nodep->isOverlapped()) {
                // |-> : assign to __Vpropvar via always_comb (continuous).
                // The assign evaluates RHS once; V3Sampled snapshots the
                // result so all consequent refs read the same value.
                Ast_غوټه* سمون_Assignsp = تش_اشاره;
                لكل (Ast_غوټه* stmtp = بیان_Stmtp->stmtsp(); stmtp;) {
                    Ast_غوټه* ثابت nextp = stmtp->nextp();
                    إذا (Ast_ټاکل* ثابت assignp = VN_CAST(stmtp, ټاکل)) {
                        assignp->unlink_Fr_شا();
                        سمون_Assignsp = Ast_غوټه::جمع_کړه_بل_Null(سمون_Assignsp, assignp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(ټېل_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                إذا (سمون_Assignsp) {
                    AstAlways* ثابت alwaysp
                        = جديد AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, تش_اشاره, سمون_Assignsp};
                    m_modp->جمع_کړه_Stmtsp(alwaysp);
                }
            } وإلا {
                // |=> : assign to __Vpropvar via NBA in a clocked always block.
                // The NBA commits before the next cycle's sampled snapshot,
                // so the consequent (which already references __Vpropvar)
                // sees the captured value.
                Ast_غوټه* سمون_Assignsp = تش_اشاره;
                لكل (Ast_غوټه* stmtp = بیان_Stmtp->stmtsp(); stmtp;) {
                    Ast_غوټه* ثابت nextp = stmtp->nextp();
                    إذا (Ast_ټاکل* ثابت assignp = VN_CAST(stmtp, ټاکل)) {
                        assignp->unlink_Fr_شا();
                        Ast_غوټه_بیان* ثابت ټاکل_Lhsp = assignp->lhsp()->unlink_Fr_شا();
                        Ast_غوټه_بیان* ثابت ټاکل_Rhsp = assignp->rhsp()->unlink_Fr_شا();
                        Ast_ټاکل_Dly* ثابت dlyp = جديد Ast_ټاکل_Dly{flp, ټاکل_Lhsp, ټاکل_Rhsp};
                        VL_DO_DANGLING(ټېل_Deletep(assignp), assignp);
                        سمون_Assignsp = Ast_غوټه::جمع_کړه_بل_Null(سمون_Assignsp, dlyp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(ټېل_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                إذا (سمون_Assignsp) {
                    AstIf* ثابت condp
                        = جديد AstIf{flp, antExprp->clone_ونه_خالص(خطأ), سمون_Assignsp};
                    AstAlways* ثابت alwaysp
                        = جديد AstAlways{flp, VAlwaysKwd::ALWAYS, new_Sen_ونه(nodep), condp};
                    m_modp->جمع_کړه_Stmtsp(alwaysp);
                }
            }
        }

        إذا (AstPExpr* ثابت pexprp = VN_CAST(rhsp, PExpr)) {
            // Implication with sequence expression on RHS (IEEE 1800-2023 16.11, 16.12.7).
            // The PExpr was lowered from the property expression earlier in this pass.
            // Wrap the PExpr body with the antecedent check so the sequence only
            // starts when the antecedent holds.
            Ast_غوټه_بیان* condp;
            إذا (nodep->isOverlapped()) {
                // Overlapped implication (|->): check antecedent on same cycle.
                // disable iff is applied at the assertion level, not at the
                // antecedent gate, matching the existing non-PExpr overlapped path.
                condp = lhsp;
            } وإلا {
                // Non-overlapped implication (|=>): check antecedent from previous cycle
                إذا (m_disablep) {
                    lhsp
                        = جديد AstAnd{flp, جديد AstNot{flp, m_disablep->clone_ونه_خالص(خطأ)}, lhsp};
                }
                AstPast* ثابت pastp
                    = جديد AstPast{flp, lhsp, تش_اشاره, تش_اشاره, /* propertyTiming */ ریښتیا};
                pastp->dtypeFrom(lhsp);
                pastp->sentreep(new_Sen_ونه(nodep));
                condp = pastp;
            }
            // Wrap existing PExpr body: if (antecedent) { <original body> } else { /* vacuous pass
            // */ }
            // Only the statements are guarded; declarations stay directly in the block. After
            // V3Fork moves the block's statements into a task, V3Task only handles variables
            // that are direct statements of the task.
            AstBegin* ثابت bodyp = pexprp->bodyp();
            AstIf* ثابت guardp = جديد AstIf{flp, condp};
            لكل (Ast_غوټه* stmtp = bodyp->stmtsp(); stmtp;) {
                Ast_غوټه* ثابت nextp = stmtp->nextp();
                إذا (!VN_IS(stmtp, Var)) guardp->جمع_کړه_Thensp(stmtp->unlink_Fr_شا());
                stmtp = nextp;
            }
            bodyp->جمع_کړه_Stmtsp(guardp);
            nodep->replaceWith(pexprp);
            // Don't iterate pexprp here -- it was already iterated when created
            // (in visit(AstSExpr*)), so delays and disable iff are already processed.
        } وإلا إذا (nodep->isOverlapped()) {
            Ast_غوټه_بیان* ثابت exprp
                = m_underCover ? static_cast<Ast_غوټه_بیان*>(جديد AstLogAnd{flp, lhsp, rhsp})
                               : جديد AstLogOr{flp, جديد AstLogNot{flp, lhsp}, rhsp};
            nodep->replaceWith(exprp);
        } وإلا {
            إذا (m_disablep) {
                lhsp = جديد AstAnd{flp, جديد AstNot{flp, m_disablep->clone_ونه_خالص(خطأ)}, lhsp};
            }

            AstPast* ثابت pastp
                = جديد AstPast{flp, lhsp, تش_اشاره, تش_اشاره, /* propertyTiming */ ریښتیا};
            pastp->dtypeFrom(lhsp);
            pastp->sentreep(new_Sen_ونه(nodep));
            Ast_غوټه_بیان* ثابت exprp
                = m_underCover ? static_cast<Ast_غوټه_بیان*>(جديد AstAnd{flp, pastp, rhsp})
                               : جديد AstOr{flp, جديد AstNot{flp, pastp}, rhsp};
            exprp->dtype_سېټ_بټ();
            nodep->replaceWith(exprp);
        }
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(AstUntil* nodep) override {
        File_کرښه* ثابت flp = nodep->fileline();
        UASSERT_OBJ(
            !m_pexprp, nodep,
            "'" << nodep->verilogKwd()
                << "' in complex property expression should have been rejected by V3AssertNfa");
        إذا (nodep->isStrong()
            && (v3Global.opt.timing().is_سېټ_False() || !v3Global.opt.timing().is_سېټ_True())) {
            nodep->v3warn(E_NOTIMING, nodep->verilogKwd() << " requires --timing");
            nodep->replaceWith(جديد AstConst{flp, AstConst::بټ_False{}});
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع;
        }
        إذا (nodep->isStrong()) {
            // p s_until q / p s_until_with q: q must eventually be true. Until then, p must
            // be true on every sampled tick. For s_until, check q first: when q is true on
            // this tick, p is not required. For s_until_with, p must be true on the q tick too.
            Ast_غوټه_بیان* ثابت rawLhsp = nodep->lhsp()->unlink_Fr_شا();
            Ast_غوټه_بیان* ثابت rawRhsp = nodep->rhsp()->unlink_Fr_شا();
            AstSampled* ثابت lhsp = جديد AstSampled{flp, rawLhsp, rawLhsp->dtypep(), ریښتیا};
            AstSampled* ثابت rhsp = جديد AstSampled{flp, rawRhsp, rawRhsp->dtypep(), ریښتیا};
            Ast_غوټه_بیان* finalCondp = rhsp->clone_ونه_خالص(خطأ);
            إذا (nodep->isOverlapping()) {
                finalCondp = جديد AstLogAnd{flp, lhsp->clone_ونه_خالص(خطأ), finalCondp};
            }

            // Track active assertion attempts. Only the count is needed to emit final failures.
            AstVar* ثابت activep = جديد AstVar{flp, VVarType::MODULETEMP, m_فعال_نومونه.get(""),
                                               nodep->پیدا_کړه_Basic_DType(VBasicDTypeKwd::UINT32)};
            activep->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->جمع_کړه_Stmtsp(activep);

            AstVar* ثابت donep
                = جديد AstVar{flp, VVarType::BLOCKTEMP, "__VassertDone", nodep->پیدا_کړه_بټ_DType()};
            donep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            خپلکاری سېټ_بشپړ = [&]() {
                أرجع جديد Ast_ټاکل{flp, جديد AstVarRef{flp, donep, VAccess::WRITE},
                                     جديد AstConst{flp, AstConst::بټ_True{}}};
            };

            AstLoop* ثابت loopp = جديد AstLoop{flp};
            Ast_ټاکل* ثابت decrementVar = جديد Ast_ټاکل{
                flp, جديد AstVarRef{flp, activep, VAccess::WRITE},
                جديد AstSub{flp, جديد AstVarRef{flp, activep, VAccess::READ}, جديد AstConst{flp, 1}}};
            loopp->جمع_کړه_Stmtsp(جديد AstLoopTest{
                flp, loopp, جديد AstLogNot{flp, جديد AstVarRef{flp, donep, VAccess::READ}}});
            {
                Ast_غوټه* ثابت passp = جديد AstPExprClause{flp};
                passp->جمع_کړه_بل(decrementVar);
                passp->جمع_کړه_بل(سېټ_بشپړ());
                Ast_غوټه_بیان* passCondp = rhsp;
                إذا (nodep->isOverlapping()) {
                    passCondp = جديد AstLogAnd{flp, lhsp->clone_ونه_خالص(خطأ), passCondp};
                }
                loopp->جمع_کړه_Stmtsp(جديد AstIf{flp, passCondp, passp});
            }
            {
                Ast_غوټه* ثابت failp = جديد AstPExprClause{flp, خطأ};
                failp->جمع_کړه_بل(decrementVar->clone_ونه(خطأ));
                failp->جمع_کړه_بل(سېټ_بشپړ());
                loopp->جمع_کړه_Stmtsp(جديد AstIf{
                    flp,
                    جديد AstLogAnd{flp,
                                  جديد AstLogNot{flp, جديد AstVarRef{flp, donep, VAccess::READ}},
                                  جديد AstLogNot{flp, lhsp}},
                    failp});
            }
            AstDelay* ثابت delayp = جديد AstDelay{flp, جديد AstConst{flp, 1}, خطأ};
            delayp->timeunit(m_modp->timeunit());
            loopp->جمع_کړه_Stmtsp(جديد AstIf{
                flp, جديد AstLogNot{flp, جديد AstVarRef{flp, donep, VAccess::READ}}, delayp});

            // Main assertion block
            AstBegin* ثابت bodyp = جديد AstBegin{flp, m_بلاک_نومونه.get(""), تش_اشاره, ریښتیا};
            bodyp->جمع_کړه_Stmtsp(donep);
            bodyp->جمع_کړه_Stmtsp(جديد Ast_ټاکل{flp, جديد AstVarRef{flp, donep, VAccess::WRITE},
                                           جديد AstConst{flp, AstConst::بټ_False{}}});
            File_کرښه* ثابت flp = activep->fileline();
            bodyp->جمع_کړه_Stmtsp(
                جديد Ast_ټاکل{flp, جديد AstVarRef{flp, activep, VAccess::WRITE},
                              جديد Ast_جمع_کړه{flp, جديد AstVarRef{flp, activep, VAccess::READ},
                                         جديد AstConst{flp, 1}}});
            bodyp->جمع_کړه_Stmtsp(loopp);

            // Validate assertion condition for each active assert
            AstVar* ثابت فعال_Countp = جديد AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                    nodep->پیدا_کړه_Basic_DType(VBasicDTypeKwd::UINT32)};
            فعال_Countp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

            Ast_ټاکل* ثابت init_فعال_Countp
                = جديد Ast_ټاکل{flp, جديد AstVarRef{flp, فعال_Countp, VAccess::WRITE},
                                جديد AstVarRef{flp, activep, VAccess::READ}};
            AstLoop* ثابت finalLoopp = جديد AstLoop{flp};
            finalLoopp->جمع_کړه_Stmtsp(
                جديد AstLoopTest{flp, finalLoopp,
                                جديد AstNeq{flp, جديد AstVarRef{flp, فعال_Countp, VAccess::READ},
                                           جديد AstConst{flp, 0}}});
            finalLoopp->جمع_کړه_Stmtsp(جديد AstIf{flp, finalCondp, جديد AstPExprClause{flp},
                                            جديد AstPExprClause{flp, خطأ}});
            finalLoopp->جمع_کړه_Stmtsp(
                جديد Ast_ټاکل{flp, جديد AstVarRef{flp, فعال_Countp, VAccess::WRITE},
                              جديد AstSub{flp, جديد AstVarRef{flp, فعال_Countp, VAccess::READ},
                                         جديد AstConst{flp, 1}}});

            // Final assertion block
            AstBegin* ثابت finalp = جديد AstBegin{flp, m_بلاک_نومونه.get(""), تش_اشاره, ریښتیا};
            finalp->جمع_کړه_Stmtsp(فعال_Countp);
            finalp->جمع_کړه_Stmtsp(init_فعال_Countp);
            finalp->جمع_کړه_Stmtsp(finalLoopp);

            AstPExpr* ثابت pexprp = جديد AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
            VL_RESTORER(m_pexprp);
            VL_RESTORER(m_has_دوره_Delay);
            m_pexprp = pexprp;
            m_has_دوره_Delay = خطأ;
            iterate(bodyp);
            iterate(finalp);

            UASSERT_OBJ(!m_has_دوره_Delay, nodep,
                        "s_until cycle delay should have been handled by V3AssertNfa");

            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع;
        }
        AstLoop* ثابت loopp = جديد AstLoop{flp};
        Ast_غوټه_بیان* ثابت rhsp = nodep->rhsp()->unlink_Fr_شا();
        Ast_غوټه_بیان* ثابت lhsp = nodep->lhsp()->unlink_Fr_شا();
        AstLogAnd* ثابت loopCondp = جديد AstLogAnd{flp, lhsp, جديد AstLogNot{flp, rhsp}};
        loopp->جمع_کړه_Stmtsp(جديد AstLoopTest{flp, loopp, loopCondp});
        loopp->جمع_کړه_Stmtsp(جديد AstEventControl{flp, new_Sen_ونه(nodep), تش_اشاره});

        Ast_غوټه_بیان* ثابت rhsCopyp = rhsp->clone_ونه_خالص(خطأ);
        Ast_غوټه_بیان* ثابت passCondp
            = nodep->isOverlapping() ? جديد AstLogAnd{flp, lhsp->clone_ونه_خالص(خطأ), rhsCopyp}
                                     : rhsCopyp;
        AstBegin* ثابت beginp = جديد AstBegin{flp, m_بلاک_نومونه.get(""), loopp, ریښتیا};
        beginp->جمع_کړه_Stmtsp(
            جديد AstIf{flp, passCondp, جديد AstPExprClause{flp}, جديد AstPExprClause{flp, خطأ}});

        AstPExpr* ثابت pexprp = جديد AstPExpr{flp, beginp, nodep->dtypep()};
        pexprp->user1(1);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }

    فراغ لیدنه(AstDefaultDisable* nodep) override {
        VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
    }
    فراغ لیدنه(AstInferredDisable* nodep) override {
        Ast_غوټه* newp;
        إذا (m_defaultDisablep) {
            newp = m_defaultDisablep->condp()->clone_ونه_خالص(ریښتیا);
        } وإلا {
            newp = جديد AstConst{nodep->fileline(), AstConst::بټ_False{}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }

    فراغ لیدنه(AstPropSpec* nodep) override {
        nodep = substitutePropertyCall(nodep);
        iterate_And_بل_Null(nodep->sensesp());
        إذا (m_senip && m_senip != nodep->sensesp())
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Only one PSL clock allowed per assertion");
        ثابت AstCover* ثابت coverp = VN_CAST(nodep->backp(), Cover);
        ثابت منطقي seqEvent = coverp && coverp->isSeqEvent();
        إذا (!nodep->disablep() && m_defaultDisablep && !seqEvent) {
            nodep->disablep(m_defaultDisablep->condp()->clone_ونه_خالص(ریښتیا));
        }
        m_disablep = nodep->disablep();
        // Unlink and just keep a pointer to it, convert to sentree as needed
        m_senip = nodep->sensesp();
        iterateNull(nodep->disablep());
        إذا (!VN_AS(nodep->backp(), غوټه_Cover_Or_Assert)->سمدستي()) {
            ثابت Ast_غوټه_DType* ثابت propDtp = nodep->propp()->dtypep();
            nodep->propp(جديد AstSampled{nodep->fileline(), nodep->propp()->unlink_Fr_شا(),
                                        propDtp->dtypep(), ریښتیا});
        }
        // cover counts non-vacuous matches only (IEEE 1800-2023 16.15.2), so an
        // implication antecedent must hold; assert passes vacuously instead.
        VL_RESTORER(m_underCover);
        m_underCover = VN_IS(nodep->backp(), Cover);
        iterate(nodep->propp());
        iterateNull(nodep->سمون_Countp());
    }
    فراغ لیدنه(AstPExpr* nodep) override {
        // V3AssertNfa handles multi-cycle property expressions before this pass,
        // so the following unsupported paths are defensive and typically unreached.
        إذا (m_pexprp && m_pexprp->user1()) {  // LCOV_EXCL_START
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Complex property expression inside 'until''");
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::بټ_False{}});
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع;
        }  // LCOV_EXCL_STOP
        إذا (AstLogNot* ثابت notp = VN_CAST(nodep->backp(), LogNot)) {
            notp->replaceWith(nodep->unlink_Fr_شا());
            VL_DO_DANGLING(ټېل_Deletep(notp), notp);
            iterate(nodep);
            أرجع;
        }
        // Sequence expression as antecedent of implication is not yet supported
        إذا (AstImplication* ثابت implp = VN_CAST(nodep->backp(), Implication)) {
            إذا (implp->lhsp() == nodep) {  // LCOV_EXCL_START
                implp->v3warn(E_UNSUPPORTED,
                              "Unsupported: Implication with sequence expression as antecedent");
                nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::بټ_False{}});
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                أرجع;
            }  // LCOV_EXCL_STOP
        }
        VL_RESTORER(m_pexprp);
        VL_RESTORER(m_disableSeqIfp);
        m_pexprp = nodep;

        إذا (m_disablep) {
            ثابت AstSampled* sampledp = تش_اشاره;
            إذا (m_disablep->exists([&sampledp](ثابت AstSampled* ثابت sp) {
                    sampledp = sp;
                    أرجع ریښتیا;
                })) {
                sampledp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: $sampled inside disabled condition of a sequence");
                m_disablep = جديد AstConst{m_disablep->fileline(), AstConst::بټ_False{}};
                // always a copy is used, so remove it now
                ټېل_Deletep(m_disablep);  // m_disablep used later
            }
            File_کرښه* ثابت flp = nodep->fileline();
            // Add counter which counts times the condition turned true
            AstVar* ثابت disableCntp
                = جديد AstVar{flp, VVarType::MODULETEMP, m_disable_Cnt_نومونه.get(""),
                             nodep->پیدا_کړه_Basic_DType(VBasicDTypeKwd::UINT32)};
            disableCntp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->جمع_کړه_Stmtsp(disableCntp);
            AstVarRef* ثابت readCntRefp = جديد AstVarRef{flp, disableCntp, VAccess::READ};
            AstVarRef* ثابت writeCntRefp = جديد AstVarRef{flp, disableCntp, VAccess::WRITE};
            Ast_ټاکل* ثابت incrStmtp = جديد Ast_ټاکل{
                flp, writeCntRefp, جديد Ast_جمع_کړه{flp, readCntRefp, جديد AstConst{flp, 1}}};
            AstAlways* ثابت alwaysp
                = جديد AstAlways{flp, VAlwaysKwd::ALWAYS,
                                جديد Ast_Sen_ونه{flp, جديد Ast_Sen_توکی{flp, VEdgeType::ET_POSEDGE,
                                                                   m_disablep->clone_ونه(خطأ)}},
                                incrStmtp};
            disableCntp->جمع_کړه_بل_دلته(alwaysp);

            // Store value of that counter at the beginning of sequence evaluation
            AstBegin* ثابت bodyp = nodep->bodyp();
            AstVar* ثابت initialCntp = جديد AstVar{flp, VVarType::BLOCKTEMP, "__VinitialCnt",
                                                   nodep->پیدا_کړه_Basic_DType(VBasicDTypeKwd::UINT32)};
            initialCntp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            Ast_ټاکل* ثابت assignp
                = جديد Ast_ټاکل{flp, جديد AstVarRef{flp, initialCntp, VAccess::WRITE},
                                readCntRefp->clone_ونه(خطأ)};
            // Prepend to the sequence body to keep statement list structure valid.
            UASSERT_OBJ(!bodyp->name().تشه(), bodyp, "Sequence body should be named");
            Ast_غوټه* ثابت newStmtsp = Ast_غوټه::جمع_کړه_بل<Ast_غوټه, Ast_غوټه>(initialCntp, assignp);
            bodyp->prependStmtsp(newStmtsp);

            m_disableSeqIfp
                = جديد AstIf{flp, جديد AstEq{flp, جديد AstVarRef{flp, initialCntp, VAccess::READ},
                                           readCntRefp->clone_ونه(خطأ)}};
            // Delete it, because it is always copied before insetion to the AST
            ټېل_Deletep(m_disableSeqIfp);  // m_disableSeqIfp used later
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultClkEvtVarp);
        VL_RESTORER(m_defaultDisablep);
        VL_RESTORER(m_modp);
        إذا (!VN_IS(nodep, Class)) {
            // A class nested in a module is in the scope of the module's default clocking
            m_defaultClockingp = تش_اشاره;
            m_defaultClkEvtVarp = تش_اشاره;
        }
        m_defaultDisablep = nodep->defaultDisablep();
        m_modp = nodep;
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_Gen_بلاک* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstProperty* nodep) override {
        // The body will be visited when will be substituted in place of property reference
        // (AstFuncRef)
        VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
    }
    فراغ لیدنه(AstSequence* nodep) override {
        // Sequence declarations are not visited directly; their bodies are inlined
        // at call sites by visit(AstFuncRef*). Collect for post-traversal cleanup.
        m_seqs_منزل_Cleanup.ټېل_شا(nodep);
    }
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    څرګند AssertPreVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        { AssertPreClockingVisitor{nodep}; }
        // Process
        iterate(nodep);
        // Fix up varref names
        لكل (AstVarXRef* xrefp : m_xrefsp) xrefp->name(xrefp->varp()->name());
        // Clean up sequence declarations after inlining.
        // Referenced sequences that were inlined have isReferenced cleared.
        // Remaining referenced sequences are in unsupported contexts (e.g. @seq event).
        لكل (AstSequence* seqp : m_seqs_منزل_Cleanup) {
            إذا (seqp->isReferenced()) {
                seqp->v3warn(E_UNSUPPORTED,
                             "Unsupported: sequence referenced outside assertion property");
            } وإلا {
                VL_DO_DANGLING(seqp->unlink_Fr_شا()->delete_ونه(), seqp);
            }
        }
    }
    ~AssertPreVisitor() override = افتراضي;
};

//######################################################################
// Top Assert class

فراغ V3AssertPre::assertPreAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { AssertPreVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ونه("assertpre", 0, dump_ونه_Either_Level() >= 3);
}
