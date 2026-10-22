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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3AssertPre.h"

#شامل "V3Assert.h"
#شامل "V3Const.h"
#شامل "V3Task.h"
#شامل "V3Timing.h"
#شامل "V3UniqueNames.h"

#شامل <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clocking blocks of clocking items

جماعت AssertPreClockingVisitor final : عوامی VNVisitorConst {
    // Captures the clocking block of each clocking item before AssertPreVisitor, which needs it
    // at synchronous drives, often visited before the clocking block, e.g. in a parent module
    // NODE STATE
    // AstClockingItem::user3p()  // AstClocking*. Clocking block of the item, user3 claimed by
    //                            //               AssertPreVisitor

    // VISITORS
    خالی دورہ(AstNetlist* nodep) override { iterate_And_اگلا_Const_Null(nodep->modulesp()); }
    خالی دورہ(Ast_گرہ_Module* nodep) override { iterate_And_اگلا_Const_Null(nodep->stmtsp()); }
    خالی دورہ(Ast_Gen_بلاک* nodep) override { iterate_And_اگلا_Const_Null(nodep->itemsp()); }
    خالی دورہ(AstClocking* nodep) override {
        برائے (Ast_گرہ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            اگر (Ast_Clocking_شے* ثابت citemp = VN_CAST(itemp, Clocking_شے)) {
                citemp->user3p(nodep);
            }
        }
    }
    // Clocking blocks are only in modules and their generate blocks
    خالی دورہ(Ast_گرہ*) override {}

عوامی:
    // CONSTRUCTORS
    صریح AssertPreClockingVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~AssertPreClockingVisitor() override = طےشدہ;
};

//######################################################################
// Assert class functions

