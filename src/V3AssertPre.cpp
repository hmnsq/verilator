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
    शून्य visit(AstNetlist* nodep) override { iterateAndNextConstNull(nodep->modulesp()); }
    शून्य visit(Ast_गाँठो_Module* nodep) override { iterateAndNextConstNull(nodep->stmtsp()); }
    शून्य visit(AstGenBlock* nodep) override { iterateAndNextConstNull(nodep->itemsp()); }
    शून्य visit(AstClocking* nodep) override {
        लागि (Ast_गाँठो* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            यदि (AstClockingItem* अचर citemp = VN_CAST(itemp, ClockingItem)) {
                citemp->user3p(nodep);
            }
        }
    }
    // Clocking blocks are only in modules and their generate blocks
    शून्य visit(Ast_गाँठो*) override {}

सार्वजनिक:
    // CONSTRUCTORS
    explicit AssertPreClockingVisitor(AstNetlist* nodep) { iterateConst(nodep); }
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
        Ast_गाँठो_अभिव्यक्ति* refp;  // Reference to the driven clockvar
    };

    // NODE STATE
    // AstClockingItem::user1p()         // AstVar*.      varp() of ClockingItem after unlink
    // AstClockingItem::user2p()         // AstVar*.      Flag set by drives of output clockvar
    // AstClockingItem::user3p()         // AstClocking*. Clocking block of the item, captured
    //                                   //               by AssertPreClockingVisitor
    // AstPExpr::user1()                 // bool.         Created from AstUntil
    अचर VNUser1InUse m_inuser1;
    अचर VNUser2InUse m_inuser2;
    अचर VNUser3InUse m_inuser3;
    // STATE
    // Current context:
    AstNetlist* अचर m_netlistp = nullptr;  // Current netlist
    Ast_गाँठो_Module* m_modp = nullptr;  // Current module
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
    Ast_गाँठो_अभिव्यक्ति* m_disablep = nullptr;  // Last disable
    AstIf* m_disableSeqIfp = nullptr;  // Used for handling disable iff in sequences
    AstPExpr* m_pexprp = nullptr;  // Last AstPExpr
    तार्किक m_underCover = असत्य;  // True if the enclosing assertion is a cover
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
    तार्किक m_inAssign = असत्य;  // True if in an AssignNode
    तार्किक m_inAssignDlyLhs = असत्य;  // True if in AssignDly's LHS
    तार्किक m_inSynchDrive = असत्य;  // True if in synchronous drive
    तार्किक m_inConcatAssign = असत्य;  // True if in assignment to a concatenation
    std::vector<SynchDrive> m_drives;  // Clockvars written by the synchronous drive
    तार्किक m_hasCycleDelay = असत्य;  // True if node has cycle delay beneath
    std::vector<AstVarXRef*> m_xrefsp;  // list of xrefs that need name fixup
    std::vector<AstSequence*> m_seqsToCleanup;  // Sequences to clean up after traversal

    // METHODS

    // Flag set by each drive of an output clockvar, added to the module with its clocking item
    AstVar* getCreateDrivenVarp(AstClockingItem* itemp) {
        यदि (!itemp->user2p()) {
            AstVar* अचर varp = नूतन AstVar{itemp->fileline(), VVarType::MODULETEMP,
                                            m_drivenNames.get(itemp), itemp->findBitDType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            itemp->user2p(varp);
        }
        फर्काऊ VN_AS(itemp->user2p(), Var);
    }
    // Reference to a variable in the scope of a driven clockvar, made like the clockvar's
    स्थिर Ast_गाँठो_अभिव्यक्ति* newDriveRefp(File_पङ्क्ति* flp, अचर SynchDrive& drive, AstVar* varp,
                                     अचर VAccess& access) {
        यदि (अचर AstVarXRef* अचर xrefp = VN_CAST(drive.refp, VarXRef)) {
            फर्काऊ नूतन AstVarXRef{flp, varp, xrefp->dotted(), access};
        } अन्यथा यदि (अचर AstMemberSel* अचर selp = VN_CAST(drive.refp, MemberSel)) {
            // The interface expression is evaluated again, so it must not have side effects;
            // cloneTreePure warns if it has any
            AstMemberSel* अचर newSelp
                = नूतन AstMemberSel{flp, selp->fromp()->clone_रूख_Pure(असत्य), varp};
            newSelp->access(access);
            फर्काऊ newSelp;
        }
        फर्काऊ नूतन AstVarRef{flp, varp, access};
    }
    // Assignment setting the drive flag of a driven clockvar
    AstAssign* newDrivenSetp(File_पङ्क्ति* flp, अचर SynchDrive& drive) {
        AstVar* अचर varp = getCreateDrivenVarp(drive.itemp);
        AstAssign* अचर setp = नूतन AstAssign{flp, newDriveRefp(flp, drive, varp, VAccess::WRITE),
                                              नूतन AstConst{flp, AstConst::BitTrue{}}};
        setp->user1(सत्य);
        फर्काऊ setp;
    }
    // Clocking block of a clocking item
    स्थिर AstClocking* clockingOf(अचर AstClockingItem* itemp) {
        UASSERT_OBJ(itemp->user3p(), itemp, "Clocking item not captured in a clocking block");
        फर्काऊ VN_AS(itemp->user3p(), Clocking);
    }
    // Clocking event of a driven clockvar, referenced from the drive like the clockvar, or
    // nullptr if unsupported, which is reported on the drive's cycle delay
    Ast_Sen_रूख* newDriveSentreep(AstDelay* delayp, अचर SynchDrive& drive) {
        Ast_Sen_रूख* अचर origp = clockingOf(drive.itemp)->sentreep();
        यदि (VN_IS(drive.refp, VarRef)) फर्काऊ origp->clone_रूख(असत्य);
        यदि (origp->exists([](अचर AstVarXRef*) { फर्काऊ सत्य; })) {
            // The reference is relative to the clocking block
            delayp->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in synchronous drive to"
                                          " clockvar in another scope, whose clocking event"
                                          " has a hierarchical reference");
            फर्काऊ nullptr;
        }
        अचर AstMemberSel* अचर selp = VN_CAST(drive.refp, MemberSel);
        // Processes outside classes evaluate their events also when not waiting on them,
        // possibly before the interface reference is set, so the event is guarded against a
        // null reference, which needs an integral value; scheduling does not guard it
        अचर तार्किक guardNull = selp && !VN_IS(m_modp, Class);
        यदि (guardNull) {
            लागि (AstSenItem* itemp = origp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                यदि (!itemp->sensp()->dtypep()->skipRefp()->isIntegralOrPacked()) {
                    delayp->v3warn(E_UNSUPPORTED,
                                   "Unsupported: cycle delay in synchronous drive through"
                                   " an interface reference outside a class, to clocking"
                                   " block with non-integral clocking event");
                    फर्काऊ nullptr;
                }
            }
        }
        // Interface declaring the clocking block, if driven through an interface reference
        AstIface* अचर ifacep
            = selp ? VN_AS(selp->fromp()->dtypep()->skipRefp(), IfaceRefDType)->ifaceViaCellp()
                   : nullptr;
        Ast_Sen_रूख* अचर sentreep = origp->clone_रूख(असत्य);
        std::vector<AstVarRef*> refps;
        sentreep->foreach([&](AstVarRef* refp) {
            यदि (!refp->classOrPackagep()) refps.थप_back(refp);
        });
        लागि (AstVarRef* अचर refp : refps) {
            AstVar* अचर varp = refp->varp();
            यदि (ifacep) {
                // Mark the variable as read through an interface reference, as V3Width does for
                // a member select. The mark is the interface declaring the variable, whose
                // instances all share it, so drives through references to any instances agree.
                UASSERT_OBJ(!varp->sensIfacep() || varp->sensIfacep() == ifacep, refp,
                            "Variable read through references to different interfaces");
                varp->sensIfacep(ifacep);
            }
            refp->replaceWith(newDriveRefp(refp->fileline(), drive, varp, refp->access()));
            VL_DO_DANGLING(थप_Deletep(refp), refp);
        }
        यदि (guardNull) {
            लागि (AstSenItem* itemp = sentreep->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                guardNullIface(itemp, selp->fromp());
            }
        }
        फर्काऊ sentreep;
    }
    // Make an integral clocking event through an interface reference false while the reference
    // is null
    स्थिर शून्य guardNullIface(AstSenItem* sensesp, Ast_गाँठो_अभिव्यक्ति* fromp) {
        File_पङ्क्ति* अचर flp = sensesp->fileline();
        Ast_गाँठो_अभिव्यक्ति* अचर sensp = sensesp->sensp()->unlinkFrBack();
        sensesp->sensp(नूतन AstCond{flp, newNotNullp(flp, fromp), sensp,
                                   नूतन AstConst{flp, AstConst::DTyped{}, sensp->dtypep()}});
        यदि (Ast_गाँठो_अभिव्यक्ति* अचर condp = sensesp->condp()) {
            condp->unlinkFrBack();
            sensesp->condp(नूतन AstLogAnd{flp, newNotNullp(flp, fromp), condp});
        }
    }
    स्थिर Ast_गाँठो_अभिव्यक्ति* newNotNullp(File_पङ्क्ति* flp, Ast_गाँठो_अभिव्यक्ति* fromp) {
        फर्काऊ नूतन AstNeq{flp, fromp->clone_रूख_Pure(असत्य), नूतन AstConst{flp, AstConst::Null{}}};
    }
    // Procedural '##0': if the default clocking event has not yet occurred in this time step,
    // wait for it; otherwise continue without suspension (IEEE 1800-2023 14.11)
    AstIf* newCycleDelayZerop(File_पङ्क्ति* flp) अचर {
        AstVar* अचर evtVarp = m_defaultClkEvtVarp;
        UASSERT_OBJ(evtVarp, flp, "Default clocking event var not pre-created");
        AstCMethodHard* अचर isTriggeredp = नूतन AstCMethodHard{
            flp, नूतन AstVarRef{flp, evtVarp, VAccess::READ}, VCMethod::EVENT_IS_TRIGGERED};
        isTriggeredp->dtypeSetBit();
        AstEventControl* अचर waitp = नूतन AstEventControl{
            flp,
            नूतन Ast_Sen_रूख{flp, नूतन AstSenItem{flp, VEdgeType::ET_EVENT,
                                               नूतन AstVarRef{flp, evtVarp, VAccess::READ}}},
            nullptr};
        फर्काऊ नूतन AstIf{flp, नूतन AstNot{flp, isTriggeredp}, waitp};
    }

    स्थिर शून्य checkSamplingFuncDType(Ast_गाँठो_अभिव्यक्ति* nodep, अचर Ast_गाँठो* exprp) {
        अचर Ast_गाँठो_DType* अचर dtypep = exprp->dtypep()->skipRefp();
        यदि (!dtypep->isIntegralOrPacked()) {
            nodep->v3error("Expected numeric type, but got a " << dtypep->prettyDTypeNameQ()
                                                               << " data type");
        }
    }

    Ast_Sen_रूख* new_Sen_रूख(Ast_गाँठो* nodep, Ast_Sen_रूख* useTreep = nullptr,
                           Ast_गाँठो_Cover_Or_Assert* cassertp = nullptr) {
        // Create sentree based on clocked or default clock
        // Return nullptr for always
        यदि (useTreep) फर्काऊ useTreep;
        Ast_Sen_रूख* newp = nullptr;
        AstSenItem* senip = m_senip;
        तार्किक fromAlways = असत्य;
        यदि (!senip && m_defaultClockingp) senip = m_defaultClockingp->sentreep()->sensesp();
        यदि (!senip) {
            senip = m_seniAlwaysp;
            fromAlways = सत्य;
        }
        यदि (!senip) {
            nodep->v3error("Concurrent assertion has no clock (IEEE 1800-2023 16.16)\n"
                           << nodep->warnMore()
                           << "... Suggest provide a clocking event, a default"
                              " clocking, or a clocked procedural context");
            newp = नूतन Ast_Sen_रूख{nodep->fileline(), nullptr};
        } अन्यथा {
            यदि (cassertp && fromAlways) cassertp->senFromAlways(सत्य);
            newp = नूतन Ast_Sen_रूख{nodep->fileline(), senip->clone_रूख(सत्य)};
        }
        फर्काऊ newp;
    }
    Ast_गाँठो_अभिव्यक्ति* getSequenceBodyExprp(अचर AstSequence* अचर seqp) अचर {
        // The statements in AstSequence are optional AstVar (ports) followed by body expr.
        Ast_गाँठो* bodyp = seqp->stmtsp();
        जबसम्म (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        फर्काऊ VN_CAST(bodyp, गाँठो_अभिव्यक्ति);
    }
    AstPropSpec* getPropertyExprp(अचर AstProperty* अचर propp) {
        // Statements in AstProperty: AstVar (ports/local vars),
        // AstInitialStaticStmt/AstInitialAutomaticStmt (var init), AstPropSpec (body).
        Ast_गाँठो* propExprp = propp->stmtsp();
        जबसम्म (propExprp
               && (VN_IS(propExprp, Var) || VN_IS(propExprp, InitialStaticStmt)
                   || VN_IS(propExprp, InitialAutomaticStmt))) {
            propExprp = propExprp->nextp();
        }
        फर्काऊ VN_CAST(propExprp, PropSpec);
    }
    शून्य substituteSequenceCall(AstFuncRef* funcrefp, AstSequence* seqp) {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Inline the sequence body at the call site, replacing the FuncRef
        Ast_गाँठो_अभिव्यक्ति* bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        // Clone the body expression since the sequence may be referenced multiple times
        Ast_गाँठो_अभिव्यक्ति* clonedp = bodyExprp->clone_रूख(असत्य);
        // Build substitution map, then do a single traversal to replace all formals
        // (textual substitution per IEEE 16.8.2).
        अचर V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<अचर AstVar*, Ast_गाँठो_अभिव्यक्ति*> portMap;
        लागि (अचर स्वतः& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            अचर स्वतः it = portMap.find(refp->varp());
            यदि (it != portMap.end()) {
                refp->replaceWith(it->second->clone_रूख(असत्य));
                VL_DO_DANGLING(थप_Deletep(refp), refp);
            }
        });
        लागि (अचर स्वतः& tconnect : tconnects) {
            थप_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        // Replace the FuncRef with the inlined body
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(थप_Deletep(funcrefp), funcrefp);
        // Clear referenced flag; sequences with isReferenced==false are deleted in assertPreAll
        seqp->isReferenced(असत्य);
    }
    AstPropSpec* clonePropertyBody(AstFuncRef* funcrefp, अचर AstProperty* propp) {
        AstPropSpec* propExprp = getPropertyExprp(propp);
        // Substitute inner property call before copying in order to not doing the same for
        // each call of outer property call.
        propExprp = substitutePropertyCall(propExprp);
        // Clone subtree after substitution. It is needed, because property might be called
        // multiple times with different arguments.
        propExprp = propExprp->clone_रूख(असत्य);
        // Build substitution maps for formal arguments and property-local
        // variables, then perform a single foreach to apply all replacements.
        // Map port vars to their actual argument expressions
        अचर V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propp->stmtsp());
        std::unordered_map<अचर AstVar*, Ast_गाँठो_अभिव्यक्ति*> portMap;
        लागि (अचर स्वतः& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }

        // Promote property-local variables (non-port vars, IEEE 16.10) to
        // module-level __Vpropvar temps. Cross-cycle persistence is handled
        // by the match item lowering in visit(AstImplication*).
        std::unordered_map<अचर AstVar*, AstVar*> localVarMap;
        लागि (Ast_गाँठो* stmtp = propp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (AstVar* अचर varp = VN_CAST(stmtp, Var)) {
                यदि (!varp->isIO()) {
                    अचर string newName = m_propVarNames.get(varp);
                    AstVar* अचर newVarp = नूतन AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->addStmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        // Single traversal: substitute ports and update local var references
        propExprp->foreach([&](AstVarRef* refp) {
            अचर स्वतः portIt = portMap.find(refp->varp());
            यदि (portIt != portMap.end()) {
                refp->replaceWith(portIt->second->clone_रूख(असत्य));
                VL_DO_DANGLING(थप_Deletep(refp), refp);
                फर्काऊ;
            }
            अचर स्वतः localIt = localVarMap.find(refp->varp());
            यदि (localIt != localVarMap.end()) refp->varp(localIt->second);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        // Clean up argument expressions
        लागि (अचर स्वतः& tconnect : tconnects) {
            थप_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(propExprp, funcrefp);
        फर्काऊ propExprp;
    }
    AstPropSpec* substitutePropertyCall(AstPropSpec* nodep) {
        यदि (AstFuncRef* अचर funcrefp = VN_CAST(nodep->propp(), FuncRef)) {
            यदि (अचर AstProperty* अचर propp = VN_CAST(funcrefp->taskp(), Property)) {
                AstPropSpec* अचर propExprp = clonePropertyBody(funcrefp, propp);

                // Handle case with 2 disable iff statement (IEEE 1800-2023 16.12.1)
                यदि (nodep->disablep() && propExprp->disablep()) {
                    nodep->v3error("disable iff expression before property call and in its "
                                   "body is not legal");
                    थप_Deletep(propExprp->disablep()->unlinkFrBack());
                }
                // If disable iff is in outer property, move it to inner
                यदि (nodep->disablep()) {
                    Ast_गाँठो_अभिव्यक्ति* अचर disablep = nodep->disablep()->unlinkFrBack();
                    propExprp->disablep(disablep);
                }

                यदि (nodep->sensesp() && propExprp->sensesp()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Clock event before property call and in its body");
                    थप_Deletep(propExprp->sensesp()->unlinkFrBack());
                }
                // If clock event is in outer property, move it to inner
                यदि (nodep->sensesp()) {
                    AstSenItem* अचर sensesp = nodep->sensesp();
                    sensesp->unlinkFrBack();
                    propExprp->sensesp(sensesp);
                }

                // Now substitute property reference with property body
                nodep->replaceWith(propExprp);
                VL_DO_DANGLING(थप_Deletep(nodep), nodep);
                फर्काऊ propExprp;
            }
        }
        फर्काऊ nodep;
    }

    शून्य substituteNestedPropertyCall(AstFuncRef* funcrefp, अचर AstProperty* propp) {
        AstPropSpec* अचर specp = clonePropertyBody(funcrefp, propp);
        Ast_गाँठो* replp = nullptr;
        यदि (specp->disablep()) {
            funcrefp->v3error("Illegal 'disable iff' in a nested property instance"
                              " (IEEE 1800-2023 16.12.1)");
        } अन्यथा यदि (specp->sensesp()) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: clock event in a nested property instance");
        } अन्यथा यदि (specp->propp()->exists(
                       [](अचर Ast_गाँठो_अभिव्यक्ति* np) { फर्काऊ np->isMultiCycleSva(); })) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: multi-cycle body in a nested property instance");
        } अन्यथा {
            replp = specp->propp()->unlinkFrBack();
        }
        यदि (!replp) replp = नूतन AstConst{funcrefp->fileline(), AstConst::BitFalse{}};
        funcrefp->replaceWith(replp);
        VL_DO_DANGLING(थप_Deletep(funcrefp), funcrefp);
        VL_DO_DANGLING(थप_Deletep(specp), specp);
    }

    // VISITORS
    //========== Statements
    शून्य visit(AstClocking* अचर nodep) override {
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
        यदि (nodep->eventp()) nodep->addNextHere(nodep->eventp()->unlinkFrBack());
        VL_DO_DANGLING(थप_Deletep(nodep->unlinkFrBack()), nodep);
    }
    शून्य visit(AstModportClockingRef* अचर nodep) override {
        // It has to be converted to a list of ModportClockingVarRefs,
        // because clocking blocks are removed in this pass
        लागि (Ast_गाँठो* itemp = nodep->clockingp()->itemsp(); itemp; itemp = itemp->nextp()) {
            यदि (AstClockingItem* अचर citemp = VN_CAST(itemp, ClockingItem)) {
                यदि (AstVar* अचर varp
                    = citemp->varp() ? citemp->varp() : VN_AS(citemp->user1p(), Var)) {
                    AstModportVarRef* अचर modVarp = नूतन AstModportVarRef{
                        nodep->fileline(), varp->name(), citemp->direction()};
                    modVarp->varp(varp);
                    nodep->addNextHere(modVarp);
                    यदि (citemp->direction().isOutput()) {
                        // Drives through the modport set the drive flag
                        AstVar* अचर drivenVarp = getCreateDrivenVarp(citemp);
                        AstModportVarRef* अचर drivenRefp = नूतन AstModportVarRef{
                            nodep->fileline(), drivenVarp->name(), VDirection::OUTPUT};
                        drivenRefp->varp(drivenVarp);
                        nodep->addNextHere(drivenRefp);
                    }
                }
            }
        }
        VL_DO_DANGLING(थप_Deletep(nodep->unlinkFrBack()), nodep);
    }
    शून्य visit(AstClockingItem* अचर nodep) override {
        // Get a ref to the sampled/driven variable
        AstVar* अचर varp = nodep->varp();
        यदि (!varp) {
            // Unused item
            फर्काऊ;
        }
        // Flag set by drives of an output clockvar, possibly visited before this item
        यदि (nodep->direction().isOutput()) m_modp->addStmtsp(getCreateDrivenVarp(nodep));
        File_पङ्क्ति* अचर flp = nodep->fileline();
        V3Const::constifyEdit(nodep->skewp());
        यदि (!VN_IS(nodep->skewp(), Const)) {
            nodep->skewp()->v3error("Skew must be constant (IEEE 1800-2023 14.4)");
            फर्काऊ;
        }
        AstConst* अचर skewp = VN_AS(nodep->skewp(), Const);
        यदि (skewp->अङ्क().isNegative()) skewp->v3error("Skew cannot be negative");
        Ast_गाँठो_अभिव्यक्ति* अचर exprp = nodep->exprp();
        varp->name(m_clockingp->name() + "__DOT__" + varp->name());
        m_clockingp->addNextHere(varp->unlinkFrBack());
        nodep->user1p(varp);
        varp->user1p(nodep);
        यदि (nodep->direction() == VDirection::OUTPUT) {
            exprp->foreach([](अचर Ast_गाँठो_Var_Ref* varrefp) {
                // Prevent confusing BLKANDNBLK warnings on clockvars due to generated assignments
                varrefp->fileline()->warnOff(V3ErrorCode::BLKANDNBLK, सत्य);
                // Mark the driven signal so later passes (e.g. V3Inst port
                // connection) know the drive flows out from inside the cell.
                // Item expressions are plain signal references, so only the
                // driven signal is marked, never index selects.
                varrefp->varp()->isClockingDriven(सत्य);
            });
            AstVarRef* अचर skewedReadRefp = नूतन AstVarRef{flp, varp, VAccess::READ};
            skewedReadRefp->user1(सत्य);
            // Initialize the clockvar
            AstVarRef* अचर skewedWriteRefp = नूतन AstVarRef{flp, varp, VAccess::WRITE};
            skewedWriteRefp->user1(सत्य);
            AstInitialStatic* अचर initClockvarp = नूतन AstInitialStatic{
                flp, नूतन AstAssign{flp, skewedWriteRefp, exprp->clone_रूख_Pure(असत्य)}};
            m_modp->addStmtsp(initClockvarp);
            // Each drive sets a flag, so the signal is also assigned when driven with the same
            // value again, e.g. after another assignment to the signal (IEEE 1800-2023 14.16.2)
            AstVar* अचर drivenVarp = getCreateDrivenVarp(nodep);
            m_modp->addStmtsp(नूतन AstInitialStatic{
                flp, नूतन AstAssign{flp, नूतन AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                   नूतन AstConst{flp, AstConst::BitFalse{}}}});
            // Assign the clockvar to the actual var if it was driven
            AstAssign* अचर assignp
                = नूतन AstAssign{flp, exprp->clone_रूख_Pure(असत्य), skewedReadRefp};
            AstIf* अचर ifp
                = नूतन AstIf{flp, नूतन AstVarRef{flp, drivenVarp, VAccess::READ},
                            नूतन AstAssign{flp, नूतन AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                          नूतन AstConst{flp, AstConst::BitFalse{}}}};
            ifp->addThensp(assignp);
            // Only in the time step of the clocking event
            AstCMethodHard* अचर trigp = नूतन AstCMethodHard{
                nodep->fileline(), नूतन AstVarRef{flp, m_clockingp->ensureEventp(), VAccess::READ},
                VCMethod::EVENT_IS_TRIGGERED};
            trigp->dtypeSetBit();
            ifp->condp(नूतन AstLogAnd{flp, ifp->condp()->unlinkFrBack(), trigp});
            // A skew rounding to no time precision ticks is a zero skew
            Ast_गाँठो_अभिव्यक्ति* अचर ticksp
                = V3Timing::delayToTicks(skewp->clone_रूख(असत्य), m_modp->timeunit());
            अचर तार्किक zeroSkew = ticksp->isZero();
            VL_DO_DANGLING(ticksp->delete_रूख(), ticksp);
            यदि (zeroSkew || !skewp->fileline()->timingOn()) {
                // Drive the var in Re-NBA (IEEE 1800-2023 14.16), ignoring skews where timing is
                // turned off
                Ast_Sen_रूख* senTreep = m_clockingp->sentreep()->clone_रूख(असत्य);
                senTreep->addSensesp(नूतन AstSenItem{
                    flp, VEdgeType::ET_CHANGED, नूतन AstVarRef{flp, drivenVarp, VAccess::READ}});
                m_clockingp->addNextHere(नूतन AstAlwaysReactive{flp, senTreep, ifp});
            } अन्यथा {
                // Fork only the assignment, so this AlwaysPostponed can be retriggered
                // before the assignment happens
                AstFork* अचर forkp = नूतन AstFork{flp, VJoinType::JOIN_NONE};
                forkp->immediateStart(सत्य);
                forkp->addForksp(नूतन AstBegin{flp, "", assignp->unlinkFrBack(), सत्य});
                ifp->addThensp(forkp);
                // Read the clockvar in Postponed, so a drive late in the time step is applied
                // at this event plus the skew (IEEE 1800-2023 14.16)
                m_clockingp->addNextHere(नूतन AstAlwaysPostponed{flp, ifp});
                यदि (v3Global.opt.timing().isSetTrue()) {
                    AstDelay* अचर delayp = नूतन AstDelay{flp, skewp->unlinkFrBack(), असत्य};
                    delayp->timeunit(m_modp->timeunit());
                    assignp->timingControlp(delayp);
                } अन्यथा यदि (v3Global.opt.timing().isSetFalse()) {
                    nodep->v3warn(E_NOTIMING,
                                  "Clocking output skew greater than #0 requires --timing");
                } अन्यथा {
                    nodep->v3warn(E_NEEDTIMINGOPT,
                                  "Use --timing or --no-timing to specify how "
                                  "clocking output skew greater than #0 should be handled");
                }
            }
        } अन्यथा यदि (nodep->direction() == VDirection::आगत) {
            // Ref to the clockvar
            AstVarRef* अचर refp = नूतन AstVarRef{flp, varp, VAccess::WRITE};
            refp->user1(सत्य);
            यदि (skewp->अङ्क().is1Step()) {
                // #1step means the value that is sampled is always the signal's last value
                // before the clock edge (IEEE 1800-2023 14.4)
                AstSampled* अचर sampledp
                    = नूतन AstSampled{flp, exprp->clone_रूख_Pure(असत्य), exprp->dtypep(), सत्य};
                AstAssign* अचर assignp = नूतन AstAssign{flp, refp, sampledp};
                m_clockingp->addNextHere(नूतन AstAlways{
                    flp, VAlwaysKwd::ALWAYS, m_clockingp->sentreep()->clone_रूख(असत्य), assignp});
            } अन्यथा यदि (skewp->isZero()) {
                // #0 means the var has to be sampled in Observed (IEEE 1800-2023 14.13)
                AstAssign* अचर assignp = नूतन AstAssign{flp, refp, exprp->clone_रूख_Pure(असत्य)};
                m_clockingp->addNextHere(नूतन AstAlwaysObserved{
                    flp, m_clockingp->sentreep()->clone_रूख(असत्य), assignp});
            } अन्यथा {
                // Create a queue where we'll store sampled values with timestamps
                AstSampleQueueDType* अचर queueDtp
                    = नूतन AstSampleQueueDType{flp, exprp->dtypep()};
                m_netlistp->typeTablep()->addTypesp(queueDtp);
                AstVar* अचर queueVarp
                    = नूतन AstVar{flp, VVarType::MODULETEMP, "__Vqueue__" + varp->name(), queueDtp};
                queueVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_clockingp->addNextHere(queueVarp);
                // Create a process like this:
                //     always queue.push(<sampled var>);
                AstCMethodHard* अचर pushp = नूतन AstCMethodHard{
                    flp, नूतन AstVarRef{flp, queueVarp, VAccess::WRITE}, VCMethod::DYN_थप,
                    नूतन AstTime{nodep->fileline(), m_modp->timeunit()}};
                pushp->addPinsp(exprp->clone_रूख_Pure(असत्य));
                pushp->dtypeSetVoid();
                m_clockingp->addNextHere(
                    नूतन AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, pushp->makeStmt()});
                // Create a process like this:
                //     always @<clocking event> queue.pop(<skew>, /*out*/<skewed var>);
                AstCMethodHard* अचर popp = नूतन AstCMethodHard{
                    flp, नूतन AstVarRef{flp, queueVarp, VAccess::READWRITE}, VCMethod::DYN_झिक,
                    नूतन AstTime{nodep->fileline(), m_modp->timeunit()}};
                popp->addPinsp(skewp->unlinkFrBack());
                refp->access(VAccess::READWRITE);  // Only conditionally assigned
                popp->addPinsp(refp);
                popp->dtypeSetVoid();
                m_clockingp->addNextHere(नूतन AstAlways{flp, VAlwaysKwd::ALWAYS,
                                                       m_clockingp->sentreep()->clone_रूख(असत्य),
                                                       popp->makeStmt()});
            }
        } अन्यथा {
            nodep->v3fatalSrc("Invalid direction");
        }
    }
    शून्य visit(AstDelay* nodep) override {
        m_hasCycleDelay |= nodep->isCycleDelay();
        // Only cycle delays are relevant in this stage; also only process once
        यदि (!nodep->isCycleDelay()) {
            यदि (m_inSynchDrive) {
                nodep->v3error("Only cycle delays can be used in synchronous drives"
                               " (IEEE 1800-2023 14.16)");
            }
            iterateChildren(nodep);
            फर्काऊ;
        }
        यदि (m_inAssign && !m_inSynchDrive) {
            nodep->v3error("Cycle delays not allowed as intra-assignment delays"
                           " (IEEE 1800-2023 14.11)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_रूख(), nodep);
            फर्काऊ;
        }
        यदि (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBackWithNext());
        File_पङ्क्ति* अचर flp = nodep->fileline();
        Ast_गाँठो_अभिव्यक्ति* valuep = V3Const::constifyEdit(nodep->lhsp()->unlinkFrBack());
        अचर AstConst* अचर constp = VN_CAST(valuep, Const);
        यदि (!constp && m_pexprp) {
            // V3AssertNfa handles non-const delays in sequences before this pass and
            // replaces the property; this branch should never be reached.
            nodep->v3fatalSrc("Non-constant cycle delay in assertion: "
                              "should have been caught by V3AssertNfa");
        } अन्यथा यदि (constp && constp->isZero()) {
            VL_DO_DANGLING(थप_Deletep(valuep), valuep);
            यदि (m_inSynchDrive) {
                // ##0 has no effect in synchronous drives (IEEE 1800-2023 14.11)
                VL_DO_DANGLING(थप_Deletep(nodep->unlinkFrBack()), nodep);
                फर्काऊ;
            }
            यदि (m_pexprp) {
                // ##0 in sequence context = zero delay = same clock tick
                VL_DO_DANGLING(थप_Deletep(nodep->unlinkFrBack()), nodep);
                फर्काऊ;
            }
            // Procedural ##0: synchronize with default clocking event (IEEE 1800-2023 14.11)
            यदि (!m_defaultClockingp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(थप_Deletep(nodep->unlinkFrBack()), nodep);
                फर्काऊ;
            }
            nodep->replaceWith(newCycleDelayZerop(flp));
            VL_DO_DANGLING(थप_Deletep(nodep), nodep);
            फर्काऊ;
        }
        Ast_Sen_रूख* sentreep = nullptr;
        यदि (!m_drives.empty()) {
            यदि (m_inConcatAssign) {
                // Also the clockvars may be of different clocking blocks
                nodep->v3error("Cycle delays cannot be used in synchronous drives to a"
                               " concatenation (IEEE 1800-2023 14.16)");
                VL_DO_DANGLING(थप_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(थप_Deletep(valuep), valuep);
                फर्काऊ;
            }
            // Count the cycles of the driven clockvar's clocking block (IEEE 1800-2023 14.16)
            sentreep = newDriveSentreep(nodep, m_drives.front());
            यदि (!sentreep) {
                VL_DO_DANGLING(थप_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(थप_Deletep(valuep), valuep);
                फर्काऊ;
            }
        } अन्यथा यदि (!m_defaultClockingp) {
            यदि (!m_pexprp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(थप_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(थप_Deletep(valuep), valuep);
                फर्काऊ;
            }
            sentreep = नूतन Ast_Sen_रूख{flp, m_senip->clone_रूख(असत्य)};
        } अन्यथा {
            sentreep = m_defaultClockingp->sentreep()->clone_रूख(असत्य);
        }
        AstEventControl* अचर controlp
            = नूतन AstEventControl{nodep->fileline(), sentreep, nullptr};
        अचर std::string delayName = m_cycleDlyNames.get(nodep);
        Ast_गाँठो_अभिव्यक्ति* throughoutp
            = nodep->throughoutp() ? nodep->throughoutp()->unlinkFrBack() : nullptr;

        AstVar* अचर cntVarp = नूतन AstVar{flp, VVarType::BLOCKTEMP, delayName + "__counter",
                                           nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstBegin* अचर beginp = नूतन AstBegin{flp, delayName + "__block", cntVarp, सत्य};
        // A non-constant delay is counted at runtime. The count is integral of 64 bits, as
        // V3Width makes delays, or real, which is rounded (IEEE 1800-2023 6.12.1)
        यदि (valuep->isDouble()) {
            valuep = नूतन AstRToIRoundS{flp, valuep};
            valuep->dtypeFrom(cntVarp);
        } अन्यथा {
            valuep = नूतन AstSel{flp, valuep, 0, cntVarp->width()};
        }
        beginp->addStmtsp(नूतन AstAssign{flp, नूतन AstVarRef{flp, cntVarp, VAccess::WRITE}, valuep});
        // If the count is zero, a synchronous drive waits for none, like with a constant '##0',
        // and a procedural delay waits like a procedural '##0' (IEEE 1800-2023 14.11)
        यदि (!constp && !m_inSynchDrive) {
            beginp->addStmtsp(नूतन AstIf{
                flp,
                नूतन AstEq{flp, नूतन AstVarRef{flp, cntVarp, VAccess::READ}, नूतन AstConst{flp, 0}},
                newCycleDelayZerop(flp)});
        }

        // Throughout: create flag tracking whether condition held every tick
        AstVar* throughoutOkp = nullptr;
        यदि (throughoutp) {
            throughoutOkp = नूतन AstVar{flp, VVarType::BLOCKTEMP, delayName + "__throughoutOk",
                                       nodep->findBasicDType(VBasicDTypeKwd::LOGIC_IMPLICIT)};
            throughoutOkp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->addStmtsp(throughoutOkp);
            beginp->addStmtsp(नूतन AstAssign{flp, नूतन AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            नूतन AstConst{flp, AstConst::BitTrue{}}});
            // Check condition at tick 0 (sequence start, before entering loop)
            beginp->addStmtsp(
                नूतन AstIf{flp, नूतन AstLogNot{flp, throughoutp->clone_रूख_Pure(असत्य)},
                          नूतन AstAssign{flp, नूतन AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                        नूतन AstConst{flp, AstConst::BitFalse{}}}});
        }

        {
            AstLoop* अचर loopp = नूतन AstLoop{flp};
            // When throughout is present, exit loop early if condition fails
            Ast_गाँठो_अभिव्यक्ति* loopCondp
                = नूतन AstGt{flp, नूतन AstVarRef{flp, cntVarp, VAccess::READ}, नूतन AstConst{flp, 0}};
            यदि (throughoutOkp) {
                loopCondp = नूतन AstLogAnd{flp, loopCondp,
                                          नूतन AstVarRef{flp, throughoutOkp, VAccess::READ}};
            }
            loopp->addStmtsp(नूतन AstLoopTest{flp, loopp, loopCondp});
            loopp->addStmtsp(controlp);
            loopp->addStmtsp(
                नूतन AstAssign{flp, नूतन AstVarRef{flp, cntVarp, VAccess::WRITE},
                              नूतन AstSub{flp, नूतन AstVarRef{flp, cntVarp, VAccess::READ},
                                         नूतन AstConst{flp, 1}}});
            // Check throughout condition at each tick during delay (IEEE 1800-2023 16.9.9)
            यदि (throughoutp) {
                loopp->addStmtsp(
                    नूतन AstIf{flp, नूतन AstLogNot{flp, throughoutp},
                              नूतन AstAssign{flp, नूतन AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            नूतन AstConst{flp, AstConst::BitFalse{}}}});
            }
            beginp->addStmtsp(loopp);
        }
        // Compose wrappers on remaining sequence: throughout gate (inner), disable iff (outer)
        Ast_गाँठो* remainp = nodep->nextp() ? nodep->nextp()->unlinkFrBackWithNext() : nullptr;
        यदि (throughoutOkp) {
            // If condition failed during delay, fail assertion
            remainp = नूतन AstIf{flp, नूतन AstVarRef{flp, throughoutOkp, VAccess::READ}, remainp,
                                नूतन AstPExprClause{flp, /*pass=*/असत्य}};
        }
        यदि (m_disableSeqIfp && remainp) {
            AstIf* अचर disableSeqIfp = m_disableSeqIfp->clone_रूख(असत्य);
            disableSeqIfp->addThensp(remainp);
            remainp = disableSeqIfp;
        }
        यदि (remainp) {
            यदि (throughoutOkp) {
                // throughoutOkp is declared in beginp scope -- check must be inside it
                beginp->addStmtsp(remainp);
            } अन्यथा {
                nodep->addNextHere(remainp);
            }
        }
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_रूख(), nodep);
    }
    शून्य visit(Ast_Sen_रूख* nodep) override {
        यदि (m_inSynchDrive) {
            nodep->v3error("Event controls cannot be used in "
                           "synchronous drives (IEEE 1800-2023 14.16)");
        }

        अचर AstSampled* sampledp;
        यदि (nodep->exists([&sampledp](अचर AstSampled* अचर sp) {
                sampledp = sp;
                फर्काऊ सत्य;
            })) {
            sampledp->v3warn(E_UNSUPPORTED, "Unsupported: $sampled inside sensitivity list");
        }
    }
    शून्य visit(Ast_गाँठो_Var_Ref* nodep) override {
        UINFO(8, " -varref:  " << nodep);
        UINFO(8, " -varref-var-back:  " << nodep->varp()->backp());
        UINFO(8, " -varref-var-user1:  " << nodep->varp()->user1p());
        यदि (AstClockingItem* अचर itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->firstAbovep(),
                ClockingItem)) {
            यदि (nodep->user1()) फर्काऊ;

            // ensure linking still works, this has to be done only once
            यदि (AstVarXRef* xrefp = VN_CAST(nodep, VarXRef)) {
                UINFO(8, " -clockvarxref-in:  " << xrefp);
                string dotted = xrefp->dotted();
                अचर माप_प्रकार dotPos = dotted.rfind('.');
                dotted.erase(dotPos, string::npos);
                xrefp->dotted(dotted);
                UINFO(8, " -clockvarxref-out: " << xrefp);
                m_xrefsp.emplace_back(xrefp);
            }

            यदि (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            यदि (nodep->access().isWriteOrRW()) {
                यदि (itemp->direction() == VDirection::OUTPUT) {
                    यदि (!m_inAssignDlyLhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    यदि (m_inAssign) {
                        m_inSynchDrive = सत्य;
                        m_drives.थप_back({itemp, nodep});
                    }
                } अन्यथा यदि (itemp->direction() == VDirection::आगत) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
            nodep->user1(सत्य);
        }
    }
    शून्य visit(AstMemberSel* nodep) override {
        यदि (AstClockingItem* अचर itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->firstAbovep(),
                ClockingItem)) {
            यदि (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            यदि (nodep->access().isWriteOrRW()) {
                यदि (itemp->direction() == VDirection::OUTPUT) {
                    यदि (!m_inAssignDlyLhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    यदि (m_inAssign) {
                        m_inSynchDrive = सत्य;
                        m_drives.थप_back({itemp, nodep});
                    }
                } अन्यथा यदि (itemp->direction() == VDirection::आगत) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
        }
    }
    शून्य visit(Ast_गाँठो_Assign* nodep) override {
        यदि (nodep->user1()) फर्काऊ;
        VL_RESTORER(m_inAssign);
        VL_RESTORER(m_inSynchDrive);
        VL_RESTORER(m_inConcatAssign);
        VL_RESTORER_CLEAR(m_drives);
        m_inAssign = सत्य;
        m_inSynchDrive = असत्य;
        {
            // The braces of a concatenation are a replication, by one
            अचर Ast_गाँठो_अभिव्यक्ति* lhsp = nodep->lhsp();
            यदि (अचर AstReplicate* अचर repp = VN_CAST(lhsp, Replicate)) lhsp = repp->srcp();
            m_inConcatAssign = VN_IS(lhsp, Concat);
        }
        {
            VL_RESTORER(m_inAssignDlyLhs);
            m_inAssignDlyLhs = VN_IS(nodep, AssignDly);
            iterate(nodep->lhsp());
        }
        iterate(nodep->rhsp());
        यदि (nodep->timingControlp()) iterate(nodep->timingControlp());
        // A drive without a cycle delay, also after removing '##0', updates the clockvar now
        यदि (m_inSynchDrive && !nodep->timingControlp()) {
            AstAssign* अचर assignp = नूतन AstAssign{
                nodep->fileline(), nodep->lhsp()->unlinkFrBack(), nodep->rhsp()->unlinkFrBack()};
            assignp->user1(सत्य);
            nodep->replaceWith(assignp);
            VL_DO_DANGLING(nodep->delete_रूख(), nodep);
            nodep = assignp;
        }
        // Note the drives when they take effect, after the cycle delay if any. Each clockvar
        // of the LHS is driven, e.g. in a concatenation which other simulators accept, although
        // IEEE 1800-2023 14.16 does not allow it
        Ast_गाँठो* setsp = nullptr;
        लागि (अचर SynchDrive& drive : m_drives) {
            setsp = Ast_गाँठो::addNext(setsp, newDrivenSetp(nodep->fileline(), drive));
        }
        यदि (!setsp) फर्काऊ;
        यदि (AstBegin* अचर beginp = VN_CAST(nodep->timingControlp(), Begin)) {
            beginp->addStmtsp(setsp);
        } अन्यथा {
            nodep->addNextHere(setsp);
        }
    }
    शून्य visit(AstAlways* nodep) override {
        iterateAndNextNull(nodep->sentreep());
        यदि (nodep->sentreep()) m_seniAlwaysp = nodep->sentreep()->sensesp();
        iterateAndNextNull(nodep->stmtsp());
        m_seniAlwaysp = nullptr;
    }

    शून्य visit(Ast_गाँठो_Cover_Or_Assert* nodep) override {
        यदि (nodep->sentreep()) फर्काऊ;  // Already processed

        VL_RESTORER(m_senip);
        VL_RESTORER(m_disablep);
        m_senip = nullptr;
        m_disablep = nullptr;

        // Find Clocking's buried under nodep->exprsp
        iterateChildren(nodep);
        यदि (!nodep->immediate()) nodep->sentreep(new_Sen_रूख(nodep, nullptr, nodep));
    }
    शून्य visit(AstFalling* nodep) override {
        यदि (nodep->user1SetOnce()) फर्काऊ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_पङ्क्ति* अचर fl = nodep->fileline();
        Ast_गाँठो_अभिव्यक्ति* exprp = nodep->exprp()->unlinkFrBack();
        यदि (exprp->width() > 1) exprp = नूतन AstSel{fl, exprp, 0, 1};
        Ast_गाँठो_अभिव्यक्ति* अचर futurep = नूतन AstFuture{fl, exprp, new_Sen_रूख(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = नूतन AstAnd{fl, exprp->clone_रूख_Pure(असत्य), नूतन AstNot{fl, futurep}};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }
    शून्य visit(AstFell* nodep) override {
        यदि (nodep->user1SetOnce()) फर्काऊ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_पङ्क्ति* अचर fl = nodep->fileline();
        Ast_गाँठो_अभिव्यक्ति* exprp = nodep->exprp()->unlinkFrBack();
        यदि (exprp->width() > 1) exprp = नूतन AstSel{fl, exprp, 0, 1};
        Ast_Sen_रूख* sentreep = nodep->sentreep();
        यदि (sentreep) sentreep->unlinkFrBack();
        AstPast* अचर pastp = नूतन AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_रूख(nodep, sentreep));
        exprp = नूतन AstAnd{fl, pastp, नूतन AstNot{fl, exprp->clone_रूख_Pure(असत्य)}};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }
    शून्य visit(AstFuture* nodep) override {
        यदि (nodep->user1SetOnce()) फर्काऊ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        Ast_Sen_रूख* अचर sentreep = nodep->sentreep();
        यदि (sentreep) VL_DO_DANGLING(थप_Deletep(sentreep->unlinkFrBack()), sentreep);
        nodep->sentreep(new_Sen_रूख(nodep));
    }
    शून्य visit(AstPast* nodep) override {
        checkSamplingFuncDType(nodep, nodep->exprp());
        यदि (nodep->sentreep()) फर्काऊ;  // Already processed
        iterateChildren(nodep);
        nodep->sentreep(new_Sen_रूख(nodep));
    }
    शून्य visit(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- Lower standalone exact [*N] (N >= 2) via saturating counter.
        // Range/unbounded forms and SExpr-contained forms are lowered by V3AssertNfa.
        iterateChildren(nodep);
        // V3AssertNfa handles unbounded/ranged forms upstream, so this fast-path
        // is effectively unreachable when NFA is enabled.
        यदि (nodep->unbounded() || nodep->maxCountp()) फर्काऊ;  // LCOV_EXCL_LINE
        अचर AstConst* अचर constp = VN_CAST(nodep->countp(), Const);
        यदि (VL_UNLIKELY(!constp || constp->toSInt() < 1)) {
            nodep->v3fatalSrc("Consecutive repetition count must be a positive constant"
                              " (should have been caught by V3Width)");
            फर्काऊ;
        }
        अचर पूर्णाङ्क n = constp->toSInt();
        File_पङ्क्ति* अचर flp = nodep->fileline();
        Ast_गाँठो_अभिव्यक्ति* अचर exprp = nodep->exprp()->unlinkFrBack();
        यदि (n == 1) {
            nodep->replaceWith(exprp);
            VL_DO_DANGLING(थप_Deletep(nodep), nodep);
            फर्काऊ;
        }
        // Saturating counter: if (expr) cnt <= min(cnt+1, N); else cnt <= 0;
        AstVar* अचर cntVarp = नूतन AstVar{flp, VVarType::MODULETEMP, m_consRepNames.get(""),
                                           nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(cntVarp);
        Ast_गाँठो_अभिव्यक्ति* अचर अभिव्यक्ति_Clonep = exprp->clone_रूख_Pure(असत्य);
        Ast_गाँठो_अभिव्यक्ति* अचर saturatingIncrp = नूतन AstCond{
            flp,
            नूतन AstLt{flp, नूतन AstVarRef{flp, cntVarp, VAccess::READ},
                      नूतन AstConst{flp, static_cast<uint32_t>(n)}},
            नूतन AstAdd{flp, नूतन AstVarRef{flp, cntVarp, VAccess::READ}, नूतन AstConst{flp, 1u}},
            नूतन AstConst{flp, static_cast<uint32_t>(n)}};
        AstAssignDly* अचर incrAssignp
            = नूतन AstAssignDly{flp, नूतन AstVarRef{flp, cntVarp, VAccess::WRITE}, saturatingIncrp};
        AstAssignDly* अचर resetAssignp = नूतन AstAssignDly{
            flp, नूतन AstVarRef{flp, cntVarp, VAccess::WRITE}, नूतन AstConst{flp, 0u}};
        AstIf* अचर ifp = नूतन AstIf{flp, अभिव्यक्ति_Clonep, incrAssignp, resetAssignp};
        Ast_Sen_रूख* अचर senTreep = new_Sen_रूख(nodep);
        AstAlways* अचर alwaysp = नूतन AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, ifp};
        cntVarp->addNextHere(alwaysp);
        // Match: cnt >= N-1 (previous cycles via NBA) && expr (current cycle)
        Ast_गाँठो_अभिव्यक्ति* अचर cntCheckp = नूतन AstGte{flp, नूतन AstVarRef{flp, cntVarp, VAccess::READ},
                                                  नूतन AstConst{flp, static_cast<uint32_t>(n - 1)}};
        cntCheckp->dtypeSetBit();
        Ast_गाँठो_अभिव्यक्ति* अचर matchp = नूतन AstAnd{flp, cntCheckp, exprp};
        matchp->dtypeSetBit();
        nodep->replaceWith(matchp);
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }
    शून्य visit(AstRising* nodep) override {
        यदि (nodep->user1SetOnce()) फर्काऊ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_पङ्क्ति* अचर fl = nodep->fileline();
        Ast_गाँठो_अभिव्यक्ति* exprp = nodep->exprp()->unlinkFrBack();
        यदि (exprp->width() > 1) exprp = नूतन AstSel{fl, exprp, 0, 1};
        Ast_गाँठो_अभिव्यक्ति* अचर futurep = नूतन AstFuture{fl, exprp, new_Sen_रूख(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = नूतन AstAnd{fl, नूतन AstNot{fl, exprp->clone_रूख_Pure(असत्य)}, futurep};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }
    शून्य visit(AstRose* nodep) override {
        यदि (nodep->user1SetOnce()) फर्काऊ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_पङ्क्ति* अचर fl = nodep->fileline();
        Ast_गाँठो_अभिव्यक्ति* exprp = nodep->exprp()->unlinkFrBack();
        यदि (exprp->width() > 1) exprp = नूतन AstSel{fl, exprp, 0, 1};
        Ast_Sen_रूख* sentreep = nodep->sentreep();
        यदि (sentreep) sentreep->unlinkFrBack();
        AstPast* अचर pastp = नूतन AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_रूख(nodep, sentreep));
        exprp = नूतन AstAnd{fl, नूतन AstNot{fl, pastp}, exprp->clone_रूख_Pure(असत्य)};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }
    स्थिर AstAssocArrayDType* getProcessAssocArrayType(File_पङ्क्ति* अचर flp) {
        // Type of __VassertsActive___x[std::process]
        Ast_गाँठो_DType* valp
            = v3Global.rootp()->typeTablep()->findBasicDType(flp, VBasicDTypeKwd::BIT);
        AstClassRefDType* keyp
            = नूतन AstClassRefDType{flp, v3Global.rootp()->stdPackageProcessp(), nullptr};
        keyp->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        v3Global.rootp()->typeTablep()->addTypesp(keyp);
        AstAssocArrayDType* अचर typep = नूतन AstAssocArrayDType{flp, valp, keyp};
        typep->dtypep(typep);
        v3Global.rootp()->typeTablep()->addTypesp(typep);
        फर्काऊ typep;
    }
    स्थिर Ast_Stmt_अभिव्यक्ति* getProcessAssocArrayDelete(AstVarRef* अचर refp) {
        // Constructs refp.delete(std::process::self()) statement
        File_पङ्क्ति* अचर flp = refp->fileline();
        AstCMethodHard* अचर deletep = नूतन AstCMethodHard{
            flp, refp, VCMethod::ASSOC_ERASE, v3Global.rootp()->stdPackageProcessSelfp(flp)};
        deletep->dtypep(refp->findVoidDType());
        फर्काऊ नूतन Ast_Stmt_अभिव्यक्ति{flp, deletep};
    }
    स्थिर Ast_गाँठो_अभिव्यक्ति* getProcessAssocArraySize(AstVarRef* अचर refp) {
        // Constructs refp.size() statement
        AstCMethodHard* अचर sizep
            = नूतन AstCMethodHard{refp->fileline(), refp, VCMethod::ASSOC_SIZE};
        sizep->dtypep(refp->findBasicDType(VBasicDTypeKwd::UINT32));
        फर्काऊ sizep;
    }
    शून्य visit(AstSEventually* nodep) override {
        UASSERT(v3Global.rootp()->stdPackagep(), "Should be imported");

        Ast_Sen_रूख* अचर sentreep = new_Sen_रूख(nodep);
        यदि (!sentreep->sensesp()) {
            VL_DO_DANGLING(थप_Deletep(sentreep), sentreep);
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(थप_Deletep(nodep), nodep);
            फर्काऊ;
        }

        File_पङ्क्ति* अचर flp = nodep->fileline();

        // Track active assertions
        AstVar* अचर activep = नूतन AstVar{flp, VVarType::MODULETEMP, m_activeNames.get(""),
                                           getProcessAssocArrayType(flp)};
        activep->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(activep);

        // Assertion condition check
        AstLoop* अचर loopp = नूतन AstLoop{flp};
        AstSampled* अचर condp
            = नूतन AstSampled{flp, nodep->exprp()->unlinkFrBack(), nullptr, सत्य};
        loopp->addStmtsp(नूतन AstLoopTest{flp, loopp, नूतन AstLogNot{flp, condp}});
        loopp->addStmtsp(नूतन AstEventControl{flp, sentreep, nullptr});

        // Add assertion to the active set
        AstAssocSel* अचर selp = नूतन AstAssocSel{flp, नूतन AstVarRef{flp, activep, VAccess::WRITE},
                                                  v3Global.rootp()->stdPackageProcessSelfp(flp)};
        AstAssign* अचर incrementp = नूतन AstAssign{flp, selp, नूतन AstConst{flp, 1}};
        AstPExprClause* अचर clausep = नूतन AstPExprClause{flp};
        Ast_Stmt_अभिव्यक्ति* अचर deletep
            = getProcessAssocArrayDelete(नूतन AstVarRef{flp, activep, VAccess::WRITE});

        // Main assertion block
        AstBegin* अचर bodyp = नूतन AstBegin{flp, m_blockNames.get(""), nullptr, सत्य};
        bodyp->addStmtsp(incrementp);
        bodyp->addStmtsp(loopp);
        bodyp->addStmtsp(clausep);
        bodyp->addStmtsp(deletep);

        // Validate assertion condition for each active assert
        AstVar* अचर activeCountp = नूतन AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        activeCountp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstAssign* अचर initActiveCountp
            = नूतन AstAssign{flp, नूतन AstVarRef{flp, activeCountp, VAccess::WRITE},
                            getProcessAssocArraySize(नूतन AstVarRef{flp, activep, VAccess::READ})};
        AstLoop* अचर finalLoopp = नूतन AstLoop{flp};
        AstIf* अचर finalBodypCondp
            = नूतन AstIf{flp, condp->clone_रूख_Pure(असत्य), नूतन AstPExprClause{flp},
                        नूतन AstPExprClause{flp, असत्य}};
        finalLoopp->addStmtsp(
            नूतन AstLoopTest{flp, finalLoopp,
                            नूतन AstNeq{flp, नूतन AstVarRef{flp, activeCountp, VAccess::READ},
                                       नूतन AstConst{flp, 0}}});
        finalLoopp->addStmtsp(finalBodypCondp);
        finalLoopp->addStmtsp(
            नूतन AstAssign{flp, नूतन AstVarRef{flp, activeCountp, VAccess::WRITE},
                          नूतन AstSub{flp, नूतन AstVarRef{flp, activeCountp, VAccess::READ},
                                     नूतन AstConst{flp, 1}}});

        // Final assertion block
        AstBegin* अचर finalp = नूतन AstBegin{flp, m_blockNames.get(""), nullptr, सत्य};
        finalp->addStmtsp(activeCountp);
        finalp->addStmtsp(initActiveCountp);
        finalp->addStmtsp(finalLoopp);

        VL_RESTORER(m_pexprp);
        m_pexprp = नूतन AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
        VL_RESTORER(m_hasCycleDelay);
        m_hasCycleDelay = असत्य;
        iterate(bodyp);
        iterate(finalp);

        यदि (m_hasCycleDelay) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in s_eventually");
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(थप_Deletep(nodep), nodep);
            VL_DO_DANGLING(m_pexprp->delete_रूख(), m_pexprp);
            फर्काऊ;
        }

        nodep->replaceWith(m_pexprp);
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }
    शून्य visit(AstStable* nodep) override {
        यदि (nodep->user1SetOnce()) फर्काऊ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_पङ्क्ति* अचर fl = nodep->fileline();
        Ast_गाँठो_अभिव्यक्ति* exprp = nodep->exprp()->unlinkFrBack();
        Ast_Sen_रूख* sentreep = nodep->sentreep();
        यदि (sentreep) sentreep->unlinkFrBack();
        AstPast* अचर pastp = नूतन AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_रूख(nodep, sentreep));
        exprp = नूतन AstEq{fl, pastp, exprp->clone_रूख_Pure(असत्य)};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }
    शून्य visit(AstSteady* nodep) override {
        यदि (nodep->user1SetOnce()) फर्काऊ;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_पङ्क्ति* अचर fl = nodep->fileline();
        Ast_गाँठो_अभिव्यक्ति* exprp = nodep->exprp()->unlinkFrBack();
        यदि (exprp->width() > 1) exprp = नूतन AstSel{fl, exprp, 0, 1};
        Ast_गाँठो_अभिव्यक्ति* अचर futurep = नूतन AstFuture{fl, exprp, new_Sen_रूख(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = नूतन AstEq{fl, exprp->clone_रूख_Pure(असत्य), futurep};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }
    शून्य visit(AstSampled* nodep) override {
        iterateChildren(nodep);
        यदि (!nodep->internal()) checkSamplingFuncDType(nodep, nodep->exprp());
    }

    // Validate repetition count: must be a non-negative elaboration-time constant.
    // Shared by goto [->N] and nonconsecutive [=N] repetition.
    // On error, replaces nodep with BitFalse placeholder and returns nullptr.
    अचर AstConst* validate_Rep_गन्ती(Ast_गाँठो* nodep, Ast_गाँठो_अभिव्यक्ति*& countp) {
        countp = V3Const::constifyEdit(countp);
        अचर AstConst* अचर constp = VN_CAST(countp, Const);
        यदि (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(थप_Deletep(countp), countp);
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(थप_Deletep(nodep), nodep);
            फर्काऊ nullptr;
        }
        यदि (constp->toSInt() < 0) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(थप_Deletep(countp), countp);
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(थप_Deletep(nodep), nodep);
            फर्काऊ nullptr;
        }
        यदि (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(थप_Deletep(countp), countp);
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(थप_Deletep(nodep), nodep);
            फर्काऊ nullptr;
        }
        फर्काऊ constp;
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
    AstPExpr* createRepPExpr(File_पङ्क्ति* flp, Ast_गाँठो_अभिव्यक्ति* exprp, Ast_गाँठो_अभिव्यक्ति* countp,
                             Ast_गाँठो_अभिव्यक्ति* rhsp, तार्किक isOverlapped, तार्किक isNonConsec) {
        AstSenItem* अचर sensesp = m_senip;
        UASSERT_OBJ(sensesp, exprp, "Repetition requires a clock");

        अचर std::string name
            = isNonConsec ? m_nonConsRepNames.get(exprp) : m_gotoRepNames.get(exprp);
        AstVar* अचर cntVarp = नूतन AstVar{flp, VVarType::BLOCKTEMP, name + "__counter",
                                           exprp->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstBegin* अचर beginp = नूतन AstBegin{flp, name + "__block", cntVarp, सत्य};
        beginp->addStmtsp(
            नूतन AstAssign{flp, नूतन AstVarRef{flp, cntVarp, VAccess::WRITE}, नूतन AstConst{flp, 0}});

        // Counting loop: find N occurrences of expr (shared by goto and nonconsec)
        AstLoop* अचर loopp = नूतन AstLoop{flp};
        loopp->addStmtsp(नूतन AstLoopTest{flp, loopp,
                                         नूतन AstLt{flp, नूतन AstVarRef{flp, cntVarp, VAccess::READ},
                                                   countp->clone_रूख_Pure(असत्य)}});
        // if (expr) cnt++ -- sampled is applied to whole property expr by V3Assert
        loopp->addStmtsp(
            नूतन AstIf{flp, exprp,
                      नूतन AstAssign{flp, नूतन AstVarRef{flp, cntVarp, VAccess::WRITE},
                                    नूतन AstAdd{flp, नूतन AstVarRef{flp, cntVarp, VAccess::READ},
                                               नूतन AstConst{flp, 1}}}});
        loopp->addStmtsp(नूतन AstIf{
            flp, नूतन AstLt{flp, नूतन AstVarRef{flp, cntVarp, VAccess::READ}, countp},
            नूतन AstEventControl{flp, नूतन Ast_Sen_रूख{flp, sensesp->clone_रूख(असत्य)}, nullptr}});

        beginp->addStmtsp(loopp);

        यदि (isNonConsec && rhsp) {
            // IEEE 16.9.2: b[=N] = b[->N] ##1 !b[*0:$]
            // Trailing window: ##1 advance, then scan !expr cycles checking rhs.
            // Window closes when expr becomes true again (fail if rhs was never true).
            beginp->addStmtsp(नूतन AstEventControl{
                flp, नूतन Ast_Sen_रूख{flp, sensesp->clone_रूख(असत्य)}, nullptr});  // ##1
            यदि (!isOverlapped) {
                beginp->addStmtsp(नूतन AstEventControl{
                    flp, नूतन Ast_Sen_रूख{flp, sensesp->clone_रूख(असत्य)}, nullptr});  // |=>
            }
            // Window loop: check rhs at each !expr cycle (done variable for termination)
            AstVar* अचर doneVarp
                = नूतन AstVar{flp, VVarType::BLOCKTEMP, name + "__done", exprp->findBitDType()};
            doneVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->addStmtsp(doneVarp);
            beginp->addStmtsp(नूतन AstAssign{flp, नूतन AstVarRef{flp, doneVarp, VAccess::WRITE},
                                            नूतन AstConst{flp, AstConst::BitFalse{}}});
            स्वतः setDone = [&]() {
                फर्काऊ नूतन AstAssign{flp, नूतन AstVarRef{flp, doneVarp, VAccess::WRITE},
                                     नूतन AstConst{flp, AstConst::BitTrue{}}};
            };
            AstLoop* अचर windowp = नूतन AstLoop{flp};
            // LoopTest: continue while !done
            windowp->addStmtsp(नूतन AstLoopTest{
                flp, windowp, नूतन AstNot{flp, नूतन AstVarRef{flp, doneVarp, VAccess::READ}}});
            // if (expr) { fail; done = 1; } -- window closed, expr true again
            Ast_गाँठो* अचर failsp = नूतन AstPExprClause{flp, असत्य};
            failsp->addNext(setDone());
            windowp->addStmtsp(नूतन AstIf{flp, exprp->clone_रूख_Pure(असत्य), failsp});
            // if (rhs) { pass; done = 1; } -- consequent true at this !expr endpoint
            Ast_गाँठो* अचर passsp = नूतन AstPExprClause{flp, सत्य};
            passsp->addNext(setDone());
            windowp->addStmtsp(नूतन AstIf{flp, rhsp, passsp});
            // @(clk) -- advance to next cycle in window
            windowp->addStmtsp(
                नूतन AstEventControl{flp, नूतन Ast_Sen_रूख{flp, sensesp->clone_रूख(असत्य)}, nullptr});
            beginp->addStmtsp(windowp);
        } अन्यथा यदि (isNonConsec) {
            // Standalone nonconsec: ##1 into window, then pass
            beginp->addStmtsp(
                नूतन AstEventControl{flp, नूतन Ast_Sen_रूख{flp, sensesp->clone_रूख(असत्य)}, nullptr});
            beginp->addStmtsp(नूतन AstPExprClause{flp, सत्य});
        } अन्यथा यदि (rhsp) {
            // Goto rep: check consequent once at match endpoint
            यदि (!isOverlapped) {
                beginp->addStmtsp(नूतन AstEventControl{
                    flp, नूतन Ast_Sen_रूख{flp, sensesp->clone_रूख(असत्य)}, nullptr});
            }
            beginp->addStmtsp(नूतन AstIf{flp, rhsp, नूतन AstPExprClause{flp, सत्य},
                                        नूतन AstPExprClause{flp, असत्य}});
        } अन्यथा {
            // Standalone goto: pass after counting
            beginp->addStmtsp(नूतन AstPExprClause{flp, सत्य});
        }

        फर्काऊ नूतन AstPExpr{flp, beginp, exprp->findBitDType()};
    }

    // Reject [=M:N] nonconsecutive range that reached V3AssertPre.
    // [->M:N] is fully handled upstream (NFA path); only [=M:N] still lands
    // here because AstSNonConsRep lowering is not yet implemented there.
    // Replaces the node with BitFalse so lowering continues after the warning.
    तार्किक rejectNonconsecRange(Ast_गाँठो* nodep, Ast_गाँठो_अभिव्यक्ति* maxCountp) {
        यदि (!maxCountp) फर्काऊ असत्य;
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: [=M:N] nonconsecutive range repetition"
                                     " (IEEE 1800-2023 16.9.2)");
        nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}});
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
        फर्काऊ सत्य;
    }

    शून्य visit(AstSGotoRep* nodep) override {
        // Standalone goto rep (not inside implication antecedent)
        iterateChildren(nodep);
        File_पङ्क्ति* अचर flp = nodep->fileline();
        Ast_गाँठो_अभिव्यक्ति* countp = nodep->countp()->unlinkFrBack();
        यदि (!validate_Rep_गन्ती(nodep, countp)) फर्काऊ;
        Ast_गाँठो_अभिव्यक्ति* अचर exprp = nodep->exprp()->unlinkFrBack();
        AstPExpr* अचर pexprp = createRepPExpr(flp, exprp, countp, nullptr, सत्य, असत्य);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }

    शून्य visit(AstSNonConsRep* nodep) override {
        // Standalone nonconsecutive rep (not inside implication antecedent)
        iterateChildren(nodep);
        यदि (rejectNonconsecRange(nodep, nodep->maxCountp())) फर्काऊ;
        File_पङ्क्ति* अचर flp = nodep->fileline();
        Ast_गाँठो_अभिव्यक्ति* countp = nodep->countp()->unlinkFrBack();
        यदि (!validate_Rep_गन्ती(nodep, countp)) फर्काऊ;
        Ast_गाँठो_अभिव्यक्ति* अचर exprp = nodep->exprp()->unlinkFrBack();
        AstPExpr* अचर pexprp = createRepPExpr(flp, exprp, countp, nullptr, सत्य, सत्य);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }

    शून्य visit(AstFuncRef* nodep) override {
        // IEEE 1800-2023 16.8: Inline sequence instances wherever they appear
        // in the expression tree (inside implications, boolean ops, nested refs, etc.)
        यदि (AstSequence* अचर seqp = VN_CAST(nodep->taskp(), Sequence)) {
            substituteSequenceCall(nodep, seqp);
            // The FuncRef has been replaced; do not access nodep after this point.
            // The replacement node will be visited by the parent's iterateChildren.
            फर्काऊ;
        }
        यदि (अचर AstProperty* अचर propp = VN_CAST(nodep->taskp(), Property)) {
            substituteNestedPropertyCall(nodep, propp);
            फर्काऊ;
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstImplication* nodep) override {
        यदि (nodep->sentreep()) फर्काऊ;  // Already processed

        // Top-level followed-by is claimed by V3AssertNfa; anything reaching here
        // is nested inside iff/implies/or. IEEE 1800-2023 16.12.9 permits the
        // composition, but silent lowering would drop non-vacuous-fail semantics.
        यदि (nodep->isFollowedBy()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: followed-by (#-# / #=#) nested inside property operator"
                          " (iff/implies/or) (IEEE 1800-2023 16.12.9)");
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(थप_Deletep(nodep), nodep);
            फर्काऊ;
        }

        // Handle goto repetition as antecedent before iterateChildren,
        // so the standalone AstSGotoRep visitor doesn't process it
        यदि (AstSGotoRep* अचर gotop = VN_CAST(nodep->lhsp(), SGotoRep)) {
            iterateChildren(gotop);
            iterateAndNextNull(nodep->rhsp());
            File_पङ्क्ति* अचर flp = nodep->fileline();
            Ast_गाँठो_अभिव्यक्ति* countp = gotop->countp()->unlinkFrBack();
            यदि (!validate_Rep_गन्ती(nodep, countp)) फर्काऊ;
            Ast_गाँठो_अभिव्यक्ति* अचर exprp = gotop->exprp()->unlinkFrBack();
            Ast_गाँठो_अभिव्यक्ति* अचर rhsp = nodep->rhsp()->unlinkFrBack();
            AstPExpr* अचर pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), असत्य);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(थप_Deletep(nodep), nodep);
            फर्काऊ;
        }

        // Handle nonconsecutive repetition as antecedent before iterateChildren,
        // so the standalone AstSNonConsRep visitor doesn't process it
        यदि (AstSNonConsRep* अचर ncrp = VN_CAST(nodep->lhsp(), SNonConsRep)) {
            iterateChildren(ncrp);
            iterateAndNextNull(nodep->rhsp());
            यदि (rejectNonconsecRange(nodep, ncrp->maxCountp())) फर्काऊ;
            File_पङ्क्ति* अचर flp = nodep->fileline();
            Ast_गाँठो_अभिव्यक्ति* countp = ncrp->countp()->unlinkFrBack();
            यदि (!validate_Rep_गन्ती(nodep, countp)) फर्काऊ;
            Ast_गाँठो_अभिव्यक्ति* अचर exprp = ncrp->exprp()->unlinkFrBack();
            Ast_गाँठो_अभिव्यक्ति* अचर rhsp = nodep->rhsp()->unlinkFrBack();
            AstPExpr* अचर pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), सत्य);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(थप_Deletep(nodep), nodep);
            फर्काऊ;
        }

        iterateChildren(nodep);

        File_पङ्क्ति* अचर flp = nodep->fileline();
        Ast_गाँठो_अभिव्यक्ति* अचर rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_गाँठो_अभिव्यक्ति* lhsp = nodep->lhsp()->unlinkFrBack();

        // Lower sequence match items (IEEE 16.11): (expr, var = val, ...) |-> / |=>
        यदि (Ast_अभिव्यक्ति_Stmt* अचर अभिव्यक्ति_Stmtp = VN_CAST(lhsp, अभिव्यक्ति_Stmt)) {
            Ast_गाँठो_अभिव्यक्ति* अचर antExprp = अभिव्यक्ति_Stmtp->resultp()->unlinkFrBack();

            यदि (nodep->isOverlapped()) {
                // |-> : assign to __Vpropvar via always_comb (continuous).
                // The assign evaluates RHS once; V3Sampled snapshots the
                // result so all consequent refs read the same value.
                Ast_गाँठो* matchAssignsp = nullptr;
                लागि (Ast_गाँठो* stmtp = अभिव्यक्ति_Stmtp->stmtsp(); stmtp;) {
                    Ast_गाँठो* अचर nextp = stmtp->nextp();
                    यदि (AstAssign* अचर assignp = VN_CAST(stmtp, Assign)) {
                        assignp->unlinkFrBack();
                        matchAssignsp = Ast_गाँठो::addNextNull(matchAssignsp, assignp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(थप_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                यदि (matchAssignsp) {
                    AstAlways* अचर alwaysp
                        = नूतन AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, matchAssignsp};
                    m_modp->addStmtsp(alwaysp);
                }
            } अन्यथा {
                // |=> : assign to __Vpropvar via NBA in a clocked always block.
                // The NBA commits before the next cycle's sampled snapshot,
                // so the consequent (which already references __Vpropvar)
                // sees the captured value.
                Ast_गाँठो* matchAssignsp = nullptr;
                लागि (Ast_गाँठो* stmtp = अभिव्यक्ति_Stmtp->stmtsp(); stmtp;) {
                    Ast_गाँठो* अचर nextp = stmtp->nextp();
                    यदि (AstAssign* अचर assignp = VN_CAST(stmtp, Assign)) {
                        assignp->unlinkFrBack();
                        Ast_गाँठो_अभिव्यक्ति* अचर assignLhsp = assignp->lhsp()->unlinkFrBack();
                        Ast_गाँठो_अभिव्यक्ति* अचर assignRhsp = assignp->rhsp()->unlinkFrBack();
                        AstAssignDly* अचर dlyp = नूतन AstAssignDly{flp, assignLhsp, assignRhsp};
                        VL_DO_DANGLING(थप_Deletep(assignp), assignp);
                        matchAssignsp = Ast_गाँठो::addNextNull(matchAssignsp, dlyp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(थप_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                यदि (matchAssignsp) {
                    AstIf* अचर condp
                        = नूतन AstIf{flp, antExprp->clone_रूख_Pure(असत्य), matchAssignsp};
                    AstAlways* अचर alwaysp
                        = नूतन AstAlways{flp, VAlwaysKwd::ALWAYS, new_Sen_रूख(nodep), condp};
                    m_modp->addStmtsp(alwaysp);
                }
            }
        }

        यदि (AstPExpr* अचर pexprp = VN_CAST(rhsp, PExpr)) {
            // Implication with sequence expression on RHS (IEEE 1800-2023 16.11, 16.12.7).
            // The PExpr was lowered from the property expression earlier in this pass.
            // Wrap the PExpr body with the antecedent check so the sequence only
            // starts when the antecedent holds.
            Ast_गाँठो_अभिव्यक्ति* condp;
            यदि (nodep->isOverlapped()) {
                // Overlapped implication (|->): check antecedent on same cycle.
                // disable iff is applied at the assertion level, not at the
                // antecedent gate, matching the existing non-PExpr overlapped path.
                condp = lhsp;
            } अन्यथा {
                // Non-overlapped implication (|=>): check antecedent from previous cycle
                यदि (m_disablep) {
                    lhsp
                        = नूतन AstAnd{flp, नूतन AstNot{flp, m_disablep->clone_रूख_Pure(असत्य)}, lhsp};
                }
                AstPast* अचर pastp
                    = नूतन AstPast{flp, lhsp, nullptr, nullptr, /* propertyTiming */ सत्य};
                pastp->dtypeFrom(lhsp);
                pastp->sentreep(new_Sen_रूख(nodep));
                condp = pastp;
            }
            // Wrap existing PExpr body: if (antecedent) { <original body> } else { /* vacuous pass
            // */ }
            // Only the statements are guarded; declarations stay directly in the block. After
            // V3Fork moves the block's statements into a task, V3Task only handles variables
            // that are direct statements of the task.
            AstBegin* अचर bodyp = pexprp->bodyp();
            AstIf* अचर guardp = नूतन AstIf{flp, condp};
            लागि (Ast_गाँठो* stmtp = bodyp->stmtsp(); stmtp;) {
                Ast_गाँठो* अचर nextp = stmtp->nextp();
                यदि (!VN_IS(stmtp, Var)) guardp->addThensp(stmtp->unlinkFrBack());
                stmtp = nextp;
            }
            bodyp->addStmtsp(guardp);
            nodep->replaceWith(pexprp);
            // Don't iterate pexprp here -- it was already iterated when created
            // (in visit(AstSExpr*)), so delays and disable iff are already processed.
        } अन्यथा यदि (nodep->isOverlapped()) {
            Ast_गाँठो_अभिव्यक्ति* अचर exprp
                = m_underCover ? static_cast<Ast_गाँठो_अभिव्यक्ति*>(नूतन AstLogAnd{flp, lhsp, rhsp})
                               : नूतन AstLogOr{flp, नूतन AstLogNot{flp, lhsp}, rhsp};
            nodep->replaceWith(exprp);
        } अन्यथा {
            यदि (m_disablep) {
                lhsp = नूतन AstAnd{flp, नूतन AstNot{flp, m_disablep->clone_रूख_Pure(असत्य)}, lhsp};
            }

            AstPast* अचर pastp
                = नूतन AstPast{flp, lhsp, nullptr, nullptr, /* propertyTiming */ सत्य};
            pastp->dtypeFrom(lhsp);
            pastp->sentreep(new_Sen_रूख(nodep));
            Ast_गाँठो_अभिव्यक्ति* अचर exprp
                = m_underCover ? static_cast<Ast_गाँठो_अभिव्यक्ति*>(नूतन AstAnd{flp, pastp, rhsp})
                               : नूतन AstOr{flp, नूतन AstNot{flp, pastp}, rhsp};
            exprp->dtypeSetBit();
            nodep->replaceWith(exprp);
        }
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }
    शून्य visit(AstUntil* nodep) override {
        File_पङ्क्ति* अचर flp = nodep->fileline();
        UASSERT_OBJ(
            !m_pexprp, nodep,
            "'" << nodep->verilogKwd()
                << "' in complex property expression should have been rejected by V3AssertNfa");
        यदि (nodep->isStrong()
            && (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue())) {
            nodep->v3warn(E_NOTIMING, nodep->verilogKwd() << " requires --timing");
            nodep->replaceWith(नूतन AstConst{flp, AstConst::BitFalse{}});
            VL_DO_DANGLING(थप_Deletep(nodep), nodep);
            फर्काऊ;
        }
        यदि (nodep->isStrong()) {
            // p s_until q / p s_until_with q: q must eventually be true. Until then, p must
            // be true on every sampled tick. For s_until, check q first: when q is true on
            // this tick, p is not required. For s_until_with, p must be true on the q tick too.
            Ast_गाँठो_अभिव्यक्ति* अचर rawLhsp = nodep->lhsp()->unlinkFrBack();
            Ast_गाँठो_अभिव्यक्ति* अचर rawRhsp = nodep->rhsp()->unlinkFrBack();
            AstSampled* अचर lhsp = नूतन AstSampled{flp, rawLhsp, rawLhsp->dtypep(), सत्य};
            AstSampled* अचर rhsp = नूतन AstSampled{flp, rawRhsp, rawRhsp->dtypep(), सत्य};
            Ast_गाँठो_अभिव्यक्ति* finalCondp = rhsp->clone_रूख_Pure(असत्य);
            यदि (nodep->isOverlapping()) {
                finalCondp = नूतन AstLogAnd{flp, lhsp->clone_रूख_Pure(असत्य), finalCondp};
            }

            // Track active assertion attempts. Only the count is needed to emit final failures.
            AstVar* अचर activep = नूतन AstVar{flp, VVarType::MODULETEMP, m_activeNames.get(""),
                                               nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            activep->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(activep);

            AstVar* अचर donep
                = नूतन AstVar{flp, VVarType::BLOCKTEMP, "__VassertDone", nodep->findBitDType()};
            donep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            स्वतः setDone = [&]() {
                फर्काऊ नूतन AstAssign{flp, नूतन AstVarRef{flp, donep, VAccess::WRITE},
                                     नूतन AstConst{flp, AstConst::BitTrue{}}};
            };

            AstLoop* अचर loopp = नूतन AstLoop{flp};
            AstAssign* अचर decrementVar = नूतन AstAssign{
                flp, नूतन AstVarRef{flp, activep, VAccess::WRITE},
                नूतन AstSub{flp, नूतन AstVarRef{flp, activep, VAccess::READ}, नूतन AstConst{flp, 1}}};
            loopp->addStmtsp(नूतन AstLoopTest{
                flp, loopp, नूतन AstLogNot{flp, नूतन AstVarRef{flp, donep, VAccess::READ}}});
            {
                Ast_गाँठो* अचर passp = नूतन AstPExprClause{flp};
                passp->addNext(decrementVar);
                passp->addNext(setDone());
                Ast_गाँठो_अभिव्यक्ति* passCondp = rhsp;
                यदि (nodep->isOverlapping()) {
                    passCondp = नूतन AstLogAnd{flp, lhsp->clone_रूख_Pure(असत्य), passCondp};
                }
                loopp->addStmtsp(नूतन AstIf{flp, passCondp, passp});
            }
            {
                Ast_गाँठो* अचर failp = नूतन AstPExprClause{flp, असत्य};
                failp->addNext(decrementVar->clone_रूख(असत्य));
                failp->addNext(setDone());
                loopp->addStmtsp(नूतन AstIf{
                    flp,
                    नूतन AstLogAnd{flp,
                                  नूतन AstLogNot{flp, नूतन AstVarRef{flp, donep, VAccess::READ}},
                                  नूतन AstLogNot{flp, lhsp}},
                    failp});
            }
            AstDelay* अचर delayp = नूतन AstDelay{flp, नूतन AstConst{flp, 1}, असत्य};
            delayp->timeunit(m_modp->timeunit());
            loopp->addStmtsp(नूतन AstIf{
                flp, नूतन AstLogNot{flp, नूतन AstVarRef{flp, donep, VAccess::READ}}, delayp});

            // Main assertion block
            AstBegin* अचर bodyp = नूतन AstBegin{flp, m_blockNames.get(""), nullptr, सत्य};
            bodyp->addStmtsp(donep);
            bodyp->addStmtsp(नूतन AstAssign{flp, नूतन AstVarRef{flp, donep, VAccess::WRITE},
                                           नूतन AstConst{flp, AstConst::BitFalse{}}});
            File_पङ्क्ति* अचर flp = activep->fileline();
            bodyp->addStmtsp(
                नूतन AstAssign{flp, नूतन AstVarRef{flp, activep, VAccess::WRITE},
                              नूतन AstAdd{flp, नूतन AstVarRef{flp, activep, VAccess::READ},
                                         नूतन AstConst{flp, 1}}});
            bodyp->addStmtsp(loopp);

            // Validate assertion condition for each active assert
            AstVar* अचर activeCountp = नूतन AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                    nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            activeCountp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

            AstAssign* अचर initActiveCountp
                = नूतन AstAssign{flp, नूतन AstVarRef{flp, activeCountp, VAccess::WRITE},
                                नूतन AstVarRef{flp, activep, VAccess::READ}};
            AstLoop* अचर finalLoopp = नूतन AstLoop{flp};
            finalLoopp->addStmtsp(
                नूतन AstLoopTest{flp, finalLoopp,
                                नूतन AstNeq{flp, नूतन AstVarRef{flp, activeCountp, VAccess::READ},
                                           नूतन AstConst{flp, 0}}});
            finalLoopp->addStmtsp(नूतन AstIf{flp, finalCondp, नूतन AstPExprClause{flp},
                                            नूतन AstPExprClause{flp, असत्य}});
            finalLoopp->addStmtsp(
                नूतन AstAssign{flp, नूतन AstVarRef{flp, activeCountp, VAccess::WRITE},
                              नूतन AstSub{flp, नूतन AstVarRef{flp, activeCountp, VAccess::READ},
                                         नूतन AstConst{flp, 1}}});

            // Final assertion block
            AstBegin* अचर finalp = नूतन AstBegin{flp, m_blockNames.get(""), nullptr, सत्य};
            finalp->addStmtsp(activeCountp);
            finalp->addStmtsp(initActiveCountp);
            finalp->addStmtsp(finalLoopp);

            AstPExpr* अचर pexprp = नूतन AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
            VL_RESTORER(m_pexprp);
            VL_RESTORER(m_hasCycleDelay);
            m_pexprp = pexprp;
            m_hasCycleDelay = असत्य;
            iterate(bodyp);
            iterate(finalp);

            UASSERT_OBJ(!m_hasCycleDelay, nodep,
                        "s_until cycle delay should have been handled by V3AssertNfa");

            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(थप_Deletep(nodep), nodep);
            फर्काऊ;
        }
        AstLoop* अचर loopp = नूतन AstLoop{flp};
        Ast_गाँठो_अभिव्यक्ति* अचर rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_गाँठो_अभिव्यक्ति* अचर lhsp = nodep->lhsp()->unlinkFrBack();
        AstLogAnd* अचर loopCondp = नूतन AstLogAnd{flp, lhsp, नूतन AstLogNot{flp, rhsp}};
        loopp->addStmtsp(नूतन AstLoopTest{flp, loopp, loopCondp});
        loopp->addStmtsp(नूतन AstEventControl{flp, new_Sen_रूख(nodep), nullptr});

        Ast_गाँठो_अभिव्यक्ति* अचर rhsCopyp = rhsp->clone_रूख_Pure(असत्य);
        Ast_गाँठो_अभिव्यक्ति* अचर passCondp
            = nodep->isOverlapping() ? नूतन AstLogAnd{flp, lhsp->clone_रूख_Pure(असत्य), rhsCopyp}
                                     : rhsCopyp;
        AstBegin* अचर beginp = नूतन AstBegin{flp, m_blockNames.get(""), loopp, सत्य};
        beginp->addStmtsp(
            नूतन AstIf{flp, passCondp, नूतन AstPExprClause{flp}, नूतन AstPExprClause{flp, असत्य}});

        AstPExpr* अचर pexprp = नूतन AstPExpr{flp, beginp, nodep->dtypep()};
        pexprp->user1(1);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }

    शून्य visit(AstDefaultDisable* nodep) override {
        VL_DO_DANGLING(थप_Deletep(nodep->unlinkFrBack()), nodep);
    }
    शून्य visit(AstInferredDisable* nodep) override {
        Ast_गाँठो* newp;
        यदि (m_defaultDisablep) {
            newp = m_defaultDisablep->condp()->clone_रूख_Pure(सत्य);
        } अन्यथा {
            newp = नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }

    शून्य visit(AstPropSpec* nodep) override {
        nodep = substitutePropertyCall(nodep);
        iterateAndNextNull(nodep->sensesp());
        यदि (m_senip && m_senip != nodep->sensesp())
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Only one PSL clock allowed per assertion");
        अचर AstCover* अचर coverp = VN_CAST(nodep->backp(), Cover);
        अचर तार्किक seqEvent = coverp && coverp->isSeqEvent();
        यदि (!nodep->disablep() && m_defaultDisablep && !seqEvent) {
            nodep->disablep(m_defaultDisablep->condp()->clone_रूख_Pure(सत्य));
        }
        m_disablep = nodep->disablep();
        // Unlink and just keep a pointer to it, convert to sentree as needed
        m_senip = nodep->sensesp();
        iterateNull(nodep->disablep());
        यदि (!VN_AS(nodep->backp(), गाँठो_Cover_Or_Assert)->immediate()) {
            अचर Ast_गाँठो_DType* अचर propDtp = nodep->propp()->dtypep();
            nodep->propp(नूतन AstSampled{nodep->fileline(), nodep->propp()->unlinkFrBack(),
                                        propDtp->dtypep(), सत्य});
        }
        // cover counts non-vacuous matches only (IEEE 1800-2023 16.15.2), so an
        // implication antecedent must hold; assert passes vacuously instead.
        VL_RESTORER(m_underCover);
        m_underCover = VN_IS(nodep->backp(), Cover);
        iterate(nodep->propp());
        iterateNull(nodep->matchCountp());
    }
    शून्य visit(AstPExpr* nodep) override {
        // V3AssertNfa handles multi-cycle property expressions before this pass,
        // so the following unsupported paths are defensive and typically unreached.
        यदि (m_pexprp && m_pexprp->user1()) {  // LCOV_EXCL_START
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Complex property expression inside 'until''");
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(थप_Deletep(nodep), nodep);
            फर्काऊ;
        }  // LCOV_EXCL_STOP
        यदि (AstLogNot* अचर notp = VN_CAST(nodep->backp(), LogNot)) {
            notp->replaceWith(nodep->unlinkFrBack());
            VL_DO_DANGLING(थप_Deletep(notp), notp);
            iterate(nodep);
            फर्काऊ;
        }
        // Sequence expression as antecedent of implication is not yet supported
        यदि (AstImplication* अचर implp = VN_CAST(nodep->backp(), Implication)) {
            यदि (implp->lhsp() == nodep) {  // LCOV_EXCL_START
                implp->v3warn(E_UNSUPPORTED,
                              "Unsupported: Implication with sequence expression as antecedent");
                nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(थप_Deletep(nodep), nodep);
                फर्काऊ;
            }  // LCOV_EXCL_STOP
        }
        VL_RESTORER(m_pexprp);
        VL_RESTORER(m_disableSeqIfp);
        m_pexprp = nodep;

        यदि (m_disablep) {
            अचर AstSampled* sampledp = nullptr;
            यदि (m_disablep->exists([&sampledp](अचर AstSampled* अचर sp) {
                    sampledp = sp;
                    फर्काऊ सत्य;
                })) {
                sampledp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: $sampled inside disabled condition of a sequence");
                m_disablep = नूतन AstConst{m_disablep->fileline(), AstConst::BitFalse{}};
                // always a copy is used, so remove it now
                थप_Deletep(m_disablep);  // m_disablep used later
            }
            File_पङ्क्ति* अचर flp = nodep->fileline();
            // Add counter which counts times the condition turned true
            AstVar* अचर disableCntp
                = नूतन AstVar{flp, VVarType::MODULETEMP, m_disableCntNames.get(""),
                             nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            disableCntp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(disableCntp);
            AstVarRef* अचर readCntRefp = नूतन AstVarRef{flp, disableCntp, VAccess::READ};
            AstVarRef* अचर writeCntRefp = नूतन AstVarRef{flp, disableCntp, VAccess::WRITE};
            AstAssign* अचर incrStmtp = नूतन AstAssign{
                flp, writeCntRefp, नूतन AstAdd{flp, readCntRefp, नूतन AstConst{flp, 1}}};
            AstAlways* अचर alwaysp
                = नूतन AstAlways{flp, VAlwaysKwd::ALWAYS,
                                नूतन Ast_Sen_रूख{flp, नूतन AstSenItem{flp, VEdgeType::ET_POSEDGE,
                                                                   m_disablep->clone_रूख(असत्य)}},
                                incrStmtp};
            disableCntp->addNextHere(alwaysp);

            // Store value of that counter at the beginning of sequence evaluation
            AstBegin* अचर bodyp = nodep->bodyp();
            AstVar* अचर initialCntp = नूतन AstVar{flp, VVarType::BLOCKTEMP, "__VinitialCnt",
                                                   nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            initialCntp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            AstAssign* अचर assignp
                = नूतन AstAssign{flp, नूतन AstVarRef{flp, initialCntp, VAccess::WRITE},
                                readCntRefp->clone_रूख(असत्य)};
            // Prepend to the sequence body to keep statement list structure valid.
            UASSERT_OBJ(!bodyp->name().empty(), bodyp, "Sequence body should be named");
            Ast_गाँठो* अचर newStmtsp = Ast_गाँठो::addNext<Ast_गाँठो, Ast_गाँठो>(initialCntp, assignp);
            bodyp->prependStmtsp(newStmtsp);

            m_disableSeqIfp
                = नूतन AstIf{flp, नूतन AstEq{flp, नूतन AstVarRef{flp, initialCntp, VAccess::READ},
                                           readCntRefp->clone_रूख(असत्य)}};
            // Delete it, because it is always copied before insetion to the AST
            थप_Deletep(m_disableSeqIfp);  // m_disableSeqIfp used later
        }
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाँठो_Module* nodep) override {
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultClkEvtVarp);
        VL_RESTORER(m_defaultDisablep);
        VL_RESTORER(m_modp);
        यदि (!VN_IS(nodep, Class)) {
            // A class nested in a module is in the scope of the module's default clocking
            m_defaultClockingp = nullptr;
            m_defaultClkEvtVarp = nullptr;
        }
        m_defaultDisablep = nodep->defaultDisablep();
        m_modp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    शून्य visit(AstProperty* nodep) override {
        // The body will be visited when will be substituted in place of property reference
        // (AstFuncRef)
        VL_DO_DANGLING(थप_Deletep(nodep->unlinkFrBack()), nodep);
    }
    शून्य visit(AstSequence* nodep) override {
        // Sequence declarations are not visited directly; their bodies are inlined
        // at call sites by visit(AstFuncRef*). Collect for post-traversal cleanup.
        m_seqsToCleanup.थप_back(nodep);
    }
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit AssertPreVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        { AssertPreClockingVisitor{nodep}; }
        // Process
        iterate(nodep);
        // Fix up varref names
        लागि (AstVarXRef* xrefp : m_xrefsp) xrefp->name(xrefp->varp()->name());
        // Clean up sequence declarations after inlining.
        // Referenced sequences that were inlined have isReferenced cleared.
        // Remaining referenced sequences are in unsupported contexts (e.g. @seq event).
        लागि (AstSequence* seqp : m_seqsToCleanup) {
            यदि (seqp->isReferenced()) {
                seqp->v3warn(E_UNSUPPORTED,
                             "Unsupported: sequence referenced outside assertion property");
            } अन्यथा {
                VL_DO_DANGLING(seqp->unlinkFrBack()->delete_रूख(), seqp);
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
    V3Global::dump_Check_Global_रूख("assertpre", 0, dump_रूख_Either_Level() >= 3);
}
