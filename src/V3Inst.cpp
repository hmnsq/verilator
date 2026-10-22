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
        जइ (refp->access().isWriteOrRW()) refp->varp()->isContinuously(सच्च);
    });
}

// Flip the access of the driven leaves of a pin connection expression so it
// can be used on the LHS of an assignment. Only variables and bit/part
// selects over variables are flipped; index and select operands stay
// read-only. Struct-member selects cannot appear here: V3Width lowers them to
// selects before V3Inst. Returns false if the connection cannot be driven
// (e.g. arithmetic), in which case the expression is left unmodified.
स्थिर तार्किक markConnectionLhs(Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nodep) {
    जइ (Ast_ग्रन्थिः_Var_Ref* ध्रुव refp = VN_CAST(nodep, ग्रन्थिः_Var_Ref)) {
        जइ (!refp->access().isReadOnly()) प्रत्यागम मिच्छ;
        refp->access(VAccess::WRITE);
        प्रत्यागम सच्च;
    }
    // Note AstSel (bit/part select) is not an AstNodeSel (array select);
    // both expose fromp() for the selected value.
    जइ (AstSel* ध्रुव selp = VN_CAST(nodep, Sel)) प्रत्यागम markConnectionLhs(selp->fromp());
    जइ (Ast_ग्रन्थिः_Sel* ध्रुव selp = VN_CAST(nodep, ग्रन्थिः_Sel)) {
        प्रत्यागम markConnectionLhs(selp->fromp());
    }
    जइ (AstConcat* ध्रुव concatp = VN_CAST(nodep, Concat)) {
        प्रत्यागम markConnectionLhs(concatp->lhsp()) && markConnectionLhs(concatp->rhsp());
    }
    प्रत्यागम मिच्छ;
}

// Prepare a pin connection expression as the LHS of the reverse (inside-out)
// assignment. Returns the flipped expression on success, nullptr if the
// connection cannot be driven. The input is left unmodified on failure.
स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* tryConnectionLhs(Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nodep) {
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव clonep = nodep->clone_वृक्षः(मिच्छ);
    जइ (markConnectionLhs(clonep)) प्रत्यागम clonep;
    VL_DO_DANGLING(clonep->delete_वृक्षः(), clonep);
    प्रत्यागम nullptr;
}

//######################################################################
// Inst state, as a visitor of each AstNode

