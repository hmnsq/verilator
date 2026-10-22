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
    فراغ گهمو(AstNetlist* nodep) override { iterate_And_ايندڙ_Const_Null(nodep->modulesp()); }
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override { iterate_And_ايندڙ_Const_Null(nodep->stmtsp()); }
    فراغ گهمو(Ast_Gen_بلاڪ* nodep) override { iterate_And_ايندڙ_Const_Null(nodep->itemsp()); }
    فراغ گهمو(AstClocking* nodep) override {
        لكل (Ast_ڳنڍ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            إذا (Ast_Clocking_شي* ثابت citemp = VN_CAST(itemp, Clocking_شي)) {
                citemp->user3p(nodep);
            }
        }
    }
    // Clocking blocks are only in modules and their generate blocks
    فراغ گهمو(Ast_ڳنڍ*) override {}

عام:
    // CONSTRUCTORS
    واضح AssertPreClockingVisitor(AstNetlist* nodep) { iterateConst(nodep); }
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
        Ast_Clocking_شي* itemp;  // Clocking item of the driven clockvar
        Ast_ڳنڍ_اظهار* refp;  // Reference to the driven clockvar
    };

    // NODE STATE
    // AstClockingItem::user1p()         // AstVar*.      varp() of ClockingItem after unlink
    // AstClockingItem::user2p()         // AstVar*.      Flag set by drives of output clockvar
    // AstClockingItem::user3p()         // AstClocking*. Clocking block of the item, captured
    //                                   //               by AssertPreClockingVisitor
    // AstPExpr::user1()                 // bool.         Created from AstUntil
    ثابت VNUser1_اندريون_Use m_inuser1;
    ثابت VNUser2_اندريون_Use m_inuser2;
    ثابت VNUser3_اندريون_Use m_inuser3;
    // STATE
    // Current context:
    AstNetlist* ثابت m_netlistp = خالي_اشارو;  // Current netlist
    Ast_ڳنڍ_Module* m_modp = خالي_اشارو;  // Current module
    AstClocking* m_clockingp = خالي_اشارو;  // Current clocking block
    // Reset each module:
    AstClocking* m_defaultClockingp = خالي_اشارو;  // Default clocking for current module
    AstVar* m_defaultClkEvtVarp = خالي_اشارو;  // Event var for default clocking (for ##0)
    AstDefaultDisable* m_defaultDisablep = خالي_اشارو;  // Default disable for current module
    // Reset each assertion:
    Ast_Sen_شي* m_senip = خالي_اشارو;  // Last sensitivity
    // Reset each always:
    Ast_Sen_شي* m_seniAlwaysp = خالي_اشارو;  // Last sensitivity in always
    // Reset each assertion:
    Ast_ڳنڍ_اظهار* m_disablep = خالي_اشارو;  // Last disable
    AstIf* m_disableSeqIfp = خالي_اشارو;  // Used for handling disable iff in sequences
    AstPExpr* m_pexprp = خالي_اشارو;  // Last AstPExpr
    منطقي m_underCover = خطأ;  // True if the enclosing assertion is a cover
    // Other:
    V3_Unique_نالا m_چڪر_Dly_نالا{"__VcycleDly"};  // Cycle delay counter name generator
    V3_Unique_نالا m_cons_Rep_نالا{"__VconsRep"};  // Consecutive repetition counter name generator
    V3_Unique_نالا m_goto_Rep_نالا{"__VgotoRep"};  // Goto repetition counter name generator
    V3_Unique_نالا m_non_Cons_Rep_نالا{"__VnonConsRep"};  // Nonconsecutive rep name generator
    V3_Unique_نالا m_disable_Cnt_نالا{"__VdisableCnt"};  // Disable condition counter name generator
    V3_Unique_نالا m_بلاڪ_نالا{"__VassertBlock"};  // Names of blocks with temporaries
    V3_Unique_نالا m_prop_Var_نالا{"__Vpropvar"};  // Property-local variable name generator
    V3_Unique_نالا m_سرگرم_نالا{"__VassertsActive"};  // Active asserts map name generator
    V3_Unique_نالا m_driven_نالا{"__VclockingDriven"};  // Clockvar drive flag name generator
    منطقي m_اندريون_سونپيو = خطأ;  // True if in an AssignNode
    منطقي m_اندريون_سونپيو_Dly_Lhs = خطأ;  // True if in AssignDly's LHS
    منطقي m_اندريون_Synch_Drive = خطأ;  // True if in synchronous drive
    منطقي m_اندريون_Concat_سونپيو = خطأ;  // True if in assignment to a concatenation
    std::ويڪٽر<SynchDrive> m_drives;  // Clockvars written by the synchronous drive
    منطقي m_has_چڪر_Delay = خطأ;  // True if node has cycle delay beneath
    std::ويڪٽر<AstVarXRef*> m_xrefsp;  // list of xrefs that need name fixup
    std::ويڪٽر<AstSequence*> m_seqs_منزل_Cleanup;  // Sequences to clean up after traversal

    // METHODS

    // Flag set by each drive of an output clockvar, added to the module with its clocking item
    AstVar* getCreateDrivenVarp(Ast_Clocking_شي* itemp) {
        إذا (!itemp->user2p()) {
            AstVar* ثابت varp = جديد AstVar{itemp->fileline(), VVarType::MODULETEMP,
                                            m_driven_نالا.get(itemp), itemp->ڳوليو_ٻٽو_انگ_DType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            itemp->user2p(varp);
        }
        أرجع VN_AS(itemp->user2p(), Var);
    }
    // Reference to a variable in the scope of a driven clockvar, made like the clockvar's
    ساكن Ast_ڳنڍ_اظهار* newDriveRefp(File_سٽ* flp, ثابت SynchDrive& drive, AstVar* varp,
                                     ثابت VAccess& access) {
        إذا (ثابت AstVarXRef* ثابت xrefp = VN_CAST(drive.refp, VarXRef)) {
            أرجع جديد AstVarXRef{flp, varp, xrefp->dotted(), access};
        } وإلا إذا (ثابت AstMemberSel* ثابت selp = VN_CAST(drive.refp, MemberSel)) {
            // The interface expression is evaluated again, so it must not have side effects;
            // cloneTreePure warns if it has any
            AstMemberSel* ثابت newSelp
                = جديد AstMemberSel{flp, selp->fromp()->clone_وڻ_خالص(خطأ), varp};
            newSelp->access(access);
            أرجع newSelp;
        }
        أرجع جديد AstVarRef{flp, varp, access};
    }
    // Assignment setting the drive flag of a driven clockvar
    Ast_سونپيو* newDrivenSetp(File_سٽ* flp, ثابت SynchDrive& drive) {
        AstVar* ثابت varp = getCreateDrivenVarp(drive.itemp);
        Ast_سونپيو* ثابت setp = جديد Ast_سونپيو{flp, newDriveRefp(flp, drive, varp, VAccess::WRITE),
                                              جديد AstConst{flp, AstConst::ٻٽو_انگ_True{}}};
        setp->user1(سچ);
        أرجع setp;
    }
    // Clocking block of a clocking item
    ساكن AstClocking* clocking_جو(ثابت Ast_Clocking_شي* itemp) {
        UASSERT_OBJ(itemp->user3p(), itemp, "Clocking item not captured in a clocking block");
        أرجع VN_AS(itemp->user3p(), Clocking);
    }
    // Clocking event of a driven clockvar, referenced from the drive like the clockvar, or
    // nullptr if unsupported, which is reported on the drive's cycle delay
    Ast_Sen_وڻ* newDriveSentreep(AstDelay* delayp, ثابت SynchDrive& drive) {
        Ast_Sen_وڻ* ثابت origp = clocking_جو(drive.itemp)->sentreep();
        إذا (VN_IS(drive.refp, VarRef)) أرجع origp->clone_وڻ(خطأ);
        إذا (origp->exists([](ثابت AstVarXRef*) { أرجع سچ; })) {
            // The reference is relative to the clocking block
            delayp->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in synchronous drive to"
                                          " clockvar in another scope, whose clocking event"
                                          " has a hierarchical reference");
            أرجع خالي_اشارو;
        }
        ثابت AstMemberSel* ثابت selp = VN_CAST(drive.refp, MemberSel);
        // Processes outside classes evaluate their events also when not waiting on them,
        // possibly before the interface reference is set, so the event is guarded against a
        // null reference, which needs an integral value; scheduling does not guard it
        ثابت منطقي guardNull = selp && !VN_IS(m_modp, Class);
        إذا (guardNull) {
            لكل (Ast_Sen_شي* itemp = origp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Sen_شي)) {
                إذا (!itemp->sensp()->dtypep()->skipRefp()->isIntegralOrPacked()) {
                    delayp->v3warn(E_UNSUPPORTED,
                                   "Unsupported: cycle delay in synchronous drive through"
                                   " an interface reference outside a class, to clocking"
                                   " block with non-integral clocking event");
                    أرجع خالي_اشارو;
                }
            }
        }
        // Interface declaring the clocking block, if driven through an interface reference
        AstIface* ثابت ifacep
            = selp ? VN_AS(selp->fromp()->dtypep()->skipRefp(), IfaceRefDType)->iface_ذريعي_Cellp()
                   : خالي_اشارو;
        Ast_Sen_وڻ* ثابت sentreep = origp->clone_وڻ(خطأ);
        std::ويڪٽر<AstVarRef*> refps;
        sentreep->foreach([&](AstVarRef* refp) {
            إذا (!refp->classOrPackagep()) refps.ڌڪو_پٺ(refp);
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
            VL_DO_DANGLING(ڌڪو_Deletep(refp), refp);
        }
        إذا (guardNull) {
            لكل (Ast_Sen_شي* itemp = sentreep->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Sen_شي)) {
                guardNullIface(itemp, selp->fromp());
            }
        }
        أرجع sentreep;
    }
    // Make an integral clocking event through an interface reference false while the reference
    // is null
    ساكن فراغ guardNullIface(Ast_Sen_شي* sensesp, Ast_ڳنڍ_اظهار* fromp) {
        File_سٽ* ثابت flp = sensesp->fileline();
        Ast_ڳنڍ_اظهار* ثابت sensp = sensesp->sensp()->unlink_Fr_پٺ();
        sensesp->sensp(جديد AstCond{flp, newNotNullp(flp, fromp), sensp,
                                   جديد AstConst{flp, AstConst::DTyped{}, sensp->dtypep()}});
        إذا (Ast_ڳنڍ_اظهار* ثابت condp = sensesp->condp()) {
            condp->unlink_Fr_پٺ();
            sensesp->condp(جديد AstLogAnd{flp, newNotNullp(flp, fromp), condp});
        }
    }
    ساكن Ast_ڳنڍ_اظهار* newNotNullp(File_سٽ* flp, Ast_ڳنڍ_اظهار* fromp) {
        أرجع جديد AstNeq{flp, fromp->clone_وڻ_خالص(خطأ), جديد AstConst{flp, AstConst::Null{}}};
    }
    // Procedural '##0': if the default clocking event has not yet occurred in this time step,
    // wait for it; otherwise continue without suspension (IEEE 1800-2023 14.11)
    AstIf* new_چڪر_Delay_Zerop(File_سٽ* flp) ثابت {
        AstVar* ثابت evtVarp = m_defaultClkEvtVarp;
        UASSERT_OBJ(evtVarp, flp, "Default clocking event var not pre-created");
        AstCMethodHard* ثابت isTriggeredp = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, evtVarp, VAccess::READ}, VCMethod::EVENT_IS_TRIGGERED};
        isTriggeredp->dtype_سيٽ_ٻٽو_انگ();
        AstEventControl* ثابت waitp = جديد AstEventControl{
            flp,
            جديد Ast_Sen_وڻ{flp, جديد Ast_Sen_شي{flp, VEdgeType::ET_EVENT,
                                               جديد AstVarRef{flp, evtVarp, VAccess::READ}}},
            خالي_اشارو};
        أرجع جديد AstIf{flp, جديد AstNot{flp, isTriggeredp}, waitp};
    }

    ساكن فراغ checkSamplingFuncDType(Ast_ڳنڍ_اظهار* nodep, ثابت Ast_ڳنڍ* exprp) {
        ثابت Ast_ڳنڍ_DType* ثابت dtypep = exprp->dtypep()->skipRefp();
        إذا (!dtypep->isIntegralOrPacked()) {
            nodep->v3error("Expected numeric type, but got a " << dtypep->prettyDTypeNameQ()
                                                               << " data type");
        }
    }

    Ast_Sen_وڻ* new_Sen_وڻ(Ast_ڳنڍ* nodep, Ast_Sen_وڻ* useTreep = خالي_اشارو,
                           Ast_ڳنڍ_Cover_Or_Assert* cassertp = خالي_اشارو) {
        // Create sentree based on clocked or default clock
        // Return nullptr for always
        إذا (useTreep) أرجع useTreep;
        Ast_Sen_وڻ* newp = خالي_اشارو;
        Ast_Sen_شي* senip = m_senip;
        منطقي fromAlways = خطأ;
        إذا (!senip && m_defaultClockingp) senip = m_defaultClockingp->sentreep()->sensesp();
        إذا (!senip) {
            senip = m_seniAlwaysp;
            fromAlways = سچ;
        }
        إذا (!senip) {
            nodep->v3error("Concurrent assertion has no clock (IEEE 1800-2023 16.16)\n"
                           << nodep->warn_وڌيڪ()
                           << "... Suggest provide a clocking event, a default"
                              " clocking, or a clocked procedural context");
            newp = جديد Ast_Sen_وڻ{nodep->fileline(), خالي_اشارو};
        } وإلا {
            إذا (cassertp && fromAlways) cassertp->senFromAlways(سچ);
            newp = جديد Ast_Sen_وڻ{nodep->fileline(), senip->clone_وڻ(سچ)};
        }
        أرجع newp;
    }
    Ast_ڳنڍ_اظهار* getSequenceBodyExprp(ثابت AstSequence* ثابت seqp) ثابت {
        // The statements in AstSequence are optional AstVar (ports) followed by body expr.
        Ast_ڳنڍ* bodyp = seqp->stmtsp();
        طالما (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        أرجع VN_CAST(bodyp, ڳنڍ_اظهار);
    }
    AstPropSpec* getPropertyExprp(ثابت AstProperty* ثابت propp) {
        // Statements in AstProperty: AstVar (ports/local vars),
        // AstInitialStaticStmt/AstInitialAutomaticStmt (var init), AstPropSpec (body).
        Ast_ڳنڍ* propExprp = propp->stmtsp();
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
        Ast_ڳنڍ_اظهار* bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        // Clone the body expression since the sequence may be referenced multiple times
        Ast_ڳنڍ_اظهار* clonedp = bodyExprp->clone_وڻ(خطأ);
        // Build substitution map, then do a single traversal to replace all formals
        // (textual substitution per IEEE 16.8.2).
        ثابت V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<ثابت AstVar*, Ast_ڳنڍ_اظهار*> پورٽ_Map;
        لكل (ثابت خودڪار& tconnect : tconnects) {
            پورٽ_Map[tconnect.پهريون] = tconnect.ٻيو->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            ثابت خودڪار it = پورٽ_Map.ڳوليو(refp->varp());
            إذا (it != پورٽ_Map.پڇاڙي()) {
                refp->replaceWith(it->ٻيو->clone_وڻ(خطأ));
                VL_DO_DANGLING(ڌڪو_Deletep(refp), refp);
            }
        });
        لكل (ثابت خودڪار& tconnect : tconnects) {
            ڌڪو_Deletep(tconnect.ٻيو->exprp()->unlink_Fr_پٺ());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        // Replace the FuncRef with the inlined body
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(ڌڪو_Deletep(funcrefp), funcrefp);
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
        propExprp = propExprp->clone_وڻ(خطأ);
        // Build substitution maps for formal arguments and property-local
        // variables, then perform a single foreach to apply all replacements.
        // Map port vars to their actual argument expressions
        ثابت V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propp->stmtsp());
        std::unordered_map<ثابت AstVar*, Ast_ڳنڍ_اظهار*> پورٽ_Map;
        لكل (ثابت خودڪار& tconnect : tconnects) {
            پورٽ_Map[tconnect.پهريون] = tconnect.ٻيو->exprp();
        }

        // Promote property-local variables (non-port vars, IEEE 16.10) to
        // module-level __Vpropvar temps. Cross-cycle persistence is handled
        // by the match item lowering in visit(AstImplication*).
        std::unordered_map<ثابت AstVar*, AstVar*> localVarMap;
        لكل (Ast_ڳنڍ* stmtp = propp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (!varp->isIO()) {
                    ثابت string newName = m_prop_Var_نالا.get(varp);
                    AstVar* ثابت newVarp = جديد AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->جوڙيو_Stmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        // Single traversal: substitute ports and update local var references
        propExprp->foreach([&](AstVarRef* refp) {
            ثابت خودڪار پورٽ_It = پورٽ_Map.ڳوليو(refp->varp());
            إذا (پورٽ_It != پورٽ_Map.پڇاڙي()) {
                refp->replaceWith(پورٽ_It->ٻيو->clone_وڻ(خطأ));
                VL_DO_DANGLING(ڌڪو_Deletep(refp), refp);
                أرجع;
            }
            ثابت خودڪار localIt = localVarMap.ڳوليو(refp->varp());
            إذا (localIt != localVarMap.پڇاڙي()) refp->varp(localIt->ٻيو);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        // Clean up argument expressions
        لكل (ثابت خودڪار& tconnect : tconnects) {
            ڌڪو_Deletep(tconnect.ٻيو->exprp()->unlink_Fr_پٺ());
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
                    ڌڪو_Deletep(propExprp->disablep()->unlink_Fr_پٺ());
                }
                // If disable iff is in outer property, move it to inner
                إذا (nodep->disablep()) {
                    Ast_ڳنڍ_اظهار* ثابت disablep = nodep->disablep()->unlink_Fr_پٺ();
                    propExprp->disablep(disablep);
                }

                إذا (nodep->sensesp() && propExprp->sensesp()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Clock event before property call and in its body");
                    ڌڪو_Deletep(propExprp->sensesp()->unlink_Fr_پٺ());
                }
                // If clock event is in outer property, move it to inner
                إذا (nodep->sensesp()) {
                    Ast_Sen_شي* ثابت sensesp = nodep->sensesp();
                    sensesp->unlink_Fr_پٺ();
                    propExprp->sensesp(sensesp);
                }

                // Now substitute property reference with property body
                nodep->replaceWith(propExprp);
                VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
                أرجع propExprp;
            }
        }
        أرجع nodep;
    }

    فراغ substituteNestedPropertyCall(AstFuncRef* funcrefp, ثابت AstProperty* propp) {
        AstPropSpec* ثابت specp = clonePropertyBody(funcrefp, propp);
        Ast_ڳنڍ* replp = خالي_اشارو;
        إذا (specp->disablep()) {
            funcrefp->v3error("Illegal 'disable iff' in a nested property instance"
                              " (IEEE 1800-2023 16.12.1)");
        } وإلا إذا (specp->sensesp()) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: clock event in a nested property instance");
        } وإلا إذا (specp->propp()->exists(
                       [](ثابت Ast_ڳنڍ_اظهار* np) { أرجع np->is_Multi_چڪر_Sva(); })) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: multi-cycle body in a nested property instance");
        } وإلا {
            replp = specp->propp()->unlink_Fr_پٺ();
        }
        إذا (!replp) replp = جديد AstConst{funcrefp->fileline(), AstConst::ٻٽو_انگ_False{}};
        funcrefp->replaceWith(replp);
        VL_DO_DANGLING(ڌڪو_Deletep(funcrefp), funcrefp);
        VL_DO_DANGLING(ڌڪو_Deletep(specp), specp);
    }

    // VISITORS
    //========== Statements
    فراغ گهمو(AstClocking* ثابت nodep) override {
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
        إذا (nodep->eventp()) nodep->جوڙيو_ايندڙ_هتي(nodep->eventp()->unlink_Fr_پٺ());
        VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
    }
    فراغ گهمو(AstModportClockingRef* ثابت nodep) override {
        // It has to be converted to a list of ModportClockingVarRefs,
        // because clocking blocks are removed in this pass
        لكل (Ast_ڳنڍ* itemp = nodep->clockingp()->itemsp(); itemp; itemp = itemp->nextp()) {
            إذا (Ast_Clocking_شي* ثابت citemp = VN_CAST(itemp, Clocking_شي)) {
                إذا (AstVar* ثابت varp
                    = citemp->varp() ? citemp->varp() : VN_AS(citemp->user1p(), Var)) {
                    AstModportVarRef* ثابت modVarp = جديد AstModportVarRef{
                        nodep->fileline(), varp->name(), citemp->direction()};
                    modVarp->varp(varp);
                    nodep->جوڙيو_ايندڙ_هتي(modVarp);
                    إذا (citemp->direction().isOutput()) {
                        // Drives through the modport set the drive flag
                        AstVar* ثابت drivenVarp = getCreateDrivenVarp(citemp);
                        AstModportVarRef* ثابت drivenRefp = جديد AstModportVarRef{
                            nodep->fileline(), drivenVarp->name(), VDirection::OUTPUT};
                        drivenRefp->varp(drivenVarp);
                        nodep->جوڙيو_ايندڙ_هتي(drivenRefp);
                    }
                }
            }
        }
        VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
    }
    فراغ گهمو(Ast_Clocking_شي* ثابت nodep) override {
        // Get a ref to the sampled/driven variable
        AstVar* ثابت varp = nodep->varp();
        إذا (!varp) {
            // Unused item
            أرجع;
        }
        // Flag set by drives of an output clockvar, possibly visited before this item
        إذا (nodep->direction().isOutput()) m_modp->جوڙيو_Stmtsp(getCreateDrivenVarp(nodep));
        File_سٽ* ثابت flp = nodep->fileline();
        V3Const::constifyEdit(nodep->skewp());
        إذا (!VN_IS(nodep->skewp(), Const)) {
            nodep->skewp()->v3error("Skew must be constant (IEEE 1800-2023 14.4)");
            أرجع;
        }
        AstConst* ثابت skewp = VN_AS(nodep->skewp(), Const);
        إذا (skewp->انگ().isNegative()) skewp->v3error("Skew cannot be negative");
        Ast_ڳنڍ_اظهار* ثابت exprp = nodep->exprp();
        varp->name(m_clockingp->name() + "__DOT__" + varp->name());
        m_clockingp->جوڙيو_ايندڙ_هتي(varp->unlink_Fr_پٺ());
        nodep->user1p(varp);
        varp->user1p(nodep);
        إذا (nodep->direction() == VDirection::OUTPUT) {
            exprp->foreach([](ثابت Ast_ڳنڍ_Var_Ref* varrefp) {
                // Prevent confusing BLKANDNBLK warnings on clockvars due to generated assignments
                varrefp->fileline()->warnOff(V3_Error_ڪوڊ::BLKANDNBLK, سچ);
                // Mark the driven signal so later passes (e.g. V3Inst port
                // connection) know the drive flows out from inside the cell.
                // Item expressions are plain signal references, so only the
                // driven signal is marked, never index selects.
                varrefp->varp()->isClockingDriven(سچ);
            });
            AstVarRef* ثابت skewedReadRefp = جديد AstVarRef{flp, varp, VAccess::READ};
            skewedReadRefp->user1(سچ);
            // Initialize the clockvar
            AstVarRef* ثابت skewedWriteRefp = جديد AstVarRef{flp, varp, VAccess::WRITE};
            skewedWriteRefp->user1(سچ);
            AstInitialStatic* ثابت initClockvarp = جديد AstInitialStatic{
                flp, جديد Ast_سونپيو{flp, skewedWriteRefp, exprp->clone_وڻ_خالص(خطأ)}};
            m_modp->جوڙيو_Stmtsp(initClockvarp);
            // Each drive sets a flag, so the signal is also assigned when driven with the same
            // value again, e.g. after another assignment to the signal (IEEE 1800-2023 14.16.2)
            AstVar* ثابت drivenVarp = getCreateDrivenVarp(nodep);
            m_modp->جوڙيو_Stmtsp(جديد AstInitialStatic{
                flp, جديد Ast_سونپيو{flp, جديد AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                   جديد AstConst{flp, AstConst::ٻٽو_انگ_False{}}}});
            // Assign the clockvar to the actual var if it was driven
            Ast_سونپيو* ثابت assignp
                = جديد Ast_سونپيو{flp, exprp->clone_وڻ_خالص(خطأ), skewedReadRefp};
            AstIf* ثابت ifp
                = جديد AstIf{flp, جديد AstVarRef{flp, drivenVarp, VAccess::READ},
                            جديد Ast_سونپيو{flp, جديد AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                          جديد AstConst{flp, AstConst::ٻٽو_انگ_False{}}}};
            ifp->جوڙيو_Thensp(assignp);
            // Only in the time step of the clocking event
            AstCMethodHard* ثابت trigp = جديد AstCMethodHard{
                nodep->fileline(), جديد AstVarRef{flp, m_clockingp->ensureEventp(), VAccess::READ},
                VCMethod::EVENT_IS_TRIGGERED};
            trigp->dtype_سيٽ_ٻٽو_انگ();
            ifp->condp(جديد AstLogAnd{flp, ifp->condp()->unlink_Fr_پٺ(), trigp});
            // A skew rounding to no time precision ticks is a zero skew
            Ast_ڳنڍ_اظهار* ثابت ticksp
                = V3Timing::delay_منزل_Ticks(skewp->clone_وڻ(خطأ), m_modp->timeunit());
            ثابت منطقي zeroSkew = ticksp->isZero();
            VL_DO_DANGLING(ticksp->delete_وڻ(), ticksp);
            إذا (zeroSkew || !skewp->fileline()->timingOn()) {
                // Drive the var in Re-NBA (IEEE 1800-2023 14.16), ignoring skews where timing is
                // turned off
                Ast_Sen_وڻ* senTreep = m_clockingp->sentreep()->clone_وڻ(خطأ);
                senTreep->جوڙيو_Sensesp(جديد Ast_Sen_شي{
                    flp, VEdgeType::ET_بدليو, جديد AstVarRef{flp, drivenVarp, VAccess::READ}});
                m_clockingp->جوڙيو_ايندڙ_هتي(جديد AstAlwaysReactive{flp, senTreep, ifp});
            } وإلا {
                // Fork only the assignment, so this AlwaysPostponed can be retriggered
                // before the assignment happens
                AstFork* ثابت forkp = جديد AstFork{flp, VJoinType::JOIN_NONE};
                forkp->فوري_شروعات(سچ);
                forkp->جوڙيو_Forksp(جديد AstBegin{flp, "", assignp->unlink_Fr_پٺ(), سچ});
                ifp->جوڙيو_Thensp(forkp);
                // Read the clockvar in Postponed, so a drive late in the time step is applied
                // at this event plus the skew (IEEE 1800-2023 14.16)
                m_clockingp->جوڙيو_ايندڙ_هتي(جديد AstAlwaysPostponed{flp, ifp});
                إذا (v3Global.opt.timing().is_سيٽ_True()) {
                    AstDelay* ثابت delayp = جديد AstDelay{flp, skewp->unlink_Fr_پٺ(), خطأ};
                    delayp->timeunit(m_modp->timeunit());
                    assignp->timingControlp(delayp);
                } وإلا إذا (v3Global.opt.timing().is_سيٽ_False()) {
                    nodep->v3warn(E_NOTIMING,
                                  "Clocking output skew greater than #0 requires --timing");
                } وإلا {
                    nodep->v3warn(E_NEEDTIMINGOPT,
                                  "Use --timing or --no-timing to specify how "
                                  "clocking output skew greater than #0 should be handled");
                }
            }
        } وإلا إذا (nodep->direction() == VDirection::داخلا) {
            // Ref to the clockvar
            AstVarRef* ثابت refp = جديد AstVarRef{flp, varp, VAccess::WRITE};
            refp->user1(سچ);
            إذا (skewp->انگ().is1_قدم()) {
                // #1step means the value that is sampled is always the signal's last value
                // before the clock edge (IEEE 1800-2023 14.4)
                AstSampled* ثابت sampledp
                    = جديد AstSampled{flp, exprp->clone_وڻ_خالص(خطأ), exprp->dtypep(), سچ};
                Ast_سونپيو* ثابت assignp = جديد Ast_سونپيو{flp, refp, sampledp};
                m_clockingp->جوڙيو_ايندڙ_هتي(جديد AstAlways{
                    flp, VAlwaysKwd::ALWAYS, m_clockingp->sentreep()->clone_وڻ(خطأ), assignp});
            } وإلا إذا (skewp->isZero()) {
                // #0 means the var has to be sampled in Observed (IEEE 1800-2023 14.13)
                Ast_سونپيو* ثابت assignp = جديد Ast_سونپيو{flp, refp, exprp->clone_وڻ_خالص(خطأ)};
                m_clockingp->جوڙيو_ايندڙ_هتي(جديد AstAlwaysObserved{
                    flp, m_clockingp->sentreep()->clone_وڻ(خطأ), assignp});
            } وإلا {
                // Create a queue where we'll store sampled values with timestamps
                Ast_Sample_قطار_DType* ثابت قطار_Dtp
                    = جديد Ast_Sample_قطار_DType{flp, exprp->dtypep()};
                m_netlistp->typeTablep()->جوڙيو_Typesp(قطار_Dtp);
                AstVar* ثابت قطار_Varp
                    = جديد AstVar{flp, VVarType::MODULETEMP, "__Vqueue__" + varp->name(), قطار_Dtp};
                قطار_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_clockingp->جوڙيو_ايندڙ_هتي(قطار_Varp);
                // Create a process like this:
                //     always queue.push(<sampled var>);
                AstCMethodHard* ثابت pushp = جديد AstCMethodHard{
                    flp, جديد AstVarRef{flp, قطار_Varp, VAccess::WRITE}, VCMethod::DYN_ڌڪو,
                    جديد Ast_وقت{nodep->fileline(), m_modp->timeunit()}};
                pushp->جوڙيو_Pinsp(exprp->clone_وڻ_خالص(خطأ));
                pushp->dtype_سيٽ_Void();
                m_clockingp->جوڙيو_ايندڙ_هتي(
                    جديد AstAlways{flp, VAlwaysKwd::ALWAYS, خالي_اشارو, pushp->makeStmt()});
                // Create a process like this:
                //     always @<clocking event> queue.pop(<skew>, /*out*/<skewed var>);
                AstCMethodHard* ثابت popp = جديد AstCMethodHard{
                    flp, جديد AstVarRef{flp, قطار_Varp, VAccess::READWRITE}, VCMethod::DYN_ڪڍو,
                    جديد Ast_وقت{nodep->fileline(), m_modp->timeunit()}};
                popp->جوڙيو_Pinsp(skewp->unlink_Fr_پٺ());
                refp->access(VAccess::READWRITE);  // Only conditionally assigned
                popp->جوڙيو_Pinsp(refp);
                popp->dtype_سيٽ_Void();
                m_clockingp->جوڙيو_ايندڙ_هتي(جديد AstAlways{flp, VAlwaysKwd::ALWAYS,
                                                       m_clockingp->sentreep()->clone_وڻ(خطأ),
                                                       popp->makeStmt()});
            }
        } وإلا {
            nodep->v3fatalSrc("Invalid direction");
        }
    }
    فراغ گهمو(AstDelay* nodep) override {
        m_has_چڪر_Delay |= nodep->is_چڪر_Delay();
        // Only cycle delays are relevant in this stage; also only process once
        إذا (!nodep->is_چڪر_Delay()) {
            إذا (m_اندريون_Synch_Drive) {
                nodep->v3error("Only cycle delays can be used in synchronous drives"
                               " (IEEE 1800-2023 14.16)");
            }
            iterateChildren(nodep);
            أرجع;
        }
        إذا (m_اندريون_سونپيو && !m_اندريون_Synch_Drive) {
            nodep->v3error("Cycle delays not allowed as intra-assignment delays"
                           " (IEEE 1800-2023 14.11)");
            VL_DO_DANGLING(nodep->unlink_Fr_پٺ()->delete_وڻ(), nodep);
            أرجع;
        }
        إذا (nodep->stmtsp()) nodep->جوڙيو_ايندڙ_هتي(nodep->stmtsp()->unlink_Fr_پٺ_With_ايندڙ());
        File_سٽ* ثابت flp = nodep->fileline();
        Ast_ڳنڍ_اظهار* valuep = V3Const::constifyEdit(nodep->lhsp()->unlink_Fr_پٺ());
        ثابت AstConst* ثابت constp = VN_CAST(valuep, Const);
        إذا (!constp && m_pexprp) {
            // V3AssertNfa handles non-const delays in sequences before this pass and
            // replaces the property; this branch should never be reached.
            nodep->v3fatalSrc("Non-constant cycle delay in assertion: "
                              "should have been caught by V3AssertNfa");
        } وإلا إذا (constp && constp->isZero()) {
            VL_DO_DANGLING(ڌڪو_Deletep(valuep), valuep);
            إذا (m_اندريون_Synch_Drive) {
                // ##0 has no effect in synchronous drives (IEEE 1800-2023 14.11)
                VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
                أرجع;
            }
            إذا (m_pexprp) {
                // ##0 in sequence context = zero delay = same clock tick
                VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
                أرجع;
            }
            // Procedural ##0: synchronize with default clocking event (IEEE 1800-2023 14.11)
            إذا (!m_defaultClockingp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
                أرجع;
            }
            nodep->replaceWith(new_چڪر_Delay_Zerop(flp));
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            أرجع;
        }
        Ast_Sen_وڻ* sentreep = خالي_اشارو;
        إذا (!m_drives.خالي_جايون()) {
            إذا (m_اندريون_Concat_سونپيو) {
                // Also the clockvars may be of different clocking blocks
                nodep->v3error("Cycle delays cannot be used in synchronous drives to a"
                               " concatenation (IEEE 1800-2023 14.16)");
                VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
                VL_DO_DANGLING(ڌڪو_Deletep(valuep), valuep);
                أرجع;
            }
            // Count the cycles of the driven clockvar's clocking block (IEEE 1800-2023 14.16)
            sentreep = newDriveSentreep(nodep, m_drives.front());
            إذا (!sentreep) {
                VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
                VL_DO_DANGLING(ڌڪو_Deletep(valuep), valuep);
                أرجع;
            }
        } وإلا إذا (!m_defaultClockingp) {
            إذا (!m_pexprp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
                VL_DO_DANGLING(ڌڪو_Deletep(valuep), valuep);
                أرجع;
            }
            sentreep = جديد Ast_Sen_وڻ{flp, m_senip->clone_وڻ(خطأ)};
        } وإلا {
            sentreep = m_defaultClockingp->sentreep()->clone_وڻ(خطأ);
        }
        AstEventControl* ثابت controlp
            = جديد AstEventControl{nodep->fileline(), sentreep, خالي_اشارو};
        ثابت std::string delayName = m_چڪر_Dly_نالا.get(nodep);
        Ast_ڳنڍ_اظهار* throughoutp
            = nodep->throughoutp() ? nodep->throughoutp()->unlink_Fr_پٺ() : خالي_اشارو;

        AstVar* ثابت cntVarp = جديد AstVar{flp, VVarType::BLOCKTEMP, delayName + "__counter",
                                           nodep->ڳوليو_Basic_DType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstBegin* ثابت beginp = جديد AstBegin{flp, delayName + "__block", cntVarp, سچ};
        // A non-constant delay is counted at runtime. The count is integral of 64 bits, as
        // V3Width makes delays, or real, which is rounded (IEEE 1800-2023 6.12.1)
        إذا (valuep->isDouble()) {
            valuep = جديد AstRToIRoundS{flp, valuep};
            valuep->dtypeFrom(cntVarp);
        } وإلا {
            valuep = جديد AstSel{flp, valuep, 0, cntVarp->ويڪر()};
        }
        beginp->جوڙيو_Stmtsp(جديد Ast_سونپيو{flp, جديد AstVarRef{flp, cntVarp, VAccess::WRITE}, valuep});
        // If the count is zero, a synchronous drive waits for none, like with a constant '##0',
        // and a procedural delay waits like a procedural '##0' (IEEE 1800-2023 14.11)
        إذا (!constp && !m_اندريون_Synch_Drive) {
            beginp->جوڙيو_Stmtsp(جديد AstIf{
                flp,
                جديد AstEq{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ}, جديد AstConst{flp, 0}},
                new_چڪر_Delay_Zerop(flp)});
        }

        // Throughout: create flag tracking whether condition held every tick
        AstVar* throughoutOkp = خالي_اشارو;
        إذا (throughoutp) {
            throughoutOkp = جديد AstVar{flp, VVarType::BLOCKTEMP, delayName + "__throughoutOk",
                                       nodep->ڳوليو_Basic_DType(VBasicDTypeKwd::LOGIC_IMPLICIT)};
            throughoutOkp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->جوڙيو_Stmtsp(throughoutOkp);
            beginp->جوڙيو_Stmtsp(جديد Ast_سونپيو{flp, جديد AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            جديد AstConst{flp, AstConst::ٻٽو_انگ_True{}}});
            // Check condition at tick 0 (sequence start, before entering loop)
            beginp->جوڙيو_Stmtsp(
                جديد AstIf{flp, جديد AstLogNot{flp, throughoutp->clone_وڻ_خالص(خطأ)},
                          جديد Ast_سونپيو{flp, جديد AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                        جديد AstConst{flp, AstConst::ٻٽو_انگ_False{}}}});
        }

        {
            AstLoop* ثابت loopp = جديد AstLoop{flp};
            // When throughout is present, exit loop early if condition fails
            Ast_ڳنڍ_اظهار* loopCondp
                = جديد AstGt{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ}, جديد AstConst{flp, 0}};
            إذا (throughoutOkp) {
                loopCondp = جديد AstLogAnd{flp, loopCondp,
                                          جديد AstVarRef{flp, throughoutOkp, VAccess::READ}};
            }
            loopp->جوڙيو_Stmtsp(جديد AstLoopTest{flp, loopp, loopCondp});
            loopp->جوڙيو_Stmtsp(controlp);
            loopp->جوڙيو_Stmtsp(
                جديد Ast_سونپيو{flp, جديد AstVarRef{flp, cntVarp, VAccess::WRITE},
                              جديد AstSub{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ},
                                         جديد AstConst{flp, 1}}});
            // Check throughout condition at each tick during delay (IEEE 1800-2023 16.9.9)
            إذا (throughoutp) {
                loopp->جوڙيو_Stmtsp(
                    جديد AstIf{flp, جديد AstLogNot{flp, throughoutp},
                              جديد Ast_سونپيو{flp, جديد AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            جديد AstConst{flp, AstConst::ٻٽو_انگ_False{}}}});
            }
            beginp->جوڙيو_Stmtsp(loopp);
        }
        // Compose wrappers on remaining sequence: throughout gate (inner), disable iff (outer)
        Ast_ڳنڍ* remainp = nodep->nextp() ? nodep->nextp()->unlink_Fr_پٺ_With_ايندڙ() : خالي_اشارو;
        إذا (throughoutOkp) {
            // If condition failed during delay, fail assertion
            remainp = جديد AstIf{flp, جديد AstVarRef{flp, throughoutOkp, VAccess::READ}, remainp,
                                جديد AstPExprClause{flp, /*pass=*/خطأ}};
        }
        إذا (m_disableSeqIfp && remainp) {
            AstIf* ثابت disableSeqIfp = m_disableSeqIfp->clone_وڻ(خطأ);
            disableSeqIfp->جوڙيو_Thensp(remainp);
            remainp = disableSeqIfp;
        }
        إذا (remainp) {
            إذا (throughoutOkp) {
                // throughoutOkp is declared in beginp scope -- check must be inside it
                beginp->جوڙيو_Stmtsp(remainp);
            } وإلا {
                nodep->جوڙيو_ايندڙ_هتي(remainp);
            }
        }
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
    }
    فراغ گهمو(Ast_Sen_وڻ* nodep) override {
        إذا (m_اندريون_Synch_Drive) {
            nodep->v3error("Event controls cannot be used in "
                           "synchronous drives (IEEE 1800-2023 14.16)");
        }

        ثابت AstSampled* sampledp;
        إذا (nodep->exists([&sampledp](ثابت AstSampled* ثابت sp) {
                sampledp = sp;
                أرجع سچ;
            })) {
            sampledp->v3warn(E_UNSUPPORTED, "Unsupported: $sampled inside sensitivity list");
        }
    }
    فراغ گهمو(Ast_ڳنڍ_Var_Ref* nodep) override {
        UINFO(8, " -varref:  " << nodep);
        UINFO(8, " -varref-var-back:  " << nodep->varp()->backp());
        UINFO(8, " -varref-var-user1:  " << nodep->varp()->user1p());
        إذا (Ast_Clocking_شي* ثابت itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->پهريون_Abovep(),
                Clocking_شي)) {
            إذا (nodep->user1()) أرجع;

            // ensure linking still works, this has to be done only once
            إذا (AstVarXRef* xrefp = VN_CAST(nodep, VarXRef)) {
                UINFO(8, " -clockvarxref-in:  " << xrefp);
                string dotted = xrefp->dotted();
                ثابت سائيز_t dotPos = dotted.rfind('.');
                dotted.erase(dotPos, string::npos);
                xrefp->dotted(dotted);
                UINFO(8, " -clockvarxref-out: " << xrefp);
                m_xrefsp.emplace_پٺ(xrefp);
            }

            إذا (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            إذا (nodep->access().isWriteOrRW()) {
                إذا (itemp->direction() == VDirection::OUTPUT) {
                    إذا (!m_اندريون_سونپيو_Dly_Lhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    إذا (m_اندريون_سونپيو) {
                        m_اندريون_Synch_Drive = سچ;
                        m_drives.ڌڪو_پٺ({itemp, nodep});
                    }
                } وإلا إذا (itemp->direction() == VDirection::داخلا) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
            nodep->user1(سچ);
        }
    }
    فراغ گهمو(AstMemberSel* nodep) override {
        إذا (Ast_Clocking_شي* ثابت itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->پهريون_Abovep(),
                Clocking_شي)) {
            إذا (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            إذا (nodep->access().isWriteOrRW()) {
                إذا (itemp->direction() == VDirection::OUTPUT) {
                    إذا (!m_اندريون_سونپيو_Dly_Lhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    إذا (m_اندريون_سونپيو) {
                        m_اندريون_Synch_Drive = سچ;
                        m_drives.ڌڪو_پٺ({itemp, nodep});
                    }
                } وإلا إذا (itemp->direction() == VDirection::داخلا) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
        }
    }
    فراغ گهمو(Ast_ڳنڍ_سونپيو* nodep) override {
        إذا (nodep->user1()) أرجع;
        VL_RESTORER(m_اندريون_سونپيو);
        VL_RESTORER(m_اندريون_Synch_Drive);
        VL_RESTORER(m_اندريون_Concat_سونپيو);
        VL_RESTORER_CLEAR(m_drives);
        m_اندريون_سونپيو = سچ;
        m_اندريون_Synch_Drive = خطأ;
        {
            // The braces of a concatenation are a replication, by one
            ثابت Ast_ڳنڍ_اظهار* lhsp = nodep->lhsp();
            إذا (ثابت AstReplicate* ثابت repp = VN_CAST(lhsp, Replicate)) lhsp = repp->srcp();
            m_اندريون_Concat_سونپيو = VN_IS(lhsp, Concat);
        }
        {
            VL_RESTORER(m_اندريون_سونپيو_Dly_Lhs);
            m_اندريون_سونپيو_Dly_Lhs = VN_IS(nodep, سونپيو_Dly);
            iterate(nodep->lhsp());
        }
        iterate(nodep->rhsp());
        إذا (nodep->timingControlp()) iterate(nodep->timingControlp());
        // A drive without a cycle delay, also after removing '##0', updates the clockvar now
        إذا (m_اندريون_Synch_Drive && !nodep->timingControlp()) {
            Ast_سونپيو* ثابت assignp = جديد Ast_سونپيو{
                nodep->fileline(), nodep->lhsp()->unlink_Fr_پٺ(), nodep->rhsp()->unlink_Fr_پٺ()};
            assignp->user1(سچ);
            nodep->replaceWith(assignp);
            VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
            nodep = assignp;
        }
        // Note the drives when they take effect, after the cycle delay if any. Each clockvar
        // of the LHS is driven, e.g. in a concatenation which other simulators accept, although
        // IEEE 1800-2023 14.16 does not allow it
        Ast_ڳنڍ* setsp = خالي_اشارو;
        لكل (ثابت SynchDrive& drive : m_drives) {
            setsp = Ast_ڳنڍ::جوڙيو_ايندڙ(setsp, newDrivenSetp(nodep->fileline(), drive));
        }
        إذا (!setsp) أرجع;
        إذا (AstBegin* ثابت beginp = VN_CAST(nodep->timingControlp(), Begin)) {
            beginp->جوڙيو_Stmtsp(setsp);
        } وإلا {
            nodep->جوڙيو_ايندڙ_هتي(setsp);
        }
    }
    فراغ گهمو(AstAlways* nodep) override {
        iterate_And_ايندڙ_Null(nodep->sentreep());
        إذا (nodep->sentreep()) m_seniAlwaysp = nodep->sentreep()->sensesp();
        iterate_And_ايندڙ_Null(nodep->stmtsp());
        m_seniAlwaysp = خالي_اشارو;
    }

    فراغ گهمو(Ast_ڳنڍ_Cover_Or_Assert* nodep) override {
        إذا (nodep->sentreep()) أرجع;  // Already processed

        VL_RESTORER(m_senip);
        VL_RESTORER(m_disablep);
        m_senip = خالي_اشارو;
        m_disablep = خالي_اشارو;

        // Find Clocking's buried under nodep->exprsp
        iterateChildren(nodep);
        إذا (!nodep->فوري()) nodep->sentreep(new_Sen_وڻ(nodep, خالي_اشارو, nodep));
    }
    فراغ گهمو(AstFalling* nodep) override {
        إذا (nodep->user1_سيٽ_Once()) أرجع;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_سٽ* ثابت fl = nodep->fileline();
        Ast_ڳنڍ_اظهار* exprp = nodep->exprp()->unlink_Fr_پٺ();
        إذا (exprp->ويڪر() > 1) exprp = جديد AstSel{fl, exprp, 0, 1};
        Ast_ڳنڍ_اظهار* ثابت futurep = جديد AstFuture{fl, exprp, new_Sen_وڻ(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = جديد AstAnd{fl, exprp->clone_وڻ_خالص(خطأ), جديد AstNot{fl, futurep}};
        exprp->dtype_سيٽ_ٻٽو_انگ();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ گهمو(AstFell* nodep) override {
        إذا (nodep->user1_سيٽ_Once()) أرجع;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_سٽ* ثابت fl = nodep->fileline();
        Ast_ڳنڍ_اظهار* exprp = nodep->exprp()->unlink_Fr_پٺ();
        إذا (exprp->ويڪر() > 1) exprp = جديد AstSel{fl, exprp, 0, 1};
        Ast_Sen_وڻ* sentreep = nodep->sentreep();
        إذا (sentreep) sentreep->unlink_Fr_پٺ();
        AstPast* ثابت pastp = جديد AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_وڻ(nodep, sentreep));
        exprp = جديد AstAnd{fl, pastp, جديد AstNot{fl, exprp->clone_وڻ_خالص(خطأ)}};
        exprp->dtype_سيٽ_ٻٽو_انگ();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ گهمو(AstFuture* nodep) override {
        إذا (nodep->user1_سيٽ_Once()) أرجع;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        Ast_Sen_وڻ* ثابت sentreep = nodep->sentreep();
        إذا (sentreep) VL_DO_DANGLING(ڌڪو_Deletep(sentreep->unlink_Fr_پٺ()), sentreep);
        nodep->sentreep(new_Sen_وڻ(nodep));
    }
    فراغ گهمو(AstPast* nodep) override {
        checkSamplingFuncDType(nodep, nodep->exprp());
        إذا (nodep->sentreep()) أرجع;  // Already processed
        iterateChildren(nodep);
        nodep->sentreep(new_Sen_وڻ(nodep));
    }
    فراغ گهمو(AstSConsRep* nodep) override {
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
        File_سٽ* ثابت flp = nodep->fileline();
        Ast_ڳنڍ_اظهار* ثابت exprp = nodep->exprp()->unlink_Fr_پٺ();
        إذا (n == 1) {
            nodep->replaceWith(exprp);
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            أرجع;
        }
        // Saturating counter: if (expr) cnt <= min(cnt+1, N); else cnt <= 0;
        AstVar* ثابت cntVarp = جديد AstVar{flp, VVarType::MODULETEMP, m_cons_Rep_نالا.get(""),
                                           nodep->ڳوليو_Basic_DType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->جوڙيو_Stmtsp(cntVarp);
        Ast_ڳنڍ_اظهار* ثابت اظهار_Clonep = exprp->clone_وڻ_خالص(خطأ);
        Ast_ڳنڍ_اظهار* ثابت saturatingIncrp = جديد AstCond{
            flp,
            جديد AstLt{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ},
                      جديد AstConst{flp, static_cast<uint32_t>(n)}},
            جديد Ast_جوڙيو{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ}, جديد AstConst{flp, 1u}},
            جديد AstConst{flp, static_cast<uint32_t>(n)}};
        Ast_سونپيو_Dly* ثابت incrAssignp
            = جديد Ast_سونپيو_Dly{flp, جديد AstVarRef{flp, cntVarp, VAccess::WRITE}, saturatingIncrp};
        Ast_سونپيو_Dly* ثابت resetAssignp = جديد Ast_سونپيو_Dly{
            flp, جديد AstVarRef{flp, cntVarp, VAccess::WRITE}, جديد AstConst{flp, 0u}};
        AstIf* ثابت ifp = جديد AstIf{flp, اظهار_Clonep, incrAssignp, resetAssignp};
        Ast_Sen_وڻ* ثابت senTreep = new_Sen_وڻ(nodep);
        AstAlways* ثابت alwaysp = جديد AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, ifp};
        cntVarp->جوڙيو_ايندڙ_هتي(alwaysp);
        // Match: cnt >= N-1 (previous cycles via NBA) && expr (current cycle)
        Ast_ڳنڍ_اظهار* ثابت cntCheckp = جديد AstGte{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ},
                                                  جديد AstConst{flp, static_cast<uint32_t>(n - 1)}};
        cntCheckp->dtype_سيٽ_ٻٽو_انگ();
        Ast_ڳنڍ_اظهار* ثابت matchp = جديد AstAnd{flp, cntCheckp, exprp};
        matchp->dtype_سيٽ_ٻٽو_انگ();
        nodep->replaceWith(matchp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ گهمو(AstRising* nodep) override {
        إذا (nodep->user1_سيٽ_Once()) أرجع;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_سٽ* ثابت fl = nodep->fileline();
        Ast_ڳنڍ_اظهار* exprp = nodep->exprp()->unlink_Fr_پٺ();
        إذا (exprp->ويڪر() > 1) exprp = جديد AstSel{fl, exprp, 0, 1};
        Ast_ڳنڍ_اظهار* ثابت futurep = جديد AstFuture{fl, exprp, new_Sen_وڻ(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = جديد AstAnd{fl, جديد AstNot{fl, exprp->clone_وڻ_خالص(خطأ)}, futurep};
        exprp->dtype_سيٽ_ٻٽو_انگ();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ گهمو(AstRose* nodep) override {
        إذا (nodep->user1_سيٽ_Once()) أرجع;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_سٽ* ثابت fl = nodep->fileline();
        Ast_ڳنڍ_اظهار* exprp = nodep->exprp()->unlink_Fr_پٺ();
        إذا (exprp->ويڪر() > 1) exprp = جديد AstSel{fl, exprp, 0, 1};
        Ast_Sen_وڻ* sentreep = nodep->sentreep();
        إذا (sentreep) sentreep->unlink_Fr_پٺ();
        AstPast* ثابت pastp = جديد AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_وڻ(nodep, sentreep));
        exprp = جديد AstAnd{fl, جديد AstNot{fl, pastp}, exprp->clone_وڻ_خالص(خطأ)};
        exprp->dtype_سيٽ_ٻٽو_انگ();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    ساكن AstAssocArrayDType* getProcessAssocArrayType(File_سٽ* ثابت flp) {
        // Type of __VassertsActive___x[std::process]
        Ast_ڳنڍ_DType* valp
            = v3Global.rootp()->typeTablep()->ڳوليو_Basic_DType(flp, VBasicDTypeKwd::ٻٽو_انگ);
        AstClassRefDType* keyp
            = جديد AstClassRefDType{flp, v3Global.rootp()->stdPackageProcessp(), خالي_اشارو};
        keyp->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        v3Global.rootp()->typeTablep()->جوڙيو_Typesp(keyp);
        AstAssocArrayDType* ثابت typep = جديد AstAssocArrayDType{flp, valp, keyp};
        typep->dtypep(typep);
        v3Global.rootp()->typeTablep()->جوڙيو_Typesp(typep);
        أرجع typep;
    }
    ساكن Ast_Stmt_اظهار* getProcessAssocArrayDelete(AstVarRef* ثابت refp) {
        // Constructs refp.delete(std::process::self()) statement
        File_سٽ* ثابت flp = refp->fileline();
        AstCMethodHard* ثابت deletep = جديد AstCMethodHard{
            flp, refp, VCMethod::ASSOC_ERASE, v3Global.rootp()->stdPackageProcessSelfp(flp)};
        deletep->dtypep(refp->ڳوليو_Void_DType());
        أرجع جديد Ast_Stmt_اظهار{flp, deletep};
    }
    ساكن Ast_ڳنڍ_اظهار* get_Process_Assoc_Array_سائيز(AstVarRef* ثابت refp) {
        // Constructs refp.size() statement
        AstCMethodHard* ثابت sizep
            = جديد AstCMethodHard{refp->fileline(), refp, VCMethod::ASSOC_سائيز};
        sizep->dtypep(refp->ڳوليو_Basic_DType(VBasicDTypeKwd::UINT32));
        أرجع sizep;
    }
    فراغ گهمو(AstSEventually* nodep) override {
        UASSERT(v3Global.rootp()->stdPackagep(), "Should be imported");

        Ast_Sen_وڻ* ثابت sentreep = new_Sen_وڻ(nodep);
        إذا (!sentreep->sensesp()) {
            VL_DO_DANGLING(ڌڪو_Deletep(sentreep), sentreep);
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::ٻٽو_انگ_False{}});
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            أرجع;
        }

        File_سٽ* ثابت flp = nodep->fileline();

        // Track active assertions
        AstVar* ثابت activep = جديد AstVar{flp, VVarType::MODULETEMP, m_سرگرم_نالا.get(""),
                                           getProcessAssocArrayType(flp)};
        activep->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->جوڙيو_Stmtsp(activep);

        // Assertion condition check
        AstLoop* ثابت loopp = جديد AstLoop{flp};
        AstSampled* ثابت condp
            = جديد AstSampled{flp, nodep->exprp()->unlink_Fr_پٺ(), خالي_اشارو, سچ};
        loopp->جوڙيو_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLogNot{flp, condp}});
        loopp->جوڙيو_Stmtsp(جديد AstEventControl{flp, sentreep, خالي_اشارو});

        // Add assertion to the active set
        AstAssocSel* ثابت selp = جديد AstAssocSel{flp, جديد AstVarRef{flp, activep, VAccess::WRITE},
                                                  v3Global.rootp()->stdPackageProcessSelfp(flp)};
        Ast_سونپيو* ثابت incrementp = جديد Ast_سونپيو{flp, selp, جديد AstConst{flp, 1}};
        AstPExprClause* ثابت clausep = جديد AstPExprClause{flp};
        Ast_Stmt_اظهار* ثابت deletep
            = getProcessAssocArrayDelete(جديد AstVarRef{flp, activep, VAccess::WRITE});

        // Main assertion block
        AstBegin* ثابت bodyp = جديد AstBegin{flp, m_بلاڪ_نالا.get(""), خالي_اشارو, سچ};
        bodyp->جوڙيو_Stmtsp(incrementp);
        bodyp->جوڙيو_Stmtsp(loopp);
        bodyp->جوڙيو_Stmtsp(clausep);
        bodyp->جوڙيو_Stmtsp(deletep);

        // Validate assertion condition for each active assert
        AstVar* ثابت سرگرم_Countp = جديد AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                nodep->ڳوليو_Basic_DType(VBasicDTypeKwd::UINT32)};
        سرگرم_Countp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        Ast_سونپيو* ثابت init_سرگرم_Countp
            = جديد Ast_سونپيو{flp, جديد AstVarRef{flp, سرگرم_Countp, VAccess::WRITE},
                            get_Process_Assoc_Array_سائيز(جديد AstVarRef{flp, activep, VAccess::READ})};
        AstLoop* ثابت finalLoopp = جديد AstLoop{flp};
        AstIf* ثابت finalBodypCondp
            = جديد AstIf{flp, condp->clone_وڻ_خالص(خطأ), جديد AstPExprClause{flp},
                        جديد AstPExprClause{flp, خطأ}};
        finalLoopp->جوڙيو_Stmtsp(
            جديد AstLoopTest{flp, finalLoopp,
                            جديد AstNeq{flp, جديد AstVarRef{flp, سرگرم_Countp, VAccess::READ},
                                       جديد AstConst{flp, 0}}});
        finalLoopp->جوڙيو_Stmtsp(finalBodypCondp);
        finalLoopp->جوڙيو_Stmtsp(
            جديد Ast_سونپيو{flp, جديد AstVarRef{flp, سرگرم_Countp, VAccess::WRITE},
                          جديد AstSub{flp, جديد AstVarRef{flp, سرگرم_Countp, VAccess::READ},
                                     جديد AstConst{flp, 1}}});

        // Final assertion block
        AstBegin* ثابت finalp = جديد AstBegin{flp, m_بلاڪ_نالا.get(""), خالي_اشارو, سچ};
        finalp->جوڙيو_Stmtsp(سرگرم_Countp);
        finalp->جوڙيو_Stmtsp(init_سرگرم_Countp);
        finalp->جوڙيو_Stmtsp(finalLoopp);

        VL_RESTORER(m_pexprp);
        m_pexprp = جديد AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
        VL_RESTORER(m_has_چڪر_Delay);
        m_has_چڪر_Delay = خطأ;
        iterate(bodyp);
        iterate(finalp);

        إذا (m_has_چڪر_Delay) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in s_eventually");
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::ٻٽو_انگ_False{}});
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            VL_DO_DANGLING(m_pexprp->delete_وڻ(), m_pexprp);
            أرجع;
        }

        nodep->replaceWith(m_pexprp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ گهمو(AstStable* nodep) override {
        إذا (nodep->user1_سيٽ_Once()) أرجع;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_سٽ* ثابت fl = nodep->fileline();
        Ast_ڳنڍ_اظهار* exprp = nodep->exprp()->unlink_Fr_پٺ();
        Ast_Sen_وڻ* sentreep = nodep->sentreep();
        إذا (sentreep) sentreep->unlink_Fr_پٺ();
        AstPast* ثابت pastp = جديد AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_وڻ(nodep, sentreep));
        exprp = جديد AstEq{fl, pastp, exprp->clone_وڻ_خالص(خطأ)};
        exprp->dtype_سيٽ_ٻٽو_انگ();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ گهمو(AstSteady* nodep) override {
        إذا (nodep->user1_سيٽ_Once()) أرجع;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_سٽ* ثابت fl = nodep->fileline();
        Ast_ڳنڍ_اظهار* exprp = nodep->exprp()->unlink_Fr_پٺ();
        إذا (exprp->ويڪر() > 1) exprp = جديد AstSel{fl, exprp, 0, 1};
        Ast_ڳنڍ_اظهار* ثابت futurep = جديد AstFuture{fl, exprp, new_Sen_وڻ(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = جديد AstEq{fl, exprp->clone_وڻ_خالص(خطأ), futurep};
        exprp->dtype_سيٽ_ٻٽو_انگ();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ گهمو(AstSampled* nodep) override {
        iterateChildren(nodep);
        إذا (!nodep->اندروني_2()) checkSamplingFuncDType(nodep, nodep->exprp());
    }

    // Validate repetition count: must be a non-negative elaboration-time constant.
    // Shared by goto [->N] and nonconsecutive [=N] repetition.
    // On error, replaces nodep with BitFalse placeholder and returns nullptr.
    ثابت AstConst* validate_Rep_ڳڻپ(Ast_ڳنڍ* nodep, Ast_ڳنڍ_اظهار*& countp) {
        countp = V3Const::constifyEdit(countp);
        ثابت AstConst* ثابت constp = VN_CAST(countp, Const);
        إذا (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ڌڪو_Deletep(countp), countp);
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::ٻٽو_انگ_False_Erroring{}});
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            أرجع خالي_اشارو;
        }
        إذا (constp->منزل_SInt() < 0) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ڌڪو_Deletep(countp), countp);
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::ٻٽو_انگ_False_Erroring{}});
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            أرجع خالي_اشارو;
        }
        إذا (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ڌڪو_Deletep(countp), countp);
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::ٻٽو_انگ_False{}});
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            أرجع خالي_اشارو;
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
    AstPExpr* createRepPExpr(File_سٽ* flp, Ast_ڳنڍ_اظهار* exprp, Ast_ڳنڍ_اظهار* countp,
                             Ast_ڳنڍ_اظهار* rhsp, منطقي isOverlapped, منطقي isNonConsec) {
        Ast_Sen_شي* ثابت sensesp = m_senip;
        UASSERT_OBJ(sensesp, exprp, "Repetition requires a clock");

        ثابت std::string name
            = isNonConsec ? m_non_Cons_Rep_نالا.get(exprp) : m_goto_Rep_نالا.get(exprp);
        AstVar* ثابت cntVarp = جديد AstVar{flp, VVarType::BLOCKTEMP, name + "__counter",
                                           exprp->ڳوليو_Basic_DType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstBegin* ثابت beginp = جديد AstBegin{flp, name + "__block", cntVarp, سچ};
        beginp->جوڙيو_Stmtsp(
            جديد Ast_سونپيو{flp, جديد AstVarRef{flp, cntVarp, VAccess::WRITE}, جديد AstConst{flp, 0}});

        // Counting loop: find N occurrences of expr (shared by goto and nonconsec)
        AstLoop* ثابت loopp = جديد AstLoop{flp};
        loopp->جوڙيو_Stmtsp(جديد AstLoopTest{flp, loopp,
                                         جديد AstLt{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ},
                                                   countp->clone_وڻ_خالص(خطأ)}});
        // if (expr) cnt++ -- sampled is applied to whole property expr by V3Assert
        loopp->جوڙيو_Stmtsp(
            جديد AstIf{flp, exprp,
                      جديد Ast_سونپيو{flp, جديد AstVarRef{flp, cntVarp, VAccess::WRITE},
                                    جديد Ast_جوڙيو{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ},
                                               جديد AstConst{flp, 1}}}});
        loopp->جوڙيو_Stmtsp(جديد AstIf{
            flp, جديد AstLt{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ}, countp},
            جديد AstEventControl{flp, جديد Ast_Sen_وڻ{flp, sensesp->clone_وڻ(خطأ)}, خالي_اشارو}});

        beginp->جوڙيو_Stmtsp(loopp);

        إذا (isNonConsec && rhsp) {
            // IEEE 16.9.2: b[=N] = b[->N] ##1 !b[*0:$]
            // Trailing window: ##1 advance, then scan !expr cycles checking rhs.
            // Window closes when expr becomes true again (fail if rhs was never true).
            beginp->جوڙيو_Stmtsp(جديد AstEventControl{
                flp, جديد Ast_Sen_وڻ{flp, sensesp->clone_وڻ(خطأ)}, خالي_اشارو});  // ##1
            إذا (!isOverlapped) {
                beginp->جوڙيو_Stmtsp(جديد AstEventControl{
                    flp, جديد Ast_Sen_وڻ{flp, sensesp->clone_وڻ(خطأ)}, خالي_اشارو});  // |=>
            }
            // Window loop: check rhs at each !expr cycle (done variable for termination)
            AstVar* ثابت پورو_Varp
                = جديد AstVar{flp, VVarType::BLOCKTEMP, name + "__done", exprp->ڳوليو_ٻٽو_انگ_DType()};
            پورو_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->جوڙيو_Stmtsp(پورو_Varp);
            beginp->جوڙيو_Stmtsp(جديد Ast_سونپيو{flp, جديد AstVarRef{flp, پورو_Varp, VAccess::WRITE},
                                            جديد AstConst{flp, AstConst::ٻٽو_انگ_False{}}});
            خودڪار سيٽ_پورو = [&]() {
                أرجع جديد Ast_سونپيو{flp, جديد AstVarRef{flp, پورو_Varp, VAccess::WRITE},
                                     جديد AstConst{flp, AstConst::ٻٽو_انگ_True{}}};
            };
            AstLoop* ثابت windowp = جديد AstLoop{flp};
            // LoopTest: continue while !done
            windowp->جوڙيو_Stmtsp(جديد AstLoopTest{
                flp, windowp, جديد AstNot{flp, جديد AstVarRef{flp, پورو_Varp, VAccess::READ}}});
            // if (expr) { fail; done = 1; } -- window closed, expr true again
            Ast_ڳنڍ* ثابت failsp = جديد AstPExprClause{flp, خطأ};
            failsp->جوڙيو_ايندڙ(سيٽ_پورو());
            windowp->جوڙيو_Stmtsp(جديد AstIf{flp, exprp->clone_وڻ_خالص(خطأ), failsp});
            // if (rhs) { pass; done = 1; } -- consequent true at this !expr endpoint
            Ast_ڳنڍ* ثابت passsp = جديد AstPExprClause{flp, سچ};
            passsp->جوڙيو_ايندڙ(سيٽ_پورو());
            windowp->جوڙيو_Stmtsp(جديد AstIf{flp, rhsp, passsp});
            // @(clk) -- advance to next cycle in window
            windowp->جوڙيو_Stmtsp(
                جديد AstEventControl{flp, جديد Ast_Sen_وڻ{flp, sensesp->clone_وڻ(خطأ)}, خالي_اشارو});
            beginp->جوڙيو_Stmtsp(windowp);
        } وإلا إذا (isNonConsec) {
            // Standalone nonconsec: ##1 into window, then pass
            beginp->جوڙيو_Stmtsp(
                جديد AstEventControl{flp, جديد Ast_Sen_وڻ{flp, sensesp->clone_وڻ(خطأ)}, خالي_اشارو});
            beginp->جوڙيو_Stmtsp(جديد AstPExprClause{flp, سچ});
        } وإلا إذا (rhsp) {
            // Goto rep: check consequent once at match endpoint
            إذا (!isOverlapped) {
                beginp->جوڙيو_Stmtsp(جديد AstEventControl{
                    flp, جديد Ast_Sen_وڻ{flp, sensesp->clone_وڻ(خطأ)}, خالي_اشارو});
            }
            beginp->جوڙيو_Stmtsp(جديد AstIf{flp, rhsp, جديد AstPExprClause{flp, سچ},
                                        جديد AstPExprClause{flp, خطأ}});
        } وإلا {
            // Standalone goto: pass after counting
            beginp->جوڙيو_Stmtsp(جديد AstPExprClause{flp, سچ});
        }

        أرجع جديد AstPExpr{flp, beginp, exprp->ڳوليو_ٻٽو_انگ_DType()};
    }

    // Reject [=M:N] nonconsecutive range that reached V3AssertPre.
    // [->M:N] is fully handled upstream (NFA path); only [=M:N] still lands
    // here because AstSNonConsRep lowering is not yet implemented there.
    // Replaces the node with BitFalse so lowering continues after the warning.
    منطقي rejectNonconsecRange(Ast_ڳنڍ* nodep, Ast_ڳنڍ_اظهار* maxCountp) {
        إذا (!maxCountp) أرجع خطأ;
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: [=M:N] nonconsecutive range repetition"
                                     " (IEEE 1800-2023 16.9.2)");
        nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::ٻٽو_انگ_False{}});
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        أرجع سچ;
    }

    فراغ گهمو(AstSGotoRep* nodep) override {
        // Standalone goto rep (not inside implication antecedent)
        iterateChildren(nodep);
        File_سٽ* ثابت flp = nodep->fileline();
        Ast_ڳنڍ_اظهار* countp = nodep->countp()->unlink_Fr_پٺ();
        إذا (!validate_Rep_ڳڻپ(nodep, countp)) أرجع;
        Ast_ڳنڍ_اظهار* ثابت exprp = nodep->exprp()->unlink_Fr_پٺ();
        AstPExpr* ثابت pexprp = createRepPExpr(flp, exprp, countp, خالي_اشارو, سچ, خطأ);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }

    فراغ گهمو(AstSNonConsRep* nodep) override {
        // Standalone nonconsecutive rep (not inside implication antecedent)
        iterateChildren(nodep);
        إذا (rejectNonconsecRange(nodep, nodep->maxCountp())) أرجع;
        File_سٽ* ثابت flp = nodep->fileline();
        Ast_ڳنڍ_اظهار* countp = nodep->countp()->unlink_Fr_پٺ();
        إذا (!validate_Rep_ڳڻپ(nodep, countp)) أرجع;
        Ast_ڳنڍ_اظهار* ثابت exprp = nodep->exprp()->unlink_Fr_پٺ();
        AstPExpr* ثابت pexprp = createRepPExpr(flp, exprp, countp, خالي_اشارو, سچ, سچ);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }

    فراغ گهمو(AstFuncRef* nodep) override {
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
    فراغ گهمو(AstImplication* nodep) override {
        إذا (nodep->sentreep()) أرجع;  // Already processed

        // Top-level followed-by is claimed by V3AssertNfa; anything reaching here
        // is nested inside iff/implies/or. IEEE 1800-2023 16.12.9 permits the
        // composition, but silent lowering would drop non-vacuous-fail semantics.
        إذا (nodep->isFollowedBy()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: followed-by (#-# / #=#) nested inside property operator"
                          " (iff/implies/or) (IEEE 1800-2023 16.12.9)");
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::ٻٽو_انگ_False{}});
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            أرجع;
        }

        // Handle goto repetition as antecedent before iterateChildren,
        // so the standalone AstSGotoRep visitor doesn't process it
        إذا (AstSGotoRep* ثابت gotop = VN_CAST(nodep->lhsp(), SGotoRep)) {
            iterateChildren(gotop);
            iterate_And_ايندڙ_Null(nodep->rhsp());
            File_سٽ* ثابت flp = nodep->fileline();
            Ast_ڳنڍ_اظهار* countp = gotop->countp()->unlink_Fr_پٺ();
            إذا (!validate_Rep_ڳڻپ(nodep, countp)) أرجع;
            Ast_ڳنڍ_اظهار* ثابت exprp = gotop->exprp()->unlink_Fr_پٺ();
            Ast_ڳنڍ_اظهار* ثابت rhsp = nodep->rhsp()->unlink_Fr_پٺ();
            AstPExpr* ثابت pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), خطأ);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            أرجع;
        }

        // Handle nonconsecutive repetition as antecedent before iterateChildren,
        // so the standalone AstSNonConsRep visitor doesn't process it
        إذا (AstSNonConsRep* ثابت ncrp = VN_CAST(nodep->lhsp(), SNonConsRep)) {
            iterateChildren(ncrp);
            iterate_And_ايندڙ_Null(nodep->rhsp());
            إذا (rejectNonconsecRange(nodep, ncrp->maxCountp())) أرجع;
            File_سٽ* ثابت flp = nodep->fileline();
            Ast_ڳنڍ_اظهار* countp = ncrp->countp()->unlink_Fr_پٺ();
            إذا (!validate_Rep_ڳڻپ(nodep, countp)) أرجع;
            Ast_ڳنڍ_اظهار* ثابت exprp = ncrp->exprp()->unlink_Fr_پٺ();
            Ast_ڳنڍ_اظهار* ثابت rhsp = nodep->rhsp()->unlink_Fr_پٺ();
            AstPExpr* ثابت pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), سچ);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            أرجع;
        }

        iterateChildren(nodep);

        File_سٽ* ثابت flp = nodep->fileline();
        Ast_ڳنڍ_اظهار* ثابت rhsp = nodep->rhsp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* lhsp = nodep->lhsp()->unlink_Fr_پٺ();

        // Lower sequence match items (IEEE 16.11): (expr, var = val, ...) |-> / |=>
        إذا (Ast_اظهار_Stmt* ثابت اظهار_Stmtp = VN_CAST(lhsp, اظهار_Stmt)) {
            Ast_ڳنڍ_اظهار* ثابت antExprp = اظهار_Stmtp->resultp()->unlink_Fr_پٺ();

            إذا (nodep->isOverlapped()) {
                // |-> : assign to __Vpropvar via always_comb (continuous).
                // The assign evaluates RHS once; V3Sampled snapshots the
                // result so all consequent refs read the same value.
                Ast_ڳنڍ* ميلاپ_Assignsp = خالي_اشارو;
                لكل (Ast_ڳنڍ* stmtp = اظهار_Stmtp->stmtsp(); stmtp;) {
                    Ast_ڳنڍ* ثابت nextp = stmtp->nextp();
                    إذا (Ast_سونپيو* ثابت assignp = VN_CAST(stmtp, سونپيو)) {
                        assignp->unlink_Fr_پٺ();
                        ميلاپ_Assignsp = Ast_ڳنڍ::جوڙيو_ايندڙ_Null(ميلاپ_Assignsp, assignp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(ڌڪو_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                إذا (ميلاپ_Assignsp) {
                    AstAlways* ثابت alwaysp
                        = جديد AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, خالي_اشارو, ميلاپ_Assignsp};
                    m_modp->جوڙيو_Stmtsp(alwaysp);
                }
            } وإلا {
                // |=> : assign to __Vpropvar via NBA in a clocked always block.
                // The NBA commits before the next cycle's sampled snapshot,
                // so the consequent (which already references __Vpropvar)
                // sees the captured value.
                Ast_ڳنڍ* ميلاپ_Assignsp = خالي_اشارو;
                لكل (Ast_ڳنڍ* stmtp = اظهار_Stmtp->stmtsp(); stmtp;) {
                    Ast_ڳنڍ* ثابت nextp = stmtp->nextp();
                    إذا (Ast_سونپيو* ثابت assignp = VN_CAST(stmtp, سونپيو)) {
                        assignp->unlink_Fr_پٺ();
                        Ast_ڳنڍ_اظهار* ثابت سونپيو_Lhsp = assignp->lhsp()->unlink_Fr_پٺ();
                        Ast_ڳنڍ_اظهار* ثابت سونپيو_Rhsp = assignp->rhsp()->unlink_Fr_پٺ();
                        Ast_سونپيو_Dly* ثابت dlyp = جديد Ast_سونپيو_Dly{flp, سونپيو_Lhsp, سونپيو_Rhsp};
                        VL_DO_DANGLING(ڌڪو_Deletep(assignp), assignp);
                        ميلاپ_Assignsp = Ast_ڳنڍ::جوڙيو_ايندڙ_Null(ميلاپ_Assignsp, dlyp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(ڌڪو_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                إذا (ميلاپ_Assignsp) {
                    AstIf* ثابت condp
                        = جديد AstIf{flp, antExprp->clone_وڻ_خالص(خطأ), ميلاپ_Assignsp};
                    AstAlways* ثابت alwaysp
                        = جديد AstAlways{flp, VAlwaysKwd::ALWAYS, new_Sen_وڻ(nodep), condp};
                    m_modp->جوڙيو_Stmtsp(alwaysp);
                }
            }
        }

        إذا (AstPExpr* ثابت pexprp = VN_CAST(rhsp, PExpr)) {
            // Implication with sequence expression on RHS (IEEE 1800-2023 16.11, 16.12.7).
            // The PExpr was lowered from the property expression earlier in this pass.
            // Wrap the PExpr body with the antecedent check so the sequence only
            // starts when the antecedent holds.
            Ast_ڳنڍ_اظهار* condp;
            إذا (nodep->isOverlapped()) {
                // Overlapped implication (|->): check antecedent on same cycle.
                // disable iff is applied at the assertion level, not at the
                // antecedent gate, matching the existing non-PExpr overlapped path.
                condp = lhsp;
            } وإلا {
                // Non-overlapped implication (|=>): check antecedent from previous cycle
                إذا (m_disablep) {
                    lhsp
                        = جديد AstAnd{flp, جديد AstNot{flp, m_disablep->clone_وڻ_خالص(خطأ)}, lhsp};
                }
                AstPast* ثابت pastp
                    = جديد AstPast{flp, lhsp, خالي_اشارو, خالي_اشارو, /* propertyTiming */ سچ};
                pastp->dtypeFrom(lhsp);
                pastp->sentreep(new_Sen_وڻ(nodep));
                condp = pastp;
            }
            // Wrap existing PExpr body: if (antecedent) { <original body> } else { /* vacuous pass
            // */ }
            // Only the statements are guarded; declarations stay directly in the block. After
            // V3Fork moves the block's statements into a task, V3Task only handles variables
            // that are direct statements of the task.
            AstBegin* ثابت bodyp = pexprp->bodyp();
            AstIf* ثابت guardp = جديد AstIf{flp, condp};
            لكل (Ast_ڳنڍ* stmtp = bodyp->stmtsp(); stmtp;) {
                Ast_ڳنڍ* ثابت nextp = stmtp->nextp();
                إذا (!VN_IS(stmtp, Var)) guardp->جوڙيو_Thensp(stmtp->unlink_Fr_پٺ());
                stmtp = nextp;
            }
            bodyp->جوڙيو_Stmtsp(guardp);
            nodep->replaceWith(pexprp);
            // Don't iterate pexprp here -- it was already iterated when created
            // (in visit(AstSExpr*)), so delays and disable iff are already processed.
        } وإلا إذا (nodep->isOverlapped()) {
            Ast_ڳنڍ_اظهار* ثابت exprp
                = m_underCover ? static_cast<Ast_ڳنڍ_اظهار*>(جديد AstLogAnd{flp, lhsp, rhsp})
                               : جديد AstLogOr{flp, جديد AstLogNot{flp, lhsp}, rhsp};
            nodep->replaceWith(exprp);
        } وإلا {
            إذا (m_disablep) {
                lhsp = جديد AstAnd{flp, جديد AstNot{flp, m_disablep->clone_وڻ_خالص(خطأ)}, lhsp};
            }

            AstPast* ثابت pastp
                = جديد AstPast{flp, lhsp, خالي_اشارو, خالي_اشارو, /* propertyTiming */ سچ};
            pastp->dtypeFrom(lhsp);
            pastp->sentreep(new_Sen_وڻ(nodep));
            Ast_ڳنڍ_اظهار* ثابت exprp
                = m_underCover ? static_cast<Ast_ڳنڍ_اظهار*>(جديد AstAnd{flp, pastp, rhsp})
                               : جديد AstOr{flp, جديد AstNot{flp, pastp}, rhsp};
            exprp->dtype_سيٽ_ٻٽو_انگ();
            nodep->replaceWith(exprp);
        }
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ گهمو(AstUntil* nodep) override {
        File_سٽ* ثابت flp = nodep->fileline();
        UASSERT_OBJ(
            !m_pexprp, nodep,
            "'" << nodep->verilogKwd()
                << "' in complex property expression should have been rejected by V3AssertNfa");
        إذا (nodep->isStrong()
            && (v3Global.opt.timing().is_سيٽ_False() || !v3Global.opt.timing().is_سيٽ_True())) {
            nodep->v3warn(E_NOTIMING, nodep->verilogKwd() << " requires --timing");
            nodep->replaceWith(جديد AstConst{flp, AstConst::ٻٽو_انگ_False{}});
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            أرجع;
        }
        إذا (nodep->isStrong()) {
            // p s_until q / p s_until_with q: q must eventually be true. Until then, p must
            // be true on every sampled tick. For s_until, check q first: when q is true on
            // this tick, p is not required. For s_until_with, p must be true on the q tick too.
            Ast_ڳنڍ_اظهار* ثابت rawLhsp = nodep->lhsp()->unlink_Fr_پٺ();
            Ast_ڳنڍ_اظهار* ثابت rawRhsp = nodep->rhsp()->unlink_Fr_پٺ();
            AstSampled* ثابت lhsp = جديد AstSampled{flp, rawLhsp, rawLhsp->dtypep(), سچ};
            AstSampled* ثابت rhsp = جديد AstSampled{flp, rawRhsp, rawRhsp->dtypep(), سچ};
            Ast_ڳنڍ_اظهار* finalCondp = rhsp->clone_وڻ_خالص(خطأ);
            إذا (nodep->isOverlapping()) {
                finalCondp = جديد AstLogAnd{flp, lhsp->clone_وڻ_خالص(خطأ), finalCondp};
            }

            // Track active assertion attempts. Only the count is needed to emit final failures.
            AstVar* ثابت activep = جديد AstVar{flp, VVarType::MODULETEMP, m_سرگرم_نالا.get(""),
                                               nodep->ڳوليو_Basic_DType(VBasicDTypeKwd::UINT32)};
            activep->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->جوڙيو_Stmtsp(activep);

            AstVar* ثابت donep
                = جديد AstVar{flp, VVarType::BLOCKTEMP, "__VassertDone", nodep->ڳوليو_ٻٽو_انگ_DType()};
            donep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            خودڪار سيٽ_پورو = [&]() {
                أرجع جديد Ast_سونپيو{flp, جديد AstVarRef{flp, donep, VAccess::WRITE},
                                     جديد AstConst{flp, AstConst::ٻٽو_انگ_True{}}};
            };

            AstLoop* ثابت loopp = جديد AstLoop{flp};
            Ast_سونپيو* ثابت decrementVar = جديد Ast_سونپيو{
                flp, جديد AstVarRef{flp, activep, VAccess::WRITE},
                جديد AstSub{flp, جديد AstVarRef{flp, activep, VAccess::READ}, جديد AstConst{flp, 1}}};
            loopp->جوڙيو_Stmtsp(جديد AstLoopTest{
                flp, loopp, جديد AstLogNot{flp, جديد AstVarRef{flp, donep, VAccess::READ}}});
            {
                Ast_ڳنڍ* ثابت passp = جديد AstPExprClause{flp};
                passp->جوڙيو_ايندڙ(decrementVar);
                passp->جوڙيو_ايندڙ(سيٽ_پورو());
                Ast_ڳنڍ_اظهار* passCondp = rhsp;
                إذا (nodep->isOverlapping()) {
                    passCondp = جديد AstLogAnd{flp, lhsp->clone_وڻ_خالص(خطأ), passCondp};
                }
                loopp->جوڙيو_Stmtsp(جديد AstIf{flp, passCondp, passp});
            }
            {
                Ast_ڳنڍ* ثابت failp = جديد AstPExprClause{flp, خطأ};
                failp->جوڙيو_ايندڙ(decrementVar->clone_وڻ(خطأ));
                failp->جوڙيو_ايندڙ(سيٽ_پورو());
                loopp->جوڙيو_Stmtsp(جديد AstIf{
                    flp,
                    جديد AstLogAnd{flp,
                                  جديد AstLogNot{flp, جديد AstVarRef{flp, donep, VAccess::READ}},
                                  جديد AstLogNot{flp, lhsp}},
                    failp});
            }
            AstDelay* ثابت delayp = جديد AstDelay{flp, جديد AstConst{flp, 1}, خطأ};
            delayp->timeunit(m_modp->timeunit());
            loopp->جوڙيو_Stmtsp(جديد AstIf{
                flp, جديد AstLogNot{flp, جديد AstVarRef{flp, donep, VAccess::READ}}, delayp});

            // Main assertion block
            AstBegin* ثابت bodyp = جديد AstBegin{flp, m_بلاڪ_نالا.get(""), خالي_اشارو, سچ};
            bodyp->جوڙيو_Stmtsp(donep);
            bodyp->جوڙيو_Stmtsp(جديد Ast_سونپيو{flp, جديد AstVarRef{flp, donep, VAccess::WRITE},
                                           جديد AstConst{flp, AstConst::ٻٽو_انگ_False{}}});
            File_سٽ* ثابت flp = activep->fileline();
            bodyp->جوڙيو_Stmtsp(
                جديد Ast_سونپيو{flp, جديد AstVarRef{flp, activep, VAccess::WRITE},
                              جديد Ast_جوڙيو{flp, جديد AstVarRef{flp, activep, VAccess::READ},
                                         جديد AstConst{flp, 1}}});
            bodyp->جوڙيو_Stmtsp(loopp);

            // Validate assertion condition for each active assert
            AstVar* ثابت سرگرم_Countp = جديد AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                    nodep->ڳوليو_Basic_DType(VBasicDTypeKwd::UINT32)};
            سرگرم_Countp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

            Ast_سونپيو* ثابت init_سرگرم_Countp
                = جديد Ast_سونپيو{flp, جديد AstVarRef{flp, سرگرم_Countp, VAccess::WRITE},
                                جديد AstVarRef{flp, activep, VAccess::READ}};
            AstLoop* ثابت finalLoopp = جديد AstLoop{flp};
            finalLoopp->جوڙيو_Stmtsp(
                جديد AstLoopTest{flp, finalLoopp,
                                جديد AstNeq{flp, جديد AstVarRef{flp, سرگرم_Countp, VAccess::READ},
                                           جديد AstConst{flp, 0}}});
            finalLoopp->جوڙيو_Stmtsp(جديد AstIf{flp, finalCondp, جديد AstPExprClause{flp},
                                            جديد AstPExprClause{flp, خطأ}});
            finalLoopp->جوڙيو_Stmtsp(
                جديد Ast_سونپيو{flp, جديد AstVarRef{flp, سرگرم_Countp, VAccess::WRITE},
                              جديد AstSub{flp, جديد AstVarRef{flp, سرگرم_Countp, VAccess::READ},
                                         جديد AstConst{flp, 1}}});

            // Final assertion block
            AstBegin* ثابت finalp = جديد AstBegin{flp, m_بلاڪ_نالا.get(""), خالي_اشارو, سچ};
            finalp->جوڙيو_Stmtsp(سرگرم_Countp);
            finalp->جوڙيو_Stmtsp(init_سرگرم_Countp);
            finalp->جوڙيو_Stmtsp(finalLoopp);

            AstPExpr* ثابت pexprp = جديد AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
            VL_RESTORER(m_pexprp);
            VL_RESTORER(m_has_چڪر_Delay);
            m_pexprp = pexprp;
            m_has_چڪر_Delay = خطأ;
            iterate(bodyp);
            iterate(finalp);

            UASSERT_OBJ(!m_has_چڪر_Delay, nodep,
                        "s_until cycle delay should have been handled by V3AssertNfa");

            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            أرجع;
        }
        AstLoop* ثابت loopp = جديد AstLoop{flp};
        Ast_ڳنڍ_اظهار* ثابت rhsp = nodep->rhsp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* ثابت lhsp = nodep->lhsp()->unlink_Fr_پٺ();
        AstLogAnd* ثابت loopCondp = جديد AstLogAnd{flp, lhsp, جديد AstLogNot{flp, rhsp}};
        loopp->جوڙيو_Stmtsp(جديد AstLoopTest{flp, loopp, loopCondp});
        loopp->جوڙيو_Stmtsp(جديد AstEventControl{flp, new_Sen_وڻ(nodep), خالي_اشارو});

        Ast_ڳنڍ_اظهار* ثابت rhsCopyp = rhsp->clone_وڻ_خالص(خطأ);
        Ast_ڳنڍ_اظهار* ثابت passCondp
            = nodep->isOverlapping() ? جديد AstLogAnd{flp, lhsp->clone_وڻ_خالص(خطأ), rhsCopyp}
                                     : rhsCopyp;
        AstBegin* ثابت beginp = جديد AstBegin{flp, m_بلاڪ_نالا.get(""), loopp, سچ};
        beginp->جوڙيو_Stmtsp(
            جديد AstIf{flp, passCondp, جديد AstPExprClause{flp}, جديد AstPExprClause{flp, خطأ}});

        AstPExpr* ثابت pexprp = جديد AstPExpr{flp, beginp, nodep->dtypep()};
        pexprp->user1(1);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }

    فراغ گهمو(AstDefaultDisable* nodep) override {
        VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
    }
    فراغ گهمو(AstInferredDisable* nodep) override {
        Ast_ڳنڍ* newp;
        إذا (m_defaultDisablep) {
            newp = m_defaultDisablep->condp()->clone_وڻ_خالص(سچ);
        } وإلا {
            newp = جديد AstConst{nodep->fileline(), AstConst::ٻٽو_انگ_False{}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }

    فراغ گهمو(AstPropSpec* nodep) override {
        nodep = substitutePropertyCall(nodep);
        iterate_And_ايندڙ_Null(nodep->sensesp());
        إذا (m_senip && m_senip != nodep->sensesp())
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Only one PSL clock allowed per assertion");
        ثابت AstCover* ثابت coverp = VN_CAST(nodep->backp(), Cover);
        ثابت منطقي seqEvent = coverp && coverp->isSeqEvent();
        إذا (!nodep->disablep() && m_defaultDisablep && !seqEvent) {
            nodep->disablep(m_defaultDisablep->condp()->clone_وڻ_خالص(سچ));
        }
        m_disablep = nodep->disablep();
        // Unlink and just keep a pointer to it, convert to sentree as needed
        m_senip = nodep->sensesp();
        iterateNull(nodep->disablep());
        إذا (!VN_AS(nodep->backp(), ڳنڍ_Cover_Or_Assert)->فوري()) {
            ثابت Ast_ڳنڍ_DType* ثابت propDtp = nodep->propp()->dtypep();
            nodep->propp(جديد AstSampled{nodep->fileline(), nodep->propp()->unlink_Fr_پٺ(),
                                        propDtp->dtypep(), سچ});
        }
        // cover counts non-vacuous matches only (IEEE 1800-2023 16.15.2), so an
        // implication antecedent must hold; assert passes vacuously instead.
        VL_RESTORER(m_underCover);
        m_underCover = VN_IS(nodep->backp(), Cover);
        iterate(nodep->propp());
        iterateNull(nodep->ميلاپ_Countp());
    }
    فراغ گهمو(AstPExpr* nodep) override {
        // V3AssertNfa handles multi-cycle property expressions before this pass,
        // so the following unsupported paths are defensive and typically unreached.
        إذا (m_pexprp && m_pexprp->user1()) {  // LCOV_EXCL_START
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Complex property expression inside 'until''");
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::ٻٽو_انگ_False{}});
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            أرجع;
        }  // LCOV_EXCL_STOP
        إذا (AstLogNot* ثابت notp = VN_CAST(nodep->backp(), LogNot)) {
            notp->replaceWith(nodep->unlink_Fr_پٺ());
            VL_DO_DANGLING(ڌڪو_Deletep(notp), notp);
            iterate(nodep);
            أرجع;
        }
        // Sequence expression as antecedent of implication is not yet supported
        إذا (AstImplication* ثابت implp = VN_CAST(nodep->backp(), Implication)) {
            إذا (implp->lhsp() == nodep) {  // LCOV_EXCL_START
                implp->v3warn(E_UNSUPPORTED,
                              "Unsupported: Implication with sequence expression as antecedent");
                nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::ٻٽو_انگ_False{}});
                VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
                أرجع;
            }  // LCOV_EXCL_STOP
        }
        VL_RESTORER(m_pexprp);
        VL_RESTORER(m_disableSeqIfp);
        m_pexprp = nodep;

        إذا (m_disablep) {
            ثابت AstSampled* sampledp = خالي_اشارو;
            إذا (m_disablep->exists([&sampledp](ثابت AstSampled* ثابت sp) {
                    sampledp = sp;
                    أرجع سچ;
                })) {
                sampledp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: $sampled inside disabled condition of a sequence");
                m_disablep = جديد AstConst{m_disablep->fileline(), AstConst::ٻٽو_انگ_False{}};
                // always a copy is used, so remove it now
                ڌڪو_Deletep(m_disablep);  // m_disablep used later
            }
            File_سٽ* ثابت flp = nodep->fileline();
            // Add counter which counts times the condition turned true
            AstVar* ثابت disableCntp
                = جديد AstVar{flp, VVarType::MODULETEMP, m_disable_Cnt_نالا.get(""),
                             nodep->ڳوليو_Basic_DType(VBasicDTypeKwd::UINT32)};
            disableCntp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->جوڙيو_Stmtsp(disableCntp);
            AstVarRef* ثابت readCntRefp = جديد AstVarRef{flp, disableCntp, VAccess::READ};
            AstVarRef* ثابت writeCntRefp = جديد AstVarRef{flp, disableCntp, VAccess::WRITE};
            Ast_سونپيو* ثابت incrStmtp = جديد Ast_سونپيو{
                flp, writeCntRefp, جديد Ast_جوڙيو{flp, readCntRefp, جديد AstConst{flp, 1}}};
            AstAlways* ثابت alwaysp
                = جديد AstAlways{flp, VAlwaysKwd::ALWAYS,
                                جديد Ast_Sen_وڻ{flp, جديد Ast_Sen_شي{flp, VEdgeType::ET_POSEDGE,
                                                                   m_disablep->clone_وڻ(خطأ)}},
                                incrStmtp};
            disableCntp->جوڙيو_ايندڙ_هتي(alwaysp);

            // Store value of that counter at the beginning of sequence evaluation
            AstBegin* ثابت bodyp = nodep->bodyp();
            AstVar* ثابت initialCntp = جديد AstVar{flp, VVarType::BLOCKTEMP, "__VinitialCnt",
                                                   nodep->ڳوليو_Basic_DType(VBasicDTypeKwd::UINT32)};
            initialCntp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            Ast_سونپيو* ثابت assignp
                = جديد Ast_سونپيو{flp, جديد AstVarRef{flp, initialCntp, VAccess::WRITE},
                                readCntRefp->clone_وڻ(خطأ)};
            // Prepend to the sequence body to keep statement list structure valid.
            UASSERT_OBJ(!bodyp->name().خالي_جايون(), bodyp, "Sequence body should be named");
            Ast_ڳنڍ* ثابت newStmtsp = Ast_ڳنڍ::جوڙيو_ايندڙ<Ast_ڳنڍ, Ast_ڳنڍ>(initialCntp, assignp);
            bodyp->prependStmtsp(newStmtsp);

            m_disableSeqIfp
                = جديد AstIf{flp, جديد AstEq{flp, جديد AstVarRef{flp, initialCntp, VAccess::READ},
                                           readCntRefp->clone_وڻ(خطأ)}};
            // Delete it, because it is always copied before insetion to the AST
            ڌڪو_Deletep(m_disableSeqIfp);  // m_disableSeqIfp used later
        }
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultClkEvtVarp);
        VL_RESTORER(m_defaultDisablep);
        VL_RESTORER(m_modp);
        إذا (!VN_IS(nodep, Class)) {
            // A class nested in a module is in the scope of the module's default clocking
            m_defaultClockingp = خالي_اشارو;
            m_defaultClkEvtVarp = خالي_اشارو;
        }
        m_defaultDisablep = nodep->defaultDisablep();
        m_modp = nodep;
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_Gen_بلاڪ* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    فراغ گهمو(AstProperty* nodep) override {
        // The body will be visited when will be substituted in place of property reference
        // (AstFuncRef)
        VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
    }
    فراغ گهمو(AstSequence* nodep) override {
        // Sequence declarations are not visited directly; their bodies are inlined
        // at call sites by visit(AstFuncRef*). Collect for post-traversal cleanup.
        m_seqs_منزل_Cleanup.ڌڪو_پٺ(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح AssertPreVisitor(AstNetlist* nodep)
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
                VL_DO_DANGLING(seqp->unlink_Fr_پٺ()->delete_وڻ(), seqp);
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
    V3Global::dump_Check_Global_وڻ("assertpre", 0, dump_وڻ_Either_Level() >= 3);
}
