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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3AssertPre.h"

#അന്തര്ഭാവ "V3Assert.h"
#അന്തര്ഭാവ "V3Const.h"
#അന്തര്ഭാവ "V3Task.h"
#അന്തര്ഭാവ "V3Timing.h"
#അന്തര്ഭാവ "V3UniqueNames.h"

#അന്തര്ഭാവ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clocking blocks of clocking items

വർഗ്ഗം AssertPreClockingVisitor final : പൊതു VNVisitorConst {
    // Captures the clocking block of each clocking item before AssertPreVisitor, which needs it
    // at synchronous drives, often visited before the clocking block, e.g. in a parent module
    // NODE STATE
    // AstClockingItem::user3p()  // AstClocking*. Clocking block of the item, user3 claimed by
    //                            //               AssertPreVisitor

    // VISITORS
    ശൂന്യം visit(AstNetlist* nodep) override { iterateAndNextConstNull(nodep->modulesp()); }
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override { iterateAndNextConstNull(nodep->stmtsp()); }
    ശൂന്യം visit(AstGenBlock* nodep) override { iterateAndNextConstNull(nodep->itemsp()); }
    ശൂന്യം visit(AstClocking* nodep) override {
        വേണ്ടി (Ast_കണ്ണി* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            എങ്കിൽ (AstClockingItem* സ്ഥിരം citemp = VN_CAST(itemp, ClockingItem)) {
                citemp->user3p(nodep);
            }
        }
    }
    // Clocking blocks are only in modules and their generate blocks
    ശൂന്യം visit(Ast_കണ്ണി*) override {}

പൊതു:
    // CONSTRUCTORS
    explicit AssertPreClockingVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~AssertPreClockingVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Assert class functions

വർഗ്ഗം AssertPreVisitor final : പൊതു VNVisitor {
    // Removes clocks and other pre-optimizations
    // Eventually inlines calls to sequences, properties, etc.
    // We're not parsing the tree, or anything more complicated.
സ്വകാര്യ:
    // TYPES
    ഘടന SynchDrive final {
        AstClockingItem* itemp;  // Clocking item of the driven clockvar
        Ast_കണ്ണി_വ്യഞ്ജകം* refp;  // Reference to the driven clockvar
    };

    // NODE STATE
    // AstClockingItem::user1p()         // AstVar*.      varp() of ClockingItem after unlink
    // AstClockingItem::user2p()         // AstVar*.      Flag set by drives of output clockvar
    // AstClockingItem::user3p()         // AstClocking*. Clocking block of the item, captured
    //                                   //               by AssertPreClockingVisitor
    // AstPExpr::user1()                 // bool.         Created from AstUntil
    സ്ഥിരം VNUser1InUse m_inuser1;
    സ്ഥിരം VNUser2InUse m_inuser2;
    സ്ഥിരം VNUser3InUse m_inuser3;
    // STATE
    // Current context:
    AstNetlist* സ്ഥിരം m_netlistp = nullptr;  // Current netlist
    Ast_കണ്ണി_Module* m_modp = nullptr;  // Current module
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
    Ast_കണ്ണി_വ്യഞ്ജകം* m_disablep = nullptr;  // Last disable
    AstIf* m_disableSeqIfp = nullptr;  // Used for handling disable iff in sequences
    AstPExpr* m_pexprp = nullptr;  // Last AstPExpr
    തർക്കം m_underCover = അസത്യം;  // True if the enclosing assertion is a cover
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
    തർക്കം m_inAssign = അസത്യം;  // True if in an AssignNode
    തർക്കം m_inAssignDlyLhs = അസത്യം;  // True if in AssignDly's LHS
    തർക്കം m_inSynchDrive = അസത്യം;  // True if in synchronous drive
    തർക്കം m_inConcatAssign = അസത്യം;  // True if in assignment to a concatenation
    std::vector<SynchDrive> m_drives;  // Clockvars written by the synchronous drive
    തർക്കം m_hasCycleDelay = അസത്യം;  // True if node has cycle delay beneath
    std::vector<AstVarXRef*> m_xrefsp;  // list of xrefs that need name fixup
    std::vector<AstSequence*> m_seqsToCleanup;  // Sequences to clean up after traversal

    // METHODS

    // Flag set by each drive of an output clockvar, added to the module with its clocking item
    AstVar* getCreateDrivenVarp(AstClockingItem* itemp) {
        എങ്കിൽ (!itemp->user2p()) {
            AstVar* സ്ഥിരം varp = പുതിയ AstVar{itemp->fileline(), VVarType::MODULETEMP,
                                            m_drivenNames.get(itemp), itemp->findBitDType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            itemp->user2p(varp);
        }
        തിരികെ VN_AS(itemp->user2p(), Var);
    }
    // Reference to a variable in the scope of a driven clockvar, made like the clockvar's
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* newDriveRefp(File_വരി* flp, സ്ഥിരം SynchDrive& drive, AstVar* varp,
                                     സ്ഥിരം VAccess& access) {
        എങ്കിൽ (സ്ഥിരം AstVarXRef* സ്ഥിരം xrefp = VN_CAST(drive.refp, VarXRef)) {
            തിരികെ പുതിയ AstVarXRef{flp, varp, xrefp->dotted(), access};
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstMemberSel* സ്ഥിരം selp = VN_CAST(drive.refp, MemberSel)) {
            // The interface expression is evaluated again, so it must not have side effects;
            // cloneTreePure warns if it has any
            AstMemberSel* സ്ഥിരം newSelp
                = പുതിയ AstMemberSel{flp, selp->fromp()->clone_വൃക്ഷം_Pure(അസത്യം), varp};
            newSelp->access(access);
            തിരികെ newSelp;
        }
        തിരികെ പുതിയ AstVarRef{flp, varp, access};
    }
    // Assignment setting the drive flag of a driven clockvar
    AstAssign* newDrivenSetp(File_വരി* flp, സ്ഥിരം SynchDrive& drive) {
        AstVar* സ്ഥിരം varp = getCreateDrivenVarp(drive.itemp);
        AstAssign* സ്ഥിരം setp = പുതിയ AstAssign{flp, newDriveRefp(flp, drive, varp, VAccess::WRITE),
                                              പുതിയ AstConst{flp, AstConst::BitTrue{}}};
        setp->user1(സത്യം);
        തിരികെ setp;
    }
    // Clocking block of a clocking item
    സ്ഥിതം AstClocking* clockingOf(സ്ഥിരം AstClockingItem* itemp) {
        UASSERT_OBJ(itemp->user3p(), itemp, "Clocking item not captured in a clocking block");
        തിരികെ VN_AS(itemp->user3p(), Clocking);
    }
    // Clocking event of a driven clockvar, referenced from the drive like the clockvar, or
    // nullptr if unsupported, which is reported on the drive's cycle delay
    Ast_Sen_വൃക്ഷം* newDriveSentreep(AstDelay* delayp, സ്ഥിരം SynchDrive& drive) {
        Ast_Sen_വൃക്ഷം* സ്ഥിരം origp = clockingOf(drive.itemp)->sentreep();
        എങ്കിൽ (VN_IS(drive.refp, VarRef)) തിരികെ origp->clone_വൃക്ഷം(അസത്യം);
        എങ്കിൽ (origp->exists([](സ്ഥിരം AstVarXRef*) { തിരികെ സത്യം; })) {
            // The reference is relative to the clocking block
            delayp->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in synchronous drive to"
                                          " clockvar in another scope, whose clocking event"
                                          " has a hierarchical reference");
            തിരികെ nullptr;
        }
        സ്ഥിരം AstMemberSel* സ്ഥിരം selp = VN_CAST(drive.refp, MemberSel);
        // Processes outside classes evaluate their events also when not waiting on them,
        // possibly before the interface reference is set, so the event is guarded against a
        // null reference, which needs an integral value; scheduling does not guard it
        സ്ഥിരം തർക്കം guardNull = selp && !VN_IS(m_modp, Class);
        എങ്കിൽ (guardNull) {
            വേണ്ടി (AstSenItem* itemp = origp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                എങ്കിൽ (!itemp->sensp()->dtypep()->skipRefp()->isIntegralOrPacked()) {
                    delayp->v3warn(E_UNSUPPORTED,
                                   "Unsupported: cycle delay in synchronous drive through"
                                   " an interface reference outside a class, to clocking"
                                   " block with non-integral clocking event");
                    തിരികെ nullptr;
                }
            }
        }
        // Interface declaring the clocking block, if driven through an interface reference
        AstIface* സ്ഥിരം ifacep
            = selp ? VN_AS(selp->fromp()->dtypep()->skipRefp(), IfaceRefDType)->ifaceViaCellp()
                   : nullptr;
        Ast_Sen_വൃക്ഷം* സ്ഥിരം sentreep = origp->clone_വൃക്ഷം(അസത്യം);
        std::vector<AstVarRef*> refps;
        sentreep->foreach([&](AstVarRef* refp) {
            എങ്കിൽ (!refp->classOrPackagep()) refps.തള്ളുക_back(refp);
        });
        വേണ്ടി (AstVarRef* സ്ഥിരം refp : refps) {
            AstVar* സ്ഥിരം varp = refp->varp();
            എങ്കിൽ (ifacep) {
                // Mark the variable as read through an interface reference, as V3Width does for
                // a member select. The mark is the interface declaring the variable, whose
                // instances all share it, so drives through references to any instances agree.
                UASSERT_OBJ(!varp->sensIfacep() || varp->sensIfacep() == ifacep, refp,
                            "Variable read through references to different interfaces");
                varp->sensIfacep(ifacep);
            }
            refp->replaceWith(newDriveRefp(refp->fileline(), drive, varp, refp->access()));
            VL_DO_DANGLING(തള്ളുക_Deletep(refp), refp);
        }
        എങ്കിൽ (guardNull) {
            വേണ്ടി (AstSenItem* itemp = sentreep->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                guardNullIface(itemp, selp->fromp());
            }
        }
        തിരികെ sentreep;
    }
    // Make an integral clocking event through an interface reference false while the reference
    // is null
    സ്ഥിതം ശൂന്യം guardNullIface(AstSenItem* sensesp, Ast_കണ്ണി_വ്യഞ്ജകം* fromp) {
        File_വരി* സ്ഥിരം flp = sensesp->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം sensp = sensesp->sensp()->unlinkFrBack();
        sensesp->sensp(പുതിയ AstCond{flp, newNotNullp(flp, fromp), sensp,
                                   പുതിയ AstConst{flp, AstConst::DTyped{}, sensp->dtypep()}});
        എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp = sensesp->condp()) {
            condp->unlinkFrBack();
            sensesp->condp(പുതിയ AstLogAnd{flp, newNotNullp(flp, fromp), condp});
        }
    }
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* newNotNullp(File_വരി* flp, Ast_കണ്ണി_വ്യഞ്ജകം* fromp) {
        തിരികെ പുതിയ AstNeq{flp, fromp->clone_വൃക്ഷം_Pure(അസത്യം), പുതിയ AstConst{flp, AstConst::Null{}}};
    }
    // Procedural '##0': if the default clocking event has not yet occurred in this time step,
    // wait for it; otherwise continue without suspension (IEEE 1800-2023 14.11)
    AstIf* newCycleDelayZerop(File_വരി* flp) സ്ഥിരം {
        AstVar* സ്ഥിരം evtVarp = m_defaultClkEvtVarp;
        UASSERT_OBJ(evtVarp, flp, "Default clocking event var not pre-created");
        AstCMethodHard* സ്ഥിരം isTriggeredp = പുതിയ AstCMethodHard{
            flp, പുതിയ AstVarRef{flp, evtVarp, VAccess::READ}, VCMethod::EVENT_IS_TRIGGERED};
        isTriggeredp->dtypeSetBit();
        AstEventControl* സ്ഥിരം waitp = പുതിയ AstEventControl{
            flp,
            പുതിയ Ast_Sen_വൃക്ഷം{flp, പുതിയ AstSenItem{flp, VEdgeType::ET_EVENT,
                                               പുതിയ AstVarRef{flp, evtVarp, VAccess::READ}}},
            nullptr};
        തിരികെ പുതിയ AstIf{flp, പുതിയ AstNot{flp, isTriggeredp}, waitp};
    }

    സ്ഥിതം ശൂന്യം checkSamplingFuncDType(Ast_കണ്ണി_വ്യഞ്ജകം* nodep, സ്ഥിരം Ast_കണ്ണി* exprp) {
        സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep = exprp->dtypep()->skipRefp();
        എങ്കിൽ (!dtypep->isIntegralOrPacked()) {
            nodep->v3error("Expected numeric type, but got a " << dtypep->prettyDTypeNameQ()
                                                               << " data type");
        }
    }

    Ast_Sen_വൃക്ഷം* new_Sen_വൃക്ഷം(Ast_കണ്ണി* nodep, Ast_Sen_വൃക്ഷം* useTreep = nullptr,
                           Ast_കണ്ണി_Cover_Or_Assert* cassertp = nullptr) {
        // Create sentree based on clocked or default clock
        // Return nullptr for always
        എങ്കിൽ (useTreep) തിരികെ useTreep;
        Ast_Sen_വൃക്ഷം* newp = nullptr;
        AstSenItem* senip = m_senip;
        തർക്കം fromAlways = അസത്യം;
        എങ്കിൽ (!senip && m_defaultClockingp) senip = m_defaultClockingp->sentreep()->sensesp();
        എങ്കിൽ (!senip) {
            senip = m_seniAlwaysp;
            fromAlways = സത്യം;
        }
        എങ്കിൽ (!senip) {
            nodep->v3error("Concurrent assertion has no clock (IEEE 1800-2023 16.16)\n"
                           << nodep->warnMore()
                           << "... Suggest provide a clocking event, a default"
                              " clocking, or a clocked procedural context");
            newp = പുതിയ Ast_Sen_വൃക്ഷം{nodep->fileline(), nullptr};
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (cassertp && fromAlways) cassertp->senFromAlways(സത്യം);
            newp = പുതിയ Ast_Sen_വൃക്ഷം{nodep->fileline(), senip->clone_വൃക്ഷം(സത്യം)};
        }
        തിരികെ newp;
    }
    Ast_കണ്ണി_വ്യഞ്ജകം* getSequenceBodyExprp(സ്ഥിരം AstSequence* സ്ഥിരം seqp) സ്ഥിരം {
        // The statements in AstSequence are optional AstVar (ports) followed by body expr.
        Ast_കണ്ണി* bodyp = seqp->stmtsp();
        ആകുമ്പോൾ (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        തിരികെ VN_CAST(bodyp, കണ്ണി_വ്യഞ്ജകം);
    }
    AstPropSpec* getPropertyExprp(സ്ഥിരം AstProperty* സ്ഥിരം propp) {
        // Statements in AstProperty: AstVar (ports/local vars),
        // AstInitialStaticStmt/AstInitialAutomaticStmt (var init), AstPropSpec (body).
        Ast_കണ്ണി* propExprp = propp->stmtsp();
        ആകുമ്പോൾ (propExprp
               && (VN_IS(propExprp, Var) || VN_IS(propExprp, InitialStaticStmt)
                   || VN_IS(propExprp, InitialAutomaticStmt))) {
            propExprp = propExprp->nextp();
        }
        തിരികെ VN_CAST(propExprp, PropSpec);
    }
    ശൂന്യം substituteSequenceCall(AstFuncRef* funcrefp, AstSequence* seqp) {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Inline the sequence body at the call site, replacing the FuncRef
        Ast_കണ്ണി_വ്യഞ്ജകം* bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        // Clone the body expression since the sequence may be referenced multiple times
        Ast_കണ്ണി_വ്യഞ്ജകം* clonedp = bodyExprp->clone_വൃക്ഷം(അസത്യം);
        // Build substitution map, then do a single traversal to replace all formals
        // (textual substitution per IEEE 16.8.2).
        സ്ഥിരം V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<സ്ഥിരം AstVar*, Ast_കണ്ണി_വ്യഞ്ജകം*> portMap;
        വേണ്ടി (സ്ഥിരം auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            സ്ഥിരം auto it = portMap.find(refp->varp());
            എങ്കിൽ (it != portMap.end()) {
                refp->replaceWith(it->second->clone_വൃക്ഷം(അസത്യം));
                VL_DO_DANGLING(തള്ളുക_Deletep(refp), refp);
            }
        });
        വേണ്ടി (സ്ഥിരം auto& tconnect : tconnects) {
            തള്ളുക_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        // Replace the FuncRef with the inlined body
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(തള്ളുക_Deletep(funcrefp), funcrefp);
        // Clear referenced flag; sequences with isReferenced==false are deleted in assertPreAll
        seqp->isReferenced(അസത്യം);
    }
    AstPropSpec* clonePropertyBody(AstFuncRef* funcrefp, സ്ഥിരം AstProperty* propp) {
        AstPropSpec* propExprp = getPropertyExprp(propp);
        // Substitute inner property call before copying in order to not doing the same for
        // each call of outer property call.
        propExprp = substitutePropertyCall(propExprp);
        // Clone subtree after substitution. It is needed, because property might be called
        // multiple times with different arguments.
        propExprp = propExprp->clone_വൃക്ഷം(അസത്യം);
        // Build substitution maps for formal arguments and property-local
        // variables, then perform a single foreach to apply all replacements.
        // Map port vars to their actual argument expressions
        സ്ഥിരം V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propp->stmtsp());
        std::unordered_map<സ്ഥിരം AstVar*, Ast_കണ്ണി_വ്യഞ്ജകം*> portMap;
        വേണ്ടി (സ്ഥിരം auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }

        // Promote property-local variables (non-port vars, IEEE 16.10) to
        // module-level __Vpropvar temps. Cross-cycle persistence is handled
        // by the match item lowering in visit(AstImplication*).
        std::unordered_map<സ്ഥിരം AstVar*, AstVar*> localVarMap;
        വേണ്ടി (Ast_കണ്ണി* stmtp = propp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(stmtp, Var)) {
                എങ്കിൽ (!varp->isIO()) {
                    സ്ഥിരം string newName = m_propVarNames.get(varp);
                    AstVar* സ്ഥിരം newVarp = പുതിയ AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->addStmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        // Single traversal: substitute ports and update local var references
        propExprp->foreach([&](AstVarRef* refp) {
            സ്ഥിരം auto portIt = portMap.find(refp->varp());
            എങ്കിൽ (portIt != portMap.end()) {
                refp->replaceWith(portIt->second->clone_വൃക്ഷം(അസത്യം));
                VL_DO_DANGLING(തള്ളുക_Deletep(refp), refp);
                തിരികെ;
            }
            സ്ഥിരം auto localIt = localVarMap.find(refp->varp());
            എങ്കിൽ (localIt != localVarMap.end()) refp->varp(localIt->second);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        // Clean up argument expressions
        വേണ്ടി (സ്ഥിരം auto& tconnect : tconnects) {
            തള്ളുക_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(propExprp, funcrefp);
        തിരികെ propExprp;
    }
    AstPropSpec* substitutePropertyCall(AstPropSpec* nodep) {
        എങ്കിൽ (AstFuncRef* സ്ഥിരം funcrefp = VN_CAST(nodep->propp(), FuncRef)) {
            എങ്കിൽ (സ്ഥിരം AstProperty* സ്ഥിരം propp = VN_CAST(funcrefp->taskp(), Property)) {
                AstPropSpec* സ്ഥിരം propExprp = clonePropertyBody(funcrefp, propp);

                // Handle case with 2 disable iff statement (IEEE 1800-2023 16.12.1)
                എങ്കിൽ (nodep->disablep() && propExprp->disablep()) {
                    nodep->v3error("disable iff expression before property call and in its "
                                   "body is not legal");
                    തള്ളുക_Deletep(propExprp->disablep()->unlinkFrBack());
                }
                // If disable iff is in outer property, move it to inner
                എങ്കിൽ (nodep->disablep()) {
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം disablep = nodep->disablep()->unlinkFrBack();
                    propExprp->disablep(disablep);
                }

                എങ്കിൽ (nodep->sensesp() && propExprp->sensesp()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Clock event before property call and in its body");
                    തള്ളുക_Deletep(propExprp->sensesp()->unlinkFrBack());
                }
                // If clock event is in outer property, move it to inner
                എങ്കിൽ (nodep->sensesp()) {
                    AstSenItem* സ്ഥിരം sensesp = nodep->sensesp();
                    sensesp->unlinkFrBack();
                    propExprp->sensesp(sensesp);
                }

                // Now substitute property reference with property body
                nodep->replaceWith(propExprp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                തിരികെ propExprp;
            }
        }
        തിരികെ nodep;
    }

    ശൂന്യം substituteNestedPropertyCall(AstFuncRef* funcrefp, സ്ഥിരം AstProperty* propp) {
        AstPropSpec* സ്ഥിരം specp = clonePropertyBody(funcrefp, propp);
        Ast_കണ്ണി* replp = nullptr;
        എങ്കിൽ (specp->disablep()) {
            funcrefp->v3error("Illegal 'disable iff' in a nested property instance"
                              " (IEEE 1800-2023 16.12.1)");
        } അല്ലെങ്കിൽ എങ്കിൽ (specp->sensesp()) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: clock event in a nested property instance");
        } അല്ലെങ്കിൽ എങ്കിൽ (specp->propp()->exists(
                       [](സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* np) { തിരികെ np->isMultiCycleSva(); })) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: multi-cycle body in a nested property instance");
        } അല്ലെങ്കിൽ {
            replp = specp->propp()->unlinkFrBack();
        }
        എങ്കിൽ (!replp) replp = പുതിയ AstConst{funcrefp->fileline(), AstConst::BitFalse{}};
        funcrefp->replaceWith(replp);
        VL_DO_DANGLING(തള്ളുക_Deletep(funcrefp), funcrefp);
        VL_DO_DANGLING(തള്ളുക_Deletep(specp), specp);
    }

    // VISITORS
    //========== Statements
    ശൂന്യം visit(AstClocking* സ്ഥിരം nodep) override {
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        UINFO(8, "   CLOCKING" << nodep);
        എങ്കിൽ (nodep->isDefault()) {
            എങ്കിൽ (m_defaultClockingp) {
                nodep->v3error("Only one default clocking block allowed per module"
                               " (IEEE 1800-2023 14.12)");
            } അല്ലെങ്കിൽ {
                m_defaultClockingp = nodep;
                m_defaultClkEvtVarp = nodep->ensureEventp();
            }
        }
        iterateChildren(nodep);
        എങ്കിൽ (nodep->eventp()) nodep->addNextHere(nodep->eventp()->unlinkFrBack());
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ശൂന്യം visit(AstModportClockingRef* സ്ഥിരം nodep) override {
        // It has to be converted to a list of ModportClockingVarRefs,
        // because clocking blocks are removed in this pass
        വേണ്ടി (Ast_കണ്ണി* itemp = nodep->clockingp()->itemsp(); itemp; itemp = itemp->nextp()) {
            എങ്കിൽ (AstClockingItem* സ്ഥിരം citemp = VN_CAST(itemp, ClockingItem)) {
                എങ്കിൽ (AstVar* സ്ഥിരം varp
                    = citemp->varp() ? citemp->varp() : VN_AS(citemp->user1p(), Var)) {
                    AstModportVarRef* സ്ഥിരം modVarp = പുതിയ AstModportVarRef{
                        nodep->fileline(), varp->name(), citemp->direction()};
                    modVarp->varp(varp);
                    nodep->addNextHere(modVarp);
                    എങ്കിൽ (citemp->direction().isOutput()) {
                        // Drives through the modport set the drive flag
                        AstVar* സ്ഥിരം drivenVarp = getCreateDrivenVarp(citemp);
                        AstModportVarRef* സ്ഥിരം drivenRefp = പുതിയ AstModportVarRef{
                            nodep->fileline(), drivenVarp->name(), VDirection::OUTPUT};
                        drivenRefp->varp(drivenVarp);
                        nodep->addNextHere(drivenRefp);
                    }
                }
            }
        }
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ശൂന്യം visit(AstClockingItem* സ്ഥിരം nodep) override {
        // Get a ref to the sampled/driven variable
        AstVar* സ്ഥിരം varp = nodep->varp();
        എങ്കിൽ (!varp) {
            // Unused item
            തിരികെ;
        }
        // Flag set by drives of an output clockvar, possibly visited before this item
        എങ്കിൽ (nodep->direction().isOutput()) m_modp->addStmtsp(getCreateDrivenVarp(nodep));
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        V3Const::constifyEdit(nodep->skewp());
        എങ്കിൽ (!VN_IS(nodep->skewp(), Const)) {
            nodep->skewp()->v3error("Skew must be constant (IEEE 1800-2023 14.4)");
            തിരികെ;
        }
        AstConst* സ്ഥിരം skewp = VN_AS(nodep->skewp(), Const);
        എങ്കിൽ (skewp->അക്കം().isNegative()) skewp->v3error("Skew cannot be negative");
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = nodep->exprp();
        varp->name(m_clockingp->name() + "__DOT__" + varp->name());
        m_clockingp->addNextHere(varp->unlinkFrBack());
        nodep->user1p(varp);
        varp->user1p(nodep);
        എങ്കിൽ (nodep->direction() == VDirection::OUTPUT) {
            exprp->foreach([](സ്ഥിരം Ast_കണ്ണി_Var_Ref* varrefp) {
                // Prevent confusing BLKANDNBLK warnings on clockvars due to generated assignments
                varrefp->fileline()->warnOff(V3ErrorCode::BLKANDNBLK, സത്യം);
                // Mark the driven signal so later passes (e.g. V3Inst port
                // connection) know the drive flows out from inside the cell.
                // Item expressions are plain signal references, so only the
                // driven signal is marked, never index selects.
                varrefp->varp()->isClockingDriven(സത്യം);
            });
            AstVarRef* സ്ഥിരം skewedReadRefp = പുതിയ AstVarRef{flp, varp, VAccess::READ};
            skewedReadRefp->user1(സത്യം);
            // Initialize the clockvar
            AstVarRef* സ്ഥിരം skewedWriteRefp = പുതിയ AstVarRef{flp, varp, VAccess::WRITE};
            skewedWriteRefp->user1(സത്യം);
            AstInitialStatic* സ്ഥിരം initClockvarp = പുതിയ AstInitialStatic{
                flp, പുതിയ AstAssign{flp, skewedWriteRefp, exprp->clone_വൃക്ഷം_Pure(അസത്യം)}};
            m_modp->addStmtsp(initClockvarp);
            // Each drive sets a flag, so the signal is also assigned when driven with the same
            // value again, e.g. after another assignment to the signal (IEEE 1800-2023 14.16.2)
            AstVar* സ്ഥിരം drivenVarp = getCreateDrivenVarp(nodep);
            m_modp->addStmtsp(പുതിയ AstInitialStatic{
                flp, പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                   പുതിയ AstConst{flp, AstConst::BitFalse{}}}});
            // Assign the clockvar to the actual var if it was driven
            AstAssign* സ്ഥിരം assignp
                = പുതിയ AstAssign{flp, exprp->clone_വൃക്ഷം_Pure(അസത്യം), skewedReadRefp};
            AstIf* സ്ഥിരം ifp
                = പുതിയ AstIf{flp, പുതിയ AstVarRef{flp, drivenVarp, VAccess::READ},
                            പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                          പുതിയ AstConst{flp, AstConst::BitFalse{}}}};
            ifp->addThensp(assignp);
            // Only in the time step of the clocking event
            AstCMethodHard* സ്ഥിരം trigp = പുതിയ AstCMethodHard{
                nodep->fileline(), പുതിയ AstVarRef{flp, m_clockingp->ensureEventp(), VAccess::READ},
                VCMethod::EVENT_IS_TRIGGERED};
            trigp->dtypeSetBit();
            ifp->condp(പുതിയ AstLogAnd{flp, ifp->condp()->unlinkFrBack(), trigp});
            // A skew rounding to no time precision ticks is a zero skew
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം ticksp
                = V3Timing::delayToTicks(skewp->clone_വൃക്ഷം(അസത്യം), m_modp->timeunit());
            സ്ഥിരം തർക്കം zeroSkew = ticksp->isZero();
            VL_DO_DANGLING(ticksp->delete_വൃക്ഷം(), ticksp);
            എങ്കിൽ (zeroSkew || !skewp->fileline()->timingOn()) {
                // Drive the var in Re-NBA (IEEE 1800-2023 14.16), ignoring skews where timing is
                // turned off
                Ast_Sen_വൃക്ഷം* senTreep = m_clockingp->sentreep()->clone_വൃക്ഷം(അസത്യം);
                senTreep->addSensesp(പുതിയ AstSenItem{
                    flp, VEdgeType::ET_CHANGED, പുതിയ AstVarRef{flp, drivenVarp, VAccess::READ}});
                m_clockingp->addNextHere(പുതിയ AstAlwaysReactive{flp, senTreep, ifp});
            } അല്ലെങ്കിൽ {
                // Fork only the assignment, so this AlwaysPostponed can be retriggered
                // before the assignment happens
                AstFork* സ്ഥിരം forkp = പുതിയ AstFork{flp, VJoinType::JOIN_NONE};
                forkp->immediateStart(സത്യം);
                forkp->addForksp(പുതിയ AstBegin{flp, "", assignp->unlinkFrBack(), സത്യം});
                ifp->addThensp(forkp);
                // Read the clockvar in Postponed, so a drive late in the time step is applied
                // at this event plus the skew (IEEE 1800-2023 14.16)
                m_clockingp->addNextHere(പുതിയ AstAlwaysPostponed{flp, ifp});
                എങ്കിൽ (v3Global.opt.timing().isSetTrue()) {
                    AstDelay* സ്ഥിരം delayp = പുതിയ AstDelay{flp, skewp->unlinkFrBack(), അസത്യം};
                    delayp->timeunit(m_modp->timeunit());
                    assignp->timingControlp(delayp);
                } അല്ലെങ്കിൽ എങ്കിൽ (v3Global.opt.timing().isSetFalse()) {
                    nodep->v3warn(E_NOTIMING,
                                  "Clocking output skew greater than #0 requires --timing");
                } അല്ലെങ്കിൽ {
                    nodep->v3warn(E_NEEDTIMINGOPT,
                                  "Use --timing or --no-timing to specify how "
                                  "clocking output skew greater than #0 should be handled");
                }
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->direction() == VDirection::നിവേശം) {
            // Ref to the clockvar
            AstVarRef* സ്ഥിരം refp = പുതിയ AstVarRef{flp, varp, VAccess::WRITE};
            refp->user1(സത്യം);
            എങ്കിൽ (skewp->അക്കം().is1Step()) {
                // #1step means the value that is sampled is always the signal's last value
                // before the clock edge (IEEE 1800-2023 14.4)
                AstSampled* സ്ഥിരം sampledp
                    = പുതിയ AstSampled{flp, exprp->clone_വൃക്ഷം_Pure(അസത്യം), exprp->dtypep(), സത്യം};
                AstAssign* സ്ഥിരം assignp = പുതിയ AstAssign{flp, refp, sampledp};
                m_clockingp->addNextHere(പുതിയ AstAlways{
                    flp, VAlwaysKwd::ALWAYS, m_clockingp->sentreep()->clone_വൃക്ഷം(അസത്യം), assignp});
            } അല്ലെങ്കിൽ എങ്കിൽ (skewp->isZero()) {
                // #0 means the var has to be sampled in Observed (IEEE 1800-2023 14.13)
                AstAssign* സ്ഥിരം assignp = പുതിയ AstAssign{flp, refp, exprp->clone_വൃക്ഷം_Pure(അസത്യം)};
                m_clockingp->addNextHere(പുതിയ AstAlwaysObserved{
                    flp, m_clockingp->sentreep()->clone_വൃക്ഷം(അസത്യം), assignp});
            } അല്ലെങ്കിൽ {
                // Create a queue where we'll store sampled values with timestamps
                AstSampleQueueDType* സ്ഥിരം queueDtp
                    = പുതിയ AstSampleQueueDType{flp, exprp->dtypep()};
                m_netlistp->typeTablep()->addTypesp(queueDtp);
                AstVar* സ്ഥിരം queueVarp
                    = പുതിയ AstVar{flp, VVarType::MODULETEMP, "__Vqueue__" + varp->name(), queueDtp};
                queueVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_clockingp->addNextHere(queueVarp);
                // Create a process like this:
                //     always queue.push(<sampled var>);
                AstCMethodHard* സ്ഥിരം pushp = പുതിയ AstCMethodHard{
                    flp, പുതിയ AstVarRef{flp, queueVarp, VAccess::WRITE}, VCMethod::DYN_തള്ളുക,
                    പുതിയ AstTime{nodep->fileline(), m_modp->timeunit()}};
                pushp->addPinsp(exprp->clone_വൃക്ഷം_Pure(അസത്യം));
                pushp->dtypeSetVoid();
                m_clockingp->addNextHere(
                    പുതിയ AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, pushp->makeStmt()});
                // Create a process like this:
                //     always @<clocking event> queue.pop(<skew>, /*out*/<skewed var>);
                AstCMethodHard* സ്ഥിരം popp = പുതിയ AstCMethodHard{
                    flp, പുതിയ AstVarRef{flp, queueVarp, VAccess::READWRITE}, VCMethod::DYN_എടുക്കുക,
                    പുതിയ AstTime{nodep->fileline(), m_modp->timeunit()}};
                popp->addPinsp(skewp->unlinkFrBack());
                refp->access(VAccess::READWRITE);  // Only conditionally assigned
                popp->addPinsp(refp);
                popp->dtypeSetVoid();
                m_clockingp->addNextHere(പുതിയ AstAlways{flp, VAlwaysKwd::ALWAYS,
                                                       m_clockingp->sentreep()->clone_വൃക്ഷം(അസത്യം),
                                                       popp->makeStmt()});
            }
        } അല്ലെങ്കിൽ {
            nodep->v3fatalSrc("Invalid direction");
        }
    }
    ശൂന്യം visit(AstDelay* nodep) override {
        m_hasCycleDelay |= nodep->isCycleDelay();
        // Only cycle delays are relevant in this stage; also only process once
        എങ്കിൽ (!nodep->isCycleDelay()) {
            എങ്കിൽ (m_inSynchDrive) {
                nodep->v3error("Only cycle delays can be used in synchronous drives"
                               " (IEEE 1800-2023 14.16)");
            }
            iterateChildren(nodep);
            തിരികെ;
        }
        എങ്കിൽ (m_inAssign && !m_inSynchDrive) {
            nodep->v3error("Cycle delays not allowed as intra-assignment delays"
                           " (IEEE 1800-2023 14.11)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
            തിരികെ;
        }
        എങ്കിൽ (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBackWithNext());
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* valuep = V3Const::constifyEdit(nodep->lhsp()->unlinkFrBack());
        സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(valuep, Const);
        എങ്കിൽ (!constp && m_pexprp) {
            // V3AssertNfa handles non-const delays in sequences before this pass and
            // replaces the property; this branch should never be reached.
            nodep->v3fatalSrc("Non-constant cycle delay in assertion: "
                              "should have been caught by V3AssertNfa");
        } അല്ലെങ്കിൽ എങ്കിൽ (constp && constp->isZero()) {
            VL_DO_DANGLING(തള്ളുക_Deletep(valuep), valuep);
            എങ്കിൽ (m_inSynchDrive) {
                // ##0 has no effect in synchronous drives (IEEE 1800-2023 14.11)
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
                തിരികെ;
            }
            എങ്കിൽ (m_pexprp) {
                // ##0 in sequence context = zero delay = same clock tick
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
                തിരികെ;
            }
            // Procedural ##0: synchronize with default clocking event (IEEE 1800-2023 14.11)
            എങ്കിൽ (!m_defaultClockingp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
                തിരികെ;
            }
            nodep->replaceWith(newCycleDelayZerop(flp));
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ;
        }
        Ast_Sen_വൃക്ഷം* sentreep = nullptr;
        എങ്കിൽ (!m_drives.empty()) {
            എങ്കിൽ (m_inConcatAssign) {
                // Also the clockvars may be of different clocking blocks
                nodep->v3error("Cycle delays cannot be used in synchronous drives to a"
                               " concatenation (IEEE 1800-2023 14.16)");
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(തള്ളുക_Deletep(valuep), valuep);
                തിരികെ;
            }
            // Count the cycles of the driven clockvar's clocking block (IEEE 1800-2023 14.16)
            sentreep = newDriveSentreep(nodep, m_drives.front());
            എങ്കിൽ (!sentreep) {
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(തള്ളുക_Deletep(valuep), valuep);
                തിരികെ;
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (!m_defaultClockingp) {
            എങ്കിൽ (!m_pexprp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(തള്ളുക_Deletep(valuep), valuep);
                തിരികെ;
            }
            sentreep = പുതിയ Ast_Sen_വൃക്ഷം{flp, m_senip->clone_വൃക്ഷം(അസത്യം)};
        } അല്ലെങ്കിൽ {
            sentreep = m_defaultClockingp->sentreep()->clone_വൃക്ഷം(അസത്യം);
        }
        AstEventControl* സ്ഥിരം controlp
            = പുതിയ AstEventControl{nodep->fileline(), sentreep, nullptr};
        സ്ഥിരം std::string delayName = m_cycleDlyNames.get(nodep);
        Ast_കണ്ണി_വ്യഞ്ജകം* throughoutp
            = nodep->throughoutp() ? nodep->throughoutp()->unlinkFrBack() : nullptr;

        AstVar* സ്ഥിരം cntVarp = പുതിയ AstVar{flp, VVarType::BLOCKTEMP, delayName + "__counter",
                                           nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstBegin* സ്ഥിരം beginp = പുതിയ AstBegin{flp, delayName + "__block", cntVarp, സത്യം};
        // A non-constant delay is counted at runtime. The count is integral of 64 bits, as
        // V3Width makes delays, or real, which is rounded (IEEE 1800-2023 6.12.1)
        എങ്കിൽ (valuep->isDouble()) {
            valuep = പുതിയ AstRToIRoundS{flp, valuep};
            valuep->dtypeFrom(cntVarp);
        } അല്ലെങ്കിൽ {
            valuep = പുതിയ AstSel{flp, valuep, 0, cntVarp->width()};
        }
        beginp->addStmtsp(പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, cntVarp, VAccess::WRITE}, valuep});
        // If the count is zero, a synchronous drive waits for none, like with a constant '##0',
        // and a procedural delay waits like a procedural '##0' (IEEE 1800-2023 14.11)
        എങ്കിൽ (!constp && !m_inSynchDrive) {
            beginp->addStmtsp(പുതിയ AstIf{
                flp,
                പുതിയ AstEq{flp, പുതിയ AstVarRef{flp, cntVarp, VAccess::READ}, പുതിയ AstConst{flp, 0}},
                newCycleDelayZerop(flp)});
        }

        // Throughout: create flag tracking whether condition held every tick
        AstVar* throughoutOkp = nullptr;
        എങ്കിൽ (throughoutp) {
            throughoutOkp = പുതിയ AstVar{flp, VVarType::BLOCKTEMP, delayName + "__throughoutOk",
                                       nodep->findBasicDType(VBasicDTypeKwd::LOGIC_IMPLICIT)};
            throughoutOkp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->addStmtsp(throughoutOkp);
            beginp->addStmtsp(പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            പുതിയ AstConst{flp, AstConst::BitTrue{}}});
            // Check condition at tick 0 (sequence start, before entering loop)
            beginp->addStmtsp(
                പുതിയ AstIf{flp, പുതിയ AstLogNot{flp, throughoutp->clone_വൃക്ഷം_Pure(അസത്യം)},
                          പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                        പുതിയ AstConst{flp, AstConst::BitFalse{}}}});
        }

        {
            AstLoop* സ്ഥിരം loopp = പുതിയ AstLoop{flp};
            // When throughout is present, exit loop early if condition fails
            Ast_കണ്ണി_വ്യഞ്ജകം* loopCondp
                = പുതിയ AstGt{flp, പുതിയ AstVarRef{flp, cntVarp, VAccess::READ}, പുതിയ AstConst{flp, 0}};
            എങ്കിൽ (throughoutOkp) {
                loopCondp = പുതിയ AstLogAnd{flp, loopCondp,
                                          പുതിയ AstVarRef{flp, throughoutOkp, VAccess::READ}};
            }
            loopp->addStmtsp(പുതിയ AstLoopTest{flp, loopp, loopCondp});
            loopp->addStmtsp(controlp);
            loopp->addStmtsp(
                പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, cntVarp, VAccess::WRITE},
                              പുതിയ AstSub{flp, പുതിയ AstVarRef{flp, cntVarp, VAccess::READ},
                                         പുതിയ AstConst{flp, 1}}});
            // Check throughout condition at each tick during delay (IEEE 1800-2023 16.9.9)
            എങ്കിൽ (throughoutp) {
                loopp->addStmtsp(
                    പുതിയ AstIf{flp, പുതിയ AstLogNot{flp, throughoutp},
                              പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            പുതിയ AstConst{flp, AstConst::BitFalse{}}}});
            }
            beginp->addStmtsp(loopp);
        }
        // Compose wrappers on remaining sequence: throughout gate (inner), disable iff (outer)
        Ast_കണ്ണി* remainp = nodep->nextp() ? nodep->nextp()->unlinkFrBackWithNext() : nullptr;
        എങ്കിൽ (throughoutOkp) {
            // If condition failed during delay, fail assertion
            remainp = പുതിയ AstIf{flp, പുതിയ AstVarRef{flp, throughoutOkp, VAccess::READ}, remainp,
                                പുതിയ AstPExprClause{flp, /*pass=*/അസത്യം}};
        }
        എങ്കിൽ (m_disableSeqIfp && remainp) {
            AstIf* സ്ഥിരം disableSeqIfp = m_disableSeqIfp->clone_വൃക്ഷം(അസത്യം);
            disableSeqIfp->addThensp(remainp);
            remainp = disableSeqIfp;
        }
        എങ്കിൽ (remainp) {
            എങ്കിൽ (throughoutOkp) {
                // throughoutOkp is declared in beginp scope -- check must be inside it
                beginp->addStmtsp(remainp);
            } അല്ലെങ്കിൽ {
                nodep->addNextHere(remainp);
            }
        }
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
    }
    ശൂന്യം visit(Ast_Sen_വൃക്ഷം* nodep) override {
        എങ്കിൽ (m_inSynchDrive) {
            nodep->v3error("Event controls cannot be used in "
                           "synchronous drives (IEEE 1800-2023 14.16)");
        }

        സ്ഥിരം AstSampled* sampledp;
        എങ്കിൽ (nodep->exists([&sampledp](സ്ഥിരം AstSampled* സ്ഥിരം sp) {
                sampledp = sp;
                തിരികെ സത്യം;
            })) {
            sampledp->v3warn(E_UNSUPPORTED, "Unsupported: $sampled inside sensitivity list");
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Var_Ref* nodep) override {
        UINFO(8, " -varref:  " << nodep);
        UINFO(8, " -varref-var-back:  " << nodep->varp()->backp());
        UINFO(8, " -varref-var-user1:  " << nodep->varp()->user1p());
        എങ്കിൽ (AstClockingItem* സ്ഥിരം itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->firstAbovep(),
                ClockingItem)) {
            എങ്കിൽ (nodep->user1()) തിരികെ;

            // ensure linking still works, this has to be done only once
            എങ്കിൽ (AstVarXRef* xrefp = VN_CAST(nodep, VarXRef)) {
                UINFO(8, " -clockvarxref-in:  " << xrefp);
                string dotted = xrefp->dotted();
                സ്ഥിരം size_t dotPos = dotted.rfind('.');
                dotted.erase(dotPos, string::npos);
                xrefp->dotted(dotted);
                UINFO(8, " -clockvarxref-out: " << xrefp);
                m_xrefsp.emplace_back(xrefp);
            }

            എങ്കിൽ (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            എങ്കിൽ (nodep->access().isWriteOrRW()) {
                എങ്കിൽ (itemp->direction() == VDirection::OUTPUT) {
                    എങ്കിൽ (!m_inAssignDlyLhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    എങ്കിൽ (m_inAssign) {
                        m_inSynchDrive = സത്യം;
                        m_drives.തള്ളുക_back({itemp, nodep});
                    }
                } അല്ലെങ്കിൽ എങ്കിൽ (itemp->direction() == VDirection::നിവേശം) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
            nodep->user1(സത്യം);
        }
    }
    ശൂന്യം visit(AstMemberSel* nodep) override {
        എങ്കിൽ (AstClockingItem* സ്ഥിരം itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->firstAbovep(),
                ClockingItem)) {
            എങ്കിൽ (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            എങ്കിൽ (nodep->access().isWriteOrRW()) {
                എങ്കിൽ (itemp->direction() == VDirection::OUTPUT) {
                    എങ്കിൽ (!m_inAssignDlyLhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    എങ്കിൽ (m_inAssign) {
                        m_inSynchDrive = സത്യം;
                        m_drives.തള്ളുക_back({itemp, nodep});
                    }
                } അല്ലെങ്കിൽ എങ്കിൽ (itemp->direction() == VDirection::നിവേശം) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Assign* nodep) override {
        എങ്കിൽ (nodep->user1()) തിരികെ;
        VL_RESTORER(m_inAssign);
        VL_RESTORER(m_inSynchDrive);
        VL_RESTORER(m_inConcatAssign);
        VL_RESTORER_CLEAR(m_drives);
        m_inAssign = സത്യം;
        m_inSynchDrive = അസത്യം;
        {
            // The braces of a concatenation are a replication, by one
            സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* lhsp = nodep->lhsp();
            എങ്കിൽ (സ്ഥിരം AstReplicate* സ്ഥിരം repp = VN_CAST(lhsp, Replicate)) lhsp = repp->srcp();
            m_inConcatAssign = VN_IS(lhsp, Concat);
        }
        {
            VL_RESTORER(m_inAssignDlyLhs);
            m_inAssignDlyLhs = VN_IS(nodep, AssignDly);
            iterate(nodep->lhsp());
        }
        iterate(nodep->rhsp());
        എങ്കിൽ (nodep->timingControlp()) iterate(nodep->timingControlp());
        // A drive without a cycle delay, also after removing '##0', updates the clockvar now
        എങ്കിൽ (m_inSynchDrive && !nodep->timingControlp()) {
            AstAssign* സ്ഥിരം assignp = പുതിയ AstAssign{
                nodep->fileline(), nodep->lhsp()->unlinkFrBack(), nodep->rhsp()->unlinkFrBack()};
            assignp->user1(സത്യം);
            nodep->replaceWith(assignp);
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
            nodep = assignp;
        }
        // Note the drives when they take effect, after the cycle delay if any. Each clockvar
        // of the LHS is driven, e.g. in a concatenation which other simulators accept, although
        // IEEE 1800-2023 14.16 does not allow it
        Ast_കണ്ണി* setsp = nullptr;
        വേണ്ടി (സ്ഥിരം SynchDrive& drive : m_drives) {
            setsp = Ast_കണ്ണി::addNext(setsp, newDrivenSetp(nodep->fileline(), drive));
        }
        എങ്കിൽ (!setsp) തിരികെ;
        എങ്കിൽ (AstBegin* സ്ഥിരം beginp = VN_CAST(nodep->timingControlp(), Begin)) {
            beginp->addStmtsp(setsp);
        } അല്ലെങ്കിൽ {
            nodep->addNextHere(setsp);
        }
    }
    ശൂന്യം visit(AstAlways* nodep) override {
        iterateAndNextNull(nodep->sentreep());
        എങ്കിൽ (nodep->sentreep()) m_seniAlwaysp = nodep->sentreep()->sensesp();
        iterateAndNextNull(nodep->stmtsp());
        m_seniAlwaysp = nullptr;
    }

    ശൂന്യം visit(Ast_കണ്ണി_Cover_Or_Assert* nodep) override {
        എങ്കിൽ (nodep->sentreep()) തിരികെ;  // Already processed

        VL_RESTORER(m_senip);
        VL_RESTORER(m_disablep);
        m_senip = nullptr;
        m_disablep = nullptr;

        // Find Clocking's buried under nodep->exprsp
        iterateChildren(nodep);
        എങ്കിൽ (!nodep->immediate()) nodep->sentreep(new_Sen_വൃക്ഷം(nodep, nullptr, nodep));
    }
    ശൂന്യം visit(AstFalling* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* exprp = nodep->exprp()->unlinkFrBack();
        എങ്കിൽ (exprp->width() > 1) exprp = പുതിയ AstSel{fl, exprp, 0, 1};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം futurep = പുതിയ AstFuture{fl, exprp, new_Sen_വൃക്ഷം(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = പുതിയ AstAnd{fl, exprp->clone_വൃക്ഷം_Pure(അസത്യം), പുതിയ AstNot{fl, futurep}};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(AstFell* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* exprp = nodep->exprp()->unlinkFrBack();
        എങ്കിൽ (exprp->width() > 1) exprp = പുതിയ AstSel{fl, exprp, 0, 1};
        Ast_Sen_വൃക്ഷം* sentreep = nodep->sentreep();
        എങ്കിൽ (sentreep) sentreep->unlinkFrBack();
        AstPast* സ്ഥിരം pastp = പുതിയ AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_വൃക്ഷം(nodep, sentreep));
        exprp = പുതിയ AstAnd{fl, pastp, പുതിയ AstNot{fl, exprp->clone_വൃക്ഷം_Pure(അസത്യം)}};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(AstFuture* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        Ast_Sen_വൃക്ഷം* സ്ഥിരം sentreep = nodep->sentreep();
        എങ്കിൽ (sentreep) VL_DO_DANGLING(തള്ളുക_Deletep(sentreep->unlinkFrBack()), sentreep);
        nodep->sentreep(new_Sen_വൃക്ഷം(nodep));
    }
    ശൂന്യം visit(AstPast* nodep) override {
        checkSamplingFuncDType(nodep, nodep->exprp());
        എങ്കിൽ (nodep->sentreep()) തിരികെ;  // Already processed
        iterateChildren(nodep);
        nodep->sentreep(new_Sen_വൃക്ഷം(nodep));
    }
    ശൂന്യം visit(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- Lower standalone exact [*N] (N >= 2) via saturating counter.
        // Range/unbounded forms and SExpr-contained forms are lowered by V3AssertNfa.
        iterateChildren(nodep);
        // V3AssertNfa handles unbounded/ranged forms upstream, so this fast-path
        // is effectively unreachable when NFA is enabled.
        എങ്കിൽ (nodep->unbounded() || nodep->maxCountp()) തിരികെ;  // LCOV_EXCL_LINE
        സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(nodep->countp(), Const);
        എങ്കിൽ (VL_UNLIKELY(!constp || constp->toSInt() < 1)) {
            nodep->v3fatalSrc("Consecutive repetition count must be a positive constant"
                              " (should have been caught by V3Width)");
            തിരികെ;
        }
        സ്ഥിരം പൂർണ്ണസംഖ്യ n = constp->toSInt();
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = nodep->exprp()->unlinkFrBack();
        എങ്കിൽ (n == 1) {
            nodep->replaceWith(exprp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ;
        }
        // Saturating counter: if (expr) cnt <= min(cnt+1, N); else cnt <= 0;
        AstVar* സ്ഥിരം cntVarp = പുതിയ AstVar{flp, VVarType::MODULETEMP, m_consRepNames.get(""),
                                           nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(cntVarp);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം വ്യഞ്ജകം_Clonep = exprp->clone_വൃക്ഷം_Pure(അസത്യം);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം saturatingIncrp = പുതിയ AstCond{
            flp,
            പുതിയ AstLt{flp, പുതിയ AstVarRef{flp, cntVarp, VAccess::READ},
                      പുതിയ AstConst{flp, static_cast<uint32_t>(n)}},
            പുതിയ AstAdd{flp, പുതിയ AstVarRef{flp, cntVarp, VAccess::READ}, പുതിയ AstConst{flp, 1u}},
            പുതിയ AstConst{flp, static_cast<uint32_t>(n)}};
        AstAssignDly* സ്ഥിരം incrAssignp
            = പുതിയ AstAssignDly{flp, പുതിയ AstVarRef{flp, cntVarp, VAccess::WRITE}, saturatingIncrp};
        AstAssignDly* സ്ഥിരം resetAssignp = പുതിയ AstAssignDly{
            flp, പുതിയ AstVarRef{flp, cntVarp, VAccess::WRITE}, പുതിയ AstConst{flp, 0u}};
        AstIf* സ്ഥിരം ifp = പുതിയ AstIf{flp, വ്യഞ്ജകം_Clonep, incrAssignp, resetAssignp};
        Ast_Sen_വൃക്ഷം* സ്ഥിരം senTreep = new_Sen_വൃക്ഷം(nodep);
        AstAlways* സ്ഥിരം alwaysp = പുതിയ AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, ifp};
        cntVarp->addNextHere(alwaysp);
        // Match: cnt >= N-1 (previous cycles via NBA) && expr (current cycle)
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം cntCheckp = പുതിയ AstGte{flp, പുതിയ AstVarRef{flp, cntVarp, VAccess::READ},
                                                  പുതിയ AstConst{flp, static_cast<uint32_t>(n - 1)}};
        cntCheckp->dtypeSetBit();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം matchp = പുതിയ AstAnd{flp, cntCheckp, exprp};
        matchp->dtypeSetBit();
        nodep->replaceWith(matchp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(AstRising* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* exprp = nodep->exprp()->unlinkFrBack();
        എങ്കിൽ (exprp->width() > 1) exprp = പുതിയ AstSel{fl, exprp, 0, 1};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം futurep = പുതിയ AstFuture{fl, exprp, new_Sen_വൃക്ഷം(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = പുതിയ AstAnd{fl, പുതിയ AstNot{fl, exprp->clone_വൃക്ഷം_Pure(അസത്യം)}, futurep};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(AstRose* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* exprp = nodep->exprp()->unlinkFrBack();
        എങ്കിൽ (exprp->width() > 1) exprp = പുതിയ AstSel{fl, exprp, 0, 1};
        Ast_Sen_വൃക്ഷം* sentreep = nodep->sentreep();
        എങ്കിൽ (sentreep) sentreep->unlinkFrBack();
        AstPast* സ്ഥിരം pastp = പുതിയ AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_വൃക്ഷം(nodep, sentreep));
        exprp = പുതിയ AstAnd{fl, പുതിയ AstNot{fl, pastp}, exprp->clone_വൃക്ഷം_Pure(അസത്യം)};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    സ്ഥിതം AstAssocArrayDType* getProcessAssocArrayType(File_വരി* സ്ഥിരം flp) {
        // Type of __VassertsActive___x[std::process]
        Ast_കണ്ണി_DType* valp
            = v3Global.rootp()->typeTablep()->findBasicDType(flp, VBasicDTypeKwd::BIT);
        AstClassRefDType* keyp
            = പുതിയ AstClassRefDType{flp, v3Global.rootp()->stdPackageProcessp(), nullptr};
        keyp->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        v3Global.rootp()->typeTablep()->addTypesp(keyp);
        AstAssocArrayDType* സ്ഥിരം typep = പുതിയ AstAssocArrayDType{flp, valp, keyp};
        typep->dtypep(typep);
        v3Global.rootp()->typeTablep()->addTypesp(typep);
        തിരികെ typep;
    }
    സ്ഥിതം Ast_Stmt_വ്യഞ്ജകം* getProcessAssocArrayDelete(AstVarRef* സ്ഥിരം refp) {
        // Constructs refp.delete(std::process::self()) statement
        File_വരി* സ്ഥിരം flp = refp->fileline();
        AstCMethodHard* സ്ഥിരം deletep = പുതിയ AstCMethodHard{
            flp, refp, VCMethod::ASSOC_ERASE, v3Global.rootp()->stdPackageProcessSelfp(flp)};
        deletep->dtypep(refp->findVoidDType());
        തിരികെ പുതിയ Ast_Stmt_വ്യഞ്ജകം{flp, deletep};
    }
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* getProcessAssocArraySize(AstVarRef* സ്ഥിരം refp) {
        // Constructs refp.size() statement
        AstCMethodHard* സ്ഥിരം sizep
            = പുതിയ AstCMethodHard{refp->fileline(), refp, VCMethod::ASSOC_SIZE};
        sizep->dtypep(refp->findBasicDType(VBasicDTypeKwd::UINT32));
        തിരികെ sizep;
    }
    ശൂന്യം visit(AstSEventually* nodep) override {
        UASSERT(v3Global.rootp()->stdPackagep(), "Should be imported");

        Ast_Sen_വൃക്ഷം* സ്ഥിരം sentreep = new_Sen_വൃക്ഷം(nodep);
        എങ്കിൽ (!sentreep->sensesp()) {
            VL_DO_DANGLING(തള്ളുക_Deletep(sentreep), sentreep);
            nodep->replaceWith(പുതിയ AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ;
        }

        File_വരി* സ്ഥിരം flp = nodep->fileline();

        // Track active assertions
        AstVar* സ്ഥിരം activep = പുതിയ AstVar{flp, VVarType::MODULETEMP, m_activeNames.get(""),
                                           getProcessAssocArrayType(flp)};
        activep->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(activep);

        // Assertion condition check
        AstLoop* സ്ഥിരം loopp = പുതിയ AstLoop{flp};
        AstSampled* സ്ഥിരം condp
            = പുതിയ AstSampled{flp, nodep->exprp()->unlinkFrBack(), nullptr, സത്യം};
        loopp->addStmtsp(പുതിയ AstLoopTest{flp, loopp, പുതിയ AstLogNot{flp, condp}});
        loopp->addStmtsp(പുതിയ AstEventControl{flp, sentreep, nullptr});

        // Add assertion to the active set
        AstAssocSel* സ്ഥിരം selp = പുതിയ AstAssocSel{flp, പുതിയ AstVarRef{flp, activep, VAccess::WRITE},
                                                  v3Global.rootp()->stdPackageProcessSelfp(flp)};
        AstAssign* സ്ഥിരം incrementp = പുതിയ AstAssign{flp, selp, പുതിയ AstConst{flp, 1}};
        AstPExprClause* സ്ഥിരം clausep = പുതിയ AstPExprClause{flp};
        Ast_Stmt_വ്യഞ്ജകം* സ്ഥിരം deletep
            = getProcessAssocArrayDelete(പുതിയ AstVarRef{flp, activep, VAccess::WRITE});

        // Main assertion block
        AstBegin* സ്ഥിരം bodyp = പുതിയ AstBegin{flp, m_blockNames.get(""), nullptr, സത്യം};
        bodyp->addStmtsp(incrementp);
        bodyp->addStmtsp(loopp);
        bodyp->addStmtsp(clausep);
        bodyp->addStmtsp(deletep);

        // Validate assertion condition for each active assert
        AstVar* സ്ഥിരം activeCountp = പുതിയ AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        activeCountp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstAssign* സ്ഥിരം initActiveCountp
            = പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, activeCountp, VAccess::WRITE},
                            getProcessAssocArraySize(പുതിയ AstVarRef{flp, activep, VAccess::READ})};
        AstLoop* സ്ഥിരം finalLoopp = പുതിയ AstLoop{flp};
        AstIf* സ്ഥിരം finalBodypCondp
            = പുതിയ AstIf{flp, condp->clone_വൃക്ഷം_Pure(അസത്യം), പുതിയ AstPExprClause{flp},
                        പുതിയ AstPExprClause{flp, അസത്യം}};
        finalLoopp->addStmtsp(
            പുതിയ AstLoopTest{flp, finalLoopp,
                            പുതിയ AstNeq{flp, പുതിയ AstVarRef{flp, activeCountp, VAccess::READ},
                                       പുതിയ AstConst{flp, 0}}});
        finalLoopp->addStmtsp(finalBodypCondp);
        finalLoopp->addStmtsp(
            പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, activeCountp, VAccess::WRITE},
                          പുതിയ AstSub{flp, പുതിയ AstVarRef{flp, activeCountp, VAccess::READ},
                                     പുതിയ AstConst{flp, 1}}});

        // Final assertion block
        AstBegin* സ്ഥിരം finalp = പുതിയ AstBegin{flp, m_blockNames.get(""), nullptr, സത്യം};
        finalp->addStmtsp(activeCountp);
        finalp->addStmtsp(initActiveCountp);
        finalp->addStmtsp(finalLoopp);

        VL_RESTORER(m_pexprp);
        m_pexprp = പുതിയ AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
        VL_RESTORER(m_hasCycleDelay);
        m_hasCycleDelay = അസത്യം;
        iterate(bodyp);
        iterate(finalp);

        എങ്കിൽ (m_hasCycleDelay) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in s_eventually");
            nodep->replaceWith(പുതിയ AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            VL_DO_DANGLING(m_pexprp->delete_വൃക്ഷം(), m_pexprp);
            തിരികെ;
        }

        nodep->replaceWith(m_pexprp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(AstStable* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* exprp = nodep->exprp()->unlinkFrBack();
        Ast_Sen_വൃക്ഷം* sentreep = nodep->sentreep();
        എങ്കിൽ (sentreep) sentreep->unlinkFrBack();
        AstPast* സ്ഥിരം pastp = പുതിയ AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_വൃക്ഷം(nodep, sentreep));
        exprp = പുതിയ AstEq{fl, pastp, exprp->clone_വൃക്ഷം_Pure(അസത്യം)};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(AstSteady* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* exprp = nodep->exprp()->unlinkFrBack();
        എങ്കിൽ (exprp->width() > 1) exprp = പുതിയ AstSel{fl, exprp, 0, 1};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം futurep = പുതിയ AstFuture{fl, exprp, new_Sen_വൃക്ഷം(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = പുതിയ AstEq{fl, exprp->clone_വൃക്ഷം_Pure(അസത്യം), futurep};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(AstSampled* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (!nodep->internal()) checkSamplingFuncDType(nodep, nodep->exprp());
    }

    // Validate repetition count: must be a non-negative elaboration-time constant.
    // Shared by goto [->N] and nonconsecutive [=N] repetition.
    // On error, replaces nodep with BitFalse placeholder and returns nullptr.
    സ്ഥിരം AstConst* validate_Rep_എണ്ണം(Ast_കണ്ണി* nodep, Ast_കണ്ണി_വ്യഞ്ജകം*& countp) {
        countp = V3Const::constifyEdit(countp);
        സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(countp, Const);
        എങ്കിൽ (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(തള്ളുക_Deletep(countp), countp);
            nodep->replaceWith(പുതിയ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ nullptr;
        }
        എങ്കിൽ (constp->toSInt() < 0) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(തള്ളുക_Deletep(countp), countp);
            nodep->replaceWith(പുതിയ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ nullptr;
        }
        എങ്കിൽ (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(തള്ളുക_Deletep(countp), countp);
            nodep->replaceWith(പുതിയ AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ nullptr;
        }
        തിരികെ constp;
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
    AstPExpr* createRepPExpr(File_വരി* flp, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, Ast_കണ്ണി_വ്യഞ്ജകം* countp,
                             Ast_കണ്ണി_വ്യഞ്ജകം* rhsp, തർക്കം isOverlapped, തർക്കം isNonConsec) {
        AstSenItem* സ്ഥിരം sensesp = m_senip;
        UASSERT_OBJ(sensesp, exprp, "Repetition requires a clock");

        സ്ഥിരം std::string name
            = isNonConsec ? m_nonConsRepNames.get(exprp) : m_gotoRepNames.get(exprp);
        AstVar* സ്ഥിരം cntVarp = പുതിയ AstVar{flp, VVarType::BLOCKTEMP, name + "__counter",
                                           exprp->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstBegin* സ്ഥിരം beginp = പുതിയ AstBegin{flp, name + "__block", cntVarp, സത്യം};
        beginp->addStmtsp(
            പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, cntVarp, VAccess::WRITE}, പുതിയ AstConst{flp, 0}});

        // Counting loop: find N occurrences of expr (shared by goto and nonconsec)
        AstLoop* സ്ഥിരം loopp = പുതിയ AstLoop{flp};
        loopp->addStmtsp(പുതിയ AstLoopTest{flp, loopp,
                                         പുതിയ AstLt{flp, പുതിയ AstVarRef{flp, cntVarp, VAccess::READ},
                                                   countp->clone_വൃക്ഷം_Pure(അസത്യം)}});
        // if (expr) cnt++ -- sampled is applied to whole property expr by V3Assert
        loopp->addStmtsp(
            പുതിയ AstIf{flp, exprp,
                      പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, cntVarp, VAccess::WRITE},
                                    പുതിയ AstAdd{flp, പുതിയ AstVarRef{flp, cntVarp, VAccess::READ},
                                               പുതിയ AstConst{flp, 1}}}});
        loopp->addStmtsp(പുതിയ AstIf{
            flp, പുതിയ AstLt{flp, പുതിയ AstVarRef{flp, cntVarp, VAccess::READ}, countp},
            പുതിയ AstEventControl{flp, പുതിയ Ast_Sen_വൃക്ഷം{flp, sensesp->clone_വൃക്ഷം(അസത്യം)}, nullptr}});

        beginp->addStmtsp(loopp);

        എങ്കിൽ (isNonConsec && rhsp) {
            // IEEE 16.9.2: b[=N] = b[->N] ##1 !b[*0:$]
            // Trailing window: ##1 advance, then scan !expr cycles checking rhs.
            // Window closes when expr becomes true again (fail if rhs was never true).
            beginp->addStmtsp(പുതിയ AstEventControl{
                flp, പുതിയ Ast_Sen_വൃക്ഷം{flp, sensesp->clone_വൃക്ഷം(അസത്യം)}, nullptr});  // ##1
            എങ്കിൽ (!isOverlapped) {
                beginp->addStmtsp(പുതിയ AstEventControl{
                    flp, പുതിയ Ast_Sen_വൃക്ഷം{flp, sensesp->clone_വൃക്ഷം(അസത്യം)}, nullptr});  // |=>
            }
            // Window loop: check rhs at each !expr cycle (done variable for termination)
            AstVar* സ്ഥിരം doneVarp
                = പുതിയ AstVar{flp, VVarType::BLOCKTEMP, name + "__done", exprp->findBitDType()};
            doneVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->addStmtsp(doneVarp);
            beginp->addStmtsp(പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, doneVarp, VAccess::WRITE},
                                            പുതിയ AstConst{flp, AstConst::BitFalse{}}});
            auto setDone = [&]() {
                തിരികെ പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, doneVarp, VAccess::WRITE},
                                     പുതിയ AstConst{flp, AstConst::BitTrue{}}};
            };
            AstLoop* സ്ഥിരം windowp = പുതിയ AstLoop{flp};
            // LoopTest: continue while !done
            windowp->addStmtsp(പുതിയ AstLoopTest{
                flp, windowp, പുതിയ AstNot{flp, പുതിയ AstVarRef{flp, doneVarp, VAccess::READ}}});
            // if (expr) { fail; done = 1; } -- window closed, expr true again
            Ast_കണ്ണി* സ്ഥിരം failsp = പുതിയ AstPExprClause{flp, അസത്യം};
            failsp->addNext(setDone());
            windowp->addStmtsp(പുതിയ AstIf{flp, exprp->clone_വൃക്ഷം_Pure(അസത്യം), failsp});
            // if (rhs) { pass; done = 1; } -- consequent true at this !expr endpoint
            Ast_കണ്ണി* സ്ഥിരം passsp = പുതിയ AstPExprClause{flp, സത്യം};
            passsp->addNext(setDone());
            windowp->addStmtsp(പുതിയ AstIf{flp, rhsp, passsp});
            // @(clk) -- advance to next cycle in window
            windowp->addStmtsp(
                പുതിയ AstEventControl{flp, പുതിയ Ast_Sen_വൃക്ഷം{flp, sensesp->clone_വൃക്ഷം(അസത്യം)}, nullptr});
            beginp->addStmtsp(windowp);
        } അല്ലെങ്കിൽ എങ്കിൽ (isNonConsec) {
            // Standalone nonconsec: ##1 into window, then pass
            beginp->addStmtsp(
                പുതിയ AstEventControl{flp, പുതിയ Ast_Sen_വൃക്ഷം{flp, sensesp->clone_വൃക്ഷം(അസത്യം)}, nullptr});
            beginp->addStmtsp(പുതിയ AstPExprClause{flp, സത്യം});
        } അല്ലെങ്കിൽ എങ്കിൽ (rhsp) {
            // Goto rep: check consequent once at match endpoint
            എങ്കിൽ (!isOverlapped) {
                beginp->addStmtsp(പുതിയ AstEventControl{
                    flp, പുതിയ Ast_Sen_വൃക്ഷം{flp, sensesp->clone_വൃക്ഷം(അസത്യം)}, nullptr});
            }
            beginp->addStmtsp(പുതിയ AstIf{flp, rhsp, പുതിയ AstPExprClause{flp, സത്യം},
                                        പുതിയ AstPExprClause{flp, അസത്യം}});
        } അല്ലെങ്കിൽ {
            // Standalone goto: pass after counting
            beginp->addStmtsp(പുതിയ AstPExprClause{flp, സത്യം});
        }

        തിരികെ പുതിയ AstPExpr{flp, beginp, exprp->findBitDType()};
    }

    // Reject [=M:N] nonconsecutive range that reached V3AssertPre.
    // [->M:N] is fully handled upstream (NFA path); only [=M:N] still lands
    // here because AstSNonConsRep lowering is not yet implemented there.
    // Replaces the node with BitFalse so lowering continues after the warning.
    തർക്കം rejectNonconsecRange(Ast_കണ്ണി* nodep, Ast_കണ്ണി_വ്യഞ്ജകം* maxCountp) {
        എങ്കിൽ (!maxCountp) തിരികെ അസത്യം;
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: [=M:N] nonconsecutive range repetition"
                                     " (IEEE 1800-2023 16.9.2)");
        nodep->replaceWith(പുതിയ AstConst{nodep->fileline(), AstConst::BitFalse{}});
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        തിരികെ സത്യം;
    }

    ശൂന്യം visit(AstSGotoRep* nodep) override {
        // Standalone goto rep (not inside implication antecedent)
        iterateChildren(nodep);
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* countp = nodep->countp()->unlinkFrBack();
        എങ്കിൽ (!validate_Rep_എണ്ണം(nodep, countp)) തിരികെ;
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = nodep->exprp()->unlinkFrBack();
        AstPExpr* സ്ഥിരം pexprp = createRepPExpr(flp, exprp, countp, nullptr, സത്യം, അസത്യം);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }

    ശൂന്യം visit(AstSNonConsRep* nodep) override {
        // Standalone nonconsecutive rep (not inside implication antecedent)
        iterateChildren(nodep);
        എങ്കിൽ (rejectNonconsecRange(nodep, nodep->maxCountp())) തിരികെ;
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* countp = nodep->countp()->unlinkFrBack();
        എങ്കിൽ (!validate_Rep_എണ്ണം(nodep, countp)) തിരികെ;
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = nodep->exprp()->unlinkFrBack();
        AstPExpr* സ്ഥിരം pexprp = createRepPExpr(flp, exprp, countp, nullptr, സത്യം, സത്യം);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }

    ശൂന്യം visit(AstFuncRef* nodep) override {
        // IEEE 1800-2023 16.8: Inline sequence instances wherever they appear
        // in the expression tree (inside implications, boolean ops, nested refs, etc.)
        എങ്കിൽ (AstSequence* സ്ഥിരം seqp = VN_CAST(nodep->taskp(), Sequence)) {
            substituteSequenceCall(nodep, seqp);
            // The FuncRef has been replaced; do not access nodep after this point.
            // The replacement node will be visited by the parent's iterateChildren.
            തിരികെ;
        }
        എങ്കിൽ (സ്ഥിരം AstProperty* സ്ഥിരം propp = VN_CAST(nodep->taskp(), Property)) {
            substituteNestedPropertyCall(nodep, propp);
            തിരികെ;
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstImplication* nodep) override {
        എങ്കിൽ (nodep->sentreep()) തിരികെ;  // Already processed

        // Top-level followed-by is claimed by V3AssertNfa; anything reaching here
        // is nested inside iff/implies/or. IEEE 1800-2023 16.12.9 permits the
        // composition, but silent lowering would drop non-vacuous-fail semantics.
        എങ്കിൽ (nodep->isFollowedBy()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: followed-by (#-# / #=#) nested inside property operator"
                          " (iff/implies/or) (IEEE 1800-2023 16.12.9)");
            nodep->replaceWith(പുതിയ AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ;
        }

        // Handle goto repetition as antecedent before iterateChildren,
        // so the standalone AstSGotoRep visitor doesn't process it
        എങ്കിൽ (AstSGotoRep* സ്ഥിരം gotop = VN_CAST(nodep->lhsp(), SGotoRep)) {
            iterateChildren(gotop);
            iterateAndNextNull(nodep->rhsp());
            File_വരി* സ്ഥിരം flp = nodep->fileline();
            Ast_കണ്ണി_വ്യഞ്ജകം* countp = gotop->countp()->unlinkFrBack();
            എങ്കിൽ (!validate_Rep_എണ്ണം(nodep, countp)) തിരികെ;
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = gotop->exprp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = nodep->rhsp()->unlinkFrBack();
            AstPExpr* സ്ഥിരം pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), അസത്യം);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ;
        }

        // Handle nonconsecutive repetition as antecedent before iterateChildren,
        // so the standalone AstSNonConsRep visitor doesn't process it
        എങ്കിൽ (AstSNonConsRep* സ്ഥിരം ncrp = VN_CAST(nodep->lhsp(), SNonConsRep)) {
            iterateChildren(ncrp);
            iterateAndNextNull(nodep->rhsp());
            എങ്കിൽ (rejectNonconsecRange(nodep, ncrp->maxCountp())) തിരികെ;
            File_വരി* സ്ഥിരം flp = nodep->fileline();
            Ast_കണ്ണി_വ്യഞ്ജകം* countp = ncrp->countp()->unlinkFrBack();
            എങ്കിൽ (!validate_Rep_എണ്ണം(nodep, countp)) തിരികെ;
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = ncrp->exprp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = nodep->rhsp()->unlinkFrBack();
            AstPExpr* സ്ഥിരം pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), സത്യം);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ;
        }

        iterateChildren(nodep);

        File_വരി* സ്ഥിരം flp = nodep->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* lhsp = nodep->lhsp()->unlinkFrBack();

        // Lower sequence match items (IEEE 16.11): (expr, var = val, ...) |-> / |=>
        എങ്കിൽ (Ast_വ്യഞ്ജകം_Stmt* സ്ഥിരം വ്യഞ്ജകം_Stmtp = VN_CAST(lhsp, വ്യഞ്ജകം_Stmt)) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം antExprp = വ്യഞ്ജകം_Stmtp->resultp()->unlinkFrBack();

            എങ്കിൽ (nodep->isOverlapped()) {
                // |-> : assign to __Vpropvar via always_comb (continuous).
                // The assign evaluates RHS once; V3Sampled snapshots the
                // result so all consequent refs read the same value.
                Ast_കണ്ണി* matchAssignsp = nullptr;
                വേണ്ടി (Ast_കണ്ണി* stmtp = വ്യഞ്ജകം_Stmtp->stmtsp(); stmtp;) {
                    Ast_കണ്ണി* സ്ഥിരം nextp = stmtp->nextp();
                    എങ്കിൽ (AstAssign* സ്ഥിരം assignp = VN_CAST(stmtp, Assign)) {
                        assignp->unlinkFrBack();
                        matchAssignsp = Ast_കണ്ണി::addNextNull(matchAssignsp, assignp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(തള്ളുക_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                എങ്കിൽ (matchAssignsp) {
                    AstAlways* സ്ഥിരം alwaysp
                        = പുതിയ AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, matchAssignsp};
                    m_modp->addStmtsp(alwaysp);
                }
            } അല്ലെങ്കിൽ {
                // |=> : assign to __Vpropvar via NBA in a clocked always block.
                // The NBA commits before the next cycle's sampled snapshot,
                // so the consequent (which already references __Vpropvar)
                // sees the captured value.
                Ast_കണ്ണി* matchAssignsp = nullptr;
                വേണ്ടി (Ast_കണ്ണി* stmtp = വ്യഞ്ജകം_Stmtp->stmtsp(); stmtp;) {
                    Ast_കണ്ണി* സ്ഥിരം nextp = stmtp->nextp();
                    എങ്കിൽ (AstAssign* സ്ഥിരം assignp = VN_CAST(stmtp, Assign)) {
                        assignp->unlinkFrBack();
                        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം assignLhsp = assignp->lhsp()->unlinkFrBack();
                        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം assignRhsp = assignp->rhsp()->unlinkFrBack();
                        AstAssignDly* സ്ഥിരം dlyp = പുതിയ AstAssignDly{flp, assignLhsp, assignRhsp};
                        VL_DO_DANGLING(തള്ളുക_Deletep(assignp), assignp);
                        matchAssignsp = Ast_കണ്ണി::addNextNull(matchAssignsp, dlyp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(തള്ളുക_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                എങ്കിൽ (matchAssignsp) {
                    AstIf* സ്ഥിരം condp
                        = പുതിയ AstIf{flp, antExprp->clone_വൃക്ഷം_Pure(അസത്യം), matchAssignsp};
                    AstAlways* സ്ഥിരം alwaysp
                        = പുതിയ AstAlways{flp, VAlwaysKwd::ALWAYS, new_Sen_വൃക്ഷം(nodep), condp};
                    m_modp->addStmtsp(alwaysp);
                }
            }
        }

        എങ്കിൽ (AstPExpr* സ്ഥിരം pexprp = VN_CAST(rhsp, PExpr)) {
            // Implication with sequence expression on RHS (IEEE 1800-2023 16.11, 16.12.7).
            // The PExpr was lowered from the property expression earlier in this pass.
            // Wrap the PExpr body with the antecedent check so the sequence only
            // starts when the antecedent holds.
            Ast_കണ്ണി_വ്യഞ്ജകം* condp;
            എങ്കിൽ (nodep->isOverlapped()) {
                // Overlapped implication (|->): check antecedent on same cycle.
                // disable iff is applied at the assertion level, not at the
                // antecedent gate, matching the existing non-PExpr overlapped path.
                condp = lhsp;
            } അല്ലെങ്കിൽ {
                // Non-overlapped implication (|=>): check antecedent from previous cycle
                എങ്കിൽ (m_disablep) {
                    lhsp
                        = പുതിയ AstAnd{flp, പുതിയ AstNot{flp, m_disablep->clone_വൃക്ഷം_Pure(അസത്യം)}, lhsp};
                }
                AstPast* സ്ഥിരം pastp
                    = പുതിയ AstPast{flp, lhsp, nullptr, nullptr, /* propertyTiming */ സത്യം};
                pastp->dtypeFrom(lhsp);
                pastp->sentreep(new_Sen_വൃക്ഷം(nodep));
                condp = pastp;
            }
            // Wrap existing PExpr body: if (antecedent) { <original body> } else { /* vacuous pass
            // */ }
            // Only the statements are guarded; declarations stay directly in the block. After
            // V3Fork moves the block's statements into a task, V3Task only handles variables
            // that are direct statements of the task.
            AstBegin* സ്ഥിരം bodyp = pexprp->bodyp();
            AstIf* സ്ഥിരം guardp = പുതിയ AstIf{flp, condp};
            വേണ്ടി (Ast_കണ്ണി* stmtp = bodyp->stmtsp(); stmtp;) {
                Ast_കണ്ണി* സ്ഥിരം nextp = stmtp->nextp();
                എങ്കിൽ (!VN_IS(stmtp, Var)) guardp->addThensp(stmtp->unlinkFrBack());
                stmtp = nextp;
            }
            bodyp->addStmtsp(guardp);
            nodep->replaceWith(pexprp);
            // Don't iterate pexprp here -- it was already iterated when created
            // (in visit(AstSExpr*)), so delays and disable iff are already processed.
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->isOverlapped()) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp
                = m_underCover ? static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(പുതിയ AstLogAnd{flp, lhsp, rhsp})
                               : പുതിയ AstLogOr{flp, പുതിയ AstLogNot{flp, lhsp}, rhsp};
            nodep->replaceWith(exprp);
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (m_disablep) {
                lhsp = പുതിയ AstAnd{flp, പുതിയ AstNot{flp, m_disablep->clone_വൃക്ഷം_Pure(അസത്യം)}, lhsp};
            }

            AstPast* സ്ഥിരം pastp
                = പുതിയ AstPast{flp, lhsp, nullptr, nullptr, /* propertyTiming */ സത്യം};
            pastp->dtypeFrom(lhsp);
            pastp->sentreep(new_Sen_വൃക്ഷം(nodep));
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp
                = m_underCover ? static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(പുതിയ AstAnd{flp, pastp, rhsp})
                               : പുതിയ AstOr{flp, പുതിയ AstNot{flp, pastp}, rhsp};
            exprp->dtypeSetBit();
            nodep->replaceWith(exprp);
        }
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(AstUntil* nodep) override {
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        UASSERT_OBJ(
            !m_pexprp, nodep,
            "'" << nodep->verilogKwd()
                << "' in complex property expression should have been rejected by V3AssertNfa");
        എങ്കിൽ (nodep->isStrong()
            && (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue())) {
            nodep->v3warn(E_NOTIMING, nodep->verilogKwd() << " requires --timing");
            nodep->replaceWith(പുതിയ AstConst{flp, AstConst::BitFalse{}});
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ;
        }
        എങ്കിൽ (nodep->isStrong()) {
            // p s_until q / p s_until_with q: q must eventually be true. Until then, p must
            // be true on every sampled tick. For s_until, check q first: when q is true on
            // this tick, p is not required. For s_until_with, p must be true on the q tick too.
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rawLhsp = nodep->lhsp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rawRhsp = nodep->rhsp()->unlinkFrBack();
            AstSampled* സ്ഥിരം lhsp = പുതിയ AstSampled{flp, rawLhsp, rawLhsp->dtypep(), സത്യം};
            AstSampled* സ്ഥിരം rhsp = പുതിയ AstSampled{flp, rawRhsp, rawRhsp->dtypep(), സത്യം};
            Ast_കണ്ണി_വ്യഞ്ജകം* finalCondp = rhsp->clone_വൃക്ഷം_Pure(അസത്യം);
            എങ്കിൽ (nodep->isOverlapping()) {
                finalCondp = പുതിയ AstLogAnd{flp, lhsp->clone_വൃക്ഷം_Pure(അസത്യം), finalCondp};
            }

            // Track active assertion attempts. Only the count is needed to emit final failures.
            AstVar* സ്ഥിരം activep = പുതിയ AstVar{flp, VVarType::MODULETEMP, m_activeNames.get(""),
                                               nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            activep->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(activep);

            AstVar* സ്ഥിരം donep
                = പുതിയ AstVar{flp, VVarType::BLOCKTEMP, "__VassertDone", nodep->findBitDType()};
            donep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            auto setDone = [&]() {
                തിരികെ പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, donep, VAccess::WRITE},
                                     പുതിയ AstConst{flp, AstConst::BitTrue{}}};
            };

            AstLoop* സ്ഥിരം loopp = പുതിയ AstLoop{flp};
            AstAssign* സ്ഥിരം decrementVar = പുതിയ AstAssign{
                flp, പുതിയ AstVarRef{flp, activep, VAccess::WRITE},
                പുതിയ AstSub{flp, പുതിയ AstVarRef{flp, activep, VAccess::READ}, പുതിയ AstConst{flp, 1}}};
            loopp->addStmtsp(പുതിയ AstLoopTest{
                flp, loopp, പുതിയ AstLogNot{flp, പുതിയ AstVarRef{flp, donep, VAccess::READ}}});
            {
                Ast_കണ്ണി* സ്ഥിരം passp = പുതിയ AstPExprClause{flp};
                passp->addNext(decrementVar);
                passp->addNext(setDone());
                Ast_കണ്ണി_വ്യഞ്ജകം* passCondp = rhsp;
                എങ്കിൽ (nodep->isOverlapping()) {
                    passCondp = പുതിയ AstLogAnd{flp, lhsp->clone_വൃക്ഷം_Pure(അസത്യം), passCondp};
                }
                loopp->addStmtsp(പുതിയ AstIf{flp, passCondp, passp});
            }
            {
                Ast_കണ്ണി* സ്ഥിരം failp = പുതിയ AstPExprClause{flp, അസത്യം};
                failp->addNext(decrementVar->clone_വൃക്ഷം(അസത്യം));
                failp->addNext(setDone());
                loopp->addStmtsp(പുതിയ AstIf{
                    flp,
                    പുതിയ AstLogAnd{flp,
                                  പുതിയ AstLogNot{flp, പുതിയ AstVarRef{flp, donep, VAccess::READ}},
                                  പുതിയ AstLogNot{flp, lhsp}},
                    failp});
            }
            AstDelay* സ്ഥിരം delayp = പുതിയ AstDelay{flp, പുതിയ AstConst{flp, 1}, അസത്യം};
            delayp->timeunit(m_modp->timeunit());
            loopp->addStmtsp(പുതിയ AstIf{
                flp, പുതിയ AstLogNot{flp, പുതിയ AstVarRef{flp, donep, VAccess::READ}}, delayp});

            // Main assertion block
            AstBegin* സ്ഥിരം bodyp = പുതിയ AstBegin{flp, m_blockNames.get(""), nullptr, സത്യം};
            bodyp->addStmtsp(donep);
            bodyp->addStmtsp(പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, donep, VAccess::WRITE},
                                           പുതിയ AstConst{flp, AstConst::BitFalse{}}});
            File_വരി* സ്ഥിരം flp = activep->fileline();
            bodyp->addStmtsp(
                പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, activep, VAccess::WRITE},
                              പുതിയ AstAdd{flp, പുതിയ AstVarRef{flp, activep, VAccess::READ},
                                         പുതിയ AstConst{flp, 1}}});
            bodyp->addStmtsp(loopp);

            // Validate assertion condition for each active assert
            AstVar* സ്ഥിരം activeCountp = പുതിയ AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                    nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            activeCountp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

            AstAssign* സ്ഥിരം initActiveCountp
                = പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, activeCountp, VAccess::WRITE},
                                പുതിയ AstVarRef{flp, activep, VAccess::READ}};
            AstLoop* സ്ഥിരം finalLoopp = പുതിയ AstLoop{flp};
            finalLoopp->addStmtsp(
                പുതിയ AstLoopTest{flp, finalLoopp,
                                പുതിയ AstNeq{flp, പുതിയ AstVarRef{flp, activeCountp, VAccess::READ},
                                           പുതിയ AstConst{flp, 0}}});
            finalLoopp->addStmtsp(പുതിയ AstIf{flp, finalCondp, പുതിയ AstPExprClause{flp},
                                            പുതിയ AstPExprClause{flp, അസത്യം}});
            finalLoopp->addStmtsp(
                പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, activeCountp, VAccess::WRITE},
                              പുതിയ AstSub{flp, പുതിയ AstVarRef{flp, activeCountp, VAccess::READ},
                                         പുതിയ AstConst{flp, 1}}});

            // Final assertion block
            AstBegin* സ്ഥിരം finalp = പുതിയ AstBegin{flp, m_blockNames.get(""), nullptr, സത്യം};
            finalp->addStmtsp(activeCountp);
            finalp->addStmtsp(initActiveCountp);
            finalp->addStmtsp(finalLoopp);

            AstPExpr* സ്ഥിരം pexprp = പുതിയ AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
            VL_RESTORER(m_pexprp);
            VL_RESTORER(m_hasCycleDelay);
            m_pexprp = pexprp;
            m_hasCycleDelay = അസത്യം;
            iterate(bodyp);
            iterate(finalp);

            UASSERT_OBJ(!m_hasCycleDelay, nodep,
                        "s_until cycle delay should have been handled by V3AssertNfa");

            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ;
        }
        AstLoop* സ്ഥിരം loopp = പുതിയ AstLoop{flp};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = nodep->lhsp()->unlinkFrBack();
        AstLogAnd* സ്ഥിരം loopCondp = പുതിയ AstLogAnd{flp, lhsp, പുതിയ AstLogNot{flp, rhsp}};
        loopp->addStmtsp(പുതിയ AstLoopTest{flp, loopp, loopCondp});
        loopp->addStmtsp(പുതിയ AstEventControl{flp, new_Sen_വൃക്ഷം(nodep), nullptr});

        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsCopyp = rhsp->clone_വൃക്ഷം_Pure(അസത്യം);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം passCondp
            = nodep->isOverlapping() ? പുതിയ AstLogAnd{flp, lhsp->clone_വൃക്ഷം_Pure(അസത്യം), rhsCopyp}
                                     : rhsCopyp;
        AstBegin* സ്ഥിരം beginp = പുതിയ AstBegin{flp, m_blockNames.get(""), loopp, സത്യം};
        beginp->addStmtsp(
            പുതിയ AstIf{flp, passCondp, പുതിയ AstPExprClause{flp}, പുതിയ AstPExprClause{flp, അസത്യം}});

        AstPExpr* സ്ഥിരം pexprp = പുതിയ AstPExpr{flp, beginp, nodep->dtypep()};
        pexprp->user1(1);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }

    ശൂന്യം visit(AstDefaultDisable* nodep) override {
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ശൂന്യം visit(AstInferredDisable* nodep) override {
        Ast_കണ്ണി* newp;
        എങ്കിൽ (m_defaultDisablep) {
            newp = m_defaultDisablep->condp()->clone_വൃക്ഷം_Pure(സത്യം);
        } അല്ലെങ്കിൽ {
            newp = പുതിയ AstConst{nodep->fileline(), AstConst::BitFalse{}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }

    ശൂന്യം visit(AstPropSpec* nodep) override {
        nodep = substitutePropertyCall(nodep);
        iterateAndNextNull(nodep->sensesp());
        എങ്കിൽ (m_senip && m_senip != nodep->sensesp())
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Only one PSL clock allowed per assertion");
        സ്ഥിരം AstCover* സ്ഥിരം coverp = VN_CAST(nodep->backp(), Cover);
        സ്ഥിരം തർക്കം seqEvent = coverp && coverp->isSeqEvent();
        എങ്കിൽ (!nodep->disablep() && m_defaultDisablep && !seqEvent) {
            nodep->disablep(m_defaultDisablep->condp()->clone_വൃക്ഷം_Pure(സത്യം));
        }
        m_disablep = nodep->disablep();
        // Unlink and just keep a pointer to it, convert to sentree as needed
        m_senip = nodep->sensesp();
        iterateNull(nodep->disablep());
        എങ്കിൽ (!VN_AS(nodep->backp(), കണ്ണി_Cover_Or_Assert)->immediate()) {
            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം propDtp = nodep->propp()->dtypep();
            nodep->propp(പുതിയ AstSampled{nodep->fileline(), nodep->propp()->unlinkFrBack(),
                                        propDtp->dtypep(), സത്യം});
        }
        // cover counts non-vacuous matches only (IEEE 1800-2023 16.15.2), so an
        // implication antecedent must hold; assert passes vacuously instead.
        VL_RESTORER(m_underCover);
        m_underCover = VN_IS(nodep->backp(), Cover);
        iterate(nodep->propp());
        iterateNull(nodep->matchCountp());
    }
    ശൂന്യം visit(AstPExpr* nodep) override {
        // V3AssertNfa handles multi-cycle property expressions before this pass,
        // so the following unsupported paths are defensive and typically unreached.
        എങ്കിൽ (m_pexprp && m_pexprp->user1()) {  // LCOV_EXCL_START
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Complex property expression inside 'until''");
            nodep->replaceWith(പുതിയ AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ;
        }  // LCOV_EXCL_STOP
        എങ്കിൽ (AstLogNot* സ്ഥിരം notp = VN_CAST(nodep->backp(), LogNot)) {
            notp->replaceWith(nodep->unlinkFrBack());
            VL_DO_DANGLING(തള്ളുക_Deletep(notp), notp);
            iterate(nodep);
            തിരികെ;
        }
        // Sequence expression as antecedent of implication is not yet supported
        എങ്കിൽ (AstImplication* സ്ഥിരം implp = VN_CAST(nodep->backp(), Implication)) {
            എങ്കിൽ (implp->lhsp() == nodep) {  // LCOV_EXCL_START
                implp->v3warn(E_UNSUPPORTED,
                              "Unsupported: Implication with sequence expression as antecedent");
                nodep->replaceWith(പുതിയ AstConst{nodep->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                തിരികെ;
            }  // LCOV_EXCL_STOP
        }
        VL_RESTORER(m_pexprp);
        VL_RESTORER(m_disableSeqIfp);
        m_pexprp = nodep;

        എങ്കിൽ (m_disablep) {
            സ്ഥിരം AstSampled* sampledp = nullptr;
            എങ്കിൽ (m_disablep->exists([&sampledp](സ്ഥിരം AstSampled* സ്ഥിരം sp) {
                    sampledp = sp;
                    തിരികെ സത്യം;
                })) {
                sampledp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: $sampled inside disabled condition of a sequence");
                m_disablep = പുതിയ AstConst{m_disablep->fileline(), AstConst::BitFalse{}};
                // always a copy is used, so remove it now
                തള്ളുക_Deletep(m_disablep);  // m_disablep used later
            }
            File_വരി* സ്ഥിരം flp = nodep->fileline();
            // Add counter which counts times the condition turned true
            AstVar* സ്ഥിരം disableCntp
                = പുതിയ AstVar{flp, VVarType::MODULETEMP, m_disableCntNames.get(""),
                             nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            disableCntp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(disableCntp);
            AstVarRef* സ്ഥിരം readCntRefp = പുതിയ AstVarRef{flp, disableCntp, VAccess::READ};
            AstVarRef* സ്ഥിരം writeCntRefp = പുതിയ AstVarRef{flp, disableCntp, VAccess::WRITE};
            AstAssign* സ്ഥിരം incrStmtp = പുതിയ AstAssign{
                flp, writeCntRefp, പുതിയ AstAdd{flp, readCntRefp, പുതിയ AstConst{flp, 1}}};
            AstAlways* സ്ഥിരം alwaysp
                = പുതിയ AstAlways{flp, VAlwaysKwd::ALWAYS,
                                പുതിയ Ast_Sen_വൃക്ഷം{flp, പുതിയ AstSenItem{flp, VEdgeType::ET_POSEDGE,
                                                                   m_disablep->clone_വൃക്ഷം(അസത്യം)}},
                                incrStmtp};
            disableCntp->addNextHere(alwaysp);

            // Store value of that counter at the beginning of sequence evaluation
            AstBegin* സ്ഥിരം bodyp = nodep->bodyp();
            AstVar* സ്ഥിരം initialCntp = പുതിയ AstVar{flp, VVarType::BLOCKTEMP, "__VinitialCnt",
                                                   nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            initialCntp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            AstAssign* സ്ഥിരം assignp
                = പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, initialCntp, VAccess::WRITE},
                                readCntRefp->clone_വൃക്ഷം(അസത്യം)};
            // Prepend to the sequence body to keep statement list structure valid.
            UASSERT_OBJ(!bodyp->name().empty(), bodyp, "Sequence body should be named");
            Ast_കണ്ണി* സ്ഥിരം newStmtsp = Ast_കണ്ണി::addNext<Ast_കണ്ണി, Ast_കണ്ണി>(initialCntp, assignp);
            bodyp->prependStmtsp(newStmtsp);

            m_disableSeqIfp
                = പുതിയ AstIf{flp, പുതിയ AstEq{flp, പുതിയ AstVarRef{flp, initialCntp, VAccess::READ},
                                           readCntRefp->clone_വൃക്ഷം(അസത്യം)}};
            // Delete it, because it is always copied before insetion to the AST
            തള്ളുക_Deletep(m_disableSeqIfp);  // m_disableSeqIfp used later
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultClkEvtVarp);
        VL_RESTORER(m_defaultDisablep);
        VL_RESTORER(m_modp);
        എങ്കിൽ (!VN_IS(nodep, Class)) {
            // A class nested in a module is in the scope of the module's default clocking
            m_defaultClockingp = nullptr;
            m_defaultClkEvtVarp = nullptr;
        }
        m_defaultDisablep = nodep->defaultDisablep();
        m_modp = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstProperty* nodep) override {
        // The body will be visited when will be substituted in place of property reference
        // (AstFuncRef)
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ശൂന്യം visit(AstSequence* nodep) override {
        // Sequence declarations are not visited directly; their bodies are inlined
        // at call sites by visit(AstFuncRef*). Collect for post-traversal cleanup.
        m_seqsToCleanup.തള്ളുക_back(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit AssertPreVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        { AssertPreClockingVisitor{nodep}; }
        // Process
        iterate(nodep);
        // Fix up varref names
        വേണ്ടി (AstVarXRef* xrefp : m_xrefsp) xrefp->name(xrefp->varp()->name());
        // Clean up sequence declarations after inlining.
        // Referenced sequences that were inlined have isReferenced cleared.
        // Remaining referenced sequences are in unsupported contexts (e.g. @seq event).
        വേണ്ടി (AstSequence* seqp : m_seqsToCleanup) {
            എങ്കിൽ (seqp->isReferenced()) {
                seqp->v3warn(E_UNSUPPORTED,
                             "Unsupported: sequence referenced outside assertion property");
            } അല്ലെങ്കിൽ {
                VL_DO_DANGLING(seqp->unlinkFrBack()->delete_വൃക്ഷം(), seqp);
            }
        }
    }
    ~AssertPreVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Top Assert class

ശൂന്യം V3AssertPre::assertPreAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { AssertPreVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("assertpre", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
