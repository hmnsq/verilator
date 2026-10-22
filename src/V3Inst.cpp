// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Add temporaries, such as for inst nodes
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
// V3Inst's Transformations:
//
// Each module:
//      Pins:
//          Create a wire assign to interconnect to submodule
//
//*************************************************************************

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Inst.h"

#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3Control.h"
#अन्तर्भाव "V3Width.h"

VL_DEFINE_DEBUG_FUNCTIONS;

स्थिर शून्य markContinuousLhs(Ast_ग्रन्थिः* ध्रुव nodep) {
    nodep->foreach([](Ast_ग्रन्थिः_Var_Ref* refp) {
        सचे (refp->access().isWriteOrRW()) refp->varp()->isContinuously(सच्च);
    });
}

// Flip the access of the driven leaves of a pin connection expression so it
// can be used on the LHS of an assignment. Only variables and bit/part
// selects over variables are flipped; index and select operands stay
// read-only. Struct-member selects cannot appear here: V3Width lowers them to
// selects before V3Inst. Returns false if the connection cannot be driven
// (e.g. arithmetic), in which case the expression is left unmodified.
स्थिर तार्किक markConnectionLhs(Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nodep) {
    सचे (Ast_ग्रन्थिः_Var_Ref* ध्रुव refp = VN_CAST(nodep, ग्रन्थिः_Var_Ref)) {
        सचे (!refp->access().isReadOnly()) पटिनिवत्त मिच्छा;
        refp->access(VAccess::WRITE);
        पटिनिवत्त सच्च;
    }
    // Note AstSel (bit/part select) is not an AstNodeSel (array select);
    // both expose fromp() for the selected value.
    सचे (AstSel* ध्रुव selp = VN_CAST(nodep, Sel)) पटिनिवत्त markConnectionLhs(selp->fromp());
    सचे (Ast_ग्रन्थिः_Sel* ध्रुव selp = VN_CAST(nodep, ग्रन्थिः_Sel)) {
        पटिनिवत्त markConnectionLhs(selp->fromp());
    }
    सचे (AstConcat* ध्रुव concatp = VN_CAST(nodep, Concat)) {
        पटिनिवत्त markConnectionLhs(concatp->lhsp()) && markConnectionLhs(concatp->rhsp());
    }
    पटिनिवत्त मिच्छा;
}

// Prepare a pin connection expression as the LHS of the reverse (inside-out)
// assignment. Returns the flipped expression on success, nullptr if the
// connection cannot be driven. The input is left unmodified on failure.
स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* tryConnectionLhs(Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nodep) {
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव clonep = nodep->clone_वृक्षः(मिच्छा);
    सचे (markConnectionLhs(clonep)) पटिनिवत्त clonep;
    VL_DO_DANGLING(clonep->delete_वृक्षः(), clonep);
    पटिनिवत्त nullptr;
}

//######################################################################
// Inst state, as a visitor of each AstNode

