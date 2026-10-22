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
    فراغ visit(AstNetlist* nodep) override { iterateAndNextConstNull(nodep->modulesp()); }
    فراغ visit(AstNodeModule* nodep) override { iterateAndNextConstNull(nodep->stmtsp()); }
    فراغ visit(AstGenBlock* nodep) override { iterateAndNextConstNull(nodep->itemsp()); }
    فراغ visit(AstClocking* nodep) override {
        لكل (AstNode* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            𐡄𐡍 (AstClockingItem* ثابت citemp = VN_CAST(itemp, ClockingItem)) {
                citemp->user3p(nodep);
            }
        }
    }
    // Clocking blocks are only in modules and their generate blocks
    فراغ visit(AstNode*) override {}

عام:
    // CONSTRUCTORS
    explicit AssertPreClockingVisitor(AstNetlist* nodep) { iterateConst(nodep); }
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
        AstClockingItem* itemp;  // Clocking item of the driven clockvar
        AstNodeExpr* refp;  // Reference to the driven clockvar
    };

    // NODE STATE
    // AstClockingItem::user1p()         // AstVar*.      varp() of ClockingItem after unlink
    // AstClockingItem::user2p()         // AstVar*.      Flag set by drives of output clockvar
    // AstClockingItem::user3p()         // AstClocking*. Clocking block of the item, captured
    //                                   //               by AssertPreClockingVisitor
    // AstPExpr::user1()                 // bool.         Created from AstUntil
    ثابت VNUser1InUse m_inuser1;
    ثابت VNUser2InUse m_inuser2;
    ثابت VNUser3InUse m_inuser3;
    // STATE
    // Current context:
    AstNetlist* ثابت m_netlistp = nullptr;  // Current netlist
    AstNodeModule* m_modp = nullptr;  // Current module
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
    AstNodeExpr* m_disablep = nullptr;  // Last disable
    AstIf* m_disableSeqIfp = nullptr;  // Used for handling disable iff in sequences
    AstPExpr* m_pexprp = nullptr;  // Last AstPExpr
    منطقي m_underCover = خطأ;  // True if the enclosing assertion is a cover
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
    منطقي m_inAssign = خطأ;  // True if in an AssignNode
    منطقي m_inAssignDlyLhs = خطأ;  // True if in AssignDly's LHS
    منطقي m_inSynchDrive = خطأ;  // True if in synchronous drive
    منطقي m_inConcatAssign = خطأ;  // True if in assignment to a concatenation
    std::vector<SynchDrive> m_drives;  // Clockvars written by the synchronous drive
    منطقي m_hasCycleDelay = خطأ;  // True if node has cycle delay beneath
    std::vector<AstVarXRef*> m_xrefsp;  // list of xrefs that need name fixup
    std::vector<AstSequence*> m_seqsToCleanup;  // Sequences to clean up after traversal

    // METHODS

    // Flag set by each drive of an output clockvar, added to the module with its clocking item
    AstVar* getCreateDrivenVarp(AstClockingItem* itemp) {
        𐡄𐡍 (!itemp->user2p()) {
            AstVar* ثابت varp = جديد AstVar{itemp->fileline(), VVarType::MODULETEMP,
                                            m_drivenNames.get(itemp), itemp->findBitDType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            itemp->user2p(varp);
        }
        أرجع VN_AS(itemp->user2p(), Var);
    }
    // Reference to a variable in the scope of a driven clockvar, made like the clockvar's
    ساكن AstNodeExpr* newDriveRefp(FileLine* flp, ثابت SynchDrive& drive, AstVar* varp,
                                     ثابت VAccess& access) {
        𐡄𐡍 (ثابت AstVarXRef* ثابت xrefp = VN_CAST(drive.refp, VarXRef)) {
            أرجع جديد AstVarXRef{flp, varp, xrefp->dotted(), access};
        } وإلا 𐡄𐡍 (ثابت AstMemberSel* ثابت selp = VN_CAST(drive.refp, MemberSel)) {
            // The interface expression is evaluated again, so it must not have side effects;
            // cloneTreePure warns if it has any
            AstMemberSel* ثابت newSelp
                = جديد AstMemberSel{flp, selp->fromp()->cloneTreePure(خطأ), varp};
            newSelp->access(access);
            أرجع newSelp;
        }
        أرجع جديد AstVarRef{flp, varp, access};
    }
    // Assignment setting the drive flag of a driven clockvar
    AstAssign* newDrivenSetp(FileLine* flp, ثابت SynchDrive& drive) {
        AstVar* ثابت varp = getCreateDrivenVarp(drive.itemp);
        AstAssign* ثابت setp = جديد AstAssign{flp, newDriveRefp(flp, drive, varp, VAccess::WRITE),
                                              جديد AstConst{flp, AstConst::BitTrue{}}};
        setp->user1(𐡒𐡔𐡈);
        أرجع setp;
    }
    // Clocking block of a clocking item
    ساكن AstClocking* clockingOf(ثابت AstClockingItem* itemp) {
        UASSERT_OBJ(itemp->user3p(), itemp, "Clocking item not captured in a clocking block");
        أرجع VN_AS(itemp->user3p(), Clocking);
    }
    // Clocking event of a driven clockvar, referenced from the drive like the clockvar, or
    // nullptr if unsupported, which is reported on the drive's cycle delay
    AstSenTree* newDriveSentreep(AstDelay* delayp, ثابت SynchDrive& drive) {
        AstSenTree* ثابت origp = clockingOf(drive.itemp)->sentreep();
        𐡄𐡍 (VN_IS(drive.refp, VarRef)) أرجع origp->cloneTree(خطأ);
        𐡄𐡍 (origp->exists([](ثابت AstVarXRef*) { أرجع 𐡒𐡔𐡈; })) {
            // The reference is relative to the clocking block
            delayp->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in synchronous drive to"
                                          " clockvar in another scope, whose clocking event"
                                          " has a hierarchical reference");
            أرجع nullptr;
        }
        ثابت AstMemberSel* ثابت selp = VN_CAST(drive.refp, MemberSel);
        // Processes outside classes evaluate their events also when not waiting on them,
        // possibly before the interface reference is set, so the event is guarded against a
        // null reference, which needs an integral value; scheduling does not guard it
        ثابت منطقي guardNull = selp && !VN_IS(m_modp, Class);
        𐡄𐡍 (guardNull) {
            لكل (AstSenItem* itemp = origp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                𐡄𐡍 (!itemp->sensp()->dtypep()->skipRefp()->isIntegralOrPacked()) {
                    delayp->v3warn(E_UNSUPPORTED,
                                   "Unsupported: cycle delay in synchronous drive through"
                                   " an interface reference outside a class, to clocking"
                                   " block with non-integral clocking event");
                    أرجع nullptr;
                }
            }
        }
        // Interface declaring the clocking block, if driven through an interface reference
        AstIface* ثابت ifacep
            = selp ? VN_AS(selp->fromp()->dtypep()->skipRefp(), IfaceRefDType)->ifaceViaCellp()
                   : nullptr;
        AstSenTree* ثابت sentreep = origp->cloneTree(خطأ);
        std::vector<AstVarRef*> refps;
        sentreep->foreach([&](AstVarRef* refp) {
            𐡄𐡍 (!refp->classOrPackagep()) refps.push_back(refp);
        });
        لكل (AstVarRef* ثابت refp : refps) {
            AstVar* ثابت varp = refp->varp();
            𐡄𐡍 (ifacep) {
                // Mark the variable as read through an interface reference, as V3Width does for
                // a member select. The mark is the interface declaring the variable, whose
                // instances all share it, so drives through references to any instances agree.
                UASSERT_OBJ(!varp->sensIfacep() || varp->sensIfacep() == ifacep, refp,
                            "Variable read through references to different interfaces");
                varp->sensIfacep(ifacep);
            }
            refp->replaceWith(newDriveRefp(refp->fileline(), drive, varp, refp->access()));
            VL_DO_DANGLING(pushDeletep(refp), refp);
        }
        𐡄𐡍 (guardNull) {
            لكل (AstSenItem* itemp = sentreep->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                guardNullIface(itemp, selp->fromp());
            }
        }
        أرجع sentreep;
    }
    // Make an integral clocking event through an interface reference false while the reference
    // is null
    ساكن فراغ guardNullIface(AstSenItem* sensesp, AstNodeExpr* fromp) {
        FileLine* ثابت flp = sensesp->fileline();
        AstNodeExpr* ثابت sensp = sensesp->sensp()->unlinkFrBack();
        sensesp->sensp(جديد AstCond{flp, newNotNullp(flp, fromp), sensp,
                                   جديد AstConst{flp, AstConst::DTyped{}, sensp->dtypep()}});
        𐡄𐡍 (AstNodeExpr* ثابت condp = sensesp->condp()) {
            condp->unlinkFrBack();
            sensesp->condp(جديد AstLogAnd{flp, newNotNullp(flp, fromp), condp});
        }
    }
    ساكن AstNodeExpr* newNotNullp(FileLine* flp, AstNodeExpr* fromp) {
        أرجع جديد AstNeq{flp, fromp->cloneTreePure(خطأ), جديد AstConst{flp, AstConst::Null{}}};
    }
    // Procedural '##0': if the default clocking event has not yet occurred in this time step,
    // wait for it; otherwise continue without suspension (IEEE 1800-2023 14.11)
    AstIf* newCycleDelayZerop(FileLine* flp) ثابت {
        AstVar* ثابت evtVarp = m_defaultClkEvtVarp;
        UASSERT_OBJ(evtVarp, flp, "Default clocking event var not pre-created");
        AstCMethodHard* ثابت isTriggeredp = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, evtVarp, VAccess::READ}, VCMethod::EVENT_IS_TRIGGERED};
        isTriggeredp->dtypeSetBit();
        AstEventControl* ثابت waitp = جديد AstEventControl{
            flp,
            جديد AstSenTree{flp, جديد AstSenItem{flp, VEdgeType::ET_EVENT,
                                               جديد AstVarRef{flp, evtVarp, VAccess::READ}}},
            nullptr};
        أرجع جديد AstIf{flp, جديد AstNot{flp, isTriggeredp}, waitp};
    }

    ساكن فراغ checkSamplingFuncDType(AstNodeExpr* nodep, ثابت AstNode* exprp) {
        ثابت AstNodeDType* ثابت dtypep = exprp->dtypep()->skipRefp();
        𐡄𐡍 (!dtypep->isIntegralOrPacked()) {
            nodep->v3error("Expected numeric type, but got a " << dtypep->prettyDTypeNameQ()
                                                               << " data type");
        }
    }

    AstSenTree* newSenTree(AstNode* nodep, AstSenTree* useTreep = nullptr,
                           AstNodeCoverOrAssert* cassertp = nullptr) {
        // Create sentree based on clocked or default clock
        // Return nullptr for always
        𐡄𐡍 (useTreep) أرجع useTreep;
        AstSenTree* newp = nullptr;
        AstSenItem* senip = m_senip;
        منطقي fromAlways = خطأ;
        𐡄𐡍 (!senip && m_defaultClockingp) senip = m_defaultClockingp->sentreep()->sensesp();
        𐡄𐡍 (!senip) {
            senip = m_seniAlwaysp;
            fromAlways = 𐡒𐡔𐡈;
        }
        𐡄𐡍 (!senip) {
            nodep->v3error("Concurrent assertion has no clock (IEEE 1800-2023 16.16)\n"
                           << nodep->warnMore()
                           << "... Suggest provide a clocking event, a default"
                              " clocking, or a clocked procedural context");
            newp = جديد AstSenTree{nodep->fileline(), nullptr};
        } وإلا {
            𐡄𐡍 (cassertp && fromAlways) cassertp->senFromAlways(𐡒𐡔𐡈);
            newp = جديد AstSenTree{nodep->fileline(), senip->cloneTree(𐡒𐡔𐡈)};
        }
        أرجع newp;
    }
    AstNodeExpr* getSequenceBodyExprp(ثابت AstSequence* ثابت seqp) ثابت {
        // The statements in AstSequence are optional AstVar (ports) followed by body expr.
        AstNode* bodyp = seqp->stmtsp();
        طالما (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        أرجع VN_CAST(bodyp, NodeExpr);
    }
    AstPropSpec* getPropertyExprp(ثابت AstProperty* ثابت propp) {
        // Statements in AstProperty: AstVar (ports/local vars),
        // AstInitialStaticStmt/AstInitialAutomaticStmt (var init), AstPropSpec (body).
        AstNode* propExprp = propp->stmtsp();
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
        AstNodeExpr* bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        // Clone the body expression since the sequence may be referenced multiple times
        AstNodeExpr* clonedp = bodyExprp->cloneTree(خطأ);
        // Build substitution map, then do a single traversal to replace all formals
        // (textual substitution per IEEE 16.8.2).
        ثابت V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<ثابت AstVar*, AstNodeExpr*> portMap;
        لكل (ثابت auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            ثابت auto it = portMap.find(refp->varp());
            𐡄𐡍 (it != portMap.end()) {
                refp->replaceWith(it->second->cloneTree(خطأ));
                VL_DO_DANGLING(pushDeletep(refp), refp);
            }
        });
        لكل (ثابت auto& tconnect : tconnects) {
            pushDeletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        // Replace the FuncRef with the inlined body
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(pushDeletep(funcrefp), funcrefp);
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
        propExprp = propExprp->cloneTree(خطأ);
        // Build substitution maps for formal arguments and property-local
        // variables, then perform a single foreach to apply all replacements.
        // Map port vars to their actual argument expressions
        ثابت V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propp->stmtsp());
        std::unordered_map<ثابت AstVar*, AstNodeExpr*> portMap;
        لكل (ثابت auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }

        // Promote property-local variables (non-port vars, IEEE 16.10) to
        // module-level __Vpropvar temps. Cross-cycle persistence is handled
        // by the match item lowering in visit(AstImplication*).
        std::unordered_map<ثابت AstVar*, AstVar*> localVarMap;
        لكل (AstNode* stmtp = propp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            𐡄𐡍 (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                𐡄𐡍 (!varp->isIO()) {
                    ثابت string newName = m_propVarNames.get(varp);
                    AstVar* ثابت newVarp = جديد AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->addStmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        // Single traversal: substitute ports and update local var references
        propExprp->foreach([&](AstVarRef* refp) {
            ثابت auto portIt = portMap.find(refp->varp());
            𐡄𐡍 (portIt != portMap.end()) {
                refp->replaceWith(portIt->second->cloneTree(خطأ));
                VL_DO_DANGLING(pushDeletep(refp), refp);
                أرجع;
            }
            ثابت auto localIt = localVarMap.find(refp->varp());
            𐡄𐡍 (localIt != localVarMap.end()) refp->varp(localIt->second);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        // Clean up argument expressions
        لكل (ثابت auto& tconnect : tconnects) {
            pushDeletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(propExprp, funcrefp);
        أرجع propExprp;
    }
    AstPropSpec* substitutePropertyCall(AstPropSpec* nodep) {
        𐡄𐡍 (AstFuncRef* ثابت funcrefp = VN_CAST(nodep->propp(), FuncRef)) {
            𐡄𐡍 (ثابت AstProperty* ثابت propp = VN_CAST(funcrefp->taskp(), Property)) {
                AstPropSpec* ثابت propExprp = clonePropertyBody(funcrefp, propp);

                // Handle case with 2 disable iff statement (IEEE 1800-2023 16.12.1)
                𐡄𐡍 (nodep->disablep() && propExprp->disablep()) {
                    nodep->v3error("disable iff expression before property call and in its "
                                   "body is not legal");
                    pushDeletep(propExprp->disablep()->unlinkFrBack());
                }
                // If disable iff is in outer property, move it to inner
                𐡄𐡍 (nodep->disablep()) {
                    AstNodeExpr* ثابت disablep = nodep->disablep()->unlinkFrBack();
                    propExprp->disablep(disablep);
                }

                𐡄𐡍 (nodep->sensesp() && propExprp->sensesp()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Clock event before property call and in its body");
                    pushDeletep(propExprp->sensesp()->unlinkFrBack());
                }
                // If clock event is in outer property, move it to inner
                𐡄𐡍 (nodep->sensesp()) {
                    AstSenItem* ثابت sensesp = nodep->sensesp();
                    sensesp->unlinkFrBack();
                    propExprp->sensesp(sensesp);
                }

                // Now substitute property reference with property body
                nodep->replaceWith(propExprp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
                أرجع propExprp;
            }
        }
        أرجع nodep;
    }

    فراغ substituteNestedPropertyCall(AstFuncRef* funcrefp, ثابت AstProperty* propp) {
        AstPropSpec* ثابت specp = clonePropertyBody(funcrefp, propp);
        AstNode* replp = nullptr;
        𐡄𐡍 (specp->disablep()) {
            funcrefp->v3error("Illegal 'disable iff' in a nested property instance"
                              " (IEEE 1800-2023 16.12.1)");
        } وإلا 𐡄𐡍 (specp->sensesp()) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: clock event in a nested property instance");
        } وإلا 𐡄𐡍 (specp->propp()->exists(
                       [](ثابت AstNodeExpr* np) { أرجع np->isMultiCycleSva(); })) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: multi-cycle body in a nested property instance");
        } وإلا {
            replp = specp->propp()->unlinkFrBack();
        }
        𐡄𐡍 (!replp) replp = جديد AstConst{funcrefp->fileline(), AstConst::BitFalse{}};
        funcrefp->replaceWith(replp);
        VL_DO_DANGLING(pushDeletep(funcrefp), funcrefp);
        VL_DO_DANGLING(pushDeletep(specp), specp);
    }

    // VISITORS
    //========== Statements
    فراغ visit(AstClocking* ثابت nodep) override {
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        UINFO(8, "   CLOCKING" << nodep);
        𐡄𐡍 (nodep->isDefault()) {
            𐡄𐡍 (m_defaultClockingp) {
                nodep->v3error("Only one default clocking block allowed per module"
                               " (IEEE 1800-2023 14.12)");
            } وإلا {
                m_defaultClockingp = nodep;
                m_defaultClkEvtVarp = nodep->ensureEventp();
            }
        }
        iterateChildren(nodep);
        𐡄𐡍 (nodep->eventp()) nodep->addNextHere(nodep->eventp()->unlinkFrBack());
        VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
    }
    فراغ visit(AstModportClockingRef* ثابت nodep) override {
        // It has to be converted to a list of ModportClockingVarRefs,
        // because clocking blocks are removed in this pass
        لكل (AstNode* itemp = nodep->clockingp()->itemsp(); itemp; itemp = itemp->nextp()) {
            𐡄𐡍 (AstClockingItem* ثابت citemp = VN_CAST(itemp, ClockingItem)) {
                𐡄𐡍 (AstVar* ثابت varp
                    = citemp->varp() ? citemp->varp() : VN_AS(citemp->user1p(), Var)) {
                    AstModportVarRef* ثابت modVarp = جديد AstModportVarRef{
                        nodep->fileline(), varp->name(), citemp->direction()};
                    modVarp->varp(varp);
                    nodep->addNextHere(modVarp);
                    𐡄𐡍 (citemp->direction().isOutput()) {
                        // Drives through the modport set the drive flag
                        AstVar* ثابت drivenVarp = getCreateDrivenVarp(citemp);
                        AstModportVarRef* ثابت drivenRefp = جديد AstModportVarRef{
                            nodep->fileline(), drivenVarp->name(), VDirection::OUTPUT};
                        drivenRefp->varp(drivenVarp);
                        nodep->addNextHere(drivenRefp);
                    }
                }
            }
        }
        VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
    }
    فراغ visit(AstClockingItem* ثابت nodep) override {
        // Get a ref to the sampled/driven variable
        AstVar* ثابت varp = nodep->varp();
        𐡄𐡍 (!varp) {
            // Unused item
            أرجع;
        }
        // Flag set by drives of an output clockvar, possibly visited before this item
        𐡄𐡍 (nodep->direction().isOutput()) m_modp->addStmtsp(getCreateDrivenVarp(nodep));
        FileLine* ثابت flp = nodep->fileline();
        V3Const::constifyEdit(nodep->skewp());
        𐡄𐡍 (!VN_IS(nodep->skewp(), Const)) {
            nodep->skewp()->v3error("Skew must be constant (IEEE 1800-2023 14.4)");
            أرجع;
        }
        AstConst* ثابت skewp = VN_AS(nodep->skewp(), Const);
        𐡄𐡍 (skewp->num().isNegative()) skewp->v3error("Skew cannot be negative");
        AstNodeExpr* ثابت exprp = nodep->exprp();
        varp->name(m_clockingp->name() + "__DOT__" + varp->name());
        m_clockingp->addNextHere(varp->unlinkFrBack());
        nodep->user1p(varp);
        varp->user1p(nodep);
        𐡄𐡍 (nodep->direction() == VDirection::OUTPUT) {
            exprp->foreach([](ثابت AstNodeVarRef* varrefp) {
                // Prevent confusing BLKANDNBLK warnings on clockvars due to generated assignments
                varrefp->fileline()->warnOff(V3ErrorCode::BLKANDNBLK, 𐡒𐡔𐡈);
                // Mark the driven signal so later passes (e.g. V3Inst port
                // connection) know the drive flows out from inside the cell.
                // Item expressions are plain signal references, so only the
                // driven signal is marked, never index selects.
                varrefp->varp()->isClockingDriven(𐡒𐡔𐡈);
            });
            AstVarRef* ثابت skewedReadRefp = جديد AstVarRef{flp, varp, VAccess::READ};
            skewedReadRefp->user1(𐡒𐡔𐡈);
            // Initialize the clockvar
            AstVarRef* ثابت skewedWriteRefp = جديد AstVarRef{flp, varp, VAccess::WRITE};
            skewedWriteRefp->user1(𐡒𐡔𐡈);
            AstInitialStatic* ثابت initClockvarp = جديد AstInitialStatic{
                flp, جديد AstAssign{flp, skewedWriteRefp, exprp->cloneTreePure(خطأ)}};
            m_modp->addStmtsp(initClockvarp);
            // Each drive sets a flag, so the signal is also assigned when driven with the same
            // value again, e.g. after another assignment to the signal (IEEE 1800-2023 14.16.2)
            AstVar* ثابت drivenVarp = getCreateDrivenVarp(nodep);
            m_modp->addStmtsp(جديد AstInitialStatic{
                flp, جديد AstAssign{flp, جديد AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                   جديد AstConst{flp, AstConst::BitFalse{}}}});
            // Assign the clockvar to the actual var if it was driven
            AstAssign* ثابت assignp
                = جديد AstAssign{flp, exprp->cloneTreePure(خطأ), skewedReadRefp};
            AstIf* ثابت ifp
                = جديد AstIf{flp, جديد AstVarRef{flp, drivenVarp, VAccess::READ},
                            جديد AstAssign{flp, جديد AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                          جديد AstConst{flp, AstConst::BitFalse{}}}};
            ifp->addThensp(assignp);
            // Only in the time step of the clocking event
            AstCMethodHard* ثابت trigp = جديد AstCMethodHard{
                nodep->fileline(), جديد AstVarRef{flp, m_clockingp->ensureEventp(), VAccess::READ},
                VCMethod::EVENT_IS_TRIGGERED};
            trigp->dtypeSetBit();
            ifp->condp(جديد AstLogAnd{flp, ifp->condp()->unlinkFrBack(), trigp});
            // A skew rounding to no time precision ticks is a zero skew
            AstNodeExpr* ثابت ticksp
                = V3Timing::delayToTicks(skewp->cloneTree(خطأ), m_modp->timeunit());
            ثابت منطقي zeroSkew = ticksp->isZero();
            VL_DO_DANGLING(ticksp->deleteTree(), ticksp);
            𐡄𐡍 (zeroSkew || !skewp->fileline()->timingOn()) {
                // Drive the var in Re-NBA (IEEE 1800-2023 14.16), ignoring skews where timing is
                // turned off
                AstSenTree* senTreep = m_clockingp->sentreep()->cloneTree(خطأ);
                senTreep->addSensesp(جديد AstSenItem{
                    flp, VEdgeType::ET_CHANGED, جديد AstVarRef{flp, drivenVarp, VAccess::READ}});
                m_clockingp->addNextHere(جديد AstAlwaysReactive{flp, senTreep, ifp});
            } وإلا {
                // Fork only the assignment, so this AlwaysPostponed can be retriggered
                // before the assignment happens
                AstFork* ثابت forkp = جديد AstFork{flp, VJoinType::JOIN_NONE};
                forkp->immediateStart(𐡒𐡔𐡈);
                forkp->addForksp(جديد AstBegin{flp, "", assignp->unlinkFrBack(), 𐡒𐡔𐡈});
                ifp->addThensp(forkp);
                // Read the clockvar in Postponed, so a drive late in the time step is applied
                // at this event plus the skew (IEEE 1800-2023 14.16)
                m_clockingp->addNextHere(جديد AstAlwaysPostponed{flp, ifp});
                𐡄𐡍 (v3Global.opt.timing().isSetTrue()) {
                    AstDelay* ثابت delayp = جديد AstDelay{flp, skewp->unlinkFrBack(), خطأ};
                    delayp->timeunit(m_modp->timeunit());
                    assignp->timingControlp(delayp);
                } وإلا 𐡄𐡍 (v3Global.opt.timing().isSetFalse()) {
                    nodep->v3warn(E_NOTIMING,
                                  "Clocking output skew greater than #0 requires --timing");
                } وإلا {
                    nodep->v3warn(E_NEEDTIMINGOPT,
                                  "Use --timing or --no-timing to specify how "
                                  "clocking output skew greater than #0 should be handled");
                }
            }
        } وإلا 𐡄𐡍 (nodep->direction() == VDirection::INPUT) {
            // Ref to the clockvar
            AstVarRef* ثابت refp = جديد AstVarRef{flp, varp, VAccess::WRITE};
            refp->user1(𐡒𐡔𐡈);
            𐡄𐡍 (skewp->num().is1Step()) {
                // #1step means the value that is sampled is always the signal's last value
                // before the clock edge (IEEE 1800-2023 14.4)
                AstSampled* ثابت sampledp
                    = جديد AstSampled{flp, exprp->cloneTreePure(خطأ), exprp->dtypep(), 𐡒𐡔𐡈};
                AstAssign* ثابت assignp = جديد AstAssign{flp, refp, sampledp};
                m_clockingp->addNextHere(جديد AstAlways{
                    flp, VAlwaysKwd::ALWAYS, m_clockingp->sentreep()->cloneTree(خطأ), assignp});
            } وإلا 𐡄𐡍 (skewp->isZero()) {
                // #0 means the var has to be sampled in Observed (IEEE 1800-2023 14.13)
                AstAssign* ثابت assignp = جديد AstAssign{flp, refp, exprp->cloneTreePure(خطأ)};
                m_clockingp->addNextHere(جديد AstAlwaysObserved{
                    flp, m_clockingp->sentreep()->cloneTree(خطأ), assignp});
            } وإلا {
                // Create a queue where we'll store sampled values with timestamps
                AstSampleQueueDType* ثابت queueDtp
                    = جديد AstSampleQueueDType{flp, exprp->dtypep()};
                m_netlistp->typeTablep()->addTypesp(queueDtp);
                AstVar* ثابت queueVarp
                    = جديد AstVar{flp, VVarType::MODULETEMP, "__Vqueue__" + varp->name(), queueDtp};
                queueVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_clockingp->addNextHere(queueVarp);
                // Create a process like this:
                //     always queue.push(<sampled var>);
                AstCMethodHard* ثابت pushp = جديد AstCMethodHard{
                    flp, جديد AstVarRef{flp, queueVarp, VAccess::WRITE}, VCMethod::DYN_PUSH,
                    جديد AstTime{nodep->fileline(), m_modp->timeunit()}};
                pushp->addPinsp(exprp->cloneTreePure(خطأ));
                pushp->dtypeSetVoid();
                m_clockingp->addNextHere(
                    جديد AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, pushp->makeStmt()});
                // Create a process like this:
                //     always @<clocking event> queue.pop(<skew>, /*out*/<skewed var>);
                AstCMethodHard* ثابت popp = جديد AstCMethodHard{
                    flp, جديد AstVarRef{flp, queueVarp, VAccess::READWRITE}, VCMethod::DYN_POP,
                    جديد AstTime{nodep->fileline(), m_modp->timeunit()}};
                popp->addPinsp(skewp->unlinkFrBack());
                refp->access(VAccess::READWRITE);  // Only conditionally assigned
                popp->addPinsp(refp);
                popp->dtypeSetVoid();
                m_clockingp->addNextHere(جديد AstAlways{flp, VAlwaysKwd::ALWAYS,
                                                       m_clockingp->sentreep()->cloneTree(خطأ),
                                                       popp->makeStmt()});
            }
        } وإلا {
            nodep->v3fatalSrc("Invalid direction");
        }
    }
    فراغ visit(AstDelay* nodep) override {
        m_hasCycleDelay |= nodep->isCycleDelay();
        // Only cycle delays are relevant in this stage; also only process once
        𐡄𐡍 (!nodep->isCycleDelay()) {
            𐡄𐡍 (m_inSynchDrive) {
                nodep->v3error("Only cycle delays can be used in synchronous drives"
                               " (IEEE 1800-2023 14.16)");
            }
            iterateChildren(nodep);
            أرجع;
        }
        𐡄𐡍 (m_inAssign && !m_inSynchDrive) {
            nodep->v3error("Cycle delays not allowed as intra-assignment delays"
                           " (IEEE 1800-2023 14.11)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
            أرجع;
        }
        𐡄𐡍 (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBackWithNext());
        FileLine* ثابت flp = nodep->fileline();
        AstNodeExpr* valuep = V3Const::constifyEdit(nodep->lhsp()->unlinkFrBack());
        ثابت AstConst* ثابت constp = VN_CAST(valuep, Const);
        𐡄𐡍 (!constp && m_pexprp) {
            // V3AssertNfa handles non-const delays in sequences before this pass and
            // replaces the property; this branch should never be reached.
            nodep->v3fatalSrc("Non-constant cycle delay in assertion: "
                              "should have been caught by V3AssertNfa");
        } وإلا 𐡄𐡍 (constp && constp->isZero()) {
            VL_DO_DANGLING(pushDeletep(valuep), valuep);
            𐡄𐡍 (m_inSynchDrive) {
                // ##0 has no effect in synchronous drives (IEEE 1800-2023 14.11)
                VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
                أرجع;
            }
            𐡄𐡍 (m_pexprp) {
                // ##0 in sequence context = zero delay = same clock tick
                VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
                أرجع;
            }
            // Procedural ##0: synchronize with default clocking event (IEEE 1800-2023 14.11)
            𐡄𐡍 (!m_defaultClockingp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
                أرجع;
            }
            nodep->replaceWith(newCycleDelayZerop(flp));
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            أرجع;
        }
        AstSenTree* sentreep = nullptr;
        𐡄𐡍 (!m_drives.empty()) {
            𐡄𐡍 (m_inConcatAssign) {
                // Also the clockvars may be of different clocking blocks
                nodep->v3error("Cycle delays cannot be used in synchronous drives to a"
                               " concatenation (IEEE 1800-2023 14.16)");
                VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(pushDeletep(valuep), valuep);
                أرجع;
            }
            // Count the cycles of the driven clockvar's clocking block (IEEE 1800-2023 14.16)
            sentreep = newDriveSentreep(nodep, m_drives.front());
            𐡄𐡍 (!sentreep) {
                VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(pushDeletep(valuep), valuep);
                أرجع;
            }
        } وإلا 𐡄𐡍 (!m_defaultClockingp) {
            𐡄𐡍 (!m_pexprp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(pushDeletep(valuep), valuep);
                أرجع;
            }
            sentreep = جديد AstSenTree{flp, m_senip->cloneTree(خطأ)};
        } وإلا {
            sentreep = m_defaultClockingp->sentreep()->cloneTree(خطأ);
        }
        AstEventControl* ثابت controlp
            = جديد AstEventControl{nodep->fileline(), sentreep, nullptr};
        ثابت std::string delayName = m_cycleDlyNames.get(nodep);
        AstNodeExpr* throughoutp
            = nodep->throughoutp() ? nodep->throughoutp()->unlinkFrBack() : nullptr;

        AstVar* ثابت cntVarp = جديد AstVar{flp, VVarType::BLOCKTEMP, delayName + "__counter",
                                           nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstBegin* ثابت beginp = جديد AstBegin{flp, delayName + "__block", cntVarp, 𐡒𐡔𐡈};
        // A non-constant delay is counted at runtime. The count is integral of 64 bits, as
        // V3Width makes delays, or real, which is rounded (IEEE 1800-2023 6.12.1)
        𐡄𐡍 (valuep->isDouble()) {
            valuep = جديد AstRToIRoundS{flp, valuep};
            valuep->dtypeFrom(cntVarp);
        } وإلا {
            valuep = جديد AstSel{flp, valuep, 0, cntVarp->width()};
        }
        beginp->addStmtsp(جديد AstAssign{flp, جديد AstVarRef{flp, cntVarp, VAccess::WRITE}, valuep});
        // If the count is zero, a synchronous drive waits for none, like with a constant '##0',
        // and a procedural delay waits like a procedural '##0' (IEEE 1800-2023 14.11)
        𐡄𐡍 (!constp && !m_inSynchDrive) {
            beginp->addStmtsp(جديد AstIf{
                flp,
                جديد AstEq{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ}, جديد AstConst{flp, 0}},
                newCycleDelayZerop(flp)});
        }

        // Throughout: create flag tracking whether condition held every tick
        AstVar* throughoutOkp = nullptr;
        𐡄𐡍 (throughoutp) {
            throughoutOkp = جديد AstVar{flp, VVarType::BLOCKTEMP, delayName + "__throughoutOk",
                                       nodep->findBasicDType(VBasicDTypeKwd::LOGIC_IMPLICIT)};
            throughoutOkp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->addStmtsp(throughoutOkp);
            beginp->addStmtsp(جديد AstAssign{flp, جديد AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            جديد AstConst{flp, AstConst::BitTrue{}}});
            // Check condition at tick 0 (sequence start, before entering loop)
            beginp->addStmtsp(
                جديد AstIf{flp, جديد AstLogNot{flp, throughoutp->cloneTreePure(خطأ)},
                          جديد AstAssign{flp, جديد AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                        جديد AstConst{flp, AstConst::BitFalse{}}}});
        }

        {
            AstLoop* ثابت loopp = جديد AstLoop{flp};
            // When throughout is present, exit loop early if condition fails
            AstNodeExpr* loopCondp
                = جديد AstGt{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ}, جديد AstConst{flp, 0}};
            𐡄𐡍 (throughoutOkp) {
                loopCondp = جديد AstLogAnd{flp, loopCondp,
                                          جديد AstVarRef{flp, throughoutOkp, VAccess::READ}};
            }
            loopp->addStmtsp(جديد AstLoopTest{flp, loopp, loopCondp});
            loopp->addStmtsp(controlp);
            loopp->addStmtsp(
                جديد AstAssign{flp, جديد AstVarRef{flp, cntVarp, VAccess::WRITE},
                              جديد AstSub{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ},
                                         جديد AstConst{flp, 1}}});
            // Check throughout condition at each tick during delay (IEEE 1800-2023 16.9.9)
            𐡄𐡍 (throughoutp) {
                loopp->addStmtsp(
                    جديد AstIf{flp, جديد AstLogNot{flp, throughoutp},
                              جديد AstAssign{flp, جديد AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            جديد AstConst{flp, AstConst::BitFalse{}}}});
            }
            beginp->addStmtsp(loopp);
        }
        // Compose wrappers on remaining sequence: throughout gate (inner), disable iff (outer)
        AstNode* remainp = nodep->nextp() ? nodep->nextp()->unlinkFrBackWithNext() : nullptr;
        𐡄𐡍 (throughoutOkp) {
            // If condition failed during delay, fail assertion
            remainp = جديد AstIf{flp, جديد AstVarRef{flp, throughoutOkp, VAccess::READ}, remainp,
                                جديد AstPExprClause{flp, /*pass=*/خطأ}};
        }
        𐡄𐡍 (m_disableSeqIfp && remainp) {
            AstIf* ثابت disableSeqIfp = m_disableSeqIfp->cloneTree(خطأ);
            disableSeqIfp->addThensp(remainp);
            remainp = disableSeqIfp;
        }
        𐡄𐡍 (remainp) {
            𐡄𐡍 (throughoutOkp) {
                // throughoutOkp is declared in beginp scope -- check must be inside it
                beginp->addStmtsp(remainp);
            } وإلا {
                nodep->addNextHere(remainp);
            }
        }
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->deleteTree(), nodep);
    }
    فراغ visit(AstSenTree* nodep) override {
        𐡄𐡍 (m_inSynchDrive) {
            nodep->v3error("Event controls cannot be used in "
                           "synchronous drives (IEEE 1800-2023 14.16)");
        }

        ثابت AstSampled* sampledp;
        𐡄𐡍 (nodep->exists([&sampledp](ثابت AstSampled* ثابت sp) {
                sampledp = sp;
                أرجع 𐡒𐡔𐡈;
            })) {
            sampledp->v3warn(E_UNSUPPORTED, "Unsupported: $sampled inside sensitivity list");
        }
    }
    فراغ visit(AstNodeVarRef* nodep) override {
        UINFO(8, " -varref:  " << nodep);
        UINFO(8, " -varref-var-back:  " << nodep->varp()->backp());
        UINFO(8, " -varref-var-user1:  " << nodep->varp()->user1p());
        𐡄𐡍 (AstClockingItem* ثابت itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->firstAbovep(),
                ClockingItem)) {
            𐡄𐡍 (nodep->user1()) أرجع;

            // ensure linking still works, this has to be done only once
            𐡄𐡍 (AstVarXRef* xrefp = VN_CAST(nodep, VarXRef)) {
                UINFO(8, " -clockvarxref-in:  " << xrefp);
                string dotted = xrefp->dotted();
                ثابت size_t dotPos = dotted.rfind('.');
                dotted.erase(dotPos, string::npos);
                xrefp->dotted(dotted);
                UINFO(8, " -clockvarxref-out: " << xrefp);
                m_xrefsp.emplace_back(xrefp);
            }

            𐡄𐡍 (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            𐡄𐡍 (nodep->access().isWriteOrRW()) {
                𐡄𐡍 (itemp->direction() == VDirection::OUTPUT) {
                    𐡄𐡍 (!m_inAssignDlyLhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    𐡄𐡍 (m_inAssign) {
                        m_inSynchDrive = 𐡒𐡔𐡈;
                        m_drives.push_back({itemp, nodep});
                    }
                } وإلا 𐡄𐡍 (itemp->direction() == VDirection::INPUT) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
            nodep->user1(𐡒𐡔𐡈);
        }
    }
    فراغ visit(AstMemberSel* nodep) override {
        𐡄𐡍 (AstClockingItem* ثابت itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->firstAbovep(),
                ClockingItem)) {
            𐡄𐡍 (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            𐡄𐡍 (nodep->access().isWriteOrRW()) {
                𐡄𐡍 (itemp->direction() == VDirection::OUTPUT) {
                    𐡄𐡍 (!m_inAssignDlyLhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    𐡄𐡍 (m_inAssign) {
                        m_inSynchDrive = 𐡒𐡔𐡈;
                        m_drives.push_back({itemp, nodep});
                    }
                } وإلا 𐡄𐡍 (itemp->direction() == VDirection::INPUT) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
        }
    }
    فراغ visit(AstNodeAssign* nodep) override {
        𐡄𐡍 (nodep->user1()) أرجع;
        VL_RESTORER(m_inAssign);
        VL_RESTORER(m_inSynchDrive);
        VL_RESTORER(m_inConcatAssign);
        VL_RESTORER_CLEAR(m_drives);
        m_inAssign = 𐡒𐡔𐡈;
        m_inSynchDrive = خطأ;
        {
            // The braces of a concatenation are a replication, by one
            ثابت AstNodeExpr* lhsp = nodep->lhsp();
            𐡄𐡍 (ثابت AstReplicate* ثابت repp = VN_CAST(lhsp, Replicate)) lhsp = repp->srcp();
            m_inConcatAssign = VN_IS(lhsp, Concat);
        }
        {
            VL_RESTORER(m_inAssignDlyLhs);
            m_inAssignDlyLhs = VN_IS(nodep, AssignDly);
            iterate(nodep->lhsp());
        }
        iterate(nodep->rhsp());
        𐡄𐡍 (nodep->timingControlp()) iterate(nodep->timingControlp());
        // A drive without a cycle delay, also after removing '##0', updates the clockvar now
        𐡄𐡍 (m_inSynchDrive && !nodep->timingControlp()) {
            AstAssign* ثابت assignp = جديد AstAssign{
                nodep->fileline(), nodep->lhsp()->unlinkFrBack(), nodep->rhsp()->unlinkFrBack()};
            assignp->user1(𐡒𐡔𐡈);
            nodep->replaceWith(assignp);
            VL_DO_DANGLING(nodep->deleteTree(), nodep);
            nodep = assignp;
        }
        // Note the drives when they take effect, after the cycle delay if any. Each clockvar
        // of the LHS is driven, e.g. in a concatenation which other simulators accept, although
        // IEEE 1800-2023 14.16 does not allow it
        AstNode* setsp = nullptr;
        لكل (ثابت SynchDrive& drive : m_drives) {
            setsp = AstNode::addNext(setsp, newDrivenSetp(nodep->fileline(), drive));
        }
        𐡄𐡍 (!setsp) أرجع;
        𐡄𐡍 (AstBegin* ثابت beginp = VN_CAST(nodep->timingControlp(), Begin)) {
            beginp->addStmtsp(setsp);
        } وإلا {
            nodep->addNextHere(setsp);
        }
    }
    فراغ visit(AstAlways* nodep) override {
        iterateAndNextNull(nodep->sentreep());
        𐡄𐡍 (nodep->sentreep()) m_seniAlwaysp = nodep->sentreep()->sensesp();
        iterateAndNextNull(nodep->stmtsp());
        m_seniAlwaysp = nullptr;
    }

    فراغ visit(AstNodeCoverOrAssert* nodep) override {
        𐡄𐡍 (nodep->sentreep()) أرجع;  // Already processed

        VL_RESTORER(m_senip);
        VL_RESTORER(m_disablep);
        m_senip = nullptr;
        m_disablep = nullptr;

        // Find Clocking's buried under nodep->exprsp
        iterateChildren(nodep);
        𐡄𐡍 (!nodep->immediate()) nodep->sentreep(newSenTree(nodep, nullptr, nodep));
    }
    فراغ visit(AstFalling* nodep) override {
        𐡄𐡍 (nodep->user1SetOnce()) أرجع;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        FileLine* ثابت fl = nodep->fileline();
        AstNodeExpr* exprp = nodep->exprp()->unlinkFrBack();
        𐡄𐡍 (exprp->width() > 1) exprp = جديد AstSel{fl, exprp, 0, 1};
        AstNodeExpr* ثابت futurep = جديد AstFuture{fl, exprp, newSenTree(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = جديد AstAnd{fl, exprp->cloneTreePure(خطأ), جديد AstNot{fl, futurep}};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    فراغ visit(AstFell* nodep) override {
        𐡄𐡍 (nodep->user1SetOnce()) أرجع;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        FileLine* ثابت fl = nodep->fileline();
        AstNodeExpr* exprp = nodep->exprp()->unlinkFrBack();
        𐡄𐡍 (exprp->width() > 1) exprp = جديد AstSel{fl, exprp, 0, 1};
        AstSenTree* sentreep = nodep->sentreep();
        𐡄𐡍 (sentreep) sentreep->unlinkFrBack();
        AstPast* ثابت pastp = جديد AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(newSenTree(nodep, sentreep));
        exprp = جديد AstAnd{fl, pastp, جديد AstNot{fl, exprp->cloneTreePure(خطأ)}};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    فراغ visit(AstFuture* nodep) override {
        𐡄𐡍 (nodep->user1SetOnce()) أرجع;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        AstSenTree* ثابت sentreep = nodep->sentreep();
        𐡄𐡍 (sentreep) VL_DO_DANGLING(pushDeletep(sentreep->unlinkFrBack()), sentreep);
        nodep->sentreep(newSenTree(nodep));
    }
    فراغ visit(AstPast* nodep) override {
        checkSamplingFuncDType(nodep, nodep->exprp());
        𐡄𐡍 (nodep->sentreep()) أرجع;  // Already processed
        iterateChildren(nodep);
        nodep->sentreep(newSenTree(nodep));
    }
    فراغ visit(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- Lower standalone exact [*N] (N >= 2) via saturating counter.
        // Range/unbounded forms and SExpr-contained forms are lowered by V3AssertNfa.
        iterateChildren(nodep);
        // V3AssertNfa handles unbounded/ranged forms upstream, so this fast-path
        // is effectively unreachable when NFA is enabled.
        𐡄𐡍 (nodep->unbounded() || nodep->maxCountp()) أرجع;  // LCOV_EXCL_LINE
        ثابت AstConst* ثابت constp = VN_CAST(nodep->countp(), Const);
        𐡄𐡍 (VL_UNLIKELY(!constp || constp->toSInt() < 1)) {
            nodep->v3fatalSrc("Consecutive repetition count must be a positive constant"
                              " (should have been caught by V3Width)");
            أرجع;
        }
        ثابت صحيح n = constp->toSInt();
        FileLine* ثابت flp = nodep->fileline();
        AstNodeExpr* ثابت exprp = nodep->exprp()->unlinkFrBack();
        𐡄𐡍 (n == 1) {
            nodep->replaceWith(exprp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            أرجع;
        }
        // Saturating counter: if (expr) cnt <= min(cnt+1, N); else cnt <= 0;
        AstVar* ثابت cntVarp = جديد AstVar{flp, VVarType::MODULETEMP, m_consRepNames.get(""),
                                           nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(cntVarp);
        AstNodeExpr* ثابت exprClonep = exprp->cloneTreePure(خطأ);
        AstNodeExpr* ثابت saturatingIncrp = جديد AstCond{
            flp,
            جديد AstLt{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ},
                      جديد AstConst{flp, static_cast<uint32_t>(n)}},
            جديد AstAdd{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ}, جديد AstConst{flp, 1u}},
            جديد AstConst{flp, static_cast<uint32_t>(n)}};
        AstAssignDly* ثابت incrAssignp
            = جديد AstAssignDly{flp, جديد AstVarRef{flp, cntVarp, VAccess::WRITE}, saturatingIncrp};
        AstAssignDly* ثابت resetAssignp = جديد AstAssignDly{
            flp, جديد AstVarRef{flp, cntVarp, VAccess::WRITE}, جديد AstConst{flp, 0u}};
        AstIf* ثابت ifp = جديد AstIf{flp, exprClonep, incrAssignp, resetAssignp};
        AstSenTree* ثابت senTreep = newSenTree(nodep);
        AstAlways* ثابت alwaysp = جديد AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, ifp};
        cntVarp->addNextHere(alwaysp);
        // Match: cnt >= N-1 (previous cycles via NBA) && expr (current cycle)
        AstNodeExpr* ثابت cntCheckp = جديد AstGte{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ},
                                                  جديد AstConst{flp, static_cast<uint32_t>(n - 1)}};
        cntCheckp->dtypeSetBit();
        AstNodeExpr* ثابت matchp = جديد AstAnd{flp, cntCheckp, exprp};
        matchp->dtypeSetBit();
        nodep->replaceWith(matchp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    فراغ visit(AstRising* nodep) override {
        𐡄𐡍 (nodep->user1SetOnce()) أرجع;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        FileLine* ثابت fl = nodep->fileline();
        AstNodeExpr* exprp = nodep->exprp()->unlinkFrBack();
        𐡄𐡍 (exprp->width() > 1) exprp = جديد AstSel{fl, exprp, 0, 1};
        AstNodeExpr* ثابت futurep = جديد AstFuture{fl, exprp, newSenTree(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = جديد AstAnd{fl, جديد AstNot{fl, exprp->cloneTreePure(خطأ)}, futurep};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    فراغ visit(AstRose* nodep) override {
        𐡄𐡍 (nodep->user1SetOnce()) أرجع;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        FileLine* ثابت fl = nodep->fileline();
        AstNodeExpr* exprp = nodep->exprp()->unlinkFrBack();
        𐡄𐡍 (exprp->width() > 1) exprp = جديد AstSel{fl, exprp, 0, 1};
        AstSenTree* sentreep = nodep->sentreep();
        𐡄𐡍 (sentreep) sentreep->unlinkFrBack();
        AstPast* ثابت pastp = جديد AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(newSenTree(nodep, sentreep));
        exprp = جديد AstAnd{fl, جديد AstNot{fl, pastp}, exprp->cloneTreePure(خطأ)};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    ساكن AstAssocArrayDType* getProcessAssocArrayType(FileLine* ثابت flp) {
        // Type of __VassertsActive___x[std::process]
        AstNodeDType* valp
            = v3Global.rootp()->typeTablep()->findBasicDType(flp, VBasicDTypeKwd::BIT);
        AstClassRefDType* keyp
            = جديد AstClassRefDType{flp, v3Global.rootp()->stdPackageProcessp(), nullptr};
        keyp->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        v3Global.rootp()->typeTablep()->addTypesp(keyp);
        AstAssocArrayDType* ثابت typep = جديد AstAssocArrayDType{flp, valp, keyp};
        typep->dtypep(typep);
        v3Global.rootp()->typeTablep()->addTypesp(typep);
        أرجع typep;
    }
    ساكن AstStmtExpr* getProcessAssocArrayDelete(AstVarRef* ثابت refp) {
        // Constructs refp.delete(std::process::self()) statement
        FileLine* ثابت flp = refp->fileline();
        AstCMethodHard* ثابت deletep = جديد AstCMethodHard{
            flp, refp, VCMethod::ASSOC_ERASE, v3Global.rootp()->stdPackageProcessSelfp(flp)};
        deletep->dtypep(refp->findVoidDType());
        أرجع جديد AstStmtExpr{flp, deletep};
    }
    ساكن AstNodeExpr* getProcessAssocArraySize(AstVarRef* ثابت refp) {
        // Constructs refp.size() statement
        AstCMethodHard* ثابت sizep
            = جديد AstCMethodHard{refp->fileline(), refp, VCMethod::ASSOC_SIZE};
        sizep->dtypep(refp->findBasicDType(VBasicDTypeKwd::UINT32));
        أرجع sizep;
    }
    فراغ visit(AstSEventually* nodep) override {
        UASSERT(v3Global.rootp()->stdPackagep(), "Should be imported");

        AstSenTree* ثابت sentreep = newSenTree(nodep);
        𐡄𐡍 (!sentreep->sensesp()) {
            VL_DO_DANGLING(pushDeletep(sentreep), sentreep);
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            أرجع;
        }

        FileLine* ثابت flp = nodep->fileline();

        // Track active assertions
        AstVar* ثابت activep = جديد AstVar{flp, VVarType::MODULETEMP, m_activeNames.get(""),
                                           getProcessAssocArrayType(flp)};
        activep->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(activep);

        // Assertion condition check
        AstLoop* ثابت loopp = جديد AstLoop{flp};
        AstSampled* ثابت condp
            = جديد AstSampled{flp, nodep->exprp()->unlinkFrBack(), nullptr, 𐡒𐡔𐡈};
        loopp->addStmtsp(جديد AstLoopTest{flp, loopp, جديد AstLogNot{flp, condp}});
        loopp->addStmtsp(جديد AstEventControl{flp, sentreep, nullptr});

        // Add assertion to the active set
        AstAssocSel* ثابت selp = جديد AstAssocSel{flp, جديد AstVarRef{flp, activep, VAccess::WRITE},
                                                  v3Global.rootp()->stdPackageProcessSelfp(flp)};
        AstAssign* ثابت incrementp = جديد AstAssign{flp, selp, جديد AstConst{flp, 1}};
        AstPExprClause* ثابت clausep = جديد AstPExprClause{flp};
        AstStmtExpr* ثابت deletep
            = getProcessAssocArrayDelete(جديد AstVarRef{flp, activep, VAccess::WRITE});

        // Main assertion block
        AstBegin* ثابت bodyp = جديد AstBegin{flp, m_blockNames.get(""), nullptr, 𐡒𐡔𐡈};
        bodyp->addStmtsp(incrementp);
        bodyp->addStmtsp(loopp);
        bodyp->addStmtsp(clausep);
        bodyp->addStmtsp(deletep);

        // Validate assertion condition for each active assert
        AstVar* ثابت activeCountp = جديد AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        activeCountp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstAssign* ثابت initActiveCountp
            = جديد AstAssign{flp, جديد AstVarRef{flp, activeCountp, VAccess::WRITE},
                            getProcessAssocArraySize(جديد AstVarRef{flp, activep, VAccess::READ})};
        AstLoop* ثابت finalLoopp = جديد AstLoop{flp};
        AstIf* ثابت finalBodypCondp
            = جديد AstIf{flp, condp->cloneTreePure(خطأ), جديد AstPExprClause{flp},
                        جديد AstPExprClause{flp, خطأ}};
        finalLoopp->addStmtsp(
            جديد AstLoopTest{flp, finalLoopp,
                            جديد AstNeq{flp, جديد AstVarRef{flp, activeCountp, VAccess::READ},
                                       جديد AstConst{flp, 0}}});
        finalLoopp->addStmtsp(finalBodypCondp);
        finalLoopp->addStmtsp(
            جديد AstAssign{flp, جديد AstVarRef{flp, activeCountp, VAccess::WRITE},
                          جديد AstSub{flp, جديد AstVarRef{flp, activeCountp, VAccess::READ},
                                     جديد AstConst{flp, 1}}});

        // Final assertion block
        AstBegin* ثابت finalp = جديد AstBegin{flp, m_blockNames.get(""), nullptr, 𐡒𐡔𐡈};
        finalp->addStmtsp(activeCountp);
        finalp->addStmtsp(initActiveCountp);
        finalp->addStmtsp(finalLoopp);

        VL_RESTORER(m_pexprp);
        m_pexprp = جديد AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
        VL_RESTORER(m_hasCycleDelay);
        m_hasCycleDelay = خطأ;
        iterate(bodyp);
        iterate(finalp);

        𐡄𐡍 (m_hasCycleDelay) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in s_eventually");
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            VL_DO_DANGLING(m_pexprp->deleteTree(), m_pexprp);
            أرجع;
        }

        nodep->replaceWith(m_pexprp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    فراغ visit(AstStable* nodep) override {
        𐡄𐡍 (nodep->user1SetOnce()) أرجع;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        FileLine* ثابت fl = nodep->fileline();
        AstNodeExpr* exprp = nodep->exprp()->unlinkFrBack();
        AstSenTree* sentreep = nodep->sentreep();
        𐡄𐡍 (sentreep) sentreep->unlinkFrBack();
        AstPast* ثابت pastp = جديد AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(newSenTree(nodep, sentreep));
        exprp = جديد AstEq{fl, pastp, exprp->cloneTreePure(خطأ)};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    فراغ visit(AstSteady* nodep) override {
        𐡄𐡍 (nodep->user1SetOnce()) أرجع;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        FileLine* ثابت fl = nodep->fileline();
        AstNodeExpr* exprp = nodep->exprp()->unlinkFrBack();
        𐡄𐡍 (exprp->width() > 1) exprp = جديد AstSel{fl, exprp, 0, 1};
        AstNodeExpr* ثابت futurep = جديد AstFuture{fl, exprp, newSenTree(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = جديد AstEq{fl, exprp->cloneTreePure(خطأ), futurep};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    فراغ visit(AstSampled* nodep) override {
        iterateChildren(nodep);
        𐡄𐡍 (!nodep->internal()) checkSamplingFuncDType(nodep, nodep->exprp());
    }

    // Validate repetition count: must be a non-negative elaboration-time constant.
    // Shared by goto [->N] and nonconsecutive [=N] repetition.
    // On error, replaces nodep with BitFalse placeholder and returns nullptr.
    ثابت AstConst* validateRepCount(AstNode* nodep, AstNodeExpr*& countp) {
        countp = V3Const::constifyEdit(countp);
        ثابت AstConst* ثابت constp = VN_CAST(countp, Const);
        𐡄𐡍 (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(pushDeletep(countp), countp);
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            أرجع nullptr;
        }
        𐡄𐡍 (constp->toSInt() < 0) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(pushDeletep(countp), countp);
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            أرجع nullptr;
        }
        𐡄𐡍 (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(pushDeletep(countp), countp);
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            أرجع nullptr;
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
    AstPExpr* createRepPExpr(FileLine* flp, AstNodeExpr* exprp, AstNodeExpr* countp,
                             AstNodeExpr* rhsp, منطقي isOverlapped, منطقي isNonConsec) {
        AstSenItem* ثابت sensesp = m_senip;
        UASSERT_OBJ(sensesp, exprp, "Repetition requires a clock");

        ثابت std::string name
            = isNonConsec ? m_nonConsRepNames.get(exprp) : m_gotoRepNames.get(exprp);
        AstVar* ثابت cntVarp = جديد AstVar{flp, VVarType::BLOCKTEMP, name + "__counter",
                                           exprp->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstBegin* ثابت beginp = جديد AstBegin{flp, name + "__block", cntVarp, 𐡒𐡔𐡈};
        beginp->addStmtsp(
            جديد AstAssign{flp, جديد AstVarRef{flp, cntVarp, VAccess::WRITE}, جديد AstConst{flp, 0}});

        // Counting loop: find N occurrences of expr (shared by goto and nonconsec)
        AstLoop* ثابت loopp = جديد AstLoop{flp};
        loopp->addStmtsp(جديد AstLoopTest{flp, loopp,
                                         جديد AstLt{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ},
                                                   countp->cloneTreePure(خطأ)}});
        // if (expr) cnt++ -- sampled is applied to whole property expr by V3Assert
        loopp->addStmtsp(
            جديد AstIf{flp, exprp,
                      جديد AstAssign{flp, جديد AstVarRef{flp, cntVarp, VAccess::WRITE},
                                    جديد AstAdd{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ},
                                               جديد AstConst{flp, 1}}}});
        loopp->addStmtsp(جديد AstIf{
            flp, جديد AstLt{flp, جديد AstVarRef{flp, cntVarp, VAccess::READ}, countp},
            جديد AstEventControl{flp, جديد AstSenTree{flp, sensesp->cloneTree(خطأ)}, nullptr}});

        beginp->addStmtsp(loopp);

        𐡄𐡍 (isNonConsec && rhsp) {
            // IEEE 16.9.2: b[=N] = b[->N] ##1 !b[*0:$]
            // Trailing window: ##1 advance, then scan !expr cycles checking rhs.
            // Window closes when expr becomes true again (fail if rhs was never true).
            beginp->addStmtsp(جديد AstEventControl{
                flp, جديد AstSenTree{flp, sensesp->cloneTree(خطأ)}, nullptr});  // ##1
            𐡄𐡍 (!isOverlapped) {
                beginp->addStmtsp(جديد AstEventControl{
                    flp, جديد AstSenTree{flp, sensesp->cloneTree(خطأ)}, nullptr});  // |=>
            }
            // Window loop: check rhs at each !expr cycle (done variable for termination)
            AstVar* ثابت doneVarp
                = جديد AstVar{flp, VVarType::BLOCKTEMP, name + "__done", exprp->findBitDType()};
            doneVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->addStmtsp(doneVarp);
            beginp->addStmtsp(جديد AstAssign{flp, جديد AstVarRef{flp, doneVarp, VAccess::WRITE},
                                            جديد AstConst{flp, AstConst::BitFalse{}}});
            auto setDone = [&]() {
                أرجع جديد AstAssign{flp, جديد AstVarRef{flp, doneVarp, VAccess::WRITE},
                                     جديد AstConst{flp, AstConst::BitTrue{}}};
            };
            AstLoop* ثابت windowp = جديد AstLoop{flp};
            // LoopTest: continue while !done
            windowp->addStmtsp(جديد AstLoopTest{
                flp, windowp, جديد AstNot{flp, جديد AstVarRef{flp, doneVarp, VAccess::READ}}});
            // if (expr) { fail; done = 1; } -- window closed, expr true again
            AstNode* ثابت failsp = جديد AstPExprClause{flp, خطأ};
            failsp->addNext(setDone());
            windowp->addStmtsp(جديد AstIf{flp, exprp->cloneTreePure(خطأ), failsp});
            // if (rhs) { pass; done = 1; } -- consequent true at this !expr endpoint
            AstNode* ثابت passsp = جديد AstPExprClause{flp, 𐡒𐡔𐡈};
            passsp->addNext(setDone());
            windowp->addStmtsp(جديد AstIf{flp, rhsp, passsp});
            // @(clk) -- advance to next cycle in window
            windowp->addStmtsp(
                جديد AstEventControl{flp, جديد AstSenTree{flp, sensesp->cloneTree(خطأ)}, nullptr});
            beginp->addStmtsp(windowp);
        } وإلا 𐡄𐡍 (isNonConsec) {
            // Standalone nonconsec: ##1 into window, then pass
            beginp->addStmtsp(
                جديد AstEventControl{flp, جديد AstSenTree{flp, sensesp->cloneTree(خطأ)}, nullptr});
            beginp->addStmtsp(جديد AstPExprClause{flp, 𐡒𐡔𐡈});
        } وإلا 𐡄𐡍 (rhsp) {
            // Goto rep: check consequent once at match endpoint
            𐡄𐡍 (!isOverlapped) {
                beginp->addStmtsp(جديد AstEventControl{
                    flp, جديد AstSenTree{flp, sensesp->cloneTree(خطأ)}, nullptr});
            }
            beginp->addStmtsp(جديد AstIf{flp, rhsp, جديد AstPExprClause{flp, 𐡒𐡔𐡈},
                                        جديد AstPExprClause{flp, خطأ}});
        } وإلا {
            // Standalone goto: pass after counting
            beginp->addStmtsp(جديد AstPExprClause{flp, 𐡒𐡔𐡈});
        }

        أرجع جديد AstPExpr{flp, beginp, exprp->findBitDType()};
    }

    // Reject [=M:N] nonconsecutive range that reached V3AssertPre.
    // [->M:N] is fully handled upstream (NFA path); only [=M:N] still lands
    // here because AstSNonConsRep lowering is not yet implemented there.
    // Replaces the node with BitFalse so lowering continues after the warning.
    منطقي rejectNonconsecRange(AstNode* nodep, AstNodeExpr* maxCountp) {
        𐡄𐡍 (!maxCountp) أرجع خطأ;
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: [=M:N] nonconsecutive range repetition"
                                     " (IEEE 1800-2023 16.9.2)");
        nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::BitFalse{}});
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
        أرجع 𐡒𐡔𐡈;
    }

    فراغ visit(AstSGotoRep* nodep) override {
        // Standalone goto rep (not inside implication antecedent)
        iterateChildren(nodep);
        FileLine* ثابت flp = nodep->fileline();
        AstNodeExpr* countp = nodep->countp()->unlinkFrBack();
        𐡄𐡍 (!validateRepCount(nodep, countp)) أرجع;
        AstNodeExpr* ثابت exprp = nodep->exprp()->unlinkFrBack();
        AstPExpr* ثابت pexprp = createRepPExpr(flp, exprp, countp, nullptr, 𐡒𐡔𐡈, خطأ);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    فراغ visit(AstSNonConsRep* nodep) override {
        // Standalone nonconsecutive rep (not inside implication antecedent)
        iterateChildren(nodep);
        𐡄𐡍 (rejectNonconsecRange(nodep, nodep->maxCountp())) أرجع;
        FileLine* ثابت flp = nodep->fileline();
        AstNodeExpr* countp = nodep->countp()->unlinkFrBack();
        𐡄𐡍 (!validateRepCount(nodep, countp)) أرجع;
        AstNodeExpr* ثابت exprp = nodep->exprp()->unlinkFrBack();
        AstPExpr* ثابت pexprp = createRepPExpr(flp, exprp, countp, nullptr, 𐡒𐡔𐡈, 𐡒𐡔𐡈);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    فراغ visit(AstFuncRef* nodep) override {
        // IEEE 1800-2023 16.8: Inline sequence instances wherever they appear
        // in the expression tree (inside implications, boolean ops, nested refs, etc.)
        𐡄𐡍 (AstSequence* ثابت seqp = VN_CAST(nodep->taskp(), Sequence)) {
            substituteSequenceCall(nodep, seqp);
            // The FuncRef has been replaced; do not access nodep after this point.
            // The replacement node will be visited by the parent's iterateChildren.
            أرجع;
        }
        𐡄𐡍 (ثابت AstProperty* ثابت propp = VN_CAST(nodep->taskp(), Property)) {
            substituteNestedPropertyCall(nodep, propp);
            أرجع;
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstImplication* nodep) override {
        𐡄𐡍 (nodep->sentreep()) أرجع;  // Already processed

        // Top-level followed-by is claimed by V3AssertNfa; anything reaching here
        // is nested inside iff/implies/or. IEEE 1800-2023 16.12.9 permits the
        // composition, but silent lowering would drop non-vacuous-fail semantics.
        𐡄𐡍 (nodep->isFollowedBy()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: followed-by (#-# / #=#) nested inside property operator"
                          " (iff/implies/or) (IEEE 1800-2023 16.12.9)");
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            أرجع;
        }

        // Handle goto repetition as antecedent before iterateChildren,
        // so the standalone AstSGotoRep visitor doesn't process it
        𐡄𐡍 (AstSGotoRep* ثابت gotop = VN_CAST(nodep->lhsp(), SGotoRep)) {
            iterateChildren(gotop);
            iterateAndNextNull(nodep->rhsp());
            FileLine* ثابت flp = nodep->fileline();
            AstNodeExpr* countp = gotop->countp()->unlinkFrBack();
            𐡄𐡍 (!validateRepCount(nodep, countp)) أرجع;
            AstNodeExpr* ثابت exprp = gotop->exprp()->unlinkFrBack();
            AstNodeExpr* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
            AstPExpr* ثابت pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), خطأ);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            أرجع;
        }

        // Handle nonconsecutive repetition as antecedent before iterateChildren,
        // so the standalone AstSNonConsRep visitor doesn't process it
        𐡄𐡍 (AstSNonConsRep* ثابت ncrp = VN_CAST(nodep->lhsp(), SNonConsRep)) {
            iterateChildren(ncrp);
            iterateAndNextNull(nodep->rhsp());
            𐡄𐡍 (rejectNonconsecRange(nodep, ncrp->maxCountp())) أرجع;
            FileLine* ثابت flp = nodep->fileline();
            AstNodeExpr* countp = ncrp->countp()->unlinkFrBack();
            𐡄𐡍 (!validateRepCount(nodep, countp)) أرجع;
            AstNodeExpr* ثابت exprp = ncrp->exprp()->unlinkFrBack();
            AstNodeExpr* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
            AstPExpr* ثابت pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), 𐡒𐡔𐡈);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            أرجع;
        }

        iterateChildren(nodep);

        FileLine* ثابت flp = nodep->fileline();
        AstNodeExpr* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
        AstNodeExpr* lhsp = nodep->lhsp()->unlinkFrBack();

        // Lower sequence match items (IEEE 16.11): (expr, var = val, ...) |-> / |=>
        𐡄𐡍 (AstExprStmt* ثابت exprStmtp = VN_CAST(lhsp, ExprStmt)) {
            AstNodeExpr* ثابت antExprp = exprStmtp->resultp()->unlinkFrBack();

            𐡄𐡍 (nodep->isOverlapped()) {
                // |-> : assign to __Vpropvar via always_comb (continuous).
                // The assign evaluates RHS once; V3Sampled snapshots the
                // result so all consequent refs read the same value.
                AstNode* matchAssignsp = nullptr;
                لكل (AstNode* stmtp = exprStmtp->stmtsp(); stmtp;) {
                    AstNode* ثابت nextp = stmtp->nextp();
                    𐡄𐡍 (AstAssign* ثابت assignp = VN_CAST(stmtp, Assign)) {
                        assignp->unlinkFrBack();
                        matchAssignsp = AstNode::addNextNull(matchAssignsp, assignp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(pushDeletep(lhsp), lhsp);
                lhsp = antExprp;

                𐡄𐡍 (matchAssignsp) {
                    AstAlways* ثابت alwaysp
                        = جديد AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, matchAssignsp};
                    m_modp->addStmtsp(alwaysp);
                }
            } وإلا {
                // |=> : assign to __Vpropvar via NBA in a clocked always block.
                // The NBA commits before the next cycle's sampled snapshot,
                // so the consequent (which already references __Vpropvar)
                // sees the captured value.
                AstNode* matchAssignsp = nullptr;
                لكل (AstNode* stmtp = exprStmtp->stmtsp(); stmtp;) {
                    AstNode* ثابت nextp = stmtp->nextp();
                    𐡄𐡍 (AstAssign* ثابت assignp = VN_CAST(stmtp, Assign)) {
                        assignp->unlinkFrBack();
                        AstNodeExpr* ثابت assignLhsp = assignp->lhsp()->unlinkFrBack();
                        AstNodeExpr* ثابت assignRhsp = assignp->rhsp()->unlinkFrBack();
                        AstAssignDly* ثابت dlyp = جديد AstAssignDly{flp, assignLhsp, assignRhsp};
                        VL_DO_DANGLING(pushDeletep(assignp), assignp);
                        matchAssignsp = AstNode::addNextNull(matchAssignsp, dlyp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(pushDeletep(lhsp), lhsp);
                lhsp = antExprp;

                𐡄𐡍 (matchAssignsp) {
                    AstIf* ثابت condp
                        = جديد AstIf{flp, antExprp->cloneTreePure(خطأ), matchAssignsp};
                    AstAlways* ثابت alwaysp
                        = جديد AstAlways{flp, VAlwaysKwd::ALWAYS, newSenTree(nodep), condp};
                    m_modp->addStmtsp(alwaysp);
                }
            }
        }

        𐡄𐡍 (AstPExpr* ثابت pexprp = VN_CAST(rhsp, PExpr)) {
            // Implication with sequence expression on RHS (IEEE 1800-2023 16.11, 16.12.7).
            // The PExpr was lowered from the property expression earlier in this pass.
            // Wrap the PExpr body with the antecedent check so the sequence only
            // starts when the antecedent holds.
            AstNodeExpr* condp;
            𐡄𐡍 (nodep->isOverlapped()) {
                // Overlapped implication (|->): check antecedent on same cycle.
                // disable iff is applied at the assertion level, not at the
                // antecedent gate, matching the existing non-PExpr overlapped path.
                condp = lhsp;
            } وإلا {
                // Non-overlapped implication (|=>): check antecedent from previous cycle
                𐡄𐡍 (m_disablep) {
                    lhsp
                        = جديد AstAnd{flp, جديد AstNot{flp, m_disablep->cloneTreePure(خطأ)}, lhsp};
                }
                AstPast* ثابت pastp
                    = جديد AstPast{flp, lhsp, nullptr, nullptr, /* propertyTiming */ 𐡒𐡔𐡈};
                pastp->dtypeFrom(lhsp);
                pastp->sentreep(newSenTree(nodep));
                condp = pastp;
            }
            // Wrap existing PExpr body: if (antecedent) { <original body> } else { /* vacuous pass
            // */ }
            // Only the statements are guarded; declarations stay directly in the block. After
            // V3Fork moves the block's statements into a task, V3Task only handles variables
            // that are direct statements of the task.
            AstBegin* ثابت bodyp = pexprp->bodyp();
            AstIf* ثابت guardp = جديد AstIf{flp, condp};
            لكل (AstNode* stmtp = bodyp->stmtsp(); stmtp;) {
                AstNode* ثابت nextp = stmtp->nextp();
                𐡄𐡍 (!VN_IS(stmtp, Var)) guardp->addThensp(stmtp->unlinkFrBack());
                stmtp = nextp;
            }
            bodyp->addStmtsp(guardp);
            nodep->replaceWith(pexprp);
            // Don't iterate pexprp here -- it was already iterated when created
            // (in visit(AstSExpr*)), so delays and disable iff are already processed.
        } وإلا 𐡄𐡍 (nodep->isOverlapped()) {
            AstNodeExpr* ثابت exprp
                = m_underCover ? static_cast<AstNodeExpr*>(جديد AstLogAnd{flp, lhsp, rhsp})
                               : جديد AstLogOr{flp, جديد AstLogNot{flp, lhsp}, rhsp};
            nodep->replaceWith(exprp);
        } وإلا {
            𐡄𐡍 (m_disablep) {
                lhsp = جديد AstAnd{flp, جديد AstNot{flp, m_disablep->cloneTreePure(خطأ)}, lhsp};
            }

            AstPast* ثابت pastp
                = جديد AstPast{flp, lhsp, nullptr, nullptr, /* propertyTiming */ 𐡒𐡔𐡈};
            pastp->dtypeFrom(lhsp);
            pastp->sentreep(newSenTree(nodep));
            AstNodeExpr* ثابت exprp
                = m_underCover ? static_cast<AstNodeExpr*>(جديد AstAnd{flp, pastp, rhsp})
                               : جديد AstOr{flp, جديد AstNot{flp, pastp}, rhsp};
            exprp->dtypeSetBit();
            nodep->replaceWith(exprp);
        }
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    فراغ visit(AstUntil* nodep) override {
        FileLine* ثابت flp = nodep->fileline();
        UASSERT_OBJ(
            !m_pexprp, nodep,
            "'" << nodep->verilogKwd()
                << "' in complex property expression should have been rejected by V3AssertNfa");
        𐡄𐡍 (nodep->isStrong()
            && (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue())) {
            nodep->v3warn(E_NOTIMING, nodep->verilogKwd() << " requires --timing");
            nodep->replaceWith(جديد AstConst{flp, AstConst::BitFalse{}});
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            أرجع;
        }
        𐡄𐡍 (nodep->isStrong()) {
            // p s_until q / p s_until_with q: q must eventually be true. Until then, p must
            // be true on every sampled tick. For s_until, check q first: when q is true on
            // this tick, p is not required. For s_until_with, p must be true on the q tick too.
            AstNodeExpr* ثابت rawLhsp = nodep->lhsp()->unlinkFrBack();
            AstNodeExpr* ثابت rawRhsp = nodep->rhsp()->unlinkFrBack();
            AstSampled* ثابت lhsp = جديد AstSampled{flp, rawLhsp, rawLhsp->dtypep(), 𐡒𐡔𐡈};
            AstSampled* ثابت rhsp = جديد AstSampled{flp, rawRhsp, rawRhsp->dtypep(), 𐡒𐡔𐡈};
            AstNodeExpr* finalCondp = rhsp->cloneTreePure(خطأ);
            𐡄𐡍 (nodep->isOverlapping()) {
                finalCondp = جديد AstLogAnd{flp, lhsp->cloneTreePure(خطأ), finalCondp};
            }

            // Track active assertion attempts. Only the count is needed to emit final failures.
            AstVar* ثابت activep = جديد AstVar{flp, VVarType::MODULETEMP, m_activeNames.get(""),
                                               nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            activep->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(activep);

            AstVar* ثابت donep
                = جديد AstVar{flp, VVarType::BLOCKTEMP, "__VassertDone", nodep->findBitDType()};
            donep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            auto setDone = [&]() {
                أرجع جديد AstAssign{flp, جديد AstVarRef{flp, donep, VAccess::WRITE},
                                     جديد AstConst{flp, AstConst::BitTrue{}}};
            };

            AstLoop* ثابت loopp = جديد AstLoop{flp};
            AstAssign* ثابت decrementVar = جديد AstAssign{
                flp, جديد AstVarRef{flp, activep, VAccess::WRITE},
                جديد AstSub{flp, جديد AstVarRef{flp, activep, VAccess::READ}, جديد AstConst{flp, 1}}};
            loopp->addStmtsp(جديد AstLoopTest{
                flp, loopp, جديد AstLogNot{flp, جديد AstVarRef{flp, donep, VAccess::READ}}});
            {
                AstNode* ثابت passp = جديد AstPExprClause{flp};
                passp->addNext(decrementVar);
                passp->addNext(setDone());
                AstNodeExpr* passCondp = rhsp;
                𐡄𐡍 (nodep->isOverlapping()) {
                    passCondp = جديد AstLogAnd{flp, lhsp->cloneTreePure(خطأ), passCondp};
                }
                loopp->addStmtsp(جديد AstIf{flp, passCondp, passp});
            }
            {
                AstNode* ثابت failp = جديد AstPExprClause{flp, خطأ};
                failp->addNext(decrementVar->cloneTree(خطأ));
                failp->addNext(setDone());
                loopp->addStmtsp(جديد AstIf{
                    flp,
                    جديد AstLogAnd{flp,
                                  جديد AstLogNot{flp, جديد AstVarRef{flp, donep, VAccess::READ}},
                                  جديد AstLogNot{flp, lhsp}},
                    failp});
            }
            AstDelay* ثابت delayp = جديد AstDelay{flp, جديد AstConst{flp, 1}, خطأ};
            delayp->timeunit(m_modp->timeunit());
            loopp->addStmtsp(جديد AstIf{
                flp, جديد AstLogNot{flp, جديد AstVarRef{flp, donep, VAccess::READ}}, delayp});

            // Main assertion block
            AstBegin* ثابت bodyp = جديد AstBegin{flp, m_blockNames.get(""), nullptr, 𐡒𐡔𐡈};
            bodyp->addStmtsp(donep);
            bodyp->addStmtsp(جديد AstAssign{flp, جديد AstVarRef{flp, donep, VAccess::WRITE},
                                           جديد AstConst{flp, AstConst::BitFalse{}}});
            FileLine* ثابت flp = activep->fileline();
            bodyp->addStmtsp(
                جديد AstAssign{flp, جديد AstVarRef{flp, activep, VAccess::WRITE},
                              جديد AstAdd{flp, جديد AstVarRef{flp, activep, VAccess::READ},
                                         جديد AstConst{flp, 1}}});
            bodyp->addStmtsp(loopp);

            // Validate assertion condition for each active assert
            AstVar* ثابت activeCountp = جديد AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                    nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            activeCountp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

            AstAssign* ثابت initActiveCountp
                = جديد AstAssign{flp, جديد AstVarRef{flp, activeCountp, VAccess::WRITE},
                                جديد AstVarRef{flp, activep, VAccess::READ}};
            AstLoop* ثابت finalLoopp = جديد AstLoop{flp};
            finalLoopp->addStmtsp(
                جديد AstLoopTest{flp, finalLoopp,
                                جديد AstNeq{flp, جديد AstVarRef{flp, activeCountp, VAccess::READ},
                                           جديد AstConst{flp, 0}}});
            finalLoopp->addStmtsp(جديد AstIf{flp, finalCondp, جديد AstPExprClause{flp},
                                            جديد AstPExprClause{flp, خطأ}});
            finalLoopp->addStmtsp(
                جديد AstAssign{flp, جديد AstVarRef{flp, activeCountp, VAccess::WRITE},
                              جديد AstSub{flp, جديد AstVarRef{flp, activeCountp, VAccess::READ},
                                         جديد AstConst{flp, 1}}});

            // Final assertion block
            AstBegin* ثابت finalp = جديد AstBegin{flp, m_blockNames.get(""), nullptr, 𐡒𐡔𐡈};
            finalp->addStmtsp(activeCountp);
            finalp->addStmtsp(initActiveCountp);
            finalp->addStmtsp(finalLoopp);

            AstPExpr* ثابت pexprp = جديد AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
            VL_RESTORER(m_pexprp);
            VL_RESTORER(m_hasCycleDelay);
            m_pexprp = pexprp;
            m_hasCycleDelay = خطأ;
            iterate(bodyp);
            iterate(finalp);

            UASSERT_OBJ(!m_hasCycleDelay, nodep,
                        "s_until cycle delay should have been handled by V3AssertNfa");

            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            أرجع;
        }
        AstLoop* ثابت loopp = جديد AstLoop{flp};
        AstNodeExpr* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
        AstNodeExpr* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
        AstLogAnd* ثابت loopCondp = جديد AstLogAnd{flp, lhsp, جديد AstLogNot{flp, rhsp}};
        loopp->addStmtsp(جديد AstLoopTest{flp, loopp, loopCondp});
        loopp->addStmtsp(جديد AstEventControl{flp, newSenTree(nodep), nullptr});

        AstNodeExpr* ثابت rhsCopyp = rhsp->cloneTreePure(خطأ);
        AstNodeExpr* ثابت passCondp
            = nodep->isOverlapping() ? جديد AstLogAnd{flp, lhsp->cloneTreePure(خطأ), rhsCopyp}
                                     : rhsCopyp;
        AstBegin* ثابت beginp = جديد AstBegin{flp, m_blockNames.get(""), loopp, 𐡒𐡔𐡈};
        beginp->addStmtsp(
            جديد AstIf{flp, passCondp, جديد AstPExprClause{flp}, جديد AstPExprClause{flp, خطأ}});

        AstPExpr* ثابت pexprp = جديد AstPExpr{flp, beginp, nodep->dtypep()};
        pexprp->user1(1);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    فراغ visit(AstDefaultDisable* nodep) override {
        VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
    }
    فراغ visit(AstInferredDisable* nodep) override {
        AstNode* newp;
        𐡄𐡍 (m_defaultDisablep) {
            newp = m_defaultDisablep->condp()->cloneTreePure(𐡒𐡔𐡈);
        } وإلا {
            newp = جديد AstConst{nodep->fileline(), AstConst::BitFalse{}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    فراغ visit(AstPropSpec* nodep) override {
        nodep = substitutePropertyCall(nodep);
        iterateAndNextNull(nodep->sensesp());
        𐡄𐡍 (m_senip && m_senip != nodep->sensesp())
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Only one PSL clock allowed per assertion");
        ثابت AstCover* ثابت coverp = VN_CAST(nodep->backp(), Cover);
        ثابت منطقي seqEvent = coverp && coverp->isSeqEvent();
        𐡄𐡍 (!nodep->disablep() && m_defaultDisablep && !seqEvent) {
            nodep->disablep(m_defaultDisablep->condp()->cloneTreePure(𐡒𐡔𐡈));
        }
        m_disablep = nodep->disablep();
        // Unlink and just keep a pointer to it, convert to sentree as needed
        m_senip = nodep->sensesp();
        iterateNull(nodep->disablep());
        𐡄𐡍 (!VN_AS(nodep->backp(), NodeCoverOrAssert)->immediate()) {
            ثابت AstNodeDType* ثابت propDtp = nodep->propp()->dtypep();
            nodep->propp(جديد AstSampled{nodep->fileline(), nodep->propp()->unlinkFrBack(),
                                        propDtp->dtypep(), 𐡒𐡔𐡈});
        }
        // cover counts non-vacuous matches only (IEEE 1800-2023 16.15.2), so an
        // implication antecedent must hold; assert passes vacuously instead.
        VL_RESTORER(m_underCover);
        m_underCover = VN_IS(nodep->backp(), Cover);
        iterate(nodep->propp());
        iterateNull(nodep->matchCountp());
    }
    فراغ visit(AstPExpr* nodep) override {
        // V3AssertNfa handles multi-cycle property expressions before this pass,
        // so the following unsupported paths are defensive and typically unreached.
        𐡄𐡍 (m_pexprp && m_pexprp->user1()) {  // LCOV_EXCL_START
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Complex property expression inside 'until''");
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            أرجع;
        }  // LCOV_EXCL_STOP
        𐡄𐡍 (AstLogNot* ثابت notp = VN_CAST(nodep->backp(), LogNot)) {
            notp->replaceWith(nodep->unlinkFrBack());
            VL_DO_DANGLING(pushDeletep(notp), notp);
            iterate(nodep);
            أرجع;
        }
        // Sequence expression as antecedent of implication is not yet supported
        𐡄𐡍 (AstImplication* ثابت implp = VN_CAST(nodep->backp(), Implication)) {
            𐡄𐡍 (implp->lhsp() == nodep) {  // LCOV_EXCL_START
                implp->v3warn(E_UNSUPPORTED,
                              "Unsupported: Implication with sequence expression as antecedent");
                nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
                أرجع;
            }  // LCOV_EXCL_STOP
        }
        VL_RESTORER(m_pexprp);
        VL_RESTORER(m_disableSeqIfp);
        m_pexprp = nodep;

        𐡄𐡍 (m_disablep) {
            ثابت AstSampled* sampledp = nullptr;
            𐡄𐡍 (m_disablep->exists([&sampledp](ثابت AstSampled* ثابت sp) {
                    sampledp = sp;
                    أرجع 𐡒𐡔𐡈;
                })) {
                sampledp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: $sampled inside disabled condition of a sequence");
                m_disablep = جديد AstConst{m_disablep->fileline(), AstConst::BitFalse{}};
                // always a copy is used, so remove it now
                pushDeletep(m_disablep);  // m_disablep used later
            }
            FileLine* ثابت flp = nodep->fileline();
            // Add counter which counts times the condition turned true
            AstVar* ثابت disableCntp
                = جديد AstVar{flp, VVarType::MODULETEMP, m_disableCntNames.get(""),
                             nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            disableCntp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(disableCntp);
            AstVarRef* ثابت readCntRefp = جديد AstVarRef{flp, disableCntp, VAccess::READ};
            AstVarRef* ثابت writeCntRefp = جديد AstVarRef{flp, disableCntp, VAccess::WRITE};
            AstAssign* ثابت incrStmtp = جديد AstAssign{
                flp, writeCntRefp, جديد AstAdd{flp, readCntRefp, جديد AstConst{flp, 1}}};
            AstAlways* ثابت alwaysp
                = جديد AstAlways{flp, VAlwaysKwd::ALWAYS,
                                جديد AstSenTree{flp, جديد AstSenItem{flp, VEdgeType::ET_POSEDGE,
                                                                   m_disablep->cloneTree(خطأ)}},
                                incrStmtp};
            disableCntp->addNextHere(alwaysp);

            // Store value of that counter at the beginning of sequence evaluation
            AstBegin* ثابت bodyp = nodep->bodyp();
            AstVar* ثابت initialCntp = جديد AstVar{flp, VVarType::BLOCKTEMP, "__VinitialCnt",
                                                   nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            initialCntp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            AstAssign* ثابت assignp
                = جديد AstAssign{flp, جديد AstVarRef{flp, initialCntp, VAccess::WRITE},
                                readCntRefp->cloneTree(خطأ)};
            // Prepend to the sequence body to keep statement list structure valid.
            UASSERT_OBJ(!bodyp->name().empty(), bodyp, "Sequence body should be named");
            AstNode* ثابت newStmtsp = AstNode::addNext<AstNode, AstNode>(initialCntp, assignp);
            bodyp->prependStmtsp(newStmtsp);

            m_disableSeqIfp
                = جديد AstIf{flp, جديد AstEq{flp, جديد AstVarRef{flp, initialCntp, VAccess::READ},
                                           readCntRefp->cloneTree(خطأ)}};
            // Delete it, because it is always copied before insetion to the AST
            pushDeletep(m_disableSeqIfp);  // m_disableSeqIfp used later
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstNodeModule* nodep) override {
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultClkEvtVarp);
        VL_RESTORER(m_defaultDisablep);
        VL_RESTORER(m_modp);
        𐡄𐡍 (!VN_IS(nodep, Class)) {
            // A class nested in a module is in the scope of the module's default clocking
            m_defaultClockingp = nullptr;
            m_defaultClkEvtVarp = nullptr;
        }
        m_defaultDisablep = nodep->defaultDisablep();
        m_modp = nodep;
        iterateChildren(nodep);
    }
    فراغ visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    فراغ visit(AstProperty* nodep) override {
        // The body will be visited when will be substituted in place of property reference
        // (AstFuncRef)
        VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
    }
    فراغ visit(AstSequence* nodep) override {
        // Sequence declarations are not visited directly; their bodies are inlined
        // at call sites by visit(AstFuncRef*). Collect for post-traversal cleanup.
        m_seqsToCleanup.push_back(nodep);
    }
    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit AssertPreVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        { AssertPreClockingVisitor{nodep}; }
        // Process
        iterate(nodep);
        // Fix up varref names
        لكل (AstVarXRef* xrefp : m_xrefsp) xrefp->name(xrefp->varp()->name());
        // Clean up sequence declarations after inlining.
        // Referenced sequences that were inlined have isReferenced cleared.
        // Remaining referenced sequences are in unsupported contexts (e.g. @seq event).
        لكل (AstSequence* seqp : m_seqsToCleanup) {
            𐡄𐡍 (seqp->isReferenced()) {
                seqp->v3warn(E_UNSUPPORTED,
                             "Unsupported: sequence referenced outside assertion property");
            } وإلا {
                VL_DO_DANGLING(seqp->unlinkFrBack()->deleteTree(), seqp);
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
    V3Global::dumpCheckGlobalTree("assertpre", 0, dumpTreeEitherLevel() >= 3);
}