वर्ग InstVisitor final : सार्वजनिक VNVisitor {
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
        जइ (modVarp->isIfaceRef()) प्रत्यागम मिच्छ;
        // Only a whole variable can be aliased, anything else needs the assignment
        AstVarRef* ध्रुव refp = VN_CAST(exprp, VarRef);
        जइ (!refp) प्रत्यागम मिच्छ;
        AstVar* ध्रुव व्यञ्जकम्_Varp = refp->varp();
        // A virtual interface port connected to a real interface can't be an alias
        जइ (व्यञ्जकम्_Varp->isIfaceRef()) प्रत्यागम मिच्छ;
        // A ref port must become an alias
        जइ (!modVarp->direction().isRef()) {
            // V3FsmDetect recognizes the registers of an fsm_register_wrapper instance via
            // the assignments of its pins, so leave those as assignments
            Ast_ग्रन्थिः_Module* ध्रुव cellModp = m_cellp->modp();
            जइ (V3Control::getFsmRegisterWrapper(cellModp->origName())
                || V3Control::getFsmRegisterWrapper(cellModp->prettyDehashOrigOrName())) {
                प्रत्यागम मिच्छ;
            }
            // A virtual interface method call is dispatched at run time, so the body of an
            // interface reached that way must use the signals of the instance it is called
            // on, not those of whichever instance this connection happens to be made to
            जइ (ध्रुव AstIface* ध्रुव ifacep = VN_CAST(m_cellp->modp(), Iface)) {
                जइ (ifacep->hasVirtualRef()) प्रत्यागम मिच्छ;
            }
            // Forced signals must keep their own storage, the two sides can be forced separately
            जइ (modVarp->isForced() || व्यञ्जकम्_Varp->isForced()) प्रत्यागम मिच्छ;
            // Same for public
            जइ (modVarp->isSigUserRWPublic() || व्यञ्जकम्_Varp->isSigUserRWPublic()) प्रत्यागम मिच्छ;
            // V3Tristate resolved the connected net already, and drives it from the
            // resolution it built for it, so a port merged into it would be driven by
            // the resolution of the instance as well
            जइ (व्यञ्जकम्_Varp->isTristate()) प्रत्यागम मिच्छ;
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
        प्रत्यागम सच्च;
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
        जइ (!nodep->user1()) {
            // Simplify it
            V3Inst::pinReconnectSimple(nodep, m_cellp, मिच्छ);
        }
        UINFOTREE(9, nodep, "", "Pin_oldb");
        जइ (!nodep->exprp()) प्रत्यागम;  // No-connect
        V3Inst::checkOutputShort(nodep);
        जइ (!nodep->exprp()) प्रत्यागम;  // Connection removed by checkOutputShort
        // Use user1p on the PIN to indicate we created an assign for this pin
        जइ (!nodep->user1SetOnce()) {
            // Make an ASSIGNW (expr, pin)
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = VN_AS(nodep->exprp(), ग्रन्थिः_व्यञ्जकम्)->clone_वृक्षः(मिच्छ);
            UASSERT_OBJ(exprp->width() == nodep->modVarp()->width(), nodep,
                        "Width mismatch, should have been handled in pinReconnectSimple");
            जइ (nodep->modVarp()->isInout()) {
                nodep->v3fatalSrc("Unsupported: Verilator is a 2-state simulator");
            } अण्णहा जइ (nodep->modVarp()->isWritable()) {
                जइ (!tryAliasPin(nodep, exprp)) {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = नूतन AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                             m_cellp->name(), VAccess::READ};
                    markContinuousLhs(exprp);
                    AstAssignW* ध्रुव assp = नूतन AstAssignW{exprp->fileline(), exprp, rhsp};
                    m_cellp->addNextHere(नूतन AstAlways{assp});
                }
            } अण्णहा जइ (nodep->modVarp()->isNonOutput()) {
                जइ (!tryAliasPin(nodep, exprp)) {
                    // Don't bother moving constants now,
                    // we'll be pushing the const down to the cell soon enough.
                    AstVarXRef* ध्रुव lhsp = नूतन AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                            m_cellp->name(), VAccess::WRITE};

                    markContinuousLhs(lhsp);
                    AstAssignW* ध्रुव assp = नूतन AstAssignW{exprp->fileline(), lhsp, exprp};
                    m_cellp->addNextHere(नूतन AstAlways{assp});
                    UINFOTREE(9, assp, "", "_new");
                }
            } अण्णहा जइ (nodep->modVarp()->isIfaceRef()) {
                // Create an AstAliasScope for Vars to Cells so we can
                // link with their scope later
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = नूतन AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                         m_cellp->name(), VAccess::READ};
                ध्रुव AstVarRef* ध्रुव refp = VN_CAST(exprp, VarRef);
                ध्रुव AstVarXRef* ध्रुव xrefp = VN_CAST(exprp, VarXRef);
                UASSERT_OBJ(refp || xrefp, exprp,
                            "Interfaces: Pin is not connected to a VarRef or VarXRef");
                m_cellp->addNextHere(नूतन AstAliasScope{exprp->fileline(), lhsp, exprp});
            } अण्णहा {
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

वर्ग InstStatic final {
    InstStatic() = स्वभाव;  // Static class

    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* extendOrSel(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp, ध्रुव Ast_ग्रन्थिः* cmpWidthp) {
        जइ (cmpWidthp->width() > rhsp->width()) {
            rhsp = (rhsp->isSigned() ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstExtendS{fl, rhsp})
                                     : static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstExtend{fl, rhsp}));
            // Need proper widthMin, which may differ from AstSel created above
            rhsp->dtypeFrom(cmpWidthp);
        } अण्णहा जइ (cmpWidthp->width() < rhsp->width()) {
            rhsp = नूतन AstSel{fl, rhsp, 0, cmpWidthp->width()};
            // Need proper widthMin, which may differ from AstSel created above
            rhsp->dtypeFrom(cmpWidthp);
        }
        // else don't change dtype, as might be e.g. array of something
        प्रत्यागम rhsp;
    }