वग्ग InstVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // Cleared each Cell:
    //  AstPin::user1p()        -> bool.  True if created assignment already
    ध्रुव VNUser1InUse m_inuser1;

    // STATE
    AstCell* m_cellp = nullptr;  // Current cell

    // METHODS
    // If appropriate, add an AstAlias to connect the given Cell pin to the given expression.
    // Returns true if an alias was made, in which case there is nothing else to do for this pin.
    तार्किक tryAliasPin(AstPin* nodep, Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        AstVar* ध्रुव modVarp = nodep->modVarp();
        // An interface reference is aliased via AstAliasScope below, not as a variable
        सचे (modVarp->isIfaceRef()) पटिनिवत्त मिच्छा;
        // Only a whole variable can be aliased, anything else needs the assignment
        AstVarRef* ध्रुव refp = VN_CAST(exprp, VarRef);
        सचे (!refp) पटिनिवत्त मिच्छा;
        AstVar* ध्रुव व्यञ्जकम्_Varp = refp->varp();
        // A virtual interface port connected to a real interface can't be an alias
        सचे (व्यञ्जकम्_Varp->isIfaceRef()) पटिनिवत्त मिच्छा;
        // A ref port must become an alias
        सचे (!modVarp->direction().isRef()) {
            // V3FsmDetect recognizes the registers of an fsm_register_wrapper instance via
            // the assignments of its pins, so leave those as assignments
            Ast_ग्रन्थिः_Module* ध्रुव cellModp = m_cellp->modp();
            सचे (V3Control::getFsmRegisterWrapper(cellModp->origName())
                || V3Control::getFsmRegisterWrapper(cellModp->prettyDehashOrigOrName())) {
                पटिनिवत्त मिच्छा;
            }
            // A virtual interface method call is dispatched at run time, so the body of an
            // interface reached that way must use the signals of the instance it is called
            // on, not those of whichever instance this connection happens to be made to
            सचे (ध्रुव AstIface* ध्रुव ifacep = VN_CAST(m_cellp->modp(), Iface)) {
                सचे (ifacep->hasVirtualRef()) पटिनिवत्त मिच्छा;
            }
            // Forced signals must keep their own storage, the two sides can be forced separately
            सचे (modVarp->isForced() || व्यञ्जकम्_Varp->isForced()) पटिनिवत्त मिच्छा;
            // Same for public
            सचे (modVarp->isSigUserRWPublic() || व्यञ्जकम्_Varp->isSigUserRWPublic()) पटिनिवत्त मिच्छा;
            // V3Tristate resolved the connected net already, and drives it from the
            // resolution it built for it, so a port merged into it would be driven by
            // the resolution of the instance as well
            सचे (व्यञ्जकम्_Varp->isTristate()) पटिनिवत्त मिच्छा;
        }
        // They will become the same variable, so propagate file-line and attributes
        व्यञ्जकम्_Varp->fileline()->modifyStateInherit(modVarp->fileline());
        modVarp->fileline()->modifyStateInherit(व्यञ्जकम्_Varp->fileline());
        व्यञ्जकम्_Varp->propagateAttrFrom(modVarp);
        modVarp->propagateAttrFrom(व्यञ्जकम्_Varp);
        refp->access(VAccess::READWRITE);
        File_पङ्क्तिः* ध्रुव flp = exprp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव itemsp
            = नूतन AstVarXRef{flp, modVarp, m_cellp->name(), VAccess::READWRITE};
        itemsp->addNext(exprp);
        m_cellp->addNextHere(नूतन AstAlias{flp, itemsp});
        पटिनिवत्त सच्च;
    }

    // VISITORS
    शून्य visit(AstCell* nodep) override {
        UINFO(4, "  CELL   " << nodep);
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        // VV*****  We reset user1p() on each cell!!!
        Ast_ग्रन्थिः::user1_Clear_वृक्षः();
        iterateChildren(nodep);
    }

    शून्य visit(AstPin* nodep) override {
        // PIN(p,expr) -> ASSIGNW(VARXREF(p),expr)    (if sub's input)
        //            or  ASSIGNW(expr,VARXREF(p))    (if sub's output)
        UINFO(4, "   PIN  " << nodep);
        सचे (!nodep->user1()) {
            // Simplify it
            V3Inst::pinReconnectSimple(nodep, m_cellp, मिच्छा);
        }
        UINFOTREE(9, nodep, "", "Pin_oldb");
        सचे (!nodep->exprp()) पटिनिवत्त;  // No-connect
        V3Inst::checkOutputShort(nodep);
        सचे (!nodep->exprp()) पटिनिवत्त;  // Connection removed by checkOutputShort
        // Use user1p on the PIN to indicate we created an assign for this pin
        सचे (!nodep->user1SetOnce()) {
            // Make an ASSIGNW (expr, pin)
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = VN_AS(nodep->exprp(), ग्रन्थिः_व्यञ्जकम्)->clone_वृक्षः(मिच्छा);
            UASSERT_OBJ(exprp->width() == nodep->modVarp()->width(), nodep,
                        "Width mismatch, should have been handled in pinReconnectSimple");
            सचे (nodep->modVarp()->isInout()) {
                nodep->v3fatalSrc("Unsupported: Verilator is a 2-state simulator");
            } अञ्ञथा सचे (nodep->modVarp()->isWritable()) {
                सचे (!tryAliasPin(nodep, exprp)) {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = नूतन AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                             m_cellp->name(), VAccess::READ};
                    markContinuousLhs(exprp);
                    AstAssignW* ध्रुव assp = नूतन AstAssignW{exprp->fileline(), exprp, rhsp};
                    m_cellp->addNextHere(नूतन AstAlways{assp});
                }
            } अञ्ञथा सचे (nodep->modVarp()->isNonOutput()) {
                सचे (!tryAliasPin(nodep, exprp)) {
                    // Don't bother moving constants now,
                    // we'll be pushing the const down to the cell soon enough.
                    AstVarXRef* ध्रुव lhsp = नूतन AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                            m_cellp->name(), VAccess::WRITE};

                    markContinuousLhs(lhsp);
                    AstAssignW* ध्रुव assp = नूतन AstAssignW{exprp->fileline(), lhsp, exprp};
                    m_cellp->addNextHere(नूतन AstAlways{assp});
                    UINFOTREE(9, assp, "", "_new");
                }
            } अञ्ञथा सचे (nodep->modVarp()->isIfaceRef()) {
                // Create an AstAliasScope for Vars to Cells so we can
                // link with their scope later
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = नूतन AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                         m_cellp->name(), VAccess::READ};
                ध्रुव AstVarRef* ध्रुव refp = VN_CAST(exprp, VarRef);
                ध्रुव AstVarXRef* ध्रुव xrefp = VN_CAST(exprp, VarXRef);
                UASSERT_OBJ(refp || xrefp, exprp,
                            "Interfaces: Pin is not connected to a VarRef or VarXRef");
                m_cellp->addNextHere(नूतन AstAliasScope{exprp->fileline(), lhsp, exprp});
            } अञ्ञथा {
                nodep->v3error("Assigned pin is neither input nor output");
            }
        }

        // We're done with the pin
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्षः(), nodep);
    }

    // Save some time
    शून्य visit(Ast_ग्रन्थिः_व्यञ्जकम्*) override {}
    शून्य visit(Ast_ग्रन्थिः_Assign*) override {}
    शून्य visit(AstAlways*) override {}

    //--------------------
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit InstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~InstVisitor() override = स्वभाव;
};

