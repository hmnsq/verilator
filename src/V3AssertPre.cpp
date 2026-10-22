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
    فراغ ویکھو(AstNetlist* nodep) override { iterate_And_اگلا_Const_Null(nodep->modulesp()); }
    فراغ ویکھو(Ast_گرہ_Module* nodep) override { iterate_And_اگلا_Const_Null(nodep->stmtsp()); }
    فراغ ویکھو(Ast_Gen_بلاک* nodep) override { iterate_And_اگلا_Const_Null(nodep->itemsp()); }
    فراغ ویکھو(AstClocking* nodep) override {
        لكل (Ast_گرہ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            إذا (Ast_Clocking_شے* ثابت citemp = VN_CAST(itemp, Clocking_شے)) {
                citemp->user3p(nodep);
            }
        }
    }
    // Clocking blocks are only in modules and their generate blocks
    فراغ ویکھو(Ast_گرہ*) override {}

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
        Ast_Clocking_شے* itemp;  // Clocking item of the driven clockvar
        Ast_گرہ_اظہاریہ* refp;  // Reference to the driven clockvar
    };

    // NODE STATE
    // AstClockingItem::user1p()         // AstVar*.      varp() of ClockingItem after unlink
    // AstClockingItem::user2p()         // AstVar*.      Flag set by drives of output clockvar
    // AstClockingItem::user3p()         // AstClocking*. Clocking block of the item, captured
    //                                   //               by AssertPreClockingVisitor
    // AstPExpr::user1()                 // bool.         Created from AstUntil
    ثابت VNUser1_وچ_Use m_inuser1;
    ثابت VNUser2_وچ_Use m_inuser2;
    ثابت VNUser3_وچ_Use m_inuser3;
    // STATE
    // Current context:
    AstNetlist* ثابت m_netlistp = خالی_سوچک;  // Current netlist
    Ast_گرہ_Module* m_modp = خالی_سوچک;  // Current module
    AstClocking* m_clockingp = خالی_سوچک;  // Current clocking block
    // Reset each module:
    AstClocking* m_defaultClockingp = خالی_سوچک;  // Default clocking for current module
    AstVar* m_defaultClkEvtVarp = خالی_سوچک;  // Event var for default clocking (for ##0)
    AstDefaultDisable* m_defaultDisablep = خالی_سوچک;  // Default disable for current module
    // Reset each assertion:
    Ast_Sen_شے* m_senip = خالی_سوچک;  // Last sensitivity
    // Reset each always:
    Ast_Sen_شے* m_seniAlwaysp = خالی_سوچک;  // Last sensitivity in always
    // Reset each assertion:
    Ast_گرہ_اظہاریہ* m_disablep = خالی_سوچک;  // Last disable
    AstIf* m_disableSeqIfp = خالی_سوچک;  // Used for handling disable iff in sequences
    AstPExpr* m_pexprp = خالی_سوچک;  // Last AstPExpr
    منطقي m_underCover = خطأ;  // True if the enclosing assertion is a cover
    // Other:
    V3_Unique_ناں m_چکر_Dly_ناں{"__VcycleDly"};  // Cycle delay counter name generator
    V3_Unique_ناں m_cons_Rep_ناں{"__VconsRep"};  // Consecutive repetition counter name generator
    V3_Unique_ناں m_goto_Rep_ناں{"__VgotoRep"};  // Goto repetition counter name generator
    V3_Unique_ناں m_non_Cons_Rep_ناں{"__VnonConsRep"};  // Nonconsecutive rep name generator
    V3_Unique_ناں m_disable_Cnt_ناں{"__VdisableCnt"};  // Disable condition counter name generator
    V3_Unique_ناں m_بلاک_ناں{"__VassertBlock"};  // Names of blocks with temporaries
    V3_Unique_ناں m_prop_Var_ناں{"__Vpropvar"};  // Property-local variable name generator
    V3_Unique_ناں m_سرگرم_ناں{"__VassertsActive"};  // Active asserts map name generator
    V3_Unique_ناں m_driven_ناں{"__VclockingDriven"};  // Clockvar drive flag name generator
    منطقي m_وچ_سونپو = خطأ;  // True if in an AssignNode
    منطقي m_وچ_سونپو_Dly_Lhs = خطأ;  // True if in AssignDly's LHS
    منطقي m_وچ_Synch_Drive = خطأ;  // True if in synchronous drive
    منطقي m_وچ_Concat_سونپو = خطأ;  // True if in assignment to a concatenation
    std::سمتیہ<SynchDrive> m_drives;  // Clockvars written by the synchronous drive
    منطقي m_has_چکر_Delay = خطأ;  // True if node has cycle delay beneath
    std::سمتیہ<AstVarXRef*> m_xrefsp;  // list of xrefs that need name fixup
    std::سمتیہ<AstSequence*> m_seqs_منزل_Cleanup;  // Sequences to clean up after traversal

    // METHODS

    // Flag set by each drive of an output clockvar, added to the module with its clocking item
    AstVar* getCreateDrivenVarp(Ast_Clocking_شے* itemp) {
        إذا (!itemp->user2p()) {
            AstVar* ثابت varp = جديد AstVar{itemp->fileline(), VVarType::MODULETEMP,
                                            m_driven_ناں.get(itemp), itemp->لبھو_ثنائی_ہندسہ_DType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            itemp->user2p(varp);
        }
        أرجع VN_AS(itemp->user2p(), Var);
    }
    // Reference to a variable in the scope of a driven clockvar, made like the clockvar's
    ساكن Ast_گرہ_اظہاریہ* newDriveRefp(File_سطر* flp, ثابت SynchDrive& drive, AstVar* varp,
                                     ثابت VAccess& access) {
        إذا (ثابت AstVarXRef* ثابت xrefp = VN_CAST(drive.refp, VarXRef)) {
            أرجع جديد AstVarXRef{flp, varp, xrefp->dotted(), access};
        } وإلا إذا (ثابت AstMemberSel* ثابت selp = VN_CAST(drive.refp, MemberSel)) {
            // The interface expression is evaluated again, so it must not have side effects;
            // cloneTreePure warns if it has any
            AstMemberSel* ثابت newSelp
                = جديد AstMemberSel{flp, selp->fromp()->clone_درخت_خالص(خطأ), varp};
            newSelp->access(access);
            أرجع newSelp;
        }
        أرجع جديد AstVarRef{flp, varp, access};
    }
    // Assignment setting the drive flag of a driven clockvar
    Ast_سونپو* newDrivenSetp(File_سطر* flp, ثابت SynchDrive& drive) {
        AstVar* ثابت varp = getCreateDrivenVarp(drive.itemp);
        Ast_سونپو* ثابت setp = جديد Ast_سونپو{flp, newDriveRefp(flp, drive, varp, VAccess::WRITE),
                                              جديد AstConst{flp, AstConst::ثنائی_ہندسہ_True{}}};
        setp->user1(سچ);
        أرجع setp;
    }
    // Clocking block of a clocking item
    ساكن AstClocking* clocking_دا(ثابت Ast_Clocking_شے* itemp) {
        UASSERT_OBJ(itemp->user3p(), itemp, "Clocking item not captured in a clocking block");
        أرجع VN_AS(itemp->user3p(), Clocking);
    }
    // Clocking event of a driven clockvar, referenced from the drive like the clockvar, or
    // nullptr if unsupported, which is reported on the drive's cycle delay
    Ast_Sen_درخت* newDriveSentreep(AstDelay* delayp, ثابت SynchDrive& drive) {
        Ast_Sen_درخت* ثابت origp = clocking_دا(drive.itemp)->sentreep();
        إذا (VN_IS(drive.refp, VarRef)) أرجع origp->clone_درخت(خطأ);
        إذا (origp->exists([](ثابت AstVarXRef*) { أرجع سچ; })) {
            // The reference is relative to the clocking block
            delayp->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in synchronous drive to"
                                          " clockvar in another scope, whose clocking event"
                                          " has a hierarchical reference");
            أرجع خالی_سوچک;
        }
        ثابت AstMemberSel* ثابت selp = VN_CAST(drive.refp, MemberSel);
        // Processes outside classes evaluate their events also when not waiting on them,
        // possibly before the interface reference is set, so the event is guarded against a
        // null reference, which needs an integral value; scheduling does not guard it
        ثابت منطقي guardNull = selp && !VN_IS(m_modp, Class);
        إذا (guardNull) {
            لكل (Ast_Sen_شے* itemp = origp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Sen_شے)) {
                إذا (!itemp->sensp()->dtypep()->skipRefp()->isIntegralOrPacked()) {
                    delayp->v3warn(E_UNSUPPORTED,
                                   "Unsupported: cycle delay in synchronous drive through"
                                   " an interface reference outside a class, to clocking"
                                   " block with non-integral clocking event");
                    أرجع خالی_سوچک;
                }
            }
        }
        // Interface declaring the clocking block, if driven through an interface reference
        AstIface* ثابت ifacep
            = selp ? VN_AS(selp->fromp()->dtypep()->skipRefp(), IfaceRefDType)->iface_راہیں_Cellp()
                   : خالی_سوچک;
        Ast_Sen_درخت* ثابت sentreep = origp->clone_درخت(خطأ);
        std::سمتیہ<AstVarRef*> refps;
        sentreep->foreach([&](AstVarRef* refp) {
            إذا (!refp->classOrPackagep()) refps.ڈالو_پٹھ(refp);
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
            VL_DO_DANGLING(ڈالو_Deletep(refp), refp);
        }
        إذا (guardNull) {
            لكل (Ast_Sen_شے* itemp = sentreep->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Sen_شے)) {
                guardNullIface(itemp, selp->fromp());
            }
        }
        أرجع sentreep;
    }
    // Make an integral clocking event through an interface reference false while the reference
    // is null
    ساكن فراغ guardNullIface(Ast_Sen_شے* sensesp, Ast_گرہ_اظہاریہ* fromp) {
        File_سطر* ثابت flp = sensesp->fileline();
        Ast_گرہ_اظہاریہ* ثابت sensp = sensesp->sensp()->unlink_Fr_پٹھ();
        sensesp->sensp(جديد AstCond{flp, newNotNullp(flp, fromp), sensp,
                                   جديد AstConst{flp, AstConst::DTyped{}, sensp->dtypep()}});
        إذا (Ast_گرہ_اظہاریہ* ثابت condp = sensesp->condp()) {
            condp->unlink_Fr_پٹھ();
            sensesp->condp(جديد AstLogAnd{flp, newNotNullp(flp, fromp), condp});
        }
    }
    ساكن Ast_گرہ_اظہاریہ* newNotNullp(File_سطر* flp, Ast_گرہ_اظہاریہ* fromp) {
        أرجع جديد AstNeq{flp, fromp->clone_درخت_خالص(خطأ), جديد AstConst{flp, AstConst::Null{}}};
    }
    // Procedural '##0': if the default clocking event has not yet occurred in this time step,
    // wait for it; otherwise continue without suspension (IEEE 1800-2023 14.11)
    AstIf* new_چکر_Delay_Zerop(File_سطر* flp) ثابت {
        AstVar* ثابت evtVarp = m_defaultClkEvtVarp;
        UASSERT_OBJ(evtVarp, flp, "Default clocking event var not pre-created");
        AstCMethodHard* ثابت isTriggeredp = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, evtVarp, VAccess::READ}, VCMethod::EVENT_IS_TRIGGERED};
        isTriggeredp->dtype_سیٹ_ثنائی_ہندسہ();
        AstEventControl* ثابت waitp = جديد AstEventControl{
            flp,
            جديد Ast_Sen_درخت{flp, جديد Ast_Sen_شے{flp, VEdgeType::ET_EVENT,
                                               جديد AstVarRef{flp, evtVarp, VAccess::READ}}},
            خالی_سوچک};
        أرجع جديد AstIf{flp, جديد AstNot{flp, isTriggeredp}, waitp};
    }

    ساكن فراغ checkSamplingFuncDType(Ast_گرہ_اظہاریہ* nodep, ثابت Ast_گرہ* exprp) {
        ثابت Ast_گرہ_DType* ثابت dtypep = exprp->dtypep()->skipRefp();
        إذا (!dtypep->isIntegralOrPacked()) {
            nodep->v3error("Expected numeric type, but got a " << dtypep->prettyDTypeNameQ()
                                                               << " data type");
        }
    }

    Ast_Sen_درخت* new_Sen_درخت(Ast_گرہ* nodep, Ast_Sen_درخت* useTreep = خالی_سوچک,
                           Ast_گرہ_Cover_Or_Assert* cassertp = خالی_سوچک) {
        // Create sentree based on clocked or default clock
        // Return nullptr for always
        إذا (useTreep) أرجع useTreep;
        Ast_Sen_درخت* newp = خالی_سوچک;
        Ast_Sen_شے* senip = m_senip;
        منطقي fromAlways = خطأ;
        إذا (!senip && m_defaultClockingp) senip = m_defaultClockingp->sentreep()->sensesp();
        إذا (!senip) {
            senip = m_seniAlwaysp;
            fromAlways = سچ;
        }
        إذا (!senip) {
            nodep->v3error("Concurrent assertion has no clock (IEEE 1800-2023 16.16)\n"
                           << nodep->warn_ہور()
                           << "... Suggest provide a clocking event, a default"
                              " clocking, or a clocked procedural context");
            newp = جديد Ast_Sen_درخت{nodep->fileline(), خالی_سوچک};
        } وإلا {
            إذا (cassertp && fromAlways) cassertp->senFromAlways(سچ);
            newp = جديد Ast_Sen_درخت{nodep->fileline(), senip->clone_درخت(سچ)};
        }
        أرجع newp;
    }
    Ast_گرہ_اظہاریہ* getSequenceBodyExprp(ثابت AstSequence* ثابت seqp) ثابت {
        // The statements in AstSequence are optional AstVar (ports) followed by body expr.
        Ast_گرہ* bodyp = seqp->stmtsp();
        طالما (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        أرجع VN_CAST(bodyp, گرہ_اظہاریہ);
    }
    AstPropSpec* getPropertyExprp(ثابت AstProperty* ثابت propp) {
        // Statements in AstProperty: AstVar (ports/local vars),
        // AstInitialStaticStmt/AstInitialAutomaticStmt (var init), AstPropSpec (body).
        Ast_گرہ* propExprp = propp->stmtsp();
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
        Ast_گرہ_اظہاریہ* bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        // Clone the body expression since the sequence may be referenced multiple times
        Ast_گرہ_اظہاریہ* clonedp = bodyExprp->clone_درخت(خطأ);
        // Build substitution map, then do a single traversal to replace all formals
        // (textual substitution per IEEE 16.8.2).
        ثابت V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<ثابت AstVar*, Ast_گرہ_اظہاریہ*> پورٹ_Map;
        لكل (ثابت خودکار& tconnect : tconnects) {
            پورٹ_Map[tconnect.پہلا] = tconnect.دوجا->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            ثابت خودکار it = پورٹ_Map.لبھو(refp->varp());
            إذا (it != پورٹ_Map.سرا()) {
                refp->replaceWith(it->دوجا->clone_درخت(خطأ));
                VL_DO_DANGLING(ڈالو_Deletep(refp), refp);
            }
        });
        لكل (ثابت خودکار& tconnect : tconnects) {
            ڈالو_Deletep(tconnect.دوجا->exprp()->unlink_Fr_پٹھ());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        // Replace the FuncRef with the inlined body
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(ڈالو_Deletep(funcrefp), funcrefp);
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
        propExprp = propExprp->clone_درخت(خطأ);
        // Build substitution maps for formal arguments and property-local
        // variables, then perform a single foreach to apply all replacements.
        // Map port vars to their actual argument expressions
        ثابت V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propp->stmtsp());
        std::unordered_map<ثابت AstVar*, Ast_گرہ_اظہاریہ*> پورٹ_Map;
        لكل (ثابت خودکار& tconnect : tconnects) {
            پورٹ_Map[tconnect.پہلا] = tconnect.دوجا->exprp();
        }

        // Promote property-local variables (non-port vars, IEEE 16.10) to
        // module-level __Vpropvar temps. Cross-cycle persistence is handled
        // by the match item lowering in visit(AstImplication*).
        std::unordered_map<ثابت AstVar*, AstVar*> localVarMap;
        لكل (Ast_گرہ* stmtp = propp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (!varp->isIO()) {
                    ثابت string newName = m_prop_Var_ناں.get(varp);
                    AstVar* ثابت newVarp = جديد AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->جوڑو_Stmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        // Single traversal: substitute ports and update local var references
        propExprp->foreach([&](AstVarRef* refp) {
            ثابت خودکار پورٹ_It = پورٹ_Map.لبھو(refp->varp());
            إذا (پورٹ_It != پورٹ_Map.سرا()) {
                refp->replaceWith(پورٹ_It->دوجا->clone_درخت(خطأ));
                VL_DO_DANGLING(ڈالو_Deletep(refp), refp);
                أرجع;
            }
            ثابت خودکار localIt = localVarMap.لبھو(refp->varp());
            إذا (localIt != localVarMap.سرا()) refp->varp(localIt->دوجا);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        // Clean up argument expressions
        لكل (ثابت خودکار& tconnect : tconnects) {
            ڈالو_Deletep(tconnect.دوجا->exprp()->unlink_Fr_پٹھ());
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
                    ڈالو_Deletep(propExprp->disablep()->unlink_Fr_پٹھ());
                }
                // If disable iff is in outer property, move it to inner
                إذا (nodep->disablep()) {
                    Ast_گرہ_اظہاریہ* ثابت disablep = nodep->disablep()->unlink_Fr_پٹھ();
                    propExprp->disablep(disablep);
                }

                إذا (nodep->sensesp() && propExprp->sensesp()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Clock event before property call and in its body");
                    ڈالو_Deletep(propExprp->sensesp()->unlink_Fr_پٹھ());
                }
                // If clock event is in outer property, move it to inner
                إذا (nodep->sensesp()) {
                    Ast_Sen_شے* ثابت sensesp = nodep->sensesp();
                    sensesp->unlink_Fr_پٹھ();
                    propExprp->sensesp(sensesp);
                }

                // Now substitute property reference with property body
                nodep->replaceWith(propExprp);
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                أرجع propExprp;
            }
        }
        أرجع nodep;
    }

    فراغ substituteNestedPropertyCall(AstFuncRef* funcrefp, ثابت AstProperty* propp) {
        AstPropSpec* ثابت specp = clonePropertyBody(funcrefp, propp);
        Ast_گرہ* replp = خالی_سوچک;
        إذا (specp->disablep()) {
            funcrefp->v3error("Illegal 'disable iff' in a nested property instance"
                              " (IEEE 1800-2023 16.12.1)");
        } وإلا إذا (specp->sensesp()) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: clock event in a nested property instance");
        } وإلا إذا (specp->propp()->exists(
                       [](ثابت Ast_گرہ_اظہاریہ* np) { أرجع np->is_Multi_چکر_Sva(); })) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: multi-cycle body in a nested property instance");
        } وإلا {
            replp = specp->propp()->unlink_Fr_پٹھ();
        }
        إذا (!replp) replp = جديد AstConst{funcrefp->fileline(), AstConst::ثنائی_ہندسہ_False{}};
        funcrefp->replaceWith(replp);
        VL_DO_DANGLING(ڈالو_Deletep(funcrefp), funcrefp);
        VL_DO_DANGLING(ڈالو_Deletep(specp), specp);
    }

    // VISITORS
    //========== Statements
    فراغ ویکھو(AstClocking* ثابت nodep) override {
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
        إذا (nodep->eventp()) nodep->جوڑو_اگلا_ایتھے(nodep->eventp()->unlink_Fr_پٹھ());
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_پٹھ()), nodep);
    }
    فراغ ویکھو(AstModportClockingRef* ثابت nodep) override {
        // It has to be converted to a list of ModportClockingVarRefs,
        // because clocking blocks are removed in this pass
        لكل (Ast_گرہ* itemp = nodep->clockingp()->itemsp(); itemp; itemp = itemp->nextp()) {
            إذا (Ast_Clocking_شے* ثابت citemp = VN_CAST(itemp, Clocking_شے)) {
                إذا (AstVar* ثابت varp
                    = citemp->varp() ? citemp->varp() : VN_AS(citemp->user1p(), Var)) {
                    AstModportVarRef* ثابت modVarp = جديد AstModportVarRef{
                        nodep->fileline(), varp->name(), citemp->direction()};
                    modVarp->varp(varp);
                    nodep->جوڑو_اگلا_ایتھے(modVarp);
                    إذا (citemp->direction().isOutput()) {
                        // Drives through the modport set the drive flag
                        AstVar* ثابت drivenVarp = getCreateDrivenVarp(citemp);
                        AstModportVarRef* ثابت drivenRefp = جديد AstModportVarRef{
                            nodep->fileline(), drivenVarp->name(), VDirection::OUTPUT};
                        drivenRefp->varp(drivenVarp);
                        nodep->جوڑو_اگلا_ایتھے(drivenRefp);
                    }
                }
            }
        }
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_پٹھ()), nodep);
    }
    فراغ ویکھو(Ast_Clocking_شے* ثابت nodep) override {
        // Get a ref to the sampled/driven variable
        AstVar* ثابت varp = nodep->varp();
        إذا (!varp) {
            // Unused item
            أرجع;
        }
        // Flag set by drives of an output clockvar, possibly visited before this item
        إذا (nodep->direction().isOutput()) m_modp->جوڑو_Stmtsp(getCreateDrivenVarp(nodep));
        File_سطر* ثابت flp = nodep->fileline();
        V3Const::constifyEdit(nodep->skewp());
        إذا (!VN_IS(nodep->skewp(), Const)) {
            nodep->skewp()->v3error("Skew must be constant (IEEE 1800-2023 14.4)");
            أرجع;
        }
        AstConst* ثابت skewp = VN_AS(nodep->skewp(), Const);
        إذا (skewp->عدد().isNegative()) skewp->v3error("Skew cannot be negative");
        Ast_گرہ_اظہاریہ* ثابت exprp = nodep->exprp();
        varp->name(m_clockingp->name() + "__DOT__" + varp->name());
        m_clockingp->جوڑو_اگلا_ایتھے(varp->unlink_Fr_پٹھ());
        nodep->user1p(varp);
        varp->user1p(nodep);
        إذا (nodep->direction() == VDirection::OUTPUT) {
            exprp->foreach([](ثابت Ast_گرہ_Var_Ref* varrefp) {
                // Prevent confusing BLKANDNBLK warnings on clockvars due to generated assignments
                varrefp->fileline()->warnOff(V3_Error_کوڈ::BLKANDNBLK, سچ);
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
                flp, جديد Ast_سونپو{flp, skewedWriteRefp, exprp->clone_درخت_خالص(خطأ)}};
            m_modp->جوڑو_Stmtsp(initClockvarp);
            // Each drive sets a flag, so the signal is also assigned when driven with the same
            // value again, e.g. after another assignment to the signal (IEEE 1800-2023 14.16.2)
            AstVar* ثابت drivenVarp = getCreateDrivenVarp(nodep);
            m_modp->جوڑو_Stmtsp(جديد AstInitialStatic{
                flp, جديد Ast_سونپو{flp, جديد AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                   جديد AstConst{flp, AstConst::ثنائی_ہندسہ_False{}}}});
            // Assign the clockvar to the actual var if it was driven
            Ast_سونپو* ثابت assignp
                = جديد Ast_سونپو{flp, exprp->clone_درخت_خالص(خطأ), skewedReadRefp};
            AstIf* ثابت ifp
                = جديد AstIf{flp, جديد AstVarRef{flp, drivenVarp, VAccess::READ},
                            جديد Ast_سونپو{flp, جديد AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                          جديد AstConst{flp, AstConst::ثنائی_ہندسہ_False{}}}};
            ifp->جوڑو_Thensp(assignp);
            // Only in the time step of the clocking event
            AstCMethodHard* ثابت trigp = جديد AstCMethodHard{
                nodep->fileline(), جديد AstVarRef{flp, m_clockingp->ensureEventp(), VAccess::READ},
                VCMethod::EVENT_IS_TRIGGERED};
            trigp->dtype_سیٹ_ثنائی_ہندسہ();
            ifp->condp(جديد AstLogAnd{flp, ifp->condp()->unlink_Fr_پٹھ(), trigp});
            // A skew rounding to no time precision ticks is a zero skew
            Ast_گرہ_اظہاریہ* ثابت ticksp
                = V3Timing::delay_منزل_Ticks(skewp->clone_درخت(خطأ), m_modp->timeunit());
            ثابت منطقي zeroSkew = ticksp->isZero();
            VL_DO_DANGLING(ticksp->delete_درخت(), ticksp);
            إذا (zeroSkew || !skewp->fileline()->timingOn()) {
                // Drive the var in Re-NBA (IEEE 1800-2023 14.16), ignoring skews where timing is
                // turned off
                Ast_Sen_درخت* senTreep = m_clockingp->sentreep()->clone_درخت(خطأ);
                senTreep->جوڑو_Sensesp(جديد Ast_Sen_شے{
                    flp, VEdgeType::ET_بدلیا, جديد AstVarRef{flp, drivenVarp, VAccess::READ}});
                m_clockingp->جوڑو_اگلا_ایتھے(جديد AstAlwaysReactive{flp, senTreep, ifp});
            } وإلا {
                // Fork only the assignment, so this AlwaysPostponed can be retriggered
                // before the assignment happens
                AstFork* ثابت forkp = جديد AstFork{flp, VJoinType::JOIN_NONE};
                forkp->فوری_شروع(سچ);
                forkp->جوڑو_Forksp(جديد AstBegin{flp, "", assignp->unlink_Fr_پٹھ(), سچ});
                ifp->جوڑو_Thensp(forkp);
                // Read the clockvar in Postponed, so a drive late in the time step is applied
                // at this event plus the skew (IEEE 1800-2023 14.16)
                m_clockingp->جوڑو_اگلا_ایتھے(جديد AstAlwaysPostponed{flp, ifp});
                إذا (v3Global.opt.timing().is_سیٹ_True()) {
                    AstDelay* ثابت delayp = جديد AstDelay{flp, skewp->unlink_Fr_پٹھ(), خطأ};
                    delayp->timeunit(m_modp->timeunit());
                    assignp->timingControlp(delayp);
                } وإلا إذا (v3Global.opt.timing().is_سیٹ_False()) {
                    nodep->v3warn(E_NOTIMING,
                                  "Clocking output skew greater than #0 requires --timing");
                } وإلا {
                    nodep->v3warn(E_NEEDTIMINGOPT,
                                  "Use --timing or --no-timing to specify how "
                                  "clocking output skew greater than #0 should be handled");
                }
            }
        } وإلا إذا (nodep->direction() == VDirection::داخلہ) {
            // Ref to the clockvar
            AstVarRef* ثابت refp = جديد AstVarRef{flp, varp, VAccess::WRITE};
            refp->user1(سچ);
            إذا (skewp->عدد().is1_قدم()) {
                // #1step means the value that is sampled is always the signal's last value
                // before the clock edge (IEEE 1800-2023 14.4)
                AstSampled* ثابت sampledp
                    = جديد AstSampled{flp, exprp->clone_درخت_خالص(خطأ), exprp->dtypep(), سچ};
                Ast_سونپو* ثابت assignp = جديد Ast_سونپو{flp, refp, sampledp};
                m_clockingp->جوڑو_اگلا_ایتھے(جديد AstAlways{
                    flp, VAlwaysKwd::ALWAYS, m_clockingp->sentreep()->clone_درخت(خطأ), assignp});
            } وإلا إذا (skewp->isZero()) {
                // #0 means the var has to be sampled in Observed (IEEE 1800-2023 14.13)
                Ast_سونپو* ثابت assignp = جديد Ast_سونپو{flp, refp, exprp->clone_درخت_خالص(خطأ)};
                m_clockingp->جوڑو_اگلا_ایتھے(جديد AstAlwaysObserved{
                    flp, m_clockingp->sentreep()->clone_درخت(خطأ), assignp});
            } وإلا {
                // Create a queue where we'll store sampled values with timestamps
                Ast_Sample_قطار_DType* ثابت قطار_Dtp
                    = جديد Ast_Sample_قطار_DType{flp, exprp->dtypep()};
                m_netlistp->typeTablep()->جوڑو_Typesp(قطار_Dtp);
                AstVar* ثابت قطار_Varp
                    = جديد AstVar{flp, VVarType::MODULETEMP, "__Vqueue__" + varp->name(), قطار_Dtp};
                قطار_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_clockingp->جوڑو_اگلا_ایتھے(قطار_Varp);
                // Create a process like this:
                //     always queue.push(<sampled var>);
                AstCMethodHard* ثابت pushp = جديد AstCMethodHard{
                    flp, جديد AstVarRef{flp, قطار_Varp, VAccess::WRITE}, VCMethod::DYN_ڈالو,
                    جديد Ast_ویلا{nodep->fileline(), m_modp->timeunit()}};
                pushp->جوڑو_Pinsp(exprp->clone_درخت_خالص(خطأ));
                pushp->dtype_سیٹ_Void();
                m_clockingp->جوڑو_اگلا_ایتھے(
                    جديد AstAlways{flp, VAlwaysKwd::ALWAYS, خالی_سوچک, pushp->makeStmt()});
                // Create a process like this:
                //     always @<clocking event> queue.pop(<skew>, /*out*/<skewed var>);
                AstCMethodHard* ثابت popp = جديد AstCMethodHard{
                    flp, جديد AstVarRef{flp, قطار_Varp, VAccess::READWRITE}, VCMethod::DYN_نکالو,
                    جديد Ast_ویلا{nodep->fileline(), m_modp->timeunit()}};
                popp->جوڑو_Pinsp(skewp->unlink_Fr_پٹھ());
                refp->access(VAccess::READWRITE);  // Only conditionally assigned
                popp->جوڑو_Pinsp(refp);
                popp->dtype_سیٹ_Void();
                m_clockingp->جوڑو_اگلا_ایتھے(جديد AstAlways{flp, VAlwaysKwd::ALWAYS,
                                                       m_clockingp->sentreep()->clone_درخت(خطأ),
                                                       popp->makeStmt()});
            }
        } وإلا {
            nodep->v3fatalSrc("Invalid direction");
        }
    }
    فراغ ویکھو(AstDelay* nodep) override {
        m_has_چکر_Delay |= nodep->is_چکر_Delay();
        // Only cycle delays are relevant in this stage; also only process once
        إذا (!nodep->is_چکر_Delay()) {
            إذا (m_وچ_Synch_Drive) {
                nodep->v3error("Only cycle delays can be used in synchronous drives"
                               " (IEEE 1800-2023 14.16)");
            }
            iterateChildren(nodep);
            أرجع;
        }
        إذا (m_وچ_سونپو && !m_وچ_Synch_Drive) {
            nodep->v3error("Cycle delays not allowed as intra-assignment delays"
                           " (IEEE 1800-2023 14.11)");
            VL_DO_DANGLING(nodep->unlink_Fr_پٹھ()->delete_درخت(), nodep);
            أرجع;
        }
        إذا (nodep->stmtsp()) nodep->جوڑو_اگلا_ایتھے(nodep->stmtsp()->unlink_Fr_پٹھ_With_اگلا());
        File_سطر* ثابت flp = nodep->fileline();
        Ast_گرہ_اظہاریہ* valuep = V3Const::constifyEdit(nodep->lhsp()->unlink_Fr_پٹھ());
        ثابت AstConst* ثابت constp = VN_CAST(valuep, Const);
        إذا (!constp && m_pexprp) {
            // V3AssertNfa handles non-const delays in sequences before this pass and
            // replaces the property; this branch should never be reached.
            nodep->v3fatalSrc("Non-constant cycle delay in assertion: "
                              "should have been caught by V3AssertNfa");
        } وإلا إذا (constp && constp->isZero()) {
            VL_DO_DANGLING(ڈالو_Deletep(valuep), valuep);
            إذا (m_وچ_Synch_Drive) {
                // ##0 has no effect in synchronous drives (IEEE 1800-2023 14.11)
                VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_پٹھ()), nodep);
                أرجع;
            }
            إذا (m_pexprp) {
                // ##0 in sequence context = zero delay = same clock tick
                VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_پٹھ()), nodep);
                أرجع;
            }
            // Procedural ##0: synchronize with default clocking event (IEEE 1800-2023 14.11)
            إذا (!m_defaultClockingp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_پٹھ()), nodep);
                أرجع;
            }
            nodep->replaceWith(new_چکر_Delay_Zerop(flp));
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            أرجع;
        }
        Ast_Sen_درخت* sentreep = خالی_سوچک;
        إذا (!m_drives.خالی_تھاواں()) {
            إذا (m_وچ_Concat_سونپو) {
                // Also the clockvars may be of different clocking blocks
                nodep->v3error("Cycle delays cannot be used in synchronous drives to a"
                               " concatenation (IEEE 1800-2023 14.16)");
                VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_پٹھ()), nodep);
                VL_DO_DANGLING(ڈالو_Deletep(valuep), valuep);
                أرجع;
            }
            // Count the cycles of the driven clockvar's clocking block (IEEE 1800-2023 14.16)
            sentreep = newDriveSentreep(nodep, m_drives.front());
            إذا (!sentreep) {
                VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_پٹھ()), nodep);
                VL_DO_DANGLING(ڈالو_Deletep(valuep), valuep);
                أرجع;
            }
        } وإلا إذا (!m_defaultClockingp) {
            إذا (!m_pexprp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_پٹھ()), nodep);
                VL_DO_DANGLING(ڈالو_Deletep(valuep), valuep);
                أرجع;
            }
            sentreep = جديد Ast_Sen_درخت{flp, m_senip->clone_درخت(خطأ)};
        } وإلا {
            sentreep = m_defaultClockingp->sentreep()->clone_درخت(خطأ);
        }
        AstEventControl* ثابت controlp
            = جديد AstEventControl{nodep->fileline(), sentreep, خالی_سوچک};
        ثابت std::string delayName = m_چکر_Dly_ناں.get(nodep);
        Ast_گرہ_اظہاریہ* throughoutp
            = nodep->throughoutp() ? nodep->throughoutp()->unlink_Fr_پٹھ() : خالی_سوچک;

        AstVar* ثابت cntVarp = جديد AstVar{flp, VVarType::BLOCKTEMP, delayName + "__counter",
                                           nodep->لبھو_Basic_DType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstBegin* ثابت beginp = جديد AstBegin{flp, delayName + "__block", cntVarp, سچ};
        // A non-constant delay is counted at runtime. The count is integral of 64 bits, as
        // V3Width makes delays, or real, which is rounded (IEEE 1800-2023 6.12.1)
        إذا (valuep->isDouble()) {
            valuep = جديد AstRToIRoundS{flp, valuep};
            valuep->dtypeFrom(cntVarp);
        } وإلا {
            valuep = جديد AstSel{flp, valuep, 0, cntVarp->چوڑائی()};
        }
        beginp->جوڑو_Stmtsp(جديد Ast_سونپو{flp, جديد AstVarRef{flp, cntVarp, VAccess::WRITE}, valuep});
        // If the count is zero, a synchronous drive waits for none, like with a constant '##0',
        // and a procedural delay waits like a procedural '##0' (IEEE 1800-2023 14.11)
        إذا (!constp && !m_وچ_Synch_Drive) {
            beginp->جوڑو_Stmtsp(جديد AstIf{
                flp,
                جديد AstEq{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ}, جديد AstConst{flp, 0}},
                new_چکر_Delay_Zerop(flp)});
        }

        // Throughout: create flag tracking whether condition held every tick
        AstVar* throughoutOkp = خالی_سوچک;
        إذا (throughoutp) {
            throughoutOkp = جديد AstVar{flp, VVarType::BLOCKTEMP, delayName + "__throughoutOk",
                                       nodep->لبھو_Basic_DType(VBasicDTypeKwd::LOGIC_IMPLICIT)};
            throughoutOkp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->جوڑو_Stmtsp(throughoutOkp);
            beginp->جوڑو_Stmtsp(جديد Ast_سونپو{flp, جديد AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            جديد AstConst{flp, AstConst::ثنائی_ہندسہ_True{}}});
            // Check condition at tick 0 (sequence start, before entering loop)
            beginp->جوڑو_Stmtsp(
                جديد AstIf{flp, جديد AstLogNot{flp, throughoutp->clone_درخت_خالص(خطأ)},
                          جديد Ast_سونپو{flp, جديد AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                        جديد AstConst{flp, AstConst::ثنائی_ہندسہ_False{}}}});
        }

        {
            AstLoop* ثابت loopp = جديد AstLoop{flp};
            // When throughout is present, exit loop early if condition fails
            Ast_گرہ_اظہاریہ* loopCondp
                = جديد AstGt{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ}, جديد AstConst{flp, 0}};
            إذا (throughoutOkp) {
                loopCondp = جديد AstLogAnd{flp, loopCondp,
                                          جديد AstVarRef{flp, throughoutOkp, VAccess::READ}};
            }
            loopp->جوڑو_Stmtsp(جديد AstLoopTest{flp, loopp, loopCondp});
            loopp->جوڑو_Stmtsp(controlp);
            loopp->جوڑو_Stmtsp(
                جديد Ast_سونپو{flp, جديد AstVarRef{flp, cntVarp, VAccess::WRITE},
                              جديد AstSub{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ},
                                         جديد AstConst{flp, 1}}});
            // Check throughout condition at each tick during delay (IEEE 1800-2023 16.9.9)
            إذا (throughoutp) {
                loopp->جوڑو_Stmtsp(
                    جديد AstIf{flp, جديد AstLogNot{flp, throughoutp},
                              جديد Ast_سونپو{flp, جديد AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            جديد AstConst{flp, AstConst::ثنائی_ہندسہ_False{}}}});
            }
            beginp->جوڑو_Stmtsp(loopp);
        }
        // Compose wrappers on remaining sequence: throughout gate (inner), disable iff (outer)
        Ast_گرہ* remainp = nodep->nextp() ? nodep->nextp()->unlink_Fr_پٹھ_With_اگلا() : خالی_سوچک;
        إذا (throughoutOkp) {
            // If condition failed during delay, fail assertion
            remainp = جديد AstIf{flp, جديد AstVarRef{flp, throughoutOkp, VAccess::READ}, remainp,
                                جديد AstPExprClause{flp, /*pass=*/خطأ}};
        }
        إذا (m_disableSeqIfp && remainp) {
            AstIf* ثابت disableSeqIfp = m_disableSeqIfp->clone_درخت(خطأ);
            disableSeqIfp->جوڑو_Thensp(remainp);
            remainp = disableSeqIfp;
        }
        إذا (remainp) {
            إذا (throughoutOkp) {
                // throughoutOkp is declared in beginp scope -- check must be inside it
                beginp->جوڑو_Stmtsp(remainp);
            } وإلا {
                nodep->جوڑو_اگلا_ایتھے(remainp);
            }
        }
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    فراغ ویکھو(Ast_Sen_درخت* nodep) override {
        إذا (m_وچ_Synch_Drive) {
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
    فراغ ویکھو(Ast_گرہ_Var_Ref* nodep) override {
        UINFO(8, " -varref:  " << nodep);
        UINFO(8, " -varref-var-back:  " << nodep->varp()->backp());
        UINFO(8, " -varref-var-user1:  " << nodep->varp()->user1p());
        إذا (Ast_Clocking_شے* ثابت itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->پہلا_Abovep(),
                Clocking_شے)) {
            إذا (nodep->user1()) أرجع;

            // ensure linking still works, this has to be done only once
            إذا (AstVarXRef* xrefp = VN_CAST(nodep, VarXRef)) {
                UINFO(8, " -clockvarxref-in:  " << xrefp);
                string dotted = xrefp->dotted();
                ثابت ناپ_t dotPos = dotted.rfind('.');
                dotted.erase(dotPos, string::npos);
                xrefp->dotted(dotted);
                UINFO(8, " -clockvarxref-out: " << xrefp);
                m_xrefsp.emplace_پٹھ(xrefp);
            }

            إذا (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            إذا (nodep->access().isWriteOrRW()) {
                إذا (itemp->direction() == VDirection::OUTPUT) {
                    إذا (!m_وچ_سونپو_Dly_Lhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    إذا (m_وچ_سونپو) {
                        m_وچ_Synch_Drive = سچ;
                        m_drives.ڈالو_پٹھ({itemp, nodep});
                    }
                } وإلا إذا (itemp->direction() == VDirection::داخلہ) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
            nodep->user1(سچ);
        }
    }
    فراغ ویکھو(AstMemberSel* nodep) override {
        إذا (Ast_Clocking_شے* ثابت itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->پہلا_Abovep(),
                Clocking_شے)) {
            إذا (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            إذا (nodep->access().isWriteOrRW()) {
                إذا (itemp->direction() == VDirection::OUTPUT) {
                    إذا (!m_وچ_سونپو_Dly_Lhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    إذا (m_وچ_سونپو) {
                        m_وچ_Synch_Drive = سچ;
                        m_drives.ڈالو_پٹھ({itemp, nodep});
                    }
                } وإلا إذا (itemp->direction() == VDirection::داخلہ) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
        }
    }
    فراغ ویکھو(Ast_گرہ_سونپو* nodep) override {
        إذا (nodep->user1()) أرجع;
        VL_RESTORER(m_وچ_سونپو);
        VL_RESTORER(m_وچ_Synch_Drive);
        VL_RESTORER(m_وچ_Concat_سونپو);
        VL_RESTORER_CLEAR(m_drives);
        m_وچ_سونپو = سچ;
        m_وچ_Synch_Drive = خطأ;
        {
            // The braces of a concatenation are a replication, by one
            ثابت Ast_گرہ_اظہاریہ* lhsp = nodep->lhsp();
            إذا (ثابت AstReplicate* ثابت repp = VN_CAST(lhsp, Replicate)) lhsp = repp->srcp();
            m_وچ_Concat_سونپو = VN_IS(lhsp, Concat);
        }
        {
            VL_RESTORER(m_وچ_سونپو_Dly_Lhs);
            m_وچ_سونپو_Dly_Lhs = VN_IS(nodep, سونپو_Dly);
            iterate(nodep->lhsp());
        }
        iterate(nodep->rhsp());
        إذا (nodep->timingControlp()) iterate(nodep->timingControlp());
        // A drive without a cycle delay, also after removing '##0', updates the clockvar now
        إذا (m_وچ_Synch_Drive && !nodep->timingControlp()) {
            Ast_سونپو* ثابت assignp = جديد Ast_سونپو{
                nodep->fileline(), nodep->lhsp()->unlink_Fr_پٹھ(), nodep->rhsp()->unlink_Fr_پٹھ()};
            assignp->user1(سچ);
            nodep->replaceWith(assignp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            nodep = assignp;
        }
        // Note the drives when they take effect, after the cycle delay if any. Each clockvar
        // of the LHS is driven, e.g. in a concatenation which other simulators accept, although
        // IEEE 1800-2023 14.16 does not allow it
        Ast_گرہ* setsp = خالی_سوچک;
        لكل (ثابت SynchDrive& drive : m_drives) {
            setsp = Ast_گرہ::جوڑو_اگلا(setsp, newDrivenSetp(nodep->fileline(), drive));
        }
        إذا (!setsp) أرجع;
        إذا (AstBegin* ثابت beginp = VN_CAST(nodep->timingControlp(), Begin)) {
            beginp->جوڑو_Stmtsp(setsp);
        } وإلا {
            nodep->جوڑو_اگلا_ایتھے(setsp);
        }
    }
    فراغ ویکھو(AstAlways* nodep) override {
        iterate_And_اگلا_Null(nodep->sentreep());
        إذا (nodep->sentreep()) m_seniAlwaysp = nodep->sentreep()->sensesp();
        iterate_And_اگلا_Null(nodep->stmtsp());
        m_seniAlwaysp = خالی_سوچک;
    }

    فراغ ویکھو(Ast_گرہ_Cover_Or_Assert* nodep) override {
        إذا (nodep->sentreep()) أرجع;  // Already processed

        VL_RESTORER(m_senip);
        VL_RESTORER(m_disablep);
        m_senip = خالی_سوچک;
        m_disablep = خالی_سوچک;

        // Find Clocking's buried under nodep->exprsp
        iterateChildren(nodep);
        إذا (!nodep->فوری()) nodep->sentreep(new_Sen_درخت(nodep, خالی_سوچک, nodep));
    }
    فراغ ویکھو(AstFalling* nodep) override {
        إذا (nodep->user1_سیٹ_Once()) أرجع;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گرہ_اظہاریہ* exprp = nodep->exprp()->unlink_Fr_پٹھ();
        إذا (exprp->چوڑائی() > 1) exprp = جديد AstSel{fl, exprp, 0, 1};
        Ast_گرہ_اظہاریہ* ثابت futurep = جديد AstFuture{fl, exprp, new_Sen_درخت(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = جديد AstAnd{fl, exprp->clone_درخت_خالص(خطأ), جديد AstNot{fl, futurep}};
        exprp->dtype_سیٹ_ثنائی_ہندسہ();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    فراغ ویکھو(AstFell* nodep) override {
        إذا (nodep->user1_سیٹ_Once()) أرجع;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گرہ_اظہاریہ* exprp = nodep->exprp()->unlink_Fr_پٹھ();
        إذا (exprp->چوڑائی() > 1) exprp = جديد AstSel{fl, exprp, 0, 1};
        Ast_Sen_درخت* sentreep = nodep->sentreep();
        إذا (sentreep) sentreep->unlink_Fr_پٹھ();
        AstPast* ثابت pastp = جديد AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_درخت(nodep, sentreep));
        exprp = جديد AstAnd{fl, pastp, جديد AstNot{fl, exprp->clone_درخت_خالص(خطأ)}};
        exprp->dtype_سیٹ_ثنائی_ہندسہ();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    فراغ ویکھو(AstFuture* nodep) override {
        إذا (nodep->user1_سیٹ_Once()) أرجع;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        Ast_Sen_درخت* ثابت sentreep = nodep->sentreep();
        إذا (sentreep) VL_DO_DANGLING(ڈالو_Deletep(sentreep->unlink_Fr_پٹھ()), sentreep);
        nodep->sentreep(new_Sen_درخت(nodep));
    }
    فراغ ویکھو(AstPast* nodep) override {
        checkSamplingFuncDType(nodep, nodep->exprp());
        إذا (nodep->sentreep()) أرجع;  // Already processed
        iterateChildren(nodep);
        nodep->sentreep(new_Sen_درخت(nodep));
    }
    فراغ ویکھو(AstSConsRep* nodep) override {
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
        File_سطر* ثابت flp = nodep->fileline();
        Ast_گرہ_اظہاریہ* ثابت exprp = nodep->exprp()->unlink_Fr_پٹھ();
        إذا (n == 1) {
            nodep->replaceWith(exprp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            أرجع;
        }
        // Saturating counter: if (expr) cnt <= min(cnt+1, N); else cnt <= 0;
        AstVar* ثابت cntVarp = جديد AstVar{flp, VVarType::MODULETEMP, m_cons_Rep_ناں.get(""),
                                           nodep->لبھو_Basic_DType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->جوڑو_Stmtsp(cntVarp);
        Ast_گرہ_اظہاریہ* ثابت اظہاریہ_Clonep = exprp->clone_درخت_خالص(خطأ);
        Ast_گرہ_اظہاریہ* ثابت saturatingIncrp = جديد AstCond{
            flp,
            جديد AstLt{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ},
                      جديد AstConst{flp, static_cast<uint32_t>(n)}},
            جديد Ast_جوڑو{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ}, جديد AstConst{flp, 1u}},
            جديد AstConst{flp, static_cast<uint32_t>(n)}};
        Ast_سونپو_Dly* ثابت incrAssignp
            = جديد Ast_سونپو_Dly{flp, جديد AstVarRef{flp, cntVarp, VAccess::WRITE}, saturatingIncrp};
        Ast_سونپو_Dly* ثابت resetAssignp = جديد Ast_سونپو_Dly{
            flp, جديد AstVarRef{flp, cntVarp, VAccess::WRITE}, جديد AstConst{flp, 0u}};
        AstIf* ثابت ifp = جديد AstIf{flp, اظہاریہ_Clonep, incrAssignp, resetAssignp};
        Ast_Sen_درخت* ثابت senTreep = new_Sen_درخت(nodep);
        AstAlways* ثابت alwaysp = جديد AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, ifp};
        cntVarp->جوڑو_اگلا_ایتھے(alwaysp);
        // Match: cnt >= N-1 (previous cycles via NBA) && expr (current cycle)
        Ast_گرہ_اظہاریہ* ثابت cntCheckp = جديد AstGte{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ},
                                                  جديد AstConst{flp, static_cast<uint32_t>(n - 1)}};
        cntCheckp->dtype_سیٹ_ثنائی_ہندسہ();
        Ast_گرہ_اظہاریہ* ثابت matchp = جديد AstAnd{flp, cntCheckp, exprp};
        matchp->dtype_سیٹ_ثنائی_ہندسہ();
        nodep->replaceWith(matchp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    فراغ ویکھو(AstRising* nodep) override {
        إذا (nodep->user1_سیٹ_Once()) أرجع;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گرہ_اظہاریہ* exprp = nodep->exprp()->unlink_Fr_پٹھ();
        إذا (exprp->چوڑائی() > 1) exprp = جديد AstSel{fl, exprp, 0, 1};
        Ast_گرہ_اظہاریہ* ثابت futurep = جديد AstFuture{fl, exprp, new_Sen_درخت(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = جديد AstAnd{fl, جديد AstNot{fl, exprp->clone_درخت_خالص(خطأ)}, futurep};
        exprp->dtype_سیٹ_ثنائی_ہندسہ();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    فراغ ویکھو(AstRose* nodep) override {
        إذا (nodep->user1_سیٹ_Once()) أرجع;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گرہ_اظہاریہ* exprp = nodep->exprp()->unlink_Fr_پٹھ();
        إذا (exprp->چوڑائی() > 1) exprp = جديد AstSel{fl, exprp, 0, 1};
        Ast_Sen_درخت* sentreep = nodep->sentreep();
        إذا (sentreep) sentreep->unlink_Fr_پٹھ();
        AstPast* ثابت pastp = جديد AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_درخت(nodep, sentreep));
        exprp = جديد AstAnd{fl, جديد AstNot{fl, pastp}, exprp->clone_درخت_خالص(خطأ)};
        exprp->dtype_سیٹ_ثنائی_ہندسہ();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    ساكن AstAssocArrayDType* getProcessAssocArrayType(File_سطر* ثابت flp) {
        // Type of __VassertsActive___x[std::process]
        Ast_گرہ_DType* valp
            = v3Global.rootp()->typeTablep()->لبھو_Basic_DType(flp, VBasicDTypeKwd::ثنائی_ہندسہ);
        AstClassRefDType* keyp
            = جديد AstClassRefDType{flp, v3Global.rootp()->stdPackageProcessp(), خالی_سوچک};
        keyp->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        v3Global.rootp()->typeTablep()->جوڑو_Typesp(keyp);
        AstAssocArrayDType* ثابت typep = جديد AstAssocArrayDType{flp, valp, keyp};
        typep->dtypep(typep);
        v3Global.rootp()->typeTablep()->جوڑو_Typesp(typep);
        أرجع typep;
    }
    ساكن Ast_Stmt_اظہاریہ* getProcessAssocArrayDelete(AstVarRef* ثابت refp) {
        // Constructs refp.delete(std::process::self()) statement
        File_سطر* ثابت flp = refp->fileline();
        AstCMethodHard* ثابت deletep = جديد AstCMethodHard{
            flp, refp, VCMethod::ASSOC_ERASE, v3Global.rootp()->stdPackageProcessSelfp(flp)};
        deletep->dtypep(refp->لبھو_Void_DType());
        أرجع جديد Ast_Stmt_اظہاریہ{flp, deletep};
    }
    ساكن Ast_گرہ_اظہاریہ* get_Process_Assoc_Array_ناپ(AstVarRef* ثابت refp) {
        // Constructs refp.size() statement
        AstCMethodHard* ثابت sizep
            = جديد AstCMethodHard{refp->fileline(), refp, VCMethod::ASSOC_ناپ};
        sizep->dtypep(refp->لبھو_Basic_DType(VBasicDTypeKwd::UINT32));
        أرجع sizep;
    }
    فراغ ویکھو(AstSEventually* nodep) override {
        UASSERT(v3Global.rootp()->stdPackagep(), "Should be imported");

        Ast_Sen_درخت* ثابت sentreep = new_Sen_درخت(nodep);
        إذا (!sentreep->sensesp()) {
            VL_DO_DANGLING(ڈالو_Deletep(sentreep), sentreep);
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False{}});
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            أرجع;
        }

        File_سطر* ثابت flp = nodep->fileline();

        // Track active assertions
        AstVar* ثابت activep = جديد AstVar{flp, VVarType::MODULETEMP, m_سرگرم_ناں.get(""),
                                           getProcessAssocArrayType(flp)};
        activep->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->جوڑو_Stmtsp(activep);

        // Assertion condition check
        AstLoop* ثابت loopp = جديد AstLoop{flp};
        AstSampled* ثابت condp
            = جديد AstSampled{flp, nodep->exprp()->unlink_Fr_پٹھ(), خالی_سوچک, سچ};
        loopp->جوڑو_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLogNot{flp, condp}});
        loopp->جوڑو_Stmtsp(جديد AstEventControl{flp, sentreep, خالی_سوچک});

        // Add assertion to the active set
        AstAssocSel* ثابت selp = جديد AstAssocSel{flp, جديد AstVarRef{flp, activep, VAccess::WRITE},
                                                  v3Global.rootp()->stdPackageProcessSelfp(flp)};
        Ast_سونپو* ثابت incrementp = جديد Ast_سونپو{flp, selp, جديد AstConst{flp, 1}};
        AstPExprClause* ثابت clausep = جديد AstPExprClause{flp};
        Ast_Stmt_اظہاریہ* ثابت deletep
            = getProcessAssocArrayDelete(جديد AstVarRef{flp, activep, VAccess::WRITE});

        // Main assertion block
        AstBegin* ثابت bodyp = جديد AstBegin{flp, m_بلاک_ناں.get(""), خالی_سوچک, سچ};
        bodyp->جوڑو_Stmtsp(incrementp);
        bodyp->جوڑو_Stmtsp(loopp);
        bodyp->جوڑو_Stmtsp(clausep);
        bodyp->جوڑو_Stmtsp(deletep);

        // Validate assertion condition for each active assert
        AstVar* ثابت سرگرم_Countp = جديد AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                nodep->لبھو_Basic_DType(VBasicDTypeKwd::UINT32)};
        سرگرم_Countp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        Ast_سونپو* ثابت init_سرگرم_Countp
            = جديد Ast_سونپو{flp, جديد AstVarRef{flp, سرگرم_Countp, VAccess::WRITE},
                            get_Process_Assoc_Array_ناپ(جديد AstVarRef{flp, activep, VAccess::READ})};
        AstLoop* ثابت finalLoopp = جديد AstLoop{flp};
        AstIf* ثابت finalBodypCondp
            = جديد AstIf{flp, condp->clone_درخت_خالص(خطأ), جديد AstPExprClause{flp},
                        جديد AstPExprClause{flp, خطأ}};
        finalLoopp->جوڑو_Stmtsp(
            جديد AstLoopTest{flp, finalLoopp,
                            جديد AstNeq{flp, جديد AstVarRef{flp, سرگرم_Countp, VAccess::READ},
                                       جديد AstConst{flp, 0}}});
        finalLoopp->جوڑو_Stmtsp(finalBodypCondp);
        finalLoopp->جوڑو_Stmtsp(
            جديد Ast_سونپو{flp, جديد AstVarRef{flp, سرگرم_Countp, VAccess::WRITE},
                          جديد AstSub{flp, جديد AstVarRef{flp, سرگرم_Countp, VAccess::READ},
                                     جديد AstConst{flp, 1}}});

        // Final assertion block
        AstBegin* ثابت finalp = جديد AstBegin{flp, m_بلاک_ناں.get(""), خالی_سوچک, سچ};
        finalp->جوڑو_Stmtsp(سرگرم_Countp);
        finalp->جوڑو_Stmtsp(init_سرگرم_Countp);
        finalp->جوڑو_Stmtsp(finalLoopp);

        VL_RESTORER(m_pexprp);
        m_pexprp = جديد AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
        VL_RESTORER(m_has_چکر_Delay);
        m_has_چکر_Delay = خطأ;
        iterate(bodyp);
        iterate(finalp);

        إذا (m_has_چکر_Delay) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in s_eventually");
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False{}});
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            VL_DO_DANGLING(m_pexprp->delete_درخت(), m_pexprp);
            أرجع;
        }

        nodep->replaceWith(m_pexprp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    فراغ ویکھو(AstStable* nodep) override {
        إذا (nodep->user1_سیٹ_Once()) أرجع;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گرہ_اظہاریہ* exprp = nodep->exprp()->unlink_Fr_پٹھ();
        Ast_Sen_درخت* sentreep = nodep->sentreep();
        إذا (sentreep) sentreep->unlink_Fr_پٹھ();
        AstPast* ثابت pastp = جديد AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_درخت(nodep, sentreep));
        exprp = جديد AstEq{fl, pastp, exprp->clone_درخت_خالص(خطأ)};
        exprp->dtype_سیٹ_ثنائی_ہندسہ();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    فراغ ویکھو(AstSteady* nodep) override {
        إذا (nodep->user1_سیٹ_Once()) أرجع;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گرہ_اظہاریہ* exprp = nodep->exprp()->unlink_Fr_پٹھ();
        إذا (exprp->چوڑائی() > 1) exprp = جديد AstSel{fl, exprp, 0, 1};
        Ast_گرہ_اظہاریہ* ثابت futurep = جديد AstFuture{fl, exprp, new_Sen_درخت(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = جديد AstEq{fl, exprp->clone_درخت_خالص(خطأ), futurep};
        exprp->dtype_سیٹ_ثنائی_ہندسہ();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    فراغ ویکھو(AstSampled* nodep) override {
        iterateChildren(nodep);
        إذا (!nodep->اندرونی_2()) checkSamplingFuncDType(nodep, nodep->exprp());
    }

    // Validate repetition count: must be a non-negative elaboration-time constant.
    // Shared by goto [->N] and nonconsecutive [=N] repetition.
    // On error, replaces nodep with BitFalse placeholder and returns nullptr.
    ثابت AstConst* validate_Rep_گنتی(Ast_گرہ* nodep, Ast_گرہ_اظہاریہ*& countp) {
        countp = V3Const::constifyEdit(countp);
        ثابت AstConst* ثابت constp = VN_CAST(countp, Const);
        إذا (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ڈالو_Deletep(countp), countp);
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False_Erroring{}});
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            أرجع خالی_سوچک;
        }
        إذا (constp->منزل_SInt() < 0) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ڈالو_Deletep(countp), countp);
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False_Erroring{}});
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            أرجع خالی_سوچک;
        }
        إذا (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ڈالو_Deletep(countp), countp);
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False{}});
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            أرجع خالی_سوچک;
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
    AstPExpr* createRepPExpr(File_سطر* flp, Ast_گرہ_اظہاریہ* exprp, Ast_گرہ_اظہاریہ* countp,
                             Ast_گرہ_اظہاریہ* rhsp, منطقي isOverlapped, منطقي isNonConsec) {
        Ast_Sen_شے* ثابت sensesp = m_senip;
        UASSERT_OBJ(sensesp, exprp, "Repetition requires a clock");

        ثابت std::string name
            = isNonConsec ? m_non_Cons_Rep_ناں.get(exprp) : m_goto_Rep_ناں.get(exprp);
        AstVar* ثابت cntVarp = جديد AstVar{flp, VVarType::BLOCKTEMP, name + "__counter",
                                           exprp->لبھو_Basic_DType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstBegin* ثابت beginp = جديد AstBegin{flp, name + "__block", cntVarp, سچ};
        beginp->جوڑو_Stmtsp(
            جديد Ast_سونپو{flp, جديد AstVarRef{flp, cntVarp, VAccess::WRITE}, جديد AstConst{flp, 0}});

        // Counting loop: find N occurrences of expr (shared by goto and nonconsec)
        AstLoop* ثابت loopp = جديد AstLoop{flp};
        loopp->جوڑو_Stmtsp(جديد AstLoopTest{flp, loopp,
                                         جديد AstLt{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ},
                                                   countp->clone_درخت_خالص(خطأ)}});
        // if (expr) cnt++ -- sampled is applied to whole property expr by V3Assert
        loopp->جوڑو_Stmtsp(
            جديد AstIf{flp, exprp,
                      جديد Ast_سونپو{flp, جديد AstVarRef{flp, cntVarp, VAccess::WRITE},
                                    جديد Ast_جوڑو{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ},
                                               جديد AstConst{flp, 1}}}});
        loopp->جوڑو_Stmtsp(جديد AstIf{
            flp, جديد AstLt{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ}, countp},
            جديد AstEventControl{flp, جديد Ast_Sen_درخت{flp, sensesp->clone_درخت(خطأ)}, خالی_سوچک}});

        beginp->جوڑو_Stmtsp(loopp);

        إذا (isNonConsec && rhsp) {
            // IEEE 16.9.2: b[=N] = b[->N] ##1 !b[*0:$]
            // Trailing window: ##1 advance, then scan !expr cycles checking rhs.
            // Window closes when expr becomes true again (fail if rhs was never true).
            beginp->جوڑو_Stmtsp(جديد AstEventControl{
                flp, جديد Ast_Sen_درخت{flp, sensesp->clone_درخت(خطأ)}, خالی_سوچک});  // ##1
            إذا (!isOverlapped) {
                beginp->جوڑو_Stmtsp(جديد AstEventControl{
                    flp, جديد Ast_Sen_درخت{flp, sensesp->clone_درخت(خطأ)}, خالی_سوچک});  // |=>
            }
            // Window loop: check rhs at each !expr cycle (done variable for termination)
            AstVar* ثابت پورا_Varp
                = جديد AstVar{flp, VVarType::BLOCKTEMP, name + "__done", exprp->لبھو_ثنائی_ہندسہ_DType()};
            پورا_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->جوڑو_Stmtsp(پورا_Varp);
            beginp->جوڑو_Stmtsp(جديد Ast_سونپو{flp, جديد AstVarRef{flp, پورا_Varp, VAccess::WRITE},
                                            جديد AstConst{flp, AstConst::ثنائی_ہندسہ_False{}}});
            خودکار سیٹ_پورا = [&]() {
                أرجع جديد Ast_سونپو{flp, جديد AstVarRef{flp, پورا_Varp, VAccess::WRITE},
                                     جديد AstConst{flp, AstConst::ثنائی_ہندسہ_True{}}};
            };
            AstLoop* ثابت windowp = جديد AstLoop{flp};
            // LoopTest: continue while !done
            windowp->جوڑو_Stmtsp(جديد AstLoopTest{
                flp, windowp, جديد AstNot{flp, جديد AstVarRef{flp, پورا_Varp, VAccess::READ}}});
            // if (expr) { fail; done = 1; } -- window closed, expr true again
            Ast_گرہ* ثابت failsp = جديد AstPExprClause{flp, خطأ};
            failsp->جوڑو_اگلا(سیٹ_پورا());
            windowp->جوڑو_Stmtsp(جديد AstIf{flp, exprp->clone_درخت_خالص(خطأ), failsp});
            // if (rhs) { pass; done = 1; } -- consequent true at this !expr endpoint
            Ast_گرہ* ثابت passsp = جديد AstPExprClause{flp, سچ};
            passsp->جوڑو_اگلا(سیٹ_پورا());
            windowp->جوڑو_Stmtsp(جديد AstIf{flp, rhsp, passsp});
            // @(clk) -- advance to next cycle in window
            windowp->جوڑو_Stmtsp(
                جديد AstEventControl{flp, جديد Ast_Sen_درخت{flp, sensesp->clone_درخت(خطأ)}, خالی_سوچک});
            beginp->جوڑو_Stmtsp(windowp);
        } وإلا إذا (isNonConsec) {
            // Standalone nonconsec: ##1 into window, then pass
            beginp->جوڑو_Stmtsp(
                جديد AstEventControl{flp, جديد Ast_Sen_درخت{flp, sensesp->clone_درخت(خطأ)}, خالی_سوچک});
            beginp->جوڑو_Stmtsp(جديد AstPExprClause{flp, سچ});
        } وإلا إذا (rhsp) {
            // Goto rep: check consequent once at match endpoint
            إذا (!isOverlapped) {
                beginp->جوڑو_Stmtsp(جديد AstEventControl{
                    flp, جديد Ast_Sen_درخت{flp, sensesp->clone_درخت(خطأ)}, خالی_سوچک});
            }
            beginp->جوڑو_Stmtsp(جديد AstIf{flp, rhsp, جديد AstPExprClause{flp, سچ},
                                        جديد AstPExprClause{flp, خطأ}});
        } وإلا {
            // Standalone goto: pass after counting
            beginp->جوڑو_Stmtsp(جديد AstPExprClause{flp, سچ});
        }

        أرجع جديد AstPExpr{flp, beginp, exprp->لبھو_ثنائی_ہندسہ_DType()};
    }

    // Reject [=M:N] nonconsecutive range that reached V3AssertPre.
    // [->M:N] is fully handled upstream (NFA path); only [=M:N] still lands
    // here because AstSNonConsRep lowering is not yet implemented there.
    // Replaces the node with BitFalse so lowering continues after the warning.
    منطقي rejectNonconsecRange(Ast_گرہ* nodep, Ast_گرہ_اظہاریہ* maxCountp) {
        إذا (!maxCountp) أرجع خطأ;
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: [=M:N] nonconsecutive range repetition"
                                     " (IEEE 1800-2023 16.9.2)");
        nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False{}});
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        أرجع سچ;
    }

    فراغ ویکھو(AstSGotoRep* nodep) override {
        // Standalone goto rep (not inside implication antecedent)
        iterateChildren(nodep);
        File_سطر* ثابت flp = nodep->fileline();
        Ast_گرہ_اظہاریہ* countp = nodep->countp()->unlink_Fr_پٹھ();
        إذا (!validate_Rep_گنتی(nodep, countp)) أرجع;
        Ast_گرہ_اظہاریہ* ثابت exprp = nodep->exprp()->unlink_Fr_پٹھ();
        AstPExpr* ثابت pexprp = createRepPExpr(flp, exprp, countp, خالی_سوچک, سچ, خطأ);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }

    فراغ ویکھو(AstSNonConsRep* nodep) override {
        // Standalone nonconsecutive rep (not inside implication antecedent)
        iterateChildren(nodep);
        إذا (rejectNonconsecRange(nodep, nodep->maxCountp())) أرجع;
        File_سطر* ثابت flp = nodep->fileline();
        Ast_گرہ_اظہاریہ* countp = nodep->countp()->unlink_Fr_پٹھ();
        إذا (!validate_Rep_گنتی(nodep, countp)) أرجع;
        Ast_گرہ_اظہاریہ* ثابت exprp = nodep->exprp()->unlink_Fr_پٹھ();
        AstPExpr* ثابت pexprp = createRepPExpr(flp, exprp, countp, خالی_سوچک, سچ, سچ);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }

    فراغ ویکھو(AstFuncRef* nodep) override {
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
    فراغ ویکھو(AstImplication* nodep) override {
        إذا (nodep->sentreep()) أرجع;  // Already processed

        // Top-level followed-by is claimed by V3AssertNfa; anything reaching here
        // is nested inside iff/implies/or. IEEE 1800-2023 16.12.9 permits the
        // composition, but silent lowering would drop non-vacuous-fail semantics.
        إذا (nodep->isFollowedBy()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: followed-by (#-# / #=#) nested inside property operator"
                          " (iff/implies/or) (IEEE 1800-2023 16.12.9)");
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False{}});
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            أرجع;
        }

        // Handle goto repetition as antecedent before iterateChildren,
        // so the standalone AstSGotoRep visitor doesn't process it
        إذا (AstSGotoRep* ثابت gotop = VN_CAST(nodep->lhsp(), SGotoRep)) {
            iterateChildren(gotop);
            iterate_And_اگلا_Null(nodep->rhsp());
            File_سطر* ثابت flp = nodep->fileline();
            Ast_گرہ_اظہاریہ* countp = gotop->countp()->unlink_Fr_پٹھ();
            إذا (!validate_Rep_گنتی(nodep, countp)) أرجع;
            Ast_گرہ_اظہاریہ* ثابت exprp = gotop->exprp()->unlink_Fr_پٹھ();
            Ast_گرہ_اظہاریہ* ثابت rhsp = nodep->rhsp()->unlink_Fr_پٹھ();
            AstPExpr* ثابت pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), خطأ);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            أرجع;
        }

        // Handle nonconsecutive repetition as antecedent before iterateChildren,
        // so the standalone AstSNonConsRep visitor doesn't process it
        إذا (AstSNonConsRep* ثابت ncrp = VN_CAST(nodep->lhsp(), SNonConsRep)) {
            iterateChildren(ncrp);
            iterate_And_اگلا_Null(nodep->rhsp());
            إذا (rejectNonconsecRange(nodep, ncrp->maxCountp())) أرجع;
            File_سطر* ثابت flp = nodep->fileline();
            Ast_گرہ_اظہاریہ* countp = ncrp->countp()->unlink_Fr_پٹھ();
            إذا (!validate_Rep_گنتی(nodep, countp)) أرجع;
            Ast_گرہ_اظہاریہ* ثابت exprp = ncrp->exprp()->unlink_Fr_پٹھ();
            Ast_گرہ_اظہاریہ* ثابت rhsp = nodep->rhsp()->unlink_Fr_پٹھ();
            AstPExpr* ثابت pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), سچ);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            أرجع;
        }

        iterateChildren(nodep);

        File_سطر* ثابت flp = nodep->fileline();
        Ast_گرہ_اظہاریہ* ثابت rhsp = nodep->rhsp()->unlink_Fr_پٹھ();
        Ast_گرہ_اظہاریہ* lhsp = nodep->lhsp()->unlink_Fr_پٹھ();

        // Lower sequence match items (IEEE 16.11): (expr, var = val, ...) |-> / |=>
        إذا (Ast_اظہاریہ_Stmt* ثابت اظہاریہ_Stmtp = VN_CAST(lhsp, اظہاریہ_Stmt)) {
            Ast_گرہ_اظہاریہ* ثابت antExprp = اظہاریہ_Stmtp->resultp()->unlink_Fr_پٹھ();

            إذا (nodep->isOverlapped()) {
                // |-> : assign to __Vpropvar via always_comb (continuous).
                // The assign evaluates RHS once; V3Sampled snapshots the
                // result so all consequent refs read the same value.
                Ast_گرہ* میل_Assignsp = خالی_سوچک;
                لكل (Ast_گرہ* stmtp = اظہاریہ_Stmtp->stmtsp(); stmtp;) {
                    Ast_گرہ* ثابت nextp = stmtp->nextp();
                    إذا (Ast_سونپو* ثابت assignp = VN_CAST(stmtp, سونپو)) {
                        assignp->unlink_Fr_پٹھ();
                        میل_Assignsp = Ast_گرہ::جوڑو_اگلا_Null(میل_Assignsp, assignp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(ڈالو_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                إذا (میل_Assignsp) {
                    AstAlways* ثابت alwaysp
                        = جديد AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, خالی_سوچک, میل_Assignsp};
                    m_modp->جوڑو_Stmtsp(alwaysp);
                }
            } وإلا {
                // |=> : assign to __Vpropvar via NBA in a clocked always block.
                // The NBA commits before the next cycle's sampled snapshot,
                // so the consequent (which already references __Vpropvar)
                // sees the captured value.
                Ast_گرہ* میل_Assignsp = خالی_سوچک;
                لكل (Ast_گرہ* stmtp = اظہاریہ_Stmtp->stmtsp(); stmtp;) {
                    Ast_گرہ* ثابت nextp = stmtp->nextp();
                    إذا (Ast_سونپو* ثابت assignp = VN_CAST(stmtp, سونپو)) {
                        assignp->unlink_Fr_پٹھ();
                        Ast_گرہ_اظہاریہ* ثابت سونپو_Lhsp = assignp->lhsp()->unlink_Fr_پٹھ();
                        Ast_گرہ_اظہاریہ* ثابت سونپو_Rhsp = assignp->rhsp()->unlink_Fr_پٹھ();
                        Ast_سونپو_Dly* ثابت dlyp = جديد Ast_سونپو_Dly{flp, سونپو_Lhsp, سونپو_Rhsp};
                        VL_DO_DANGLING(ڈالو_Deletep(assignp), assignp);
                        میل_Assignsp = Ast_گرہ::جوڑو_اگلا_Null(میل_Assignsp, dlyp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(ڈالو_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                إذا (میل_Assignsp) {
                    AstIf* ثابت condp
                        = جديد AstIf{flp, antExprp->clone_درخت_خالص(خطأ), میل_Assignsp};
                    AstAlways* ثابت alwaysp
                        = جديد AstAlways{flp, VAlwaysKwd::ALWAYS, new_Sen_درخت(nodep), condp};
                    m_modp->جوڑو_Stmtsp(alwaysp);
                }
            }
        }

        إذا (AstPExpr* ثابت pexprp = VN_CAST(rhsp, PExpr)) {
            // Implication with sequence expression on RHS (IEEE 1800-2023 16.11, 16.12.7).
            // The PExpr was lowered from the property expression earlier in this pass.
            // Wrap the PExpr body with the antecedent check so the sequence only
            // starts when the antecedent holds.
            Ast_گرہ_اظہاریہ* condp;
            إذا (nodep->isOverlapped()) {
                // Overlapped implication (|->): check antecedent on same cycle.
                // disable iff is applied at the assertion level, not at the
                // antecedent gate, matching the existing non-PExpr overlapped path.
                condp = lhsp;
            } وإلا {
                // Non-overlapped implication (|=>): check antecedent from previous cycle
                إذا (m_disablep) {
                    lhsp
                        = جديد AstAnd{flp, جديد AstNot{flp, m_disablep->clone_درخت_خالص(خطأ)}, lhsp};
                }
                AstPast* ثابت pastp
                    = جديد AstPast{flp, lhsp, خالی_سوچک, خالی_سوچک, /* propertyTiming */ سچ};
                pastp->dtypeFrom(lhsp);
                pastp->sentreep(new_Sen_درخت(nodep));
                condp = pastp;
            }
            // Wrap existing PExpr body: if (antecedent) { <original body> } else { /* vacuous pass
            // */ }
            // Only the statements are guarded; declarations stay directly in the block. After
            // V3Fork moves the block's statements into a task, V3Task only handles variables
            // that are direct statements of the task.
            AstBegin* ثابت bodyp = pexprp->bodyp();
            AstIf* ثابت guardp = جديد AstIf{flp, condp};
            لكل (Ast_گرہ* stmtp = bodyp->stmtsp(); stmtp;) {
                Ast_گرہ* ثابت nextp = stmtp->nextp();
                إذا (!VN_IS(stmtp, Var)) guardp->جوڑو_Thensp(stmtp->unlink_Fr_پٹھ());
                stmtp = nextp;
            }
            bodyp->جوڑو_Stmtsp(guardp);
            nodep->replaceWith(pexprp);
            // Don't iterate pexprp here -- it was already iterated when created
            // (in visit(AstSExpr*)), so delays and disable iff are already processed.
        } وإلا إذا (nodep->isOverlapped()) {
            Ast_گرہ_اظہاریہ* ثابت exprp
                = m_underCover ? static_cast<Ast_گرہ_اظہاریہ*>(جديد AstLogAnd{flp, lhsp, rhsp})
                               : جديد AstLogOr{flp, جديد AstLogNot{flp, lhsp}, rhsp};
            nodep->replaceWith(exprp);
        } وإلا {
            إذا (m_disablep) {
                lhsp = جديد AstAnd{flp, جديد AstNot{flp, m_disablep->clone_درخت_خالص(خطأ)}, lhsp};
            }

            AstPast* ثابت pastp
                = جديد AstPast{flp, lhsp, خالی_سوچک, خالی_سوچک, /* propertyTiming */ سچ};
            pastp->dtypeFrom(lhsp);
            pastp->sentreep(new_Sen_درخت(nodep));
            Ast_گرہ_اظہاریہ* ثابت exprp
                = m_underCover ? static_cast<Ast_گرہ_اظہاریہ*>(جديد AstAnd{flp, pastp, rhsp})
                               : جديد AstOr{flp, جديد AstNot{flp, pastp}, rhsp};
            exprp->dtype_سیٹ_ثنائی_ہندسہ();
            nodep->replaceWith(exprp);
        }
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    فراغ ویکھو(AstUntil* nodep) override {
        File_سطر* ثابت flp = nodep->fileline();
        UASSERT_OBJ(
            !m_pexprp, nodep,
            "'" << nodep->verilogKwd()
                << "' in complex property expression should have been rejected by V3AssertNfa");
        إذا (nodep->isStrong()
            && (v3Global.opt.timing().is_سیٹ_False() || !v3Global.opt.timing().is_سیٹ_True())) {
            nodep->v3warn(E_NOTIMING, nodep->verilogKwd() << " requires --timing");
            nodep->replaceWith(جديد AstConst{flp, AstConst::ثنائی_ہندسہ_False{}});
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            أرجع;
        }
        إذا (nodep->isStrong()) {
            // p s_until q / p s_until_with q: q must eventually be true. Until then, p must
            // be true on every sampled tick. For s_until, check q first: when q is true on
            // this tick, p is not required. For s_until_with, p must be true on the q tick too.
            Ast_گرہ_اظہاریہ* ثابت rawLhsp = nodep->lhsp()->unlink_Fr_پٹھ();
            Ast_گرہ_اظہاریہ* ثابت rawRhsp = nodep->rhsp()->unlink_Fr_پٹھ();
            AstSampled* ثابت lhsp = جديد AstSampled{flp, rawLhsp, rawLhsp->dtypep(), سچ};
            AstSampled* ثابت rhsp = جديد AstSampled{flp, rawRhsp, rawRhsp->dtypep(), سچ};
            Ast_گرہ_اظہاریہ* finalCondp = rhsp->clone_درخت_خالص(خطأ);
            إذا (nodep->isOverlapping()) {
                finalCondp = جديد AstLogAnd{flp, lhsp->clone_درخت_خالص(خطأ), finalCondp};
            }

            // Track active assertion attempts. Only the count is needed to emit final failures.
            AstVar* ثابت activep = جديد AstVar{flp, VVarType::MODULETEMP, m_سرگرم_ناں.get(""),
                                               nodep->لبھو_Basic_DType(VBasicDTypeKwd::UINT32)};
            activep->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->جوڑو_Stmtsp(activep);

            AstVar* ثابت donep
                = جديد AstVar{flp, VVarType::BLOCKTEMP, "__VassertDone", nodep->لبھو_ثنائی_ہندسہ_DType()};
            donep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            خودکار سیٹ_پورا = [&]() {
                أرجع جديد Ast_سونپو{flp, جديد AstVarRef{flp, donep, VAccess::WRITE},
                                     جديد AstConst{flp, AstConst::ثنائی_ہندسہ_True{}}};
            };

            AstLoop* ثابت loopp = جديد AstLoop{flp};
            Ast_سونپو* ثابت decrementVar = جديد Ast_سونپو{
                flp, جديد AstVarRef{flp, activep, VAccess::WRITE},
                جديد AstSub{flp, جديد AstVarRef{flp, activep, VAccess::READ}, جديد AstConst{flp, 1}}};
            loopp->جوڑو_Stmtsp(جديد AstLoopTest{
                flp, loopp, جديد AstLogNot{flp, جديد AstVarRef{flp, donep, VAccess::READ}}});
            {
                Ast_گرہ* ثابت passp = جديد AstPExprClause{flp};
                passp->جوڑو_اگلا(decrementVar);
                passp->جوڑو_اگلا(سیٹ_پورا());
                Ast_گرہ_اظہاریہ* passCondp = rhsp;
                إذا (nodep->isOverlapping()) {
                    passCondp = جديد AstLogAnd{flp, lhsp->clone_درخت_خالص(خطأ), passCondp};
                }
                loopp->جوڑو_Stmtsp(جديد AstIf{flp, passCondp, passp});
            }
            {
                Ast_گرہ* ثابت failp = جديد AstPExprClause{flp, خطأ};
                failp->جوڑو_اگلا(decrementVar->clone_درخت(خطأ));
                failp->جوڑو_اگلا(سیٹ_پورا());
                loopp->جوڑو_Stmtsp(جديد AstIf{
                    flp,
                    جديد AstLogAnd{flp,
                                  جديد AstLogNot{flp, جديد AstVarRef{flp, donep, VAccess::READ}},
                                  جديد AstLogNot{flp, lhsp}},
                    failp});
            }
            AstDelay* ثابت delayp = جديد AstDelay{flp, جديد AstConst{flp, 1}, خطأ};
            delayp->timeunit(m_modp->timeunit());
            loopp->جوڑو_Stmtsp(جديد AstIf{
                flp, جديد AstLogNot{flp, جديد AstVarRef{flp, donep, VAccess::READ}}, delayp});

            // Main assertion block
            AstBegin* ثابت bodyp = جديد AstBegin{flp, m_بلاک_ناں.get(""), خالی_سوچک, سچ};
            bodyp->جوڑو_Stmtsp(donep);
            bodyp->جوڑو_Stmtsp(جديد Ast_سونپو{flp, جديد AstVarRef{flp, donep, VAccess::WRITE},
                                           جديد AstConst{flp, AstConst::ثنائی_ہندسہ_False{}}});
            File_سطر* ثابت flp = activep->fileline();
            bodyp->جوڑو_Stmtsp(
                جديد Ast_سونپو{flp, جديد AstVarRef{flp, activep, VAccess::WRITE},
                              جديد Ast_جوڑو{flp, جديد AstVarRef{flp, activep, VAccess::READ},
                                         جديد AstConst{flp, 1}}});
            bodyp->جوڑو_Stmtsp(loopp);

            // Validate assertion condition for each active assert
            AstVar* ثابت سرگرم_Countp = جديد AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                    nodep->لبھو_Basic_DType(VBasicDTypeKwd::UINT32)};
            سرگرم_Countp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

            Ast_سونپو* ثابت init_سرگرم_Countp
                = جديد Ast_سونپو{flp, جديد AstVarRef{flp, سرگرم_Countp, VAccess::WRITE},
                                جديد AstVarRef{flp, activep, VAccess::READ}};
            AstLoop* ثابت finalLoopp = جديد AstLoop{flp};
            finalLoopp->جوڑو_Stmtsp(
                جديد AstLoopTest{flp, finalLoopp,
                                جديد AstNeq{flp, جديد AstVarRef{flp, سرگرم_Countp, VAccess::READ},
                                           جديد AstConst{flp, 0}}});
            finalLoopp->جوڑو_Stmtsp(جديد AstIf{flp, finalCondp, جديد AstPExprClause{flp},
                                            جديد AstPExprClause{flp, خطأ}});
            finalLoopp->جوڑو_Stmtsp(
                جديد Ast_سونپو{flp, جديد AstVarRef{flp, سرگرم_Countp, VAccess::WRITE},
                              جديد AstSub{flp, جديد AstVarRef{flp, سرگرم_Countp, VAccess::READ},
                                         جديد AstConst{flp, 1}}});

            // Final assertion block
            AstBegin* ثابت finalp = جديد AstBegin{flp, m_بلاک_ناں.get(""), خالی_سوچک, سچ};
            finalp->جوڑو_Stmtsp(سرگرم_Countp);
            finalp->جوڑو_Stmtsp(init_سرگرم_Countp);
            finalp->جوڑو_Stmtsp(finalLoopp);

            AstPExpr* ثابت pexprp = جديد AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
            VL_RESTORER(m_pexprp);
            VL_RESTORER(m_has_چکر_Delay);
            m_pexprp = pexprp;
            m_has_چکر_Delay = خطأ;
            iterate(bodyp);
            iterate(finalp);

            UASSERT_OBJ(!m_has_چکر_Delay, nodep,
                        "s_until cycle delay should have been handled by V3AssertNfa");

            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            أرجع;
        }
        AstLoop* ثابت loopp = جديد AstLoop{flp};
        Ast_گرہ_اظہاریہ* ثابت rhsp = nodep->rhsp()->unlink_Fr_پٹھ();
        Ast_گرہ_اظہاریہ* ثابت lhsp = nodep->lhsp()->unlink_Fr_پٹھ();
        AstLogAnd* ثابت loopCondp = جديد AstLogAnd{flp, lhsp, جديد AstLogNot{flp, rhsp}};
        loopp->جوڑو_Stmtsp(جديد AstLoopTest{flp, loopp, loopCondp});
        loopp->جوڑو_Stmtsp(جديد AstEventControl{flp, new_Sen_درخت(nodep), خالی_سوچک});

        Ast_گرہ_اظہاریہ* ثابت rhsCopyp = rhsp->clone_درخت_خالص(خطأ);
        Ast_گرہ_اظہاریہ* ثابت passCondp
            = nodep->isOverlapping() ? جديد AstLogAnd{flp, lhsp->clone_درخت_خالص(خطأ), rhsCopyp}
                                     : rhsCopyp;
        AstBegin* ثابت beginp = جديد AstBegin{flp, m_بلاک_ناں.get(""), loopp, سچ};
        beginp->جوڑو_Stmtsp(
            جديد AstIf{flp, passCondp, جديد AstPExprClause{flp}, جديد AstPExprClause{flp, خطأ}});

        AstPExpr* ثابت pexprp = جديد AstPExpr{flp, beginp, nodep->dtypep()};
        pexprp->user1(1);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }

    فراغ ویکھو(AstDefaultDisable* nodep) override {
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_پٹھ()), nodep);
    }
    فراغ ویکھو(AstInferredDisable* nodep) override {
        Ast_گرہ* newp;
        إذا (m_defaultDisablep) {
            newp = m_defaultDisablep->condp()->clone_درخت_خالص(سچ);
        } وإلا {
            newp = جديد AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False{}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }

    فراغ ویکھو(AstPropSpec* nodep) override {
        nodep = substitutePropertyCall(nodep);
        iterate_And_اگلا_Null(nodep->sensesp());
        إذا (m_senip && m_senip != nodep->sensesp())
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Only one PSL clock allowed per assertion");
        ثابت AstCover* ثابت coverp = VN_CAST(nodep->backp(), Cover);
        ثابت منطقي seqEvent = coverp && coverp->isSeqEvent();
        إذا (!nodep->disablep() && m_defaultDisablep && !seqEvent) {
            nodep->disablep(m_defaultDisablep->condp()->clone_درخت_خالص(سچ));
        }
        m_disablep = nodep->disablep();
        // Unlink and just keep a pointer to it, convert to sentree as needed
        m_senip = nodep->sensesp();
        iterateNull(nodep->disablep());
        إذا (!VN_AS(nodep->backp(), گرہ_Cover_Or_Assert)->فوری()) {
            ثابت Ast_گرہ_DType* ثابت propDtp = nodep->propp()->dtypep();
            nodep->propp(جديد AstSampled{nodep->fileline(), nodep->propp()->unlink_Fr_پٹھ(),
                                        propDtp->dtypep(), سچ});
        }
        // cover counts non-vacuous matches only (IEEE 1800-2023 16.15.2), so an
        // implication antecedent must hold; assert passes vacuously instead.
        VL_RESTORER(m_underCover);
        m_underCover = VN_IS(nodep->backp(), Cover);
        iterate(nodep->propp());
        iterateNull(nodep->میل_Countp());
    }
    فراغ ویکھو(AstPExpr* nodep) override {
        // V3AssertNfa handles multi-cycle property expressions before this pass,
        // so the following unsupported paths are defensive and typically unreached.
        إذا (m_pexprp && m_pexprp->user1()) {  // LCOV_EXCL_START
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Complex property expression inside 'until''");
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False{}});
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            أرجع;
        }  // LCOV_EXCL_STOP
        إذا (AstLogNot* ثابت notp = VN_CAST(nodep->backp(), LogNot)) {
            notp->replaceWith(nodep->unlink_Fr_پٹھ());
            VL_DO_DANGLING(ڈالو_Deletep(notp), notp);
            iterate(nodep);
            أرجع;
        }
        // Sequence expression as antecedent of implication is not yet supported
        إذا (AstImplication* ثابت implp = VN_CAST(nodep->backp(), Implication)) {
            إذا (implp->lhsp() == nodep) {  // LCOV_EXCL_START
                implp->v3warn(E_UNSUPPORTED,
                              "Unsupported: Implication with sequence expression as antecedent");
                nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False{}});
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                أرجع;
            }  // LCOV_EXCL_STOP
        }
        VL_RESTORER(m_pexprp);
        VL_RESTORER(m_disableSeqIfp);
        m_pexprp = nodep;

        إذا (m_disablep) {
            ثابت AstSampled* sampledp = خالی_سوچک;
            إذا (m_disablep->exists([&sampledp](ثابت AstSampled* ثابت sp) {
                    sampledp = sp;
                    أرجع سچ;
                })) {
                sampledp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: $sampled inside disabled condition of a sequence");
                m_disablep = جديد AstConst{m_disablep->fileline(), AstConst::ثنائی_ہندسہ_False{}};
                // always a copy is used, so remove it now
                ڈالو_Deletep(m_disablep);  // m_disablep used later
            }
            File_سطر* ثابت flp = nodep->fileline();
            // Add counter which counts times the condition turned true
            AstVar* ثابت disableCntp
                = جديد AstVar{flp, VVarType::MODULETEMP, m_disable_Cnt_ناں.get(""),
                             nodep->لبھو_Basic_DType(VBasicDTypeKwd::UINT32)};
            disableCntp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->جوڑو_Stmtsp(disableCntp);
            AstVarRef* ثابت readCntRefp = جديد AstVarRef{flp, disableCntp, VAccess::READ};
            AstVarRef* ثابت writeCntRefp = جديد AstVarRef{flp, disableCntp, VAccess::WRITE};
            Ast_سونپو* ثابت incrStmtp = جديد Ast_سونپو{
                flp, writeCntRefp, جديد Ast_جوڑو{flp, readCntRefp, جديد AstConst{flp, 1}}};
            AstAlways* ثابت alwaysp
                = جديد AstAlways{flp, VAlwaysKwd::ALWAYS,
                                جديد Ast_Sen_درخت{flp, جديد Ast_Sen_شے{flp, VEdgeType::ET_POSEDGE,
                                                                   m_disablep->clone_درخت(خطأ)}},
                                incrStmtp};
            disableCntp->جوڑو_اگلا_ایتھے(alwaysp);

            // Store value of that counter at the beginning of sequence evaluation
            AstBegin* ثابت bodyp = nodep->bodyp();
            AstVar* ثابت initialCntp = جديد AstVar{flp, VVarType::BLOCKTEMP, "__VinitialCnt",
                                                   nodep->لبھو_Basic_DType(VBasicDTypeKwd::UINT32)};
            initialCntp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            Ast_سونپو* ثابت assignp
                = جديد Ast_سونپو{flp, جديد AstVarRef{flp, initialCntp, VAccess::WRITE},
                                readCntRefp->clone_درخت(خطأ)};
            // Prepend to the sequence body to keep statement list structure valid.
            UASSERT_OBJ(!bodyp->name().خالی_تھاواں(), bodyp, "Sequence body should be named");
            Ast_گرہ* ثابت newStmtsp = Ast_گرہ::جوڑو_اگلا<Ast_گرہ, Ast_گرہ>(initialCntp, assignp);
            bodyp->prependStmtsp(newStmtsp);

            m_disableSeqIfp
                = جديد AstIf{flp, جديد AstEq{flp, جديد AstVarRef{flp, initialCntp, VAccess::READ},
                                           readCntRefp->clone_درخت(خطأ)}};
            // Delete it, because it is always copied before insetion to the AST
            ڈالو_Deletep(m_disableSeqIfp);  // m_disableSeqIfp used later
        }
        iterateChildren(nodep);
    }
    فراغ ویکھو(Ast_گرہ_Module* nodep) override {
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultClkEvtVarp);
        VL_RESTORER(m_defaultDisablep);
        VL_RESTORER(m_modp);
        إذا (!VN_IS(nodep, Class)) {
            // A class nested in a module is in the scope of the module's default clocking
            m_defaultClockingp = خالی_سوچک;
            m_defaultClkEvtVarp = خالی_سوچک;
        }
        m_defaultDisablep = nodep->defaultDisablep();
        m_modp = nodep;
        iterateChildren(nodep);
    }
    فراغ ویکھو(Ast_Gen_بلاک* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstProperty* nodep) override {
        // The body will be visited when will be substituted in place of property reference
        // (AstFuncRef)
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_پٹھ()), nodep);
    }
    فراغ ویکھو(AstSequence* nodep) override {
        // Sequence declarations are not visited directly; their bodies are inlined
        // at call sites by visit(AstFuncRef*). Collect for post-traversal cleanup.
        m_seqs_منزل_Cleanup.ڈالو_پٹھ(nodep);
    }
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

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
                VL_DO_DANGLING(seqp->unlink_Fr_پٹھ()->delete_درخت(), seqp);
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
    V3Global::dump_Check_Global_درخت("assertpre", 0, dump_درخت_Either_Level() >= 3);
}