सार्वजनिक:
    स्थिर AstAssignW* pinReconnectSimple(AstPin* pinp, AstCell* cellp, तार्किक forTristate,
                                          तार्किक alwaysCvt) {
        // If a pin connection is "simple" leave it as-is
        // Else create a intermediate wire to perform the interconnect
        // Return the new assignment, if one was made
        // Note this module calls cloneTree() via new AstVar
        AstVar* ध्रुव pinVarp = pinp->modVarp();
        जइ (!pinp->exprp()) {
            // No-connect, perhaps promote based on `unconnected_drive,
            // otherwise done
            जइ (pinVarp->direction() == VDirection::निवेशः
                && cellp->modp()->unconnectedDrive().isSetTrue()) {
                pinp->exprp(नूतन AstConst{pinp->fileline(), AstConst::All1{}});
            } अण्णहा जइ (pinVarp->direction() == VDirection::निवेशः
                       && cellp->modp()->unconnectedDrive().isSetFalse()) {
                pinp->exprp(नूतन AstConst{pinp->fileline(), AstConst::All0{}});
            } अण्णहा {
                प्रत्यागम nullptr;
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
        जइ (!alwaysCvt && connectRefp && connDTypep->same_वृक्षः(pinDTypep)
            && !connectRefp->varp()->isSc()) {  // Need the signal as a 'shell' to convert types
            // Done. Same data type
        } अण्णहा जइ (!alwaysCvt && connectRefp && connectRefp->varp()->isIfaceRef()) {
            // Done. Interface
        } अण्णहा जइ (!alwaysCvt && connectXRefp && connectXRefp->varp()
                   && connectXRefp->varp()->isIfaceRef()) {
        } अण्णहा जइ (!alwaysCvt && connBasicp && pinBasicp
                   && connBasicp->width() == pinBasicp->width()
                   && connBasicp->lo() == pinBasicp->lo()
                   && !connectRefp->varp()
                           ->isSc()  // Need the signal as a 'shell' to convert types
                   && connBasicp->width() == pinVarp->width()) {
            // Done. One to one interconnect won't need a temporary variable.
        } अण्णहा जइ (!alwaysCvt && !forTristate && VN_IS(pinp->exprp(), Const)) {
            // Done. Constant. Still check for driving an output, like below.
            V3Inst::checkOutputShort(pinp);
            जइ (!pinp->exprp()) प्रत्यागम nullptr;
        } अण्णहा {
            // Make a new temp wire
            // UINFOTREE(9, pinp, "", "in_pin");
            V3Inst::checkOutputShort(pinp);
            जइ (!pinp->exprp()) प्रत्यागम nullptr;
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
            जइ (pinVarp->isInout()) {
                pinVarp->v3fatalSrc("Unsupported: Inout connections to pins must be"
                                    " direct one-to-one connection (without any expression)");
                // V3Tristate should have cleared up before this point
            } अण्णहा जइ (pinVarp->isWritable()) {
                // See also V3Inst
                Ast_ग्रन्थिः_व्यञ्जकम्* rhsp = नूतन AstVarRef{pinp->fileline(), newvarp, VAccess::READ};
                UINFO(5, "pinRecon width " << pinVarp->width() << " >? " << rhsp->width() << " >? "
                                           << pinexprp->width());
                rhsp = extendOrSel(pinp->fileline(), rhsp, pinVarp);
                pinp->exprp(नूतन AstVarRef{newvarp->fileline(), newvarp, VAccess::WRITE});
                markContinuousLhs(pinexprp);
                जइ (VN_IS(pinexprp, ग्रन्थिः_Stream)) {
                    assignp = नूतन AstAssignW{pinp->fileline(), pinexprp, rhsp};
                    V3Width::streamAssignLowerEdit(assignp);
                } अण्णहा {
                    Ast_ग्रन्थिः_व्यञ्जकम्* rhsSelp = extendOrSel(pinp->fileline(), rhsp, pinexprp);
                    // A reversed array is assigned by the left-to-right order of elements
                    // (IEEE 1800-2023 7.6)
                    ध्रुव AstUnpackArrayDType* ध्रुव arrayp
                        = VN_CAST(pinexprp->dtypep()->skipRefp(), UnpackArrayDType);
                    जइ (arrayp && arrayp->isReverseOf(rhsSelp->dtypep())) {
                        rhsSelp = नूतन AstCvtUnpackToUnpack{pinp->fileline(), rhsSelp,
                                                           pinexprp->dtypep()};
                    }
                    assignp = नूतन AstAssignW{pinp->fileline(), pinexprp, rhsSelp};
                }
            } अण्णहा {
                // V3 width should have range/extended to make the widths correct
                newvarp->isContinuously(सच्च);
                // An input port driven from inside the cell by a clocking block
                // output flows out through the connection; V3Tristate split
                // pins resolve separately.
                Ast_ग्रन्थिः_व्यञ्जकम्* lhsp = nullptr;  // LHS of the interconnect assign
                Ast_ग्रन्थिः_व्यञ्जकम्* rhsp = nullptr;  // RHS of the interconnect assign
                जइ (forTristate || !pinVarp->isClockingDriven()) {
                    // Input-side wiring: temp fed from the connection
                    lhsp = नूतन AstVarRef{pinp->fileline(), newvarp, VAccess::WRITE};
                    rhsp = pinexprp;
                } अण्णहा जइ (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव drivenp = tryConnectionLhs(pinexprp)) {
                    // Reverse wiring: connection leaves fed from the temp.
                    // The original connection is consumed by the clone above.
                    VL_DO_DANGLING(pinexprp->delete_वृक्षः(), pinexprp);
                    markContinuousLhs(drivenp);
                    lhsp = drivenp;
                    rhsp = extendOrSel(pinp->fileline(),
                                       नूतन AstVarRef{pinp->fileline(), newvarp, VAccess::READ},
                                       drivenp);
                } अण्णहा {
                    pinp->v3error("Unsupported: Clocking block output driving input port "
                                  "through non-decomposable connection "
                                  << pinp->prettyNameQ());
                    VL_DO_DANGLING(pinexprp->delete_वृक्षः(), pinexprp);
                    प्रत्यागम nullptr;
                }
                pinp->exprp(नूतन AstVarRef{pinp->fileline(), newvarp, VAccess::READ});
                assignp = नूतन AstAssignW{pinp->fileline(), lhsp, rhsp};
            }
            जइ (assignp) cellp->addNextHere(नूतन AstAlways{assignp});
            // UINFOTREE(1, pinp, "", "out");
            // UINFOTREE(1, assignp, "", "aout");
        }
        प्रत्यागम assignp;
    }
};

//######################################################################
// Inst class functions

AstAssignW* V3Inst::pinReconnectSimple(AstPin* pinp, AstCell* cellp, तार्किक forTristate,
                                       तार्किक alwaysCvt) {
    प्रत्यागम InstStatic::pinReconnectSimple(pinp, cellp, forTristate, alwaysCvt);
}

शून्य V3Inst::checkOutputShort(ध्रुव AstPin* nodep) {
    जइ (nodep->modVarp()->direction() == VDirection::OUTPUT) {
        जइ (VN_IS(nodep->exprp(), Const) || VN_IS(nodep->exprp(), Extend)
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