//######################################################################
// Inst static function

वग्ग InstStatic final {
    InstStatic() = स्वभाव;  // Static class

    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* extendOrSel(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp, ध्रुव Ast_ग्रन्थिः* cmpWidthp) {
        सचे (cmpWidthp->width() > rhsp->width()) {
            rhsp = (rhsp->isSigned() ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstExtendS{fl, rhsp})
                                     : static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstExtend{fl, rhsp}));
            // Need proper widthMin, which may differ from AstSel created above
            rhsp->dtypeFrom(cmpWidthp);
        } अञ्ञथा सचे (cmpWidthp->width() < rhsp->width()) {
            rhsp = नूतन AstSel{fl, rhsp, 0, cmpWidthp->width()};
            // Need proper widthMin, which may differ from AstSel created above
            rhsp->dtypeFrom(cmpWidthp);
        }
        // else don't change dtype, as might be e.g. array of something
        पटिनिवत्त rhsp;
    }

सार्वजनिक:
    स्थिर AstAssignW* pinReconnectSimple(AstPin* pinp, AstCell* cellp, तार्किक forTristate,
                                          तार्किक alwaysCvt) {
        // If a pin connection is "simple" leave it as-is
        // Else create a intermediate wire to perform the interconnect
        // Return the new assignment, if one was made
        // Note this module calls cloneTree() via new AstVar
        AstVar* ध्रुव pinVarp = pinp->modVarp();
        सचे (!pinp->exprp()) {
            // No-connect, perhaps promote based on `unconnected_drive,
            // otherwise done
            सचे (pinVarp->direction() == VDirection::निवेशः
                && cellp->modp()->unconnectedDrive().isSetTrue()) {
                pinp->exprp(नूतन AstConst{pinp->fileline(), AstConst::All1{}});
            } अञ्ञथा सचे (pinVarp->direction() == VDirection::निवेशः
                       && cellp->modp()->unconnectedDrive().isSetFalse()) {
                pinp->exprp(नूतन AstConst{pinp->fileline(), AstConst::All0{}});
            } अञ्ञथा {
                पटिनिवत्त nullptr;
            }
        }
        ध्रुव AstVarRef* ध्रुव connectRefp = VN_CAST(pinp->exprp(), VarRef);
        ध्रुव AstVarXRef* ध्रुव connectXRefp = VN_CAST(pinp->exprp(), VarXRef);
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव pinDTypep = pinVarp->dtypep()->skipRefp();
        ध्रुव AstBasicDType* ध्रुव pinBasicp = VN_CAST(pinDTypep, BasicDType);
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव connDTypep
            = connectRefp ? connectRefp->varp()->dtypep()->skipRefp() : nullptr;
        ध्रुव AstBasicDType* ध्रुव connBasicp = VN_CAST(connDTypep, BasicDType);
        AstAssignW* assignp = nullptr;
        //
        सचे (!alwaysCvt && connectRefp && connDTypep->same_वृक्षः(pinDTypep)
            && !connectRefp->varp()->isSc()) {  // Need the signal as a 'shell' to convert types
            // Done. Same data type
        } अञ्ञथा सचे (!alwaysCvt && connectRefp && connectRefp->varp()->isIfaceRef()) {
            // Done. Interface
        } अञ्ञथा सचे (!alwaysCvt && connectXRefp && connectXRefp->varp()
                   && connectXRefp->varp()->isIfaceRef()) {
        } अञ्ञथा सचे (!alwaysCvt && connBasicp && pinBasicp
                   && connBasicp->width() == pinBasicp->width()
                   && connBasicp->lo() == pinBasicp->lo()
                   && !connectRefp->varp()
                           ->isSc()  // Need the signal as a 'shell' to convert types
                   && connBasicp->width() == pinVarp->width()) {
            // Done. One to one interconnect won't need a temporary variable.
        } अञ्ञथा सचे (!alwaysCvt && !forTristate && VN_IS(pinp->exprp(), Const)) {
            // Done. Constant. Still check for driving an output, like below.
            V3Inst::checkOutputShort(pinp);
            सचे (!pinp->exprp()) पटिनिवत्त nullptr;
        } अञ्ञथा {
            // Make a new temp wire
            // UINFOTREE(9, pinp, "", "in_pin");
            V3Inst::checkOutputShort(pinp);
            सचे (!pinp->exprp()) पटिनिवत्त nullptr;
            // Simplify, so stuff like '{a[0], b[0]}[1]' produced during
            // instance array expansion are brought to normal 'a[0]'
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव pinexprp
                = V3Const::constifyEdit(VN_AS(pinp->exprp(), ग्रन्थिः_व्यञ्जकम्)->unlinkFrBack());
            ध्रुव string newvarname
                = (string{pinVarp->isWritable() ? "__Vcellout" : "__Vcellinp"}
                   // Prevent name conflict if both tri & non-tri add signals
                   + (forTristate ? "t" : "") + "__" + cellp->name() + "__" + pinp->name());
            AstVar* ध्रुव newvarp
                = नूतन AstVar{pinVarp->fileline(), VVarType::MODULETEMP, newvarname, pinVarp};
            // Important to add statement next to cell, in case there is a
            // generate with same named cell
            cellp->addNextHere(newvarp);
            सचे (pinVarp->isInout()) {
                pinVarp->v3fatalSrc("Unsupported: Inout connections to pins must be"
                                    " direct one-to-one connection (without any expression)");
                // V3Tristate should have cleared up before this point
            } अञ्ञथा सचे (pinVarp->isWritable()) {
                // See also V3Inst
                Ast_ग्रन्थिः_व्यञ्जकम्* rhsp = नूतन AstVarRef{pinp->fileline(), newvarp, VAccess::READ};
                UINFO(5, "pinRecon width " << pinVarp->width() << " >? " << rhsp->width() << " >? "
                                           << pinexprp->width());
                rhsp = extendOrSel(pinp->fileline(), rhsp, pinVarp);
                pinp->exprp(नूतन AstVarRef{newvarp->fileline(), newvarp, VAccess::WRITE});
                markContinuousLhs(pinexprp);
                सचे (VN_IS(pinexprp, ग्रन्थिः_Stream)) {
                    assignp = नूतन AstAssignW{pinp->fileline(), pinexprp, rhsp};
                    V3Width::streamAssignLowerEdit(assignp);
                } अञ्ञथा {
                    Ast_ग्रन्थिः_व्यञ्जकम्* rhsSelp = extendOrSel(pinp->fileline(), rhsp, pinexprp);
                    // A reversed array is assigned by the left-to-right order of elements
                    // (IEEE 1800-2023 7.6)
                    ध्रुव AstUnpackArrayDType* ध्रुव arrayp
                        = VN_CAST(pinexprp->dtypep()->skipRefp(), UnpackArrayDType);
                    सचे (arrayp && arrayp->isReverseOf(rhsSelp->dtypep())) {
                        rhsSelp = नूतन AstCvtUnpackToUnpack{pinp->fileline(), rhsSelp,
                                                           pinexprp->dtypep()};
                    }
                    assignp = नूतन AstAssignW{pinp->fileline(), pinexprp, rhsSelp};
                }
            } अञ्ञथा {
                // V3 width should have range/extended to make the widths correct
                newvarp->isContinuously(सच्च);
                // An input port driven from inside the cell by a clocking block
                // output flows out through the connection; V3Tristate split
                // pins resolve separately.
                Ast_ग्रन्थिः_व्यञ्जकम्* lhsp = nullptr;  // LHS of the interconnect assign
                Ast_ग्रन्थिः_व्यञ्जकम्* rhsp = nullptr;  // RHS of the interconnect assign
                सचे (forTristate || !pinVarp->isClockingDriven()) {
                    // Input-side wiring: temp fed from the connection
                    lhsp = नूतन AstVarRef{pinp->fileline(), newvarp, VAccess::WRITE};
                    rhsp = pinexprp;
                } अञ्ञथा सचे (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव drivenp = tryConnectionLhs(pinexprp)) {
                    // Reverse wiring: connection leaves fed from the temp.
                    // The original connection is consumed by the clone above.
                    VL_DO_DANGLING(pinexprp->delete_वृक्षः(), pinexprp);
                    markContinuousLhs(drivenp);
                    lhsp = drivenp;
                    rhsp = extendOrSel(pinp->fileline(),
                                       नूतन AstVarRef{pinp->fileline(), newvarp, VAccess::READ},
                                       drivenp);
                } अञ्ञथा {
                    pinp->v3error("Unsupported: Clocking block output driving input port "
                                  "through non-decomposable connection "
                                  << pinp->prettyNameQ());
                    VL_DO_DANGLING(pinexprp->delete_वृक्षः(), pinexprp);
                    पटिनिवत्त nullptr;
                }
                pinp->exprp(नूतन AstVarRef{pinp->fileline(), newvarp, VAccess::READ});
                assignp = नूतन AstAssignW{pinp->fileline(), lhsp, rhsp};
            }
            सचे (assignp) cellp->addNextHere(नूतन AstAlways{assignp});
            // UINFOTREE(1, pinp, "", "out");
            // UINFOTREE(1, assignp, "", "aout");
        }
        पटिनिवत्त assignp;
    }
};

//######################################################################
// Inst class functions

AstAssignW* V3Inst::pinReconnectSimple(AstPin* pinp, AstCell* cellp, तार्किक forTristate,
                                       तार्किक alwaysCvt) {
    पटिनिवत्त InstStatic::pinReconnectSimple(pinp, cellp, forTristate, alwaysCvt);
}

शून्य V3Inst::checkOutputShort(ध्रुव AstPin* nodep) {
    सचे (nodep->modVarp()->direction() == VDirection::OUTPUT) {
        सचे (VN_IS(nodep->exprp(), Const) || VN_IS(nodep->exprp(), Extend)
            || (VN_IS(nodep->exprp(), Concat)
                && (VN_IS(VN_AS(nodep->exprp(), Concat)->lhsp(), Const)))) {
            // Uses v3warn for error, as might be found multiple times
            nodep->v3warn(E_PORTSHORT, "Output port is connected to a constant pin,"
                                       " electrical short");
            // Delete so we don't create a 'CONST = ...' assignment
            nodep->exprp()->unlinkFrBack()->delete_वृक्षः();
        }
    }
}

//######################################################################
// Inst class visitor

शून्य V3Inst::instAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { InstVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("inst", 0, dump_वृक्षः_Either_Level() >= 3);
}