جماعت AssertPreVisitor final : عوامی VNVisitor {
    // Removes clocks and other pre-optimizations
    // Eventually inlines calls to sequences, properties, etc.
    // We're not parsing the tree, or anything more complicated.
نجی:
    // TYPES
    ساخت SynchDrive final {
        Ast_Clocking_شے* itemp;  // Clocking item of the driven clockvar
        Ast_گرہ_اظہاریہ* refp;  // Reference to the driven clockvar
    };

    // NODE STATE
    // AstClockingItem::user1p()         // AstVar*.      varp() of ClockingItem after unlink
    // AstClockingItem::user2p()         // AstVar*.      Flag set by drives of output clockvar
    // AstClockingItem::user3p()         // AstClocking*. Clocking block of the item, captured
    //                                   //               by AssertPreClockingVisitor
    // AstPExpr::user1()                 // bool.         Created from AstUntil
    ثابت VNUser1_اندر_Use m_inuser1;
    ثابت VNUser2_اندر_Use m_inuser2;
    ثابت VNUser3_اندر_Use m_inuser3;
    // STATE
    // Current context:
    AstNetlist* ثابت m_netlistp = خالی_اشاریہ;  // Current netlist
    Ast_گرہ_Module* m_modp = خالی_اشاریہ;  // Current module
    AstClocking* m_clockingp = خالی_اشاریہ;  // Current clocking block
    // Reset each module:
    AstClocking* m_defaultClockingp = خالی_اشاریہ;  // Default clocking for current module
    AstVar* m_defaultClkEvtVarp = خالی_اشاریہ;  // Event var for default clocking (for ##0)
    AstDefaultDisable* m_defaultDisablep = خالی_اشاریہ;  // Default disable for current module
    // Reset each assertion:
    Ast_Sen_شے* m_senip = خالی_اشاریہ;  // Last sensitivity
    // Reset each always:
    Ast_Sen_شے* m_seniAlwaysp = خالی_اشاریہ;  // Last sensitivity in always
    // Reset each assertion:
    Ast_گرہ_اظہاریہ* m_disablep = خالی_اشاریہ;  // Last disable
    AstIf* m_disableSeqIfp = خالی_اشاریہ;  // Used for handling disable iff in sequences
    AstPExpr* m_pexprp = خالی_اشاریہ;  // Last AstPExpr
    منطقی m_underCover = جھوٹ;  // True if the enclosing assertion is a cover
    // Other:
    V3_Unique_نام m_چکر_Dly_نام{"__VcycleDly"};  // Cycle delay counter name generator
    V3_Unique_نام m_cons_Rep_نام{"__VconsRep"};  // Consecutive repetition counter name generator
    V3_Unique_نام m_goto_Rep_نام{"__VgotoRep"};  // Goto repetition counter name generator
    V3_Unique_نام m_non_Cons_Rep_نام{"__VnonConsRep"};  // Nonconsecutive rep name generator
    V3_Unique_نام m_disable_Cnt_نام{"__VdisableCnt"};  // Disable condition counter name generator
    V3_Unique_نام m_بلاک_نام{"__VassertBlock"};  // Names of blocks with temporaries
    V3_Unique_نام m_prop_Var_نام{"__Vpropvar"};  // Property-local variable name generator
    V3_Unique_نام m_فعال_نام{"__VassertsActive"};  // Active asserts map name generator
    V3_Unique_نام m_driven_نام{"__VclockingDriven"};  // Clockvar drive flag name generator
    منطقی m_اندر_تفویض = جھوٹ;  // True if in an AssignNode
    منطقی m_اندر_تفویض_Dly_Lhs = جھوٹ;  // True if in AssignDly's LHS
    منطقی m_اندر_Synch_Drive = جھوٹ;  // True if in synchronous drive
    منطقی m_اندر_Concat_تفویض = جھوٹ;  // True if in assignment to a concatenation
    std::سمتیہ<SynchDrive> m_drives;  // Clockvars written by the synchronous drive
    منطقی m_has_چکر_Delay = جھوٹ;  // True if node has cycle delay beneath
    std::سمتیہ<AstVarXRef*> m_xrefsp;  // list of xrefs that need name fixup
    std::سمتیہ<AstSequence*> m_seqs_مقصد_Cleanup;  // Sequences to clean up after traversal

    // METHODS

    // Flag set by each drive of an output clockvar, added to the module with its clocking item
    AstVar* getCreateDrivenVarp(Ast_Clocking_شے* itemp) {
        اگر (!itemp->user2p()) {
            AstVar* ثابت varp = نیا AstVar{itemp->fileline(), VVarType::MODULETEMP,
                                            m_driven_نام.get(itemp), itemp->ڈھونڈو_ثنائی_ہندسہ_DType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            itemp->user2p(varp);
        }
        واپس VN_AS(itemp->user2p(), Var);
    }
    // Reference to a variable in the scope of a driven clockvar, made like the clockvar's
    ساکن Ast_گرہ_اظہاریہ* newDriveRefp(File_سطر* flp, ثابت SynchDrive& drive, AstVar* varp,
                                     ثابت VAccess& access) {
        اگر (ثابت AstVarXRef* ثابت xrefp = VN_CAST(drive.refp, VarXRef)) {
            واپس نیا AstVarXRef{flp, varp, xrefp->dotted(), access};
        } ورنہ اگر (ثابت AstMemberSel* ثابت selp = VN_CAST(drive.refp, MemberSel)) {
            // The interface expression is evaluated again, so it must not have side effects;
            // cloneTreePure warns if it has any
            AstMemberSel* ثابت newSelp
                = نیا AstMemberSel{flp, selp->fromp()->clone_درخت_خالص(جھوٹ), varp};
            newSelp->access(access);
            واپس newSelp;
        }
        واپس نیا AstVarRef{flp, varp, access};
    }
    // Assignment setting the drive flag of a driven clockvar
    Ast_تفویض* newDrivenSetp(File_سطر* flp, ثابت SynchDrive& drive) {
        AstVar* ثابت varp = getCreateDrivenVarp(drive.itemp);
        Ast_تفویض* ثابت setp = نیا Ast_تفویض{flp, newDriveRefp(flp, drive, varp, VAccess::WRITE),
                                              نیا AstConst{flp, AstConst::ثنائی_ہندسہ_True{}}};
        setp->user1(سچ);
        واپس setp;
    }
    // Clocking block of a clocking item
    ساکن AstClocking* clocking_کا(ثابت Ast_Clocking_شے* itemp) {
        UASSERT_OBJ(itemp->user3p(), itemp, "Clocking item not captured in a clocking block");
        واپس VN_AS(itemp->user3p(), Clocking);
    }
    // Clocking event of a driven clockvar, referenced from the drive like the clockvar, or
    // nullptr if unsupported, which is reported on the drive's cycle delay
    Ast_Sen_درخت* newDriveSentreep(AstDelay* delayp, ثابت SynchDrive& drive) {
        Ast_Sen_درخت* ثابت origp = clocking_کا(drive.itemp)->sentreep();
        اگر (VN_IS(drive.refp, VarRef)) واپس origp->clone_درخت(جھوٹ);
        اگر (origp->exists([](ثابت AstVarXRef*) { واپس سچ; })) {
            // The reference is relative to the clocking block
            delayp->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in synchronous drive to"
                                          " clockvar in another scope, whose clocking event"
                                          " has a hierarchical reference");
            واپس خالی_اشاریہ;
        }
        ثابت AstMemberSel* ثابت selp = VN_CAST(drive.refp, MemberSel);
        // Processes outside classes evaluate their events also when not waiting on them,
        // possibly before the interface reference is set, so the event is guarded against a
        // null reference, which needs an integral value; scheduling does not guard it
        ثابت منطقی guardNull = selp && !VN_IS(m_modp, Class);
        اگر (guardNull) {
            برائے (Ast_Sen_شے* itemp = origp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Sen_شے)) {
                اگر (!itemp->sensp()->dtypep()->skipRefp()->isIntegralOrPacked()) {
                    delayp->v3warn(E_UNSUPPORTED,
                                   "Unsupported: cycle delay in synchronous drive through"
                                   " an interface reference outside a class, to clocking"
                                   " block with non-integral clocking event");
                    واپس خالی_اشاریہ;
                }
            }
        }
        // Interface declaring the clocking block, if driven through an interface reference
        AstIface* ثابت ifacep
            = selp ? VN_AS(selp->fromp()->dtypep()->skipRefp(), IfaceRefDType)->iface_بذریعہ_Cellp()
                   : خالی_اشاریہ;
        Ast_Sen_درخت* ثابت sentreep = origp->clone_درخت(جھوٹ);
        std::سمتیہ<AstVarRef*> refps;
        sentreep->foreach([&](AstVarRef* refp) {
            اگر (!refp->classOrPackagep()) refps.ڈالو_عقب(refp);
        });
        برائے (AstVarRef* ثابت refp : refps) {
            AstVar* ثابت varp = refp->varp();
            اگر (ifacep) {
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
        اگر (guardNull) {
            برائے (Ast_Sen_شے* itemp = sentreep->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Sen_شے)) {
                guardNullIface(itemp, selp->fromp());
            }
        }
        واپس sentreep;
    }
    // Make an integral clocking event through an interface reference false while the reference
    // is null
    ساکن خالی guardNullIface(Ast_Sen_شے* sensesp, Ast_گرہ_اظہاریہ* fromp) {
        File_سطر* ثابت flp = sensesp->fileline();
        Ast_گرہ_اظہاریہ* ثابت sensp = sensesp->sensp()->unlink_Fr_عقب();
        sensesp->sensp(نیا AstCond{flp, newNotNullp(flp, fromp), sensp,
                                   نیا AstConst{flp, AstConst::DTyped{}, sensp->dtypep()}});
        اگر (Ast_گرہ_اظہاریہ* ثابت condp = sensesp->condp()) {
            condp->unlink_Fr_عقب();
            sensesp->condp(نیا AstLogAnd{flp, newNotNullp(flp, fromp), condp});
        }
    }
    ساکن Ast_گرہ_اظہاریہ* newNotNullp(File_سطر* flp, Ast_گرہ_اظہاریہ* fromp) {
        واپس نیا AstNeq{flp, fromp->clone_درخت_خالص(جھوٹ), نیا AstConst{flp, AstConst::Null{}}};
    }
    // Procedural '##0': if the default clocking event has not yet occurred in this time step,
    // wait for it; otherwise continue without suspension (IEEE 1800-2023 14.11)
    AstIf* new_چکر_Delay_Zerop(File_سطر* flp) ثابت {
        AstVar* ثابت evtVarp = m_defaultClkEvtVarp;
        UASSERT_OBJ(evtVarp, flp, "Default clocking event var not pre-created");
        AstCMethodHard* ثابت isTriggeredp = نیا AstCMethodHard{
            flp, نیا AstVarRef{flp, evtVarp, VAccess::READ}, VCMethod::EVENT_IS_TRIGGERED};
        isTriggeredp->dtype_سیٹ_ثنائی_ہندسہ();
        AstEventControl* ثابت waitp = نیا AstEventControl{
            flp,
            نیا Ast_Sen_درخت{flp, نیا Ast_Sen_شے{flp, VEdgeType::ET_EVENT,
                                               نیا AstVarRef{flp, evtVarp, VAccess::READ}}},
            خالی_اشاریہ};
        واپس نیا AstIf{flp, نیا AstNot{flp, isTriggeredp}, waitp};
    }

    ساکن خالی checkSamplingFuncDType(Ast_گرہ_اظہاریہ* nodep, ثابت Ast_گرہ* exprp) {
        ثابت Ast_گرہ_DType* ثابت dtypep = exprp->dtypep()->skipRefp();
        اگر (!dtypep->isIntegralOrPacked()) {
            nodep->v3error("Expected numeric type, but got a " << dtypep->prettyDTypeNameQ()
                                                               << " data type");
        }
    }

    Ast_Sen_درخت* new_Sen_درخت(Ast_گرہ* nodep, Ast_Sen_درخت* useTreep = خالی_اشاریہ,
                           Ast_گرہ_Cover_Or_Assert* cassertp = خالی_اشاریہ) {
        // Create sentree based on clocked or default clock
        // Return nullptr for always
        اگر (useTreep) واپس useTreep;
        Ast_Sen_درخت* newp = خالی_اشاریہ;
        Ast_Sen_شے* senip = m_senip;
        منطقی fromAlways = جھوٹ;
        اگر (!senip && m_defaultClockingp) senip = m_defaultClockingp->sentreep()->sensesp();
        اگر (!senip) {
            senip = m_seniAlwaysp;
            fromAlways = سچ;
        }
        اگر (!senip) {
            nodep->v3error("Concurrent assertion has no clock (IEEE 1800-2023 16.16)\n"
                           << nodep->warn_مزید()
                           << "... Suggest provide a clocking event, a default"
                              " clocking, or a clocked procedural context");
            newp = نیا Ast_Sen_درخت{nodep->fileline(), خالی_اشاریہ};
        } ورنہ {
            اگر (cassertp && fromAlways) cassertp->senFromAlways(سچ);
            newp = نیا Ast_Sen_درخت{nodep->fileline(), senip->clone_درخت(سچ)};
        }
        واپس newp;
    }
    Ast_گرہ_اظہاریہ* getSequenceBodyExprp(ثابت AstSequence* ثابت seqp) ثابت {
        // The statements in AstSequence are optional AstVar (ports) followed by body expr.
        Ast_گرہ* bodyp = seqp->stmtsp();
        جب (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        واپس VN_CAST(bodyp, گرہ_اظہاریہ);
    }
    AstPropSpec* getPropertyExprp(ثابت AstProperty* ثابت propp) {
        // Statements in AstProperty: AstVar (ports/local vars),
        // AstInitialStaticStmt/AstInitialAutomaticStmt (var init), AstPropSpec (body).
        Ast_گرہ* propExprp = propp->stmtsp();
        جب (propExprp
               && (VN_IS(propExprp, Var) || VN_IS(propExprp, InitialStaticStmt)
                   || VN_IS(propExprp, InitialAutomaticStmt))) {
            propExprp = propExprp->nextp();
        }
        واپس VN_CAST(propExprp, PropSpec);
    }
    خالی substituteSequenceCall(AstFuncRef* funcrefp, AstSequence* seqp) {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Inline the sequence body at the call site, replacing the FuncRef
        Ast_گرہ_اظہاریہ* bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        // Clone the body expression since the sequence may be referenced multiple times
        Ast_گرہ_اظہاریہ* clonedp = bodyExprp->clone_درخت(جھوٹ);
        // Build substitution map, then do a single traversal to replace all formals
        // (textual substitution per IEEE 16.8.2).
        ثابت V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<ثابت AstVar*, Ast_گرہ_اظہاریہ*> پورٹ_Map;
        برائے (ثابت خودکار& tconnect : tconnects) {
            پورٹ_Map[tconnect.پہلا] = tconnect.دوسرا->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            ثابت خودکار it = پورٹ_Map.ڈھونڈو(refp->varp());
            اگر (it != پورٹ_Map.انتہا()) {
                refp->replaceWith(it->دوسرا->clone_درخت(جھوٹ));
                VL_DO_DANGLING(ڈالو_Deletep(refp), refp);
            }
        });
        برائے (ثابت خودکار& tconnect : tconnects) {
            ڈالو_Deletep(tconnect.دوسرا->exprp()->unlink_Fr_عقب());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        // Replace the FuncRef with the inlined body
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(ڈالو_Deletep(funcrefp), funcrefp);
        // Clear referenced flag; sequences with isReferenced==false are deleted in assertPreAll
        seqp->isReferenced(جھوٹ);
    }
    AstPropSpec* clonePropertyBody(AstFuncRef* funcrefp, ثابت AstProperty* propp) {
        AstPropSpec* propExprp = getPropertyExprp(propp);
        // Substitute inner property call before copying in order to not doing the same for
        // each call of outer property call.
        propExprp = substitutePropertyCall(propExprp);
        // Clone subtree after substitution. It is needed, because property might be called
        // multiple times with different arguments.
        propExprp = propExprp->clone_درخت(جھوٹ);
        // Build substitution maps for formal arguments and property-local
        // variables, then perform a single foreach to apply all replacements.
        // Map port vars to their actual argument expressions
        ثابت V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propp->stmtsp());
        std::unordered_map<ثابت AstVar*, Ast_گرہ_اظہاریہ*> پورٹ_Map;
        برائے (ثابت خودکار& tconnect : tconnects) {
            پورٹ_Map[tconnect.پہلا] = tconnect.دوسرا->exprp();
        }

        // Promote property-local variables (non-port vars, IEEE 16.10) to
        // module-level __Vpropvar temps. Cross-cycle persistence is handled
        // by the match item lowering in visit(AstImplication*).
        std::unordered_map<ثابت AstVar*, AstVar*> localVarMap;
        برائے (Ast_گرہ* stmtp = propp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                اگر (!varp->isIO()) {
                    ثابت string newName = m_prop_Var_نام.get(varp);
                    AstVar* ثابت newVarp = نیا AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->جوڑو_Stmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        // Single traversal: substitute ports and update local var references
        propExprp->foreach([&](AstVarRef* refp) {
            ثابت خودکار پورٹ_It = پورٹ_Map.ڈھونڈو(refp->varp());
            اگر (پورٹ_It != پورٹ_Map.انتہا()) {
                refp->replaceWith(پورٹ_It->دوسرا->clone_درخت(جھوٹ));
                VL_DO_DANGLING(ڈالو_Deletep(refp), refp);
                واپس;
            }
            ثابت خودکار localIt = localVarMap.ڈھونڈو(refp->varp());
            اگر (localIt != localVarMap.انتہا()) refp->varp(localIt->دوسرا);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        // Clean up argument expressions
        برائے (ثابت خودکار& tconnect : tconnects) {
            ڈالو_Deletep(tconnect.دوسرا->exprp()->unlink_Fr_عقب());
        }
        V3AssertCommon::checkSvaInstanceDelays(propExprp, funcrefp);
        واپس propExprp;
    }
    AstPropSpec* substitutePropertyCall(AstPropSpec* nodep) {
        اگر (AstFuncRef* ثابت funcrefp = VN_CAST(nodep->propp(), FuncRef)) {
            اگر (ثابت AstProperty* ثابت propp = VN_CAST(funcrefp->taskp(), Property)) {
                AstPropSpec* ثابت propExprp = clonePropertyBody(funcrefp, propp);

                // Handle case with 2 disable iff statement (IEEE 1800-2023 16.12.1)
                اگر (nodep->disablep() && propExprp->disablep()) {
                    nodep->v3error("disable iff expression before property call and in its "
                                   "body is not legal");
                    ڈالو_Deletep(propExprp->disablep()->unlink_Fr_عقب());
                }
                // If disable iff is in outer property, move it to inner
                اگر (nodep->disablep()) {
                    Ast_گرہ_اظہاریہ* ثابت disablep = nodep->disablep()->unlink_Fr_عقب();
                    propExprp->disablep(disablep);
                }

                اگر (nodep->sensesp() && propExprp->sensesp()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Clock event before property call and in its body");
                    ڈالو_Deletep(propExprp->sensesp()->unlink_Fr_عقب());
                }
                // If clock event is in outer property, move it to inner
                اگر (nodep->sensesp()) {
                    Ast_Sen_شے* ثابت sensesp = nodep->sensesp();
                    sensesp->unlink_Fr_عقب();
                    propExprp->sensesp(sensesp);
                }

                // Now substitute property reference with property body
                nodep->replaceWith(propExprp);
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                واپس propExprp;
            }
        }
        واپس nodep;
    }

    خالی substituteNestedPropertyCall(AstFuncRef* funcrefp, ثابت AstProperty* propp) {
        AstPropSpec* ثابت specp = clonePropertyBody(funcrefp, propp);
        Ast_گرہ* replp = خالی_اشاریہ;
        اگر (specp->disablep()) {
            funcrefp->v3error("Illegal 'disable iff' in a nested property instance"
                              " (IEEE 1800-2023 16.12.1)");
        } ورنہ اگر (specp->sensesp()) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: clock event in a nested property instance");
        } ورنہ اگر (specp->propp()->exists(
                       [](ثابت Ast_گرہ_اظہاریہ* np) { واپس np->is_Multi_چکر_Sva(); })) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: multi-cycle body in a nested property instance");
        } ورنہ {
            replp = specp->propp()->unlink_Fr_عقب();
        }
        اگر (!replp) replp = نیا AstConst{funcrefp->fileline(), AstConst::ثنائی_ہندسہ_False{}};
        funcrefp->replaceWith(replp);
        VL_DO_DANGLING(ڈالو_Deletep(funcrefp), funcrefp);
        VL_DO_DANGLING(ڈالو_Deletep(specp), specp);
    }

    // VISITORS
    //========== Statements
    خالی دورہ(AstClocking* ثابت nodep) override {
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        UINFO(8, "   CLOCKING" << nodep);
        اگر (nodep->isDefault()) {
            اگر (m_defaultClockingp) {
                nodep->v3error("Only one default clocking block allowed per module"
                               " (IEEE 1800-2023 14.12)");
            } ورنہ {
                m_defaultClockingp = nodep;
                m_defaultClkEvtVarp = nodep->ensureEventp();
            }
        }
        iterateChildren(nodep);
        اگر (nodep->eventp()) nodep->جوڑو_اگلا_یہاں(nodep->eventp()->unlink_Fr_عقب());
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
    }
    خالی دورہ(AstModportClockingRef* ثابت nodep) override {
        // It has to be converted to a list of ModportClockingVarRefs,
        // because clocking blocks are removed in this pass
        برائے (Ast_گرہ* itemp = nodep->clockingp()->itemsp(); itemp; itemp = itemp->nextp()) {
            اگر (Ast_Clocking_شے* ثابت citemp = VN_CAST(itemp, Clocking_شے)) {
                اگر (AstVar* ثابت varp
                    = citemp->varp() ? citemp->varp() : VN_AS(citemp->user1p(), Var)) {
                    AstModportVarRef* ثابت modVarp = نیا AstModportVarRef{
                        nodep->fileline(), varp->name(), citemp->direction()};
                    modVarp->varp(varp);
                    nodep->جوڑو_اگلا_یہاں(modVarp);
                    اگر (citemp->direction().isOutput()) {
                        // Drives through the modport set the drive flag
                        AstVar* ثابت drivenVarp = getCreateDrivenVarp(citemp);
                        AstModportVarRef* ثابت drivenRefp = نیا AstModportVarRef{
                            nodep->fileline(), drivenVarp->name(), VDirection::OUTPUT};
                        drivenRefp->varp(drivenVarp);
                        nodep->جوڑو_اگلا_یہاں(drivenRefp);
                    }
                }
            }
        }
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
    }
    خالی دورہ(Ast_Clocking_شے* ثابت nodep) override {
        // Get a ref to the sampled/driven variable
        AstVar* ثابت varp = nodep->varp();
        اگر (!varp) {
            // Unused item
            واپس;
        }
        // Flag set by drives of an output clockvar, possibly visited before this item
        اگر (nodep->direction().isOutput()) m_modp->جوڑو_Stmtsp(getCreateDrivenVarp(nodep));
        File_سطر* ثابت flp = nodep->fileline();
        V3Const::constifyEdit(nodep->skewp());
        اگر (!VN_IS(nodep->skewp(), Const)) {
            nodep->skewp()->v3error("Skew must be constant (IEEE 1800-2023 14.4)");
            واپس;
        }
        AstConst* ثابت skewp = VN_AS(nodep->skewp(), Const);
        اگر (skewp->عدد().isNegative()) skewp->v3error("Skew cannot be negative");
        Ast_گرہ_اظہاریہ* ثابت exprp = nodep->exprp();
        varp->name(m_clockingp->name() + "__DOT__" + varp->name());
        m_clockingp->جوڑو_اگلا_یہاں(varp->unlink_Fr_عقب());
        nodep->user1p(varp);
        varp->user1p(nodep);
        اگر (nodep->direction() == VDirection::OUTPUT) {
            exprp->foreach([](ثابت Ast_گرہ_Var_Ref* varrefp) {
                // Prevent confusing BLKANDNBLK warnings on clockvars due to generated assignments
                varrefp->fileline()->warnOff(V3_Error_رمز::BLKANDNBLK, سچ);
                // Mark the driven signal so later passes (e.g. V3Inst port
                // connection) know the drive flows out from inside the cell.
                // Item expressions are plain signal references, so only the
                // driven signal is marked, never index selects.
                varrefp->varp()->isClockingDriven(سچ);
            });
            AstVarRef* ثابت skewedReadRefp = نیا AstVarRef{flp, varp, VAccess::READ};
            skewedReadRefp->user1(سچ);
            // Initialize the clockvar
            AstVarRef* ثابت skewedWriteRefp = نیا AstVarRef{flp, varp, VAccess::WRITE};
            skewedWriteRefp->user1(سچ);
            AstInitialStatic* ثابت initClockvarp = نیا AstInitialStatic{
                flp, نیا Ast_تفویض{flp, skewedWriteRefp, exprp->clone_درخت_خالص(جھوٹ)}};
            m_modp->جوڑو_Stmtsp(initClockvarp);
            // Each drive sets a flag, so the signal is also assigned when driven with the same
            // value again, e.g. after another assignment to the signal (IEEE 1800-2023 14.16.2)
            AstVar* ثابت drivenVarp = getCreateDrivenVarp(nodep);
            m_modp->جوڑو_Stmtsp(نیا AstInitialStatic{
                flp, نیا Ast_تفویض{flp, نیا AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                   نیا AstConst{flp, AstConst::ثنائی_ہندسہ_False{}}}});
            // Assign the clockvar to the actual var if it was driven
            Ast_تفویض* ثابت assignp
                = نیا Ast_تفویض{flp, exprp->clone_درخت_خالص(جھوٹ), skewedReadRefp};
            AstIf* ثابت ifp
                = نیا AstIf{flp, نیا AstVarRef{flp, drivenVarp, VAccess::READ},
                            نیا Ast_تفویض{flp, نیا AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                          نیا AstConst{flp, AstConst::ثنائی_ہندسہ_False{}}}};
            ifp->جوڑو_Thensp(assignp);
            // Only in the time step of the clocking event
            AstCMethodHard* ثابت trigp = نیا AstCMethodHard{
                nodep->fileline(), نیا AstVarRef{flp, m_clockingp->ensureEventp(), VAccess::READ},
                VCMethod::EVENT_IS_TRIGGERED};
            trigp->dtype_سیٹ_ثنائی_ہندسہ();
            ifp->condp(نیا AstLogAnd{flp, ifp->condp()->unlink_Fr_عقب(), trigp});
            // A skew rounding to no time precision ticks is a zero skew
            Ast_گرہ_اظہاریہ* ثابت ticksp
                = V3Timing::delay_مقصد_Ticks(skewp->clone_درخت(جھوٹ), m_modp->timeunit());
            ثابت منطقی zeroSkew = ticksp->isZero();
            VL_DO_DANGLING(ticksp->delete_درخت(), ticksp);
            اگر (zeroSkew || !skewp->fileline()->timingOn()) {
                // Drive the var in Re-NBA (IEEE 1800-2023 14.16), ignoring skews where timing is
                // turned off
                Ast_Sen_درخت* senTreep = m_clockingp->sentreep()->clone_درخت(جھوٹ);
                senTreep->جوڑو_Sensesp(نیا Ast_Sen_شے{
                    flp, VEdgeType::ET_بدلا, نیا AstVarRef{flp, drivenVarp, VAccess::READ}});
                m_clockingp->جوڑو_اگلا_یہاں(نیا AstAlwaysReactive{flp, senTreep, ifp});
            } ورنہ {
                // Fork only the assignment, so this AlwaysPostponed can be retriggered
                // before the assignment happens
                AstFork* ثابت forkp = نیا AstFork{flp, VJoinType::JOIN_NONE};
                forkp->فوری_آغاز(سچ);
                forkp->جوڑو_Forksp(نیا AstBegin{flp, "", assignp->unlink_Fr_عقب(), سچ});
                ifp->جوڑو_Thensp(forkp);
                // Read the clockvar in Postponed, so a drive late in the time step is applied
                // at this event plus the skew (IEEE 1800-2023 14.16)
                m_clockingp->جوڑو_اگلا_یہاں(نیا AstAlwaysPostponed{flp, ifp});
                اگر (v3Global.opt.timing().is_سیٹ_True()) {
                    AstDelay* ثابت delayp = نیا AstDelay{flp, skewp->unlink_Fr_عقب(), جھوٹ};
                    delayp->timeunit(m_modp->timeunit());
                    assignp->timingControlp(delayp);
                } ورنہ اگر (v3Global.opt.timing().is_سیٹ_False()) {
                    nodep->v3warn(E_NOTIMING,
                                  "Clocking output skew greater than #0 requires --timing");
                } ورنہ {
                    nodep->v3warn(E_NEEDTIMINGOPT,
                                  "Use --timing or --no-timing to specify how "
                                  "clocking output skew greater than #0 should be handled");
                }
            }
        } ورنہ اگر (nodep->direction() == VDirection::داخلہ) {
            // Ref to the clockvar
            AstVarRef* ثابت refp = نیا AstVarRef{flp, varp, VAccess::WRITE};
            refp->user1(سچ);
            اگر (skewp->عدد().is1_قدم()) {
                // #1step means the value that is sampled is always the signal's last value
                // before the clock edge (IEEE 1800-2023 14.4)
                AstSampled* ثابت sampledp
                    = نیا AstSampled{flp, exprp->clone_درخت_خالص(جھوٹ), exprp->dtypep(), سچ};
                Ast_تفویض* ثابت assignp = نیا Ast_تفویض{flp, refp, sampledp};
                m_clockingp->جوڑو_اگلا_یہاں(نیا AstAlways{
                    flp, VAlwaysKwd::ALWAYS, m_clockingp->sentreep()->clone_درخت(جھوٹ), assignp});
            } ورنہ اگر (skewp->isZero()) {
                // #0 means the var has to be sampled in Observed (IEEE 1800-2023 14.13)
                Ast_تفویض* ثابت assignp = نیا Ast_تفویض{flp, refp, exprp->clone_درخت_خالص(جھوٹ)};
                m_clockingp->جوڑو_اگلا_یہاں(نیا AstAlwaysObserved{
                    flp, m_clockingp->sentreep()->clone_درخت(جھوٹ), assignp});
            } ورنہ {
                // Create a queue where we'll store sampled values with timestamps
                Ast_Sample_قطار_DType* ثابت قطار_Dtp
                    = نیا Ast_Sample_قطار_DType{flp, exprp->dtypep()};
                m_netlistp->typeTablep()->جوڑو_Typesp(قطار_Dtp);
                AstVar* ثابت قطار_Varp
                    = نیا AstVar{flp, VVarType::MODULETEMP, "__Vqueue__" + varp->name(), قطار_Dtp};
                قطار_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_clockingp->جوڑو_اگلا_یہاں(قطار_Varp);
                // Create a process like this:
                //     always queue.push(<sampled var>);
                AstCMethodHard* ثابت pushp = نیا AstCMethodHard{
                    flp, نیا AstVarRef{flp, قطار_Varp, VAccess::WRITE}, VCMethod::DYN_ڈالو,
                    نیا Ast_وقت{nodep->fileline(), m_modp->timeunit()}};
                pushp->جوڑو_Pinsp(exprp->clone_درخت_خالص(جھوٹ));
                pushp->dtype_سیٹ_Void();
                m_clockingp->جوڑو_اگلا_یہاں(
                    نیا AstAlways{flp, VAlwaysKwd::ALWAYS, خالی_اشاریہ, pushp->makeStmt()});
                // Create a process like this:
                //     always @<clocking event> queue.pop(<skew>, /*out*/<skewed var>);
                AstCMethodHard* ثابت popp = نیا AstCMethodHard{
                    flp, نیا AstVarRef{flp, قطار_Varp, VAccess::READWRITE}, VCMethod::DYN_نکالو,
                    نیا Ast_وقت{nodep->fileline(), m_modp->timeunit()}};
                popp->جوڑو_Pinsp(skewp->unlink_Fr_عقب());
                refp->access(VAccess::READWRITE);  // Only conditionally assigned
                popp->جوڑو_Pinsp(refp);
                popp->dtype_سیٹ_Void();
                m_clockingp->جوڑو_اگلا_یہاں(نیا AstAlways{flp, VAlwaysKwd::ALWAYS,
                                                       m_clockingp->sentreep()->clone_درخت(جھوٹ),
                                                       popp->makeStmt()});
            }
        } ورنہ {
            nodep->v3fatalSrc("Invalid direction");
        }
    }
    خالی دورہ(AstDelay* nodep) override {
        m_has_چکر_Delay |= nodep->is_چکر_Delay();
        // Only cycle delays are relevant in this stage; also only process once
        اگر (!nodep->is_چکر_Delay()) {
            اگر (m_اندر_Synch_Drive) {
                nodep->v3error("Only cycle delays can be used in synchronous drives"
                               " (IEEE 1800-2023 14.16)");
            }
            iterateChildren(nodep);
            واپس;
        }
        اگر (m_اندر_تفویض && !m_اندر_Synch_Drive) {
            nodep->v3error("Cycle delays not allowed as intra-assignment delays"
                           " (IEEE 1800-2023 14.11)");
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
            واپس;
        }
        اگر (nodep->stmtsp()) nodep->جوڑو_اگلا_یہاں(nodep->stmtsp()->unlink_Fr_عقب_With_اگلا());
        File_سطر* ثابت flp = nodep->fileline();
        Ast_گرہ_اظہاریہ* valuep = V3Const::constifyEdit(nodep->lhsp()->unlink_Fr_عقب());
        ثابت AstConst* ثابت constp = VN_CAST(valuep, Const);
        اگر (!constp && m_pexprp) {
            // V3AssertNfa handles non-const delays in sequences before this pass and
            // replaces the property; this branch should never be reached.
            nodep->v3fatalSrc("Non-constant cycle delay in assertion: "
                              "should have been caught by V3AssertNfa");
        } ورنہ اگر (constp && constp->isZero()) {
            VL_DO_DANGLING(ڈالو_Deletep(valuep), valuep);
            اگر (m_اندر_Synch_Drive) {
                // ##0 has no effect in synchronous drives (IEEE 1800-2023 14.11)
                VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
                واپس;
            }
            اگر (m_pexprp) {
                // ##0 in sequence context = zero delay = same clock tick
                VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
                واپس;
            }
            // Procedural ##0: synchronize with default clocking event (IEEE 1800-2023 14.11)
            اگر (!m_defaultClockingp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
                واپس;
            }
            nodep->replaceWith(new_چکر_Delay_Zerop(flp));
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            واپس;
        }
        Ast_Sen_درخت* sentreep = خالی_اشاریہ;
        اگر (!m_drives.خالی_خانے()) {
            اگر (m_اندر_Concat_تفویض) {
                // Also the clockvars may be of different clocking blocks
                nodep->v3error("Cycle delays cannot be used in synchronous drives to a"
                               " concatenation (IEEE 1800-2023 14.16)");
                VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
                VL_DO_DANGLING(ڈالو_Deletep(valuep), valuep);
                واپس;
            }
            // Count the cycles of the driven clockvar's clocking block (IEEE 1800-2023 14.16)
            sentreep = newDriveSentreep(nodep, m_drives.front());
            اگر (!sentreep) {
                VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
                VL_DO_DANGLING(ڈالو_Deletep(valuep), valuep);
                واپس;
            }
        } ورنہ اگر (!m_defaultClockingp) {
            اگر (!m_pexprp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
                VL_DO_DANGLING(ڈالو_Deletep(valuep), valuep);
                واپس;
            }
            sentreep = نیا Ast_Sen_درخت{flp, m_senip->clone_درخت(جھوٹ)};
        } ورنہ {
            sentreep = m_defaultClockingp->sentreep()->clone_درخت(جھوٹ);
        }
        AstEventControl* ثابت controlp
            = نیا AstEventControl{nodep->fileline(), sentreep, خالی_اشاریہ};
        ثابت std::string delayName = m_چکر_Dly_نام.get(nodep);
        Ast_گرہ_اظہاریہ* throughoutp
            = nodep->throughoutp() ? nodep->throughoutp()->unlink_Fr_عقب() : خالی_اشاریہ;

        AstVar* ثابت cntVarp = نیا AstVar{flp, VVarType::BLOCKTEMP, delayName + "__counter",
                                           nodep->ڈھونڈو_Basic_DType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstBegin* ثابت beginp = نیا AstBegin{flp, delayName + "__block", cntVarp, سچ};
        // A non-constant delay is counted at runtime. The count is integral of 64 bits, as
        // V3Width makes delays, or real, which is rounded (IEEE 1800-2023 6.12.1)
        اگر (valuep->isDouble()) {
            valuep = نیا AstRToIRoundS{flp, valuep};
            valuep->dtypeFrom(cntVarp);
        } ورنہ {
            valuep = نیا AstSel{flp, valuep, 0, cntVarp->چوڑائی()};
        }
        beginp->جوڑو_Stmtsp(نیا Ast_تفویض{flp, نیا AstVarRef{flp, cntVarp, VAccess::WRITE}, valuep});
        // If the count is zero, a synchronous drive waits for none, like with a constant '##0',
        // and a procedural delay waits like a procedural '##0' (IEEE 1800-2023 14.11)
        اگر (!constp && !m_اندر_Synch_Drive) {
            beginp->جوڑو_Stmtsp(نیا AstIf{
                flp,
                نیا AstEq{flp, نیا AstVarRef{flp, cntVarp, VAccess::READ}, نیا AstConst{flp, 0}},
                new_چکر_Delay_Zerop(flp)});
        }

        // Throughout: create flag tracking whether condition held every tick
        AstVar* throughoutOkp = خالی_اشاریہ;
        اگر (throughoutp) {
            throughoutOkp = نیا AstVar{flp, VVarType::BLOCKTEMP, delayName + "__throughoutOk",
                                       nodep->ڈھونڈو_Basic_DType(VBasicDTypeKwd::LOGIC_IMPLICIT)};
            throughoutOkp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->جوڑو_Stmtsp(throughoutOkp);
            beginp->جوڑو_Stmtsp(نیا Ast_تفویض{flp, نیا AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            نیا AstConst{flp, AstConst::ثنائی_ہندسہ_True{}}});
            // Check condition at tick 0 (sequence start, before entering loop)
            beginp->جوڑو_Stmtsp(
                نیا AstIf{flp, نیا AstLogNot{flp, throughoutp->clone_درخت_خالص(جھوٹ)},
                          نیا Ast_تفویض{flp, نیا AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                        نیا AstConst{flp, AstConst::ثنائی_ہندسہ_False{}}}});
        }

        {
            AstLoop* ثابت loopp = نیا AstLoop{flp};
            // When throughout is present, exit loop early if condition fails
            Ast_گرہ_اظہاریہ* loopCondp
                = نیا AstGt{flp, نیا AstVarRef{flp, cntVarp, VAccess::READ}, نیا AstConst{flp, 0}};
            اگر (throughoutOkp) {
                loopCondp = نیا AstLogAnd{flp, loopCondp,
                                          نیا AstVarRef{flp, throughoutOkp, VAccess::READ}};
            }
            loopp->جوڑو_Stmtsp(نیا AstLoopTest{flp, loopp, loopCondp});
            loopp->جوڑو_Stmtsp(controlp);
            loopp->جوڑو_Stmtsp(
                نیا Ast_تفویض{flp, نیا AstVarRef{flp, cntVarp, VAccess::WRITE},
                              نیا AstSub{flp, نیا AstVarRef{flp, cntVarp, VAccess::READ},
                                         نیا AstConst{flp, 1}}});
            // Check throughout condition at each tick during delay (IEEE 1800-2023 16.9.9)
            اگر (throughoutp) {
                loopp->جوڑو_Stmtsp(
                    نیا AstIf{flp, نیا AstLogNot{flp, throughoutp},
                              نیا Ast_تفویض{flp, نیا AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            نیا AstConst{flp, AstConst::ثنائی_ہندسہ_False{}}}});
            }
            beginp->جوڑو_Stmtsp(loopp);
        }
        // Compose wrappers on remaining sequence: throughout gate (inner), disable iff (outer)
        Ast_گرہ* remainp = nodep->nextp() ? nodep->nextp()->unlink_Fr_عقب_With_اگلا() : خالی_اشاریہ;
        اگر (throughoutOkp) {
            // If condition failed during delay, fail assertion
            remainp = نیا AstIf{flp, نیا AstVarRef{flp, throughoutOkp, VAccess::READ}, remainp,
                                نیا AstPExprClause{flp, /*pass=*/جھوٹ}};
        }
        اگر (m_disableSeqIfp && remainp) {
            AstIf* ثابت disableSeqIfp = m_disableSeqIfp->clone_درخت(جھوٹ);
            disableSeqIfp->جوڑو_Thensp(remainp);
            remainp = disableSeqIfp;
        }
        اگر (remainp) {
            اگر (throughoutOkp) {
                // throughoutOkp is declared in beginp scope -- check must be inside it
                beginp->جوڑو_Stmtsp(remainp);
            } ورنہ {
                nodep->جوڑو_اگلا_یہاں(remainp);
            }
        }
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    خالی دورہ(Ast_Sen_درخت* nodep) override {
        اگر (m_اندر_Synch_Drive) {
            nodep->v3error("Event controls cannot be used in "
                           "synchronous drives (IEEE 1800-2023 14.16)");
        }

        ثابت AstSampled* sampledp;
        اگر (nodep->exists([&sampledp](ثابت AstSampled* ثابت sp) {
                sampledp = sp;
                واپس سچ;
            })) {
            sampledp->v3warn(E_UNSUPPORTED, "Unsupported: $sampled inside sensitivity list");
        }
    }
    خالی دورہ(Ast_گرہ_Var_Ref* nodep) override {
        UINFO(8, " -varref:  " << nodep);
        UINFO(8, " -varref-var-back:  " << nodep->varp()->backp());
        UINFO(8, " -varref-var-user1:  " << nodep->varp()->user1p());
        اگر (Ast_Clocking_شے* ثابت itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->پہلا_Abovep(),
                Clocking_شے)) {
            اگر (nodep->user1()) واپس;

            // ensure linking still works, this has to be done only once
            اگر (AstVarXRef* xrefp = VN_CAST(nodep, VarXRef)) {
                UINFO(8, " -clockvarxref-in:  " << xrefp);
                string dotted = xrefp->dotted();
                ثابت سائز_t dotPos = dotted.rfind('.');
                dotted.erase(dotPos, string::npos);
                xrefp->dotted(dotted);
                UINFO(8, " -clockvarxref-out: " << xrefp);
                m_xrefsp.emplace_عقب(xrefp);
            }

            اگر (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            اگر (nodep->access().isWriteOrRW()) {
                اگر (itemp->direction() == VDirection::OUTPUT) {
                    اگر (!m_اندر_تفویض_Dly_Lhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    اگر (m_اندر_تفویض) {
                        m_اندر_Synch_Drive = سچ;
                        m_drives.ڈالو_عقب({itemp, nodep});
                    }
                } ورنہ اگر (itemp->direction() == VDirection::داخلہ) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
            nodep->user1(سچ);
        }
    }
    خالی دورہ(AstMemberSel* nodep) override {
        اگر (Ast_Clocking_شے* ثابت itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->پہلا_Abovep(),
                Clocking_شے)) {
            اگر (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            اگر (nodep->access().isWriteOrRW()) {
                اگر (itemp->direction() == VDirection::OUTPUT) {
                    اگر (!m_اندر_تفویض_Dly_Lhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    اگر (m_اندر_تفویض) {
                        m_اندر_Synch_Drive = سچ;
                        m_drives.ڈالو_عقب({itemp, nodep});
                    }
                } ورنہ اگر (itemp->direction() == VDirection::داخلہ) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
        }
    }
    خالی دورہ(Ast_گرہ_تفویض* nodep) override {
        اگر (nodep->user1()) واپس;
        VL_RESTORER(m_اندر_تفویض);
        VL_RESTORER(m_اندر_Synch_Drive);
        VL_RESTORER(m_اندر_Concat_تفویض);
        VL_RESTORER_CLEAR(m_drives);
        m_اندر_تفویض = سچ;
        m_اندر_Synch_Drive = جھوٹ;
        {
            // The braces of a concatenation are a replication, by one
            ثابت Ast_گرہ_اظہاریہ* lhsp = nodep->lhsp();
            اگر (ثابت AstReplicate* ثابت repp = VN_CAST(lhsp, Replicate)) lhsp = repp->srcp();
            m_اندر_Concat_تفویض = VN_IS(lhsp, Concat);
        }
        {
            VL_RESTORER(m_اندر_تفویض_Dly_Lhs);
            m_اندر_تفویض_Dly_Lhs = VN_IS(nodep, تفویض_Dly);
            iterate(nodep->lhsp());
        }
        iterate(nodep->rhsp());
        اگر (nodep->timingControlp()) iterate(nodep->timingControlp());
        // A drive without a cycle delay, also after removing '##0', updates the clockvar now
        اگر (m_اندر_Synch_Drive && !nodep->timingControlp()) {
            Ast_تفویض* ثابت assignp = نیا Ast_تفویض{
                nodep->fileline(), nodep->lhsp()->unlink_Fr_عقب(), nodep->rhsp()->unlink_Fr_عقب()};
            assignp->user1(سچ);
            nodep->replaceWith(assignp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            nodep = assignp;
        }
        // Note the drives when they take effect, after the cycle delay if any. Each clockvar
        // of the LHS is driven, e.g. in a concatenation which other simulators accept, although
        // IEEE 1800-2023 14.16 does not allow it
        Ast_گرہ* setsp = خالی_اشاریہ;
        برائے (ثابت SynchDrive& drive : m_drives) {
            setsp = Ast_گرہ::جوڑو_اگلا(setsp, newDrivenSetp(nodep->fileline(), drive));
        }
        اگر (!setsp) واپس;
        اگر (AstBegin* ثابت beginp = VN_CAST(nodep->timingControlp(), Begin)) {
            beginp->جوڑو_Stmtsp(setsp);
        } ورنہ {
            nodep->جوڑو_اگلا_یہاں(setsp);
        }
    }
    خالی دورہ(AstAlways* nodep) override {
        iterate_And_اگلا_Null(nodep->sentreep());
        اگر (nodep->sentreep()) m_seniAlwaysp = nodep->sentreep()->sensesp();
        iterate_And_اگلا_Null(nodep->stmtsp());
        m_seniAlwaysp = خالی_اشاریہ;
    }

    خالی دورہ(Ast_گرہ_Cover_Or_Assert* nodep) override {
        اگر (nodep->sentreep()) واپس;  // Already processed

        VL_RESTORER(m_senip);
        VL_RESTORER(m_disablep);
        m_senip = خالی_اشاریہ;
        m_disablep = خالی_اشاریہ;

        // Find Clocking's buried under nodep->exprsp
        iterateChildren(nodep);
        اگر (!nodep->فوری()) nodep->sentreep(new_Sen_درخت(nodep, خالی_اشاریہ, nodep));
    }
    خالی دورہ(AstFalling* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گرہ_اظہاریہ* exprp = nodep->exprp()->unlink_Fr_عقب();
        اگر (exprp->چوڑائی() > 1) exprp = نیا AstSel{fl, exprp, 0, 1};
        Ast_گرہ_اظہاریہ* ثابت futurep = نیا AstFuture{fl, exprp, new_Sen_درخت(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = نیا AstAnd{fl, exprp->clone_درخت_خالص(جھوٹ), نیا AstNot{fl, futurep}};
        exprp->dtype_سیٹ_ثنائی_ہندسہ();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    خالی دورہ(AstFell* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گرہ_اظہاریہ* exprp = nodep->exprp()->unlink_Fr_عقب();
        اگر (exprp->چوڑائی() > 1) exprp = نیا AstSel{fl, exprp, 0, 1};
        Ast_Sen_درخت* sentreep = nodep->sentreep();
        اگر (sentreep) sentreep->unlink_Fr_عقب();
        AstPast* ثابت pastp = نیا AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_درخت(nodep, sentreep));
        exprp = نیا AstAnd{fl, pastp, نیا AstNot{fl, exprp->clone_درخت_خالص(جھوٹ)}};
        exprp->dtype_سیٹ_ثنائی_ہندسہ();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    خالی دورہ(AstFuture* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        Ast_Sen_درخت* ثابت sentreep = nodep->sentreep();
        اگر (sentreep) VL_DO_DANGLING(ڈالو_Deletep(sentreep->unlink_Fr_عقب()), sentreep);
        nodep->sentreep(new_Sen_درخت(nodep));
    }
    خالی دورہ(AstPast* nodep) override {
        checkSamplingFuncDType(nodep, nodep->exprp());
        اگر (nodep->sentreep()) واپس;  // Already processed
        iterateChildren(nodep);
        nodep->sentreep(new_Sen_درخت(nodep));
    }
    خالی دورہ(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- Lower standalone exact [*N] (N >= 2) via saturating counter.
        // Range/unbounded forms and SExpr-contained forms are lowered by V3AssertNfa.
        iterateChildren(nodep);
        // V3AssertNfa handles unbounded/ranged forms upstream, so this fast-path
        // is effectively unreachable when NFA is enabled.
        اگر (nodep->unbounded() || nodep->maxCountp()) واپس;  // LCOV_EXCL_LINE
        ثابت AstConst* ثابت constp = VN_CAST(nodep->countp(), Const);
        اگر (VL_UNLIKELY(!constp || constp->مقصد_SInt() < 1)) {
            nodep->v3fatalSrc("Consecutive repetition count must be a positive constant"
                              " (should have been caught by V3Width)");
            واپس;
        }
        ثابت صحیح n = constp->مقصد_SInt();
        File_سطر* ثابت flp = nodep->fileline();
        Ast_گرہ_اظہاریہ* ثابت exprp = nodep->exprp()->unlink_Fr_عقب();
        اگر (n == 1) {
            nodep->replaceWith(exprp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            واپس;
        }
        // Saturating counter: if (expr) cnt <= min(cnt+1, N); else cnt <= 0;
        AstVar* ثابت cntVarp = نیا AstVar{flp, VVarType::MODULETEMP, m_cons_Rep_نام.get(""),
                                           nodep->ڈھونڈو_Basic_DType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->جوڑو_Stmtsp(cntVarp);
        Ast_گرہ_اظہاریہ* ثابت اظہاریہ_Clonep = exprp->clone_درخت_خالص(جھوٹ);
        Ast_گرہ_اظہاریہ* ثابت saturatingIncrp = نیا AstCond{
            flp,
            نیا AstLt{flp, نیا AstVarRef{flp, cntVarp, VAccess::READ},
                      نیا AstConst{flp, static_cast<uint32_t>(n)}},
            نیا Ast_جوڑو{flp, نیا AstVarRef{flp, cntVarp, VAccess::READ}, نیا AstConst{flp, 1u}},
            نیا AstConst{flp, static_cast<uint32_t>(n)}};
        Ast_تفویض_Dly* ثابت incrAssignp
            = نیا Ast_تفویض_Dly{flp, نیا AstVarRef{flp, cntVarp, VAccess::WRITE}, saturatingIncrp};
        Ast_تفویض_Dly* ثابت resetAssignp = نیا Ast_تفویض_Dly{
            flp, نیا AstVarRef{flp, cntVarp, VAccess::WRITE}, نیا AstConst{flp, 0u}};
        AstIf* ثابت ifp = نیا AstIf{flp, اظہاریہ_Clonep, incrAssignp, resetAssignp};
        Ast_Sen_درخت* ثابت senTreep = new_Sen_درخت(nodep);
        AstAlways* ثابت alwaysp = نیا AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, ifp};
        cntVarp->جوڑو_اگلا_یہاں(alwaysp);
        // Match: cnt >= N-1 (previous cycles via NBA) && expr (current cycle)
        Ast_گرہ_اظہاریہ* ثابت cntCheckp = نیا AstGte{flp, نیا AstVarRef{flp, cntVarp, VAccess::READ},
                                                  نیا AstConst{flp, static_cast<uint32_t>(n - 1)}};
        cntCheckp->dtype_سیٹ_ثنائی_ہندسہ();
        Ast_گرہ_اظہاریہ* ثابت matchp = نیا AstAnd{flp, cntCheckp, exprp};
        matchp->dtype_سیٹ_ثنائی_ہندسہ();
        nodep->replaceWith(matchp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    خالی دورہ(AstRising* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گرہ_اظہاریہ* exprp = nodep->exprp()->unlink_Fr_عقب();
        اگر (exprp->چوڑائی() > 1) exprp = نیا AstSel{fl, exprp, 0, 1};
        Ast_گرہ_اظہاریہ* ثابت futurep = نیا AstFuture{fl, exprp, new_Sen_درخت(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = نیا AstAnd{fl, نیا AstNot{fl, exprp->clone_درخت_خالص(جھوٹ)}, futurep};
        exprp->dtype_سیٹ_ثنائی_ہندسہ();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    خالی دورہ(AstRose* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گرہ_اظہاریہ* exprp = nodep->exprp()->unlink_Fr_عقب();
        اگر (exprp->چوڑائی() > 1) exprp = نیا AstSel{fl, exprp, 0, 1};
        Ast_Sen_درخت* sentreep = nodep->sentreep();
        اگر (sentreep) sentreep->unlink_Fr_عقب();
        AstPast* ثابت pastp = نیا AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_درخت(nodep, sentreep));
        exprp = نیا AstAnd{fl, نیا AstNot{fl, pastp}, exprp->clone_درخت_خالص(جھوٹ)};
        exprp->dtype_سیٹ_ثنائی_ہندسہ();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    ساکن AstAssocArrayDType* getProcessAssocArrayType(File_سطر* ثابت flp) {
        // Type of __VassertsActive___x[std::process]
        Ast_گرہ_DType* valp
            = v3Global.rootp()->typeTablep()->ڈھونڈو_Basic_DType(flp, VBasicDTypeKwd::ثنائی_ہندسہ);
        AstClassRefDType* keyp
            = نیا AstClassRefDType{flp, v3Global.rootp()->stdPackageProcessp(), خالی_اشاریہ};
        keyp->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        v3Global.rootp()->typeTablep()->جوڑو_Typesp(keyp);
        AstAssocArrayDType* ثابت typep = نیا AstAssocArrayDType{flp, valp, keyp};
        typep->dtypep(typep);
        v3Global.rootp()->typeTablep()->جوڑو_Typesp(typep);
        واپس typep;
    }
    ساکن Ast_Stmt_اظہاریہ* getProcessAssocArrayDelete(AstVarRef* ثابت refp) {
        // Constructs refp.delete(std::process::self()) statement
        File_سطر* ثابت flp = refp->fileline();
        AstCMethodHard* ثابت deletep = نیا AstCMethodHard{
            flp, refp, VCMethod::ASSOC_ERASE, v3Global.rootp()->stdPackageProcessSelfp(flp)};
        deletep->dtypep(refp->ڈھونڈو_Void_DType());
        واپس نیا Ast_Stmt_اظہاریہ{flp, deletep};
    }
    ساکن Ast_گرہ_اظہاریہ* get_Process_Assoc_Array_سائز(AstVarRef* ثابت refp) {
        // Constructs refp.size() statement
        AstCMethodHard* ثابت sizep
            = نیا AstCMethodHard{refp->fileline(), refp, VCMethod::ASSOC_سائز};
        sizep->dtypep(refp->ڈھونڈو_Basic_DType(VBasicDTypeKwd::UINT32));
        واپس sizep;
    }
    خالی دورہ(AstSEventually* nodep) override {
        UASSERT(v3Global.rootp()->stdPackagep(), "Should be imported");

        Ast_Sen_درخت* ثابت sentreep = new_Sen_درخت(nodep);
        اگر (!sentreep->sensesp()) {
            VL_DO_DANGLING(ڈالو_Deletep(sentreep), sentreep);
            nodep->replaceWith(نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False{}});
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            واپس;
        }

        File_سطر* ثابت flp = nodep->fileline();

        // Track active assertions
        AstVar* ثابت activep = نیا AstVar{flp, VVarType::MODULETEMP, m_فعال_نام.get(""),
                                           getProcessAssocArrayType(flp)};
        activep->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->جوڑو_Stmtsp(activep);

        // Assertion condition check
        AstLoop* ثابت loopp = نیا AstLoop{flp};
        AstSampled* ثابت condp
            = نیا AstSampled{flp, nodep->exprp()->unlink_Fr_عقب(), خالی_اشاریہ, سچ};
        loopp->جوڑو_Stmtsp(نیا AstLoopTest{flp, loopp, نیا AstLogNot{flp, condp}});
        loopp->جوڑو_Stmtsp(نیا AstEventControl{flp, sentreep, خالی_اشاریہ});

        // Add assertion to the active set
        AstAssocSel* ثابت selp = نیا AstAssocSel{flp, نیا AstVarRef{flp, activep, VAccess::WRITE},
                                                  v3Global.rootp()->stdPackageProcessSelfp(flp)};
        Ast_تفویض* ثابت incrementp = نیا Ast_تفویض{flp, selp, نیا AstConst{flp, 1}};
        AstPExprClause* ثابت clausep = نیا AstPExprClause{flp};
        Ast_Stmt_اظہاریہ* ثابت deletep
            = getProcessAssocArrayDelete(نیا AstVarRef{flp, activep, VAccess::WRITE});

        // Main assertion block
        AstBegin* ثابت bodyp = نیا AstBegin{flp, m_بلاک_نام.get(""), خالی_اشاریہ, سچ};
        bodyp->جوڑو_Stmtsp(incrementp);
        bodyp->جوڑو_Stmtsp(loopp);
        bodyp->جوڑو_Stmtsp(clausep);
        bodyp->جوڑو_Stmtsp(deletep);

        // Validate assertion condition for each active assert
        AstVar* ثابت فعال_Countp = نیا AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                nodep->ڈھونڈو_Basic_DType(VBasicDTypeKwd::UINT32)};
        فعال_Countp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        Ast_تفویض* ثابت init_فعال_Countp
            = نیا Ast_تفویض{flp, نیا AstVarRef{flp, فعال_Countp, VAccess::WRITE},
                            get_Process_Assoc_Array_سائز(نیا AstVarRef{flp, activep, VAccess::READ})};
        AstLoop* ثابت finalLoopp = نیا AstLoop{flp};
        AstIf* ثابت finalBodypCondp
            = نیا AstIf{flp, condp->clone_درخت_خالص(جھوٹ), نیا AstPExprClause{flp},
                        نیا AstPExprClause{flp, جھوٹ}};
        finalLoopp->جوڑو_Stmtsp(
            نیا AstLoopTest{flp, finalLoopp,
                            نیا AstNeq{flp, نیا AstVarRef{flp, فعال_Countp, VAccess::READ},
                                       نیا AstConst{flp, 0}}});
        finalLoopp->جوڑو_Stmtsp(finalBodypCondp);
        finalLoopp->جوڑو_Stmtsp(
            نیا Ast_تفویض{flp, نیا AstVarRef{flp, فعال_Countp, VAccess::WRITE},
                          نیا AstSub{flp, نیا AstVarRef{flp, فعال_Countp, VAccess::READ},
                                     نیا AstConst{flp, 1}}});

        // Final assertion block
        AstBegin* ثابت finalp = نیا AstBegin{flp, m_بلاک_نام.get(""), خالی_اشاریہ, سچ};
        finalp->جوڑو_Stmtsp(فعال_Countp);
        finalp->جوڑو_Stmtsp(init_فعال_Countp);
        finalp->جوڑو_Stmtsp(finalLoopp);

        VL_RESTORER(m_pexprp);
        m_pexprp = نیا AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
        VL_RESTORER(m_has_چکر_Delay);
        m_has_چکر_Delay = جھوٹ;
        iterate(bodyp);
        iterate(finalp);

        اگر (m_has_چکر_Delay) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in s_eventually");
            nodep->replaceWith(نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False{}});
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            VL_DO_DANGLING(m_pexprp->delete_درخت(), m_pexprp);
            واپس;
        }

        nodep->replaceWith(m_pexprp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    خالی دورہ(AstStable* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گرہ_اظہاریہ* exprp = nodep->exprp()->unlink_Fr_عقب();
        Ast_Sen_درخت* sentreep = nodep->sentreep();
        اگر (sentreep) sentreep->unlink_Fr_عقب();
        AstPast* ثابت pastp = نیا AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_درخت(nodep, sentreep));
        exprp = نیا AstEq{fl, pastp, exprp->clone_درخت_خالص(جھوٹ)};
        exprp->dtype_سیٹ_ثنائی_ہندسہ();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    خالی دورہ(AstSteady* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گرہ_اظہاریہ* exprp = nodep->exprp()->unlink_Fr_عقب();
        اگر (exprp->چوڑائی() > 1) exprp = نیا AstSel{fl, exprp, 0, 1};
        Ast_گرہ_اظہاریہ* ثابت futurep = نیا AstFuture{fl, exprp, new_Sen_درخت(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = نیا AstEq{fl, exprp->clone_درخت_خالص(جھوٹ), futurep};
        exprp->dtype_سیٹ_ثنائی_ہندسہ();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    خالی دورہ(AstSampled* nodep) override {
        iterateChildren(nodep);
        اگر (!nodep->اندرونی_3()) checkSamplingFuncDType(nodep, nodep->exprp());
    }

    // Validate repetition count: must be a non-negative elaboration-time constant.
    // Shared by goto [->N] and nonconsecutive [=N] repetition.
    // On error, replaces nodep with BitFalse placeholder and returns nullptr.
    ثابت AstConst* validate_Rep_گنتی(Ast_گرہ* nodep, Ast_گرہ_اظہاریہ*& countp) {
        countp = V3Const::constifyEdit(countp);
        ثابت AstConst* ثابت constp = VN_CAST(countp, Const);
        اگر (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ڈالو_Deletep(countp), countp);
            nodep->replaceWith(نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False_Erroring{}});
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            واپس خالی_اشاریہ;
        }
        اگر (constp->مقصد_SInt() < 0) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ڈالو_Deletep(countp), countp);
            nodep->replaceWith(نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False_Erroring{}});
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            واپس خالی_اشاریہ;
        }
        اگر (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ڈالو_Deletep(countp), countp);
            nodep->replaceWith(نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False{}});
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            واپس خالی_اشاریہ;
        }
        واپس constp;
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
                             Ast_گرہ_اظہاریہ* rhsp, منطقی isOverlapped, منطقی isNonConsec) {
        Ast_Sen_شے* ثابت sensesp = m_senip;
        UASSERT_OBJ(sensesp, exprp, "Repetition requires a clock");

        ثابت std::string name
            = isNonConsec ? m_non_Cons_Rep_نام.get(exprp) : m_goto_Rep_نام.get(exprp);
        AstVar* ثابت cntVarp = نیا AstVar{flp, VVarType::BLOCKTEMP, name + "__counter",
                                           exprp->ڈھونڈو_Basic_DType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstBegin* ثابت beginp = نیا AstBegin{flp, name + "__block", cntVarp, سچ};
        beginp->جوڑو_Stmtsp(
            نیا Ast_تفویض{flp, نیا AstVarRef{flp, cntVarp, VAccess::WRITE}, نیا AstConst{flp, 0}});

        // Counting loop: find N occurrences of expr (shared by goto and nonconsec)
        AstLoop* ثابت loopp = نیا AstLoop{flp};
        loopp->جوڑو_Stmtsp(نیا AstLoopTest{flp, loopp,
                                         نیا AstLt{flp, نیا AstVarRef{flp, cntVarp, VAccess::READ},
                                                   countp->clone_درخت_خالص(جھوٹ)}});
        // if (expr) cnt++ -- sampled is applied to whole property expr by V3Assert
        loopp->جوڑو_Stmtsp(
            نیا AstIf{flp, exprp,
                      نیا Ast_تفویض{flp, نیا AstVarRef{flp, cntVarp, VAccess::WRITE},
                                    نیا Ast_جوڑو{flp, نیا AstVarRef{flp, cntVarp, VAccess::READ},
                                               نیا AstConst{flp, 1}}}});
        loopp->جوڑو_Stmtsp(نیا AstIf{
            flp, نیا AstLt{flp, نیا AstVarRef{flp, cntVarp, VAccess::READ}, countp},
            نیا AstEventControl{flp, نیا Ast_Sen_درخت{flp, sensesp->clone_درخت(جھوٹ)}, خالی_اشاریہ}});

        beginp->جوڑو_Stmtsp(loopp);

        اگر (isNonConsec && rhsp) {
            // IEEE 16.9.2: b[=N] = b[->N] ##1 !b[*0:$]
            // Trailing window: ##1 advance, then scan !expr cycles checking rhs.
            // Window closes when expr becomes true again (fail if rhs was never true).
            beginp->جوڑو_Stmtsp(نیا AstEventControl{
                flp, نیا Ast_Sen_درخت{flp, sensesp->clone_درخت(جھوٹ)}, خالی_اشاریہ});  // ##1
            اگر (!isOverlapped) {
                beginp->جوڑو_Stmtsp(نیا AstEventControl{
                    flp, نیا Ast_Sen_درخت{flp, sensesp->clone_درخت(جھوٹ)}, خالی_اشاریہ});  // |=>
            }
            // Window loop: check rhs at each !expr cycle (done variable for termination)
            AstVar* ثابت مکمل_Varp
                = نیا AstVar{flp, VVarType::BLOCKTEMP, name + "__done", exprp->ڈھونڈو_ثنائی_ہندسہ_DType()};
            مکمل_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->جوڑو_Stmtsp(مکمل_Varp);
            beginp->جوڑو_Stmtsp(نیا Ast_تفویض{flp, نیا AstVarRef{flp, مکمل_Varp, VAccess::WRITE},
                                            نیا AstConst{flp, AstConst::ثنائی_ہندسہ_False{}}});
            خودکار سیٹ_مکمل = [&]() {
                واپس نیا Ast_تفویض{flp, نیا AstVarRef{flp, مکمل_Varp, VAccess::WRITE},
                                     نیا AstConst{flp, AstConst::ثنائی_ہندسہ_True{}}};
            };
            AstLoop* ثابت windowp = نیا AstLoop{flp};
            // LoopTest: continue while !done
            windowp->جوڑو_Stmtsp(نیا AstLoopTest{
                flp, windowp, نیا AstNot{flp, نیا AstVarRef{flp, مکمل_Varp, VAccess::READ}}});
            // if (expr) { fail; done = 1; } -- window closed, expr true again
            Ast_گرہ* ثابت failsp = نیا AstPExprClause{flp, جھوٹ};
            failsp->جوڑو_اگلا(سیٹ_مکمل());
            windowp->جوڑو_Stmtsp(نیا AstIf{flp, exprp->clone_درخت_خالص(جھوٹ), failsp});
            // if (rhs) { pass; done = 1; } -- consequent true at this !expr endpoint
            Ast_گرہ* ثابت passsp = نیا AstPExprClause{flp, سچ};
            passsp->جوڑو_اگلا(سیٹ_مکمل());
            windowp->جوڑو_Stmtsp(نیا AstIf{flp, rhsp, passsp});
            // @(clk) -- advance to next cycle in window
            windowp->جوڑو_Stmtsp(
                نیا AstEventControl{flp, نیا Ast_Sen_درخت{flp, sensesp->clone_درخت(جھوٹ)}, خالی_اشاریہ});
            beginp->جوڑو_Stmtsp(windowp);
        } ورنہ اگر (isNonConsec) {
            // Standalone nonconsec: ##1 into window, then pass
            beginp->جوڑو_Stmtsp(
                نیا AstEventControl{flp, نیا Ast_Sen_درخت{flp, sensesp->clone_درخت(جھوٹ)}, خالی_اشاریہ});
            beginp->جوڑو_Stmtsp(نیا AstPExprClause{flp, سچ});
        } ورنہ اگر (rhsp) {
            // Goto rep: check consequent once at match endpoint
            اگر (!isOverlapped) {
                beginp->جوڑو_Stmtsp(نیا AstEventControl{
                    flp, نیا Ast_Sen_درخت{flp, sensesp->clone_درخت(جھوٹ)}, خالی_اشاریہ});
            }
            beginp->جوڑو_Stmtsp(نیا AstIf{flp, rhsp, نیا AstPExprClause{flp, سچ},
                                        نیا AstPExprClause{flp, جھوٹ}});
        } ورنہ {
            // Standalone goto: pass after counting
            beginp->جوڑو_Stmtsp(نیا AstPExprClause{flp, سچ});
        }

        واپس نیا AstPExpr{flp, beginp, exprp->ڈھونڈو_ثنائی_ہندسہ_DType()};
    }

    // Reject [=M:N] nonconsecutive range that reached V3AssertPre.
    // [->M:N] is fully handled upstream (NFA path); only [=M:N] still lands
    // here because AstSNonConsRep lowering is not yet implemented there.
    // Replaces the node with BitFalse so lowering continues after the warning.
    منطقی rejectNonconsecRange(Ast_گرہ* nodep, Ast_گرہ_اظہاریہ* maxCountp) {
        اگر (!maxCountp) واپس جھوٹ;
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: [=M:N] nonconsecutive range repetition"
                                     " (IEEE 1800-2023 16.9.2)");
        nodep->replaceWith(نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False{}});
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        واپس سچ;
    }

    خالی دورہ(AstSGotoRep* nodep) override {
        // Standalone goto rep (not inside implication antecedent)
        iterateChildren(nodep);
        File_سطر* ثابت flp = nodep->fileline();
        Ast_گرہ_اظہاریہ* countp = nodep->countp()->unlink_Fr_عقب();
        اگر (!validate_Rep_گنتی(nodep, countp)) واپس;
        Ast_گرہ_اظہاریہ* ثابت exprp = nodep->exprp()->unlink_Fr_عقب();
        AstPExpr* ثابت pexprp = createRepPExpr(flp, exprp, countp, خالی_اشاریہ, سچ, جھوٹ);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }

    خالی دورہ(AstSNonConsRep* nodep) override {
        // Standalone nonconsecutive rep (not inside implication antecedent)
        iterateChildren(nodep);
        اگر (rejectNonconsecRange(nodep, nodep->maxCountp())) واپس;
        File_سطر* ثابت flp = nodep->fileline();
        Ast_گرہ_اظہاریہ* countp = nodep->countp()->unlink_Fr_عقب();
        اگر (!validate_Rep_گنتی(nodep, countp)) واپس;
        Ast_گرہ_اظہاریہ* ثابت exprp = nodep->exprp()->unlink_Fr_عقب();
        AstPExpr* ثابت pexprp = createRepPExpr(flp, exprp, countp, خالی_اشاریہ, سچ, سچ);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }

    خالی دورہ(AstFuncRef* nodep) override {
        // IEEE 1800-2023 16.8: Inline sequence instances wherever they appear
        // in the expression tree (inside implications, boolean ops, nested refs, etc.)
        اگر (AstSequence* ثابت seqp = VN_CAST(nodep->taskp(), Sequence)) {
            substituteSequenceCall(nodep, seqp);
            // The FuncRef has been replaced; do not access nodep after this point.
            // The replacement node will be visited by the parent's iterateChildren.
            واپس;
        }
        اگر (ثابت AstProperty* ثابت propp = VN_CAST(nodep->taskp(), Property)) {
            substituteNestedPropertyCall(nodep, propp);
            واپس;
        }
        iterateChildren(nodep);
    }
    خالی دورہ(AstImplication* nodep) override {
        اگر (nodep->sentreep()) واپس;  // Already processed

        // Top-level followed-by is claimed by V3AssertNfa; anything reaching here
        // is nested inside iff/implies/or. IEEE 1800-2023 16.12.9 permits the
        // composition, but silent lowering would drop non-vacuous-fail semantics.
        اگر (nodep->isFollowedBy()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: followed-by (#-# / #=#) nested inside property operator"
                          " (iff/implies/or) (IEEE 1800-2023 16.12.9)");
            nodep->replaceWith(نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False{}});
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            واپس;
        }

        // Handle goto repetition as antecedent before iterateChildren,
        // so the standalone AstSGotoRep visitor doesn't process it
        اگر (AstSGotoRep* ثابت gotop = VN_CAST(nodep->lhsp(), SGotoRep)) {
            iterateChildren(gotop);
            iterate_And_اگلا_Null(nodep->rhsp());
            File_سطر* ثابت flp = nodep->fileline();
            Ast_گرہ_اظہاریہ* countp = gotop->countp()->unlink_Fr_عقب();
            اگر (!validate_Rep_گنتی(nodep, countp)) واپس;
            Ast_گرہ_اظہاریہ* ثابت exprp = gotop->exprp()->unlink_Fr_عقب();
            Ast_گرہ_اظہاریہ* ثابت rhsp = nodep->rhsp()->unlink_Fr_عقب();
            AstPExpr* ثابت pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), جھوٹ);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            واپس;
        }

        // Handle nonconsecutive repetition as antecedent before iterateChildren,
        // so the standalone AstSNonConsRep visitor doesn't process it
        اگر (AstSNonConsRep* ثابت ncrp = VN_CAST(nodep->lhsp(), SNonConsRep)) {
            iterateChildren(ncrp);
            iterate_And_اگلا_Null(nodep->rhsp());
            اگر (rejectNonconsecRange(nodep, ncrp->maxCountp())) واپس;
            File_سطر* ثابت flp = nodep->fileline();
            Ast_گرہ_اظہاریہ* countp = ncrp->countp()->unlink_Fr_عقب();
            اگر (!validate_Rep_گنتی(nodep, countp)) واپس;
            Ast_گرہ_اظہاریہ* ثابت exprp = ncrp->exprp()->unlink_Fr_عقب();
            Ast_گرہ_اظہاریہ* ثابت rhsp = nodep->rhsp()->unlink_Fr_عقب();
            AstPExpr* ثابت pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), سچ);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            واپس;
        }

        iterateChildren(nodep);

        File_سطر* ثابت flp = nodep->fileline();
        Ast_گرہ_اظہاریہ* ثابت rhsp = nodep->rhsp()->unlink_Fr_عقب();
        Ast_گرہ_اظہاریہ* lhsp = nodep->lhsp()->unlink_Fr_عقب();

        // Lower sequence match items (IEEE 16.11): (expr, var = val, ...) |-> / |=>
        اگر (Ast_اظہاریہ_Stmt* ثابت اظہاریہ_Stmtp = VN_CAST(lhsp, اظہاریہ_Stmt)) {
            Ast_گرہ_اظہاریہ* ثابت antExprp = اظہاریہ_Stmtp->resultp()->unlink_Fr_عقب();

            اگر (nodep->isOverlapped()) {
                // |-> : assign to __Vpropvar via always_comb (continuous).
                // The assign evaluates RHS once; V3Sampled snapshots the
                // result so all consequent refs read the same value.
                Ast_گرہ* مطابقت_Assignsp = خالی_اشاریہ;
                برائے (Ast_گرہ* stmtp = اظہاریہ_Stmtp->stmtsp(); stmtp;) {
                    Ast_گرہ* ثابت nextp = stmtp->nextp();
                    اگر (Ast_تفویض* ثابت assignp = VN_CAST(stmtp, تفویض)) {
                        assignp->unlink_Fr_عقب();
                        مطابقت_Assignsp = Ast_گرہ::جوڑو_اگلا_Null(مطابقت_Assignsp, assignp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(ڈالو_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                اگر (مطابقت_Assignsp) {
                    AstAlways* ثابت alwaysp
                        = نیا AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, خالی_اشاریہ, مطابقت_Assignsp};
                    m_modp->جوڑو_Stmtsp(alwaysp);
                }
            } ورنہ {
                // |=> : assign to __Vpropvar via NBA in a clocked always block.
                // The NBA commits before the next cycle's sampled snapshot,
                // so the consequent (which already references __Vpropvar)
                // sees the captured value.
                Ast_گرہ* مطابقت_Assignsp = خالی_اشاریہ;
                برائے (Ast_گرہ* stmtp = اظہاریہ_Stmtp->stmtsp(); stmtp;) {
                    Ast_گرہ* ثابت nextp = stmtp->nextp();
                    اگر (Ast_تفویض* ثابت assignp = VN_CAST(stmtp, تفویض)) {
                        assignp->unlink_Fr_عقب();
                        Ast_گرہ_اظہاریہ* ثابت تفویض_Lhsp = assignp->lhsp()->unlink_Fr_عقب();
                        Ast_گرہ_اظہاریہ* ثابت تفویض_Rhsp = assignp->rhsp()->unlink_Fr_عقب();
                        Ast_تفویض_Dly* ثابت dlyp = نیا Ast_تفویض_Dly{flp, تفویض_Lhsp, تفویض_Rhsp};
                        VL_DO_DANGLING(ڈالو_Deletep(assignp), assignp);
                        مطابقت_Assignsp = Ast_گرہ::جوڑو_اگلا_Null(مطابقت_Assignsp, dlyp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(ڈالو_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                اگر (مطابقت_Assignsp) {
                    AstIf* ثابت condp
                        = نیا AstIf{flp, antExprp->clone_درخت_خالص(جھوٹ), مطابقت_Assignsp};
                    AstAlways* ثابت alwaysp
                        = نیا AstAlways{flp, VAlwaysKwd::ALWAYS, new_Sen_درخت(nodep), condp};
                    m_modp->جوڑو_Stmtsp(alwaysp);
                }
            }
        }

        اگر (AstPExpr* ثابت pexprp = VN_CAST(rhsp, PExpr)) {
            // Implication with sequence expression on RHS (IEEE 1800-2023 16.11, 16.12.7).
            // The PExpr was lowered from the property expression earlier in this pass.
            // Wrap the PExpr body with the antecedent check so the sequence only
            // starts when the antecedent holds.
            Ast_گرہ_اظہاریہ* condp;
            اگر (nodep->isOverlapped()) {
                // Overlapped implication (|->): check antecedent on same cycle.
                // disable iff is applied at the assertion level, not at the
                // antecedent gate, matching the existing non-PExpr overlapped path.
                condp = lhsp;
            } ورنہ {
                // Non-overlapped implication (|=>): check antecedent from previous cycle
                اگر (m_disablep) {
                    lhsp
                        = نیا AstAnd{flp, نیا AstNot{flp, m_disablep->clone_درخت_خالص(جھوٹ)}, lhsp};
                }
                AstPast* ثابت pastp
                    = نیا AstPast{flp, lhsp, خالی_اشاریہ, خالی_اشاریہ, /* propertyTiming */ سچ};
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
            AstIf* ثابت guardp = نیا AstIf{flp, condp};
            برائے (Ast_گرہ* stmtp = bodyp->stmtsp(); stmtp;) {
                Ast_گرہ* ثابت nextp = stmtp->nextp();
                اگر (!VN_IS(stmtp, Var)) guardp->جوڑو_Thensp(stmtp->unlink_Fr_عقب());
                stmtp = nextp;
            }
            bodyp->جوڑو_Stmtsp(guardp);
            nodep->replaceWith(pexprp);
            // Don't iterate pexprp here -- it was already iterated when created
            // (in visit(AstSExpr*)), so delays and disable iff are already processed.
        } ورنہ اگر (nodep->isOverlapped()) {
            Ast_گرہ_اظہاریہ* ثابت exprp
                = m_underCover ? static_cast<Ast_گرہ_اظہاریہ*>(نیا AstLogAnd{flp, lhsp, rhsp})
                               : نیا AstLogOr{flp, نیا AstLogNot{flp, lhsp}, rhsp};
            nodep->replaceWith(exprp);
        } ورنہ {
            اگر (m_disablep) {
                lhsp = نیا AstAnd{flp, نیا AstNot{flp, m_disablep->clone_درخت_خالص(جھوٹ)}, lhsp};
            }

            AstPast* ثابت pastp
                = نیا AstPast{flp, lhsp, خالی_اشاریہ, خالی_اشاریہ, /* propertyTiming */ سچ};
            pastp->dtypeFrom(lhsp);
            pastp->sentreep(new_Sen_درخت(nodep));
            Ast_گرہ_اظہاریہ* ثابت exprp
                = m_underCover ? static_cast<Ast_گرہ_اظہاریہ*>(نیا AstAnd{flp, pastp, rhsp})
                               : نیا AstOr{flp, نیا AstNot{flp, pastp}, rhsp};
            exprp->dtype_سیٹ_ثنائی_ہندسہ();
            nodep->replaceWith(exprp);
        }
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    خالی دورہ(AstUntil* nodep) override {
        File_سطر* ثابت flp = nodep->fileline();
        UASSERT_OBJ(
            !m_pexprp, nodep,
            "'" << nodep->verilogKwd()
                << "' in complex property expression should have been rejected by V3AssertNfa");
        اگر (nodep->isStrong()
            && (v3Global.opt.timing().is_سیٹ_False() || !v3Global.opt.timing().is_سیٹ_True())) {
            nodep->v3warn(E_NOTIMING, nodep->verilogKwd() << " requires --timing");
            nodep->replaceWith(نیا AstConst{flp, AstConst::ثنائی_ہندسہ_False{}});
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            واپس;
        }
        اگر (nodep->isStrong()) {
            // p s_until q / p s_until_with q: q must eventually be true. Until then, p must
            // be true on every sampled tick. For s_until, check q first: when q is true on
            // this tick, p is not required. For s_until_with, p must be true on the q tick too.
            Ast_گرہ_اظہاریہ* ثابت rawLhsp = nodep->lhsp()->unlink_Fr_عقب();
            Ast_گرہ_اظہاریہ* ثابت rawRhsp = nodep->rhsp()->unlink_Fr_عقب();
            AstSampled* ثابت lhsp = نیا AstSampled{flp, rawLhsp, rawLhsp->dtypep(), سچ};
            AstSampled* ثابت rhsp = نیا AstSampled{flp, rawRhsp, rawRhsp->dtypep(), سچ};
            Ast_گرہ_اظہاریہ* finalCondp = rhsp->clone_درخت_خالص(جھوٹ);
            اگر (nodep->isOverlapping()) {
                finalCondp = نیا AstLogAnd{flp, lhsp->clone_درخت_خالص(جھوٹ), finalCondp};
            }

            // Track active assertion attempts. Only the count is needed to emit final failures.
            AstVar* ثابت activep = نیا AstVar{flp, VVarType::MODULETEMP, m_فعال_نام.get(""),
                                               nodep->ڈھونڈو_Basic_DType(VBasicDTypeKwd::UINT32)};
            activep->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->جوڑو_Stmtsp(activep);

            AstVar* ثابت donep
                = نیا AstVar{flp, VVarType::BLOCKTEMP, "__VassertDone", nodep->ڈھونڈو_ثنائی_ہندسہ_DType()};
            donep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            خودکار سیٹ_مکمل = [&]() {
                واپس نیا Ast_تفویض{flp, نیا AstVarRef{flp, donep, VAccess::WRITE},
                                     نیا AstConst{flp, AstConst::ثنائی_ہندسہ_True{}}};
            };

            AstLoop* ثابت loopp = نیا AstLoop{flp};
            Ast_تفویض* ثابت decrementVar = نیا Ast_تفویض{
                flp, نیا AstVarRef{flp, activep, VAccess::WRITE},
                نیا AstSub{flp, نیا AstVarRef{flp, activep, VAccess::READ}, نیا AstConst{flp, 1}}};
            loopp->جوڑو_Stmtsp(نیا AstLoopTest{
                flp, loopp, نیا AstLogNot{flp, نیا AstVarRef{flp, donep, VAccess::READ}}});
            {
                Ast_گرہ* ثابت passp = نیا AstPExprClause{flp};
                passp->جوڑو_اگلا(decrementVar);
                passp->جوڑو_اگلا(سیٹ_مکمل());
                Ast_گرہ_اظہاریہ* passCondp = rhsp;
                اگر (nodep->isOverlapping()) {
                    passCondp = نیا AstLogAnd{flp, lhsp->clone_درخت_خالص(جھوٹ), passCondp};
                }
                loopp->جوڑو_Stmtsp(نیا AstIf{flp, passCondp, passp});
            }
            {
                Ast_گرہ* ثابت failp = نیا AstPExprClause{flp, جھوٹ};
                failp->جوڑو_اگلا(decrementVar->clone_درخت(جھوٹ));
                failp->جوڑو_اگلا(سیٹ_مکمل());
                loopp->جوڑو_Stmtsp(نیا AstIf{
                    flp,
                    نیا AstLogAnd{flp,
                                  نیا AstLogNot{flp, نیا AstVarRef{flp, donep, VAccess::READ}},
                                  نیا AstLogNot{flp, lhsp}},
                    failp});
            }
            AstDelay* ثابت delayp = نیا AstDelay{flp, نیا AstConst{flp, 1}, جھوٹ};
            delayp->timeunit(m_modp->timeunit());
            loopp->جوڑو_Stmtsp(نیا AstIf{
                flp, نیا AstLogNot{flp, نیا AstVarRef{flp, donep, VAccess::READ}}, delayp});

            // Main assertion block
            AstBegin* ثابت bodyp = نیا AstBegin{flp, m_بلاک_نام.get(""), خالی_اشاریہ, سچ};
            bodyp->جوڑو_Stmtsp(donep);
            bodyp->جوڑو_Stmtsp(نیا Ast_تفویض{flp, نیا AstVarRef{flp, donep, VAccess::WRITE},
                                           نیا AstConst{flp, AstConst::ثنائی_ہندسہ_False{}}});
            File_سطر* ثابت flp = activep->fileline();
            bodyp->جوڑو_Stmtsp(
                نیا Ast_تفویض{flp, نیا AstVarRef{flp, activep, VAccess::WRITE},
                              نیا Ast_جوڑو{flp, نیا AstVarRef{flp, activep, VAccess::READ},
                                         نیا AstConst{flp, 1}}});
            bodyp->جوڑو_Stmtsp(loopp);

            // Validate assertion condition for each active assert
            AstVar* ثابت فعال_Countp = نیا AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                    nodep->ڈھونڈو_Basic_DType(VBasicDTypeKwd::UINT32)};
            فعال_Countp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

            Ast_تفویض* ثابت init_فعال_Countp
                = نیا Ast_تفویض{flp, نیا AstVarRef{flp, فعال_Countp, VAccess::WRITE},
                                نیا AstVarRef{flp, activep, VAccess::READ}};
            AstLoop* ثابت finalLoopp = نیا AstLoop{flp};
            finalLoopp->جوڑو_Stmtsp(
                نیا AstLoopTest{flp, finalLoopp,
                                نیا AstNeq{flp, نیا AstVarRef{flp, فعال_Countp, VAccess::READ},
                                           نیا AstConst{flp, 0}}});
            finalLoopp->جوڑو_Stmtsp(نیا AstIf{flp, finalCondp, نیا AstPExprClause{flp},
                                            نیا AstPExprClause{flp, جھوٹ}});
            finalLoopp->جوڑو_Stmtsp(
                نیا Ast_تفویض{flp, نیا AstVarRef{flp, فعال_Countp, VAccess::WRITE},
                              نیا AstSub{flp, نیا AstVarRef{flp, فعال_Countp, VAccess::READ},
                                         نیا AstConst{flp, 1}}});

            // Final assertion block
            AstBegin* ثابت finalp = نیا AstBegin{flp, m_بلاک_نام.get(""), خالی_اشاریہ, سچ};
            finalp->جوڑو_Stmtsp(فعال_Countp);
            finalp->جوڑو_Stmtsp(init_فعال_Countp);
            finalp->جوڑو_Stmtsp(finalLoopp);

            AstPExpr* ثابت pexprp = نیا AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
            VL_RESTORER(m_pexprp);
            VL_RESTORER(m_has_چکر_Delay);
            m_pexprp = pexprp;
            m_has_چکر_Delay = جھوٹ;
            iterate(bodyp);
            iterate(finalp);

            UASSERT_OBJ(!m_has_چکر_Delay, nodep,
                        "s_until cycle delay should have been handled by V3AssertNfa");

            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            واپس;
        }
        AstLoop* ثابت loopp = نیا AstLoop{flp};
        Ast_گرہ_اظہاریہ* ثابت rhsp = nodep->rhsp()->unlink_Fr_عقب();
        Ast_گرہ_اظہاریہ* ثابت lhsp = nodep->lhsp()->unlink_Fr_عقب();
        AstLogAnd* ثابت loopCondp = نیا AstLogAnd{flp, lhsp, نیا AstLogNot{flp, rhsp}};
        loopp->جوڑو_Stmtsp(نیا AstLoopTest{flp, loopp, loopCondp});
        loopp->جوڑو_Stmtsp(نیا AstEventControl{flp, new_Sen_درخت(nodep), خالی_اشاریہ});

        Ast_گرہ_اظہاریہ* ثابت rhsCopyp = rhsp->clone_درخت_خالص(جھوٹ);
        Ast_گرہ_اظہاریہ* ثابت passCondp
            = nodep->isOverlapping() ? نیا AstLogAnd{flp, lhsp->clone_درخت_خالص(جھوٹ), rhsCopyp}
                                     : rhsCopyp;
        AstBegin* ثابت beginp = نیا AstBegin{flp, m_بلاک_نام.get(""), loopp, سچ};
        beginp->جوڑو_Stmtsp(
            نیا AstIf{flp, passCondp, نیا AstPExprClause{flp}, نیا AstPExprClause{flp, جھوٹ}});

        AstPExpr* ثابت pexprp = نیا AstPExpr{flp, beginp, nodep->dtypep()};
        pexprp->user1(1);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }

    خالی دورہ(AstDefaultDisable* nodep) override {
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
    }
    خالی دورہ(AstInferredDisable* nodep) override {
        Ast_گرہ* newp;
        اگر (m_defaultDisablep) {
            newp = m_defaultDisablep->condp()->clone_درخت_خالص(سچ);
        } ورنہ {
            newp = نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False{}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }

    خالی دورہ(AstPropSpec* nodep) override {
        nodep = substitutePropertyCall(nodep);
        iterate_And_اگلا_Null(nodep->sensesp());
        اگر (m_senip && m_senip != nodep->sensesp())
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Only one PSL clock allowed per assertion");
        ثابت AstCover* ثابت coverp = VN_CAST(nodep->backp(), Cover);
        ثابت منطقی seqEvent = coverp && coverp->isSeqEvent();
        اگر (!nodep->disablep() && m_defaultDisablep && !seqEvent) {
            nodep->disablep(m_defaultDisablep->condp()->clone_درخت_خالص(سچ));
        }
        m_disablep = nodep->disablep();
        // Unlink and just keep a pointer to it, convert to sentree as needed
        m_senip = nodep->sensesp();
        iterateNull(nodep->disablep());
        اگر (!VN_AS(nodep->backp(), گرہ_Cover_Or_Assert)->فوری()) {
            ثابت Ast_گرہ_DType* ثابت propDtp = nodep->propp()->dtypep();
            nodep->propp(نیا AstSampled{nodep->fileline(), nodep->propp()->unlink_Fr_عقب(),
                                        propDtp->dtypep(), سچ});
        }
        // cover counts non-vacuous matches only (IEEE 1800-2023 16.15.2), so an
        // implication antecedent must hold; assert passes vacuously instead.
        VL_RESTORER(m_underCover);
        m_underCover = VN_IS(nodep->backp(), Cover);
        iterate(nodep->propp());
        iterateNull(nodep->مطابقت_Countp());
    }
    خالی دورہ(AstPExpr* nodep) override {
        // V3AssertNfa handles multi-cycle property expressions before this pass,
        // so the following unsupported paths are defensive and typically unreached.
        اگر (m_pexprp && m_pexprp->user1()) {  // LCOV_EXCL_START
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Complex property expression inside 'until''");
            nodep->replaceWith(نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False{}});
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            واپس;
        }  // LCOV_EXCL_STOP
        اگر (AstLogNot* ثابت notp = VN_CAST(nodep->backp(), LogNot)) {
            notp->replaceWith(nodep->unlink_Fr_عقب());
            VL_DO_DANGLING(ڈالو_Deletep(notp), notp);
            iterate(nodep);
            واپس;
        }
        // Sequence expression as antecedent of implication is not yet supported
        اگر (AstImplication* ثابت implp = VN_CAST(nodep->backp(), Implication)) {
            اگر (implp->lhsp() == nodep) {  // LCOV_EXCL_START
                implp->v3warn(E_UNSUPPORTED,
                              "Unsupported: Implication with sequence expression as antecedent");
                nodep->replaceWith(نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False{}});
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                واپس;
            }  // LCOV_EXCL_STOP
        }
        VL_RESTORER(m_pexprp);
        VL_RESTORER(m_disableSeqIfp);
        m_pexprp = nodep;

        اگر (m_disablep) {
            ثابت AstSampled* sampledp = خالی_اشاریہ;
            اگر (m_disablep->exists([&sampledp](ثابت AstSampled* ثابت sp) {
                    sampledp = sp;
                    واپس سچ;
                })) {
                sampledp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: $sampled inside disabled condition of a sequence");
                m_disablep = نیا AstConst{m_disablep->fileline(), AstConst::ثنائی_ہندسہ_False{}};
                // always a copy is used, so remove it now
                ڈالو_Deletep(m_disablep);  // m_disablep used later
            }
            File_سطر* ثابت flp = nodep->fileline();
            // Add counter which counts times the condition turned true
            AstVar* ثابت disableCntp
                = نیا AstVar{flp, VVarType::MODULETEMP, m_disable_Cnt_نام.get(""),
                             nodep->ڈھونڈو_Basic_DType(VBasicDTypeKwd::UINT32)};
            disableCntp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->جوڑو_Stmtsp(disableCntp);
            AstVarRef* ثابت readCntRefp = نیا AstVarRef{flp, disableCntp, VAccess::READ};
            AstVarRef* ثابت writeCntRefp = نیا AstVarRef{flp, disableCntp, VAccess::WRITE};
            Ast_تفویض* ثابت incrStmtp = نیا Ast_تفویض{
                flp, writeCntRefp, نیا Ast_جوڑو{flp, readCntRefp, نیا AstConst{flp, 1}}};
            AstAlways* ثابت alwaysp
                = نیا AstAlways{flp, VAlwaysKwd::ALWAYS,
                                نیا Ast_Sen_درخت{flp, نیا Ast_Sen_شے{flp, VEdgeType::ET_POSEDGE,
                                                                   m_disablep->clone_درخت(جھوٹ)}},
                                incrStmtp};
            disableCntp->جوڑو_اگلا_یہاں(alwaysp);

            // Store value of that counter at the beginning of sequence evaluation
            AstBegin* ثابت bodyp = nodep->bodyp();
            AstVar* ثابت initialCntp = نیا AstVar{flp, VVarType::BLOCKTEMP, "__VinitialCnt",
                                                   nodep->ڈھونڈو_Basic_DType(VBasicDTypeKwd::UINT32)};
            initialCntp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            Ast_تفویض* ثابت assignp
                = نیا Ast_تفویض{flp, نیا AstVarRef{flp, initialCntp, VAccess::WRITE},
                                readCntRefp->clone_درخت(جھوٹ)};
            // Prepend to the sequence body to keep statement list structure valid.
            UASSERT_OBJ(!bodyp->name().خالی_خانے(), bodyp, "Sequence body should be named");
            Ast_گرہ* ثابت newStmtsp = Ast_گرہ::جوڑو_اگلا<Ast_گرہ, Ast_گرہ>(initialCntp, assignp);
            bodyp->prependStmtsp(newStmtsp);

            m_disableSeqIfp
                = نیا AstIf{flp, نیا AstEq{flp, نیا AstVarRef{flp, initialCntp, VAccess::READ},
                                           readCntRefp->clone_درخت(جھوٹ)}};
            // Delete it, because it is always copied before insetion to the AST
            ڈالو_Deletep(m_disableSeqIfp);  // m_disableSeqIfp used later
        }
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultClkEvtVarp);
        VL_RESTORER(m_defaultDisablep);
        VL_RESTORER(m_modp);
        اگر (!VN_IS(nodep, Class)) {
            // A class nested in a module is in the scope of the module's default clocking
            m_defaultClockingp = خالی_اشاریہ;
            m_defaultClkEvtVarp = خالی_اشاریہ;
        }
        m_defaultDisablep = nodep->defaultDisablep();
        m_modp = nodep;
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_Gen_بلاک* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    خالی دورہ(AstProperty* nodep) override {
        // The body will be visited when will be substituted in place of property reference
        // (AstFuncRef)
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
    }
    خالی دورہ(AstSequence* nodep) override {
        // Sequence declarations are not visited directly; their bodies are inlined
        // at call sites by visit(AstFuncRef*). Collect for post-traversal cleanup.
        m_seqs_مقصد_Cleanup.ڈالو_عقب(nodep);
    }
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح AssertPreVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        { AssertPreClockingVisitor{nodep}; }
        // Process
        iterate(nodep);
        // Fix up varref names
        برائے (AstVarXRef* xrefp : m_xrefsp) xrefp->name(xrefp->varp()->name());
        // Clean up sequence declarations after inlining.
        // Referenced sequences that were inlined have isReferenced cleared.
        // Remaining referenced sequences are in unsupported contexts (e.g. @seq event).
        برائے (AstSequence* seqp : m_seqs_مقصد_Cleanup) {
            اگر (seqp->isReferenced()) {
                seqp->v3warn(E_UNSUPPORTED,
                             "Unsupported: sequence referenced outside assertion property");
            } ورنہ {
                VL_DO_DANGLING(seqp->unlink_Fr_عقب()->delete_درخت(), seqp);
            }
        }
    }
    ~AssertPreVisitor() override = طےشدہ;
};

//######################################################################
// Top Assert class

خالی V3AssertPre::assertPreAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { AssertPreVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("assertpre", 0, dump_درخت_Either_Level() >= 3);
}
