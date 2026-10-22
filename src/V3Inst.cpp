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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Inst.h"

#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3Control.h"
#ಅನ್ತರ್ಭಾವ "V3Width.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ಸ್ಥಿರ ಶೂನ್ಯ markContinuousLhs(Ast_ಗಂಟು* ಧ್ರುವ nodep) {
    nodep->foreach([](Ast_ಗಂಟು_Var_Ref* refp) {
        ಯದಿ (refp->access().isWriteOrRW()) refp->varp()->isContinuously(ಸತ್ಯ);
    });
}

// Flip the access of the driven leaves of a pin connection expression so it
// can be used on the LHS of an assignment. Only variables and bit/part
// selects over variables are flipped; index and select operands stay
// read-only. Struct-member selects cannot appear here: V3Width lowers them to
// selects before V3Inst. Returns false if the connection cannot be driven
// (e.g. arithmetic), in which case the expression is left unmodified.
ಸ್ಥಿರ ತಾರ್ಕಿಕ markConnectionLhs(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ nodep) {
    ಯದಿ (Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ refp = VN_CAST(nodep, ಗಂಟು_Var_Ref)) {
        ಯದಿ (!refp->access().isReadOnly()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        refp->access(VAccess::WRITE);
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    // Note AstSel (bit/part select) is not an AstNodeSel (array select);
    // both expose fromp() for the selected value.
    ಯದಿ (AstSel* ಧ್ರುವ selp = VN_CAST(nodep, Sel)) ಪ್ರತ್ಯಾಗಮ markConnectionLhs(selp->fromp());
    ಯದಿ (Ast_ಗಂಟು_Sel* ಧ್ರುವ selp = VN_CAST(nodep, ಗಂಟು_Sel)) {
        ಪ್ರತ್ಯಾಗಮ markConnectionLhs(selp->fromp());
    }
    ಯದಿ (AstConcat* ಧ್ರುವ concatp = VN_CAST(nodep, Concat)) {
        ಪ್ರತ್ಯಾಗಮ markConnectionLhs(concatp->lhsp()) && markConnectionLhs(concatp->rhsp());
    }
    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
}

// Prepare a pin connection expression as the LHS of the reverse (inside-out)
// assignment. Returns the flipped expression on success, nullptr if the
// connection cannot be driven. The input is left unmodified on failure.
ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* tryConnectionLhs(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ nodep) {
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ clonep = nodep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
    ಯದಿ (markConnectionLhs(clonep)) ಪ್ರತ್ಯಾಗಮ clonep;
    VL_DO_DANGLING(clonep->delete_ವೃಕ್ಷ(), clonep);
    ಪ್ರತ್ಯಾಗಮ nullptr;
}

//######################################################################
// Inst state, as a visitor of each AstNode

ವರ್ಗ InstVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // Cleared each Cell:
    //  AstPin::user1p()        -> bool.  True if created assignment already
    ಧ್ರುವ VNUser1InUse m_inuser1;

    // STATE
    AstCell* m_cellp = nullptr;  // Current cell

    // METHODS
    // If appropriate, add an AstAlias to connect the given Cell pin to the given expression.
    // Returns true if an alias was made, in which case there is nothing else to do for this pin.
    ತಾರ್ಕಿಕ tryAliasPin(AstPin* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        AstVar* ಧ್ರುವ modVarp = nodep->modVarp();
        // An interface reference is aliased via AstAliasScope below, not as a variable
        ಯದಿ (modVarp->isIfaceRef()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Only a whole variable can be aliased, anything else needs the assignment
        AstVarRef* ಧ್ರುವ refp = VN_CAST(exprp, VarRef);
        ಯದಿ (!refp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        AstVar* ಧ್ರುವ ಅಭಿವ್ಯಕ್ತಿ_Varp = refp->varp();
        // A virtual interface port connected to a real interface can't be an alias
        ಯದಿ (ಅಭಿವ್ಯಕ್ತಿ_Varp->isIfaceRef()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // A ref port must become an alias
        ಯದಿ (!modVarp->direction().isRef()) {
            // V3FsmDetect recognizes the registers of an fsm_register_wrapper instance via
            // the assignments of its pins, so leave those as assignments
            Ast_ಗಂಟು_Module* ಧ್ರುವ cellModp = m_cellp->modp();
            ಯದಿ (V3Control::getFsmRegisterWrapper(cellModp->origName())
                || V3Control::getFsmRegisterWrapper(cellModp->prettyDehashOrigOrName())) {
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
            // A virtual interface method call is dispatched at run time, so the body of an
            // interface reached that way must use the signals of the instance it is called
            // on, not those of whichever instance this connection happens to be made to
            ಯದಿ (ಧ್ರುವ AstIface* ಧ್ರುವ ifacep = VN_CAST(m_cellp->modp(), Iface)) {
                ಯದಿ (ifacep->hasVirtualRef()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
            // Forced signals must keep their own storage, the two sides can be forced separately
            ಯದಿ (modVarp->isForced() || ಅಭಿವ್ಯಕ್ತಿ_Varp->isForced()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            // Same for public
            ಯದಿ (modVarp->isSigUserRWPublic() || ಅಭಿವ್ಯಕ್ತಿ_Varp->isSigUserRWPublic()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            // V3Tristate resolved the connected net already, and drives it from the
            // resolution it built for it, so a port merged into it would be driven by
            // the resolution of the instance as well
            ಯದಿ (ಅಭಿವ್ಯಕ್ತಿ_Varp->isTristate()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        // They will become the same variable, so propagate file-line and attributes
        ಅಭಿವ್ಯಕ್ತಿ_Varp->fileline()->modifyStateInherit(modVarp->fileline());
        modVarp->fileline()->modifyStateInherit(ಅಭಿವ್ಯಕ್ತಿ_Varp->fileline());
        ಅಭಿವ್ಯಕ್ತಿ_Varp->propagateAttrFrom(modVarp);
        modVarp->propagateAttrFrom(ಅಭಿವ್ಯಕ್ತಿ_Varp);
        refp->access(VAccess::READWRITE);
        File_ಸಾಲು* ಧ್ರುವ flp = exprp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ itemsp
            = ನೂತನ AstVarXRef{flp, modVarp, m_cellp->name(), VAccess::READWRITE};
        itemsp->addNext(exprp);
        m_cellp->addNextHere(ನೂತನ AstAlias{flp, itemsp});
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstCell* nodep) override {
        UINFO(4, "  CELL   " << nodep);
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        // VV*****  We reset user1p() on each cell!!!
        Ast_ಗಂಟು::user1_Clear_ವೃಕ್ಷ();
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(AstPin* nodep) override {
        // PIN(p,expr) -> ASSIGNW(VARXREF(p),expr)    (if sub's input)
        //            or  ASSIGNW(expr,VARXREF(p))    (if sub's output)
        UINFO(4, "   PIN  " << nodep);
        ಯದಿ (!nodep->user1()) {
            // Simplify it
            V3Inst::pinReconnectSimple(nodep, m_cellp, ಅಸತ್ಯ);
        }
        UINFOTREE(9, nodep, "", "Pin_oldb");
        ಯದಿ (!nodep->exprp()) ಪ್ರತ್ಯಾಗಮ;  // No-connect
        V3Inst::checkOutputShort(nodep);
        ಯದಿ (!nodep->exprp()) ಪ್ರತ್ಯಾಗಮ;  // Connection removed by checkOutputShort
        // Use user1p on the PIN to indicate we created an assign for this pin
        ಯದಿ (!nodep->user1SetOnce()) {
            // Make an ASSIGNW (expr, pin)
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = VN_AS(nodep->exprp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
            UASSERT_OBJ(exprp->width() == nodep->modVarp()->width(), nodep,
                        "Width mismatch, should have been handled in pinReconnectSimple");
            ಯದಿ (nodep->modVarp()->isInout()) {
                nodep->v3fatalSrc("Unsupported: Verilator is a 2-state simulator");
            } ಅನ್ಯಥಾ ಯದಿ (nodep->modVarp()->isWritable()) {
                ಯದಿ (!tryAliasPin(nodep, exprp)) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = ನೂತನ AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                             m_cellp->name(), VAccess::READ};
                    markContinuousLhs(exprp);
                    AstAssignW* ಧ್ರುವ assp = ನೂತನ AstAssignW{exprp->fileline(), exprp, rhsp};
                    m_cellp->addNextHere(ನೂತನ AstAlways{assp});
                }
            } ಅನ್ಯಥಾ ಯದಿ (nodep->modVarp()->isNonOutput()) {
                ಯದಿ (!tryAliasPin(nodep, exprp)) {
                    // Don't bother moving constants now,
                    // we'll be pushing the const down to the cell soon enough.
                    AstVarXRef* ಧ್ರುವ lhsp = ನೂತನ AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                            m_cellp->name(), VAccess::WRITE};

                    markContinuousLhs(lhsp);
                    AstAssignW* ಧ್ರುವ assp = ನೂತನ AstAssignW{exprp->fileline(), lhsp, exprp};
                    m_cellp->addNextHere(ನೂತನ AstAlways{assp});
                    UINFOTREE(9, assp, "", "_new");
                }
            } ಅನ್ಯಥಾ ಯದಿ (nodep->modVarp()->isIfaceRef()) {
                // Create an AstAliasScope for Vars to Cells so we can
                // link with their scope later
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = ನೂತನ AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                         m_cellp->name(), VAccess::READ};
                ಧ್ರುವ AstVarRef* ಧ್ರುವ refp = VN_CAST(exprp, VarRef);
                ಧ್ರುವ AstVarXRef* ಧ್ರುವ xrefp = VN_CAST(exprp, VarXRef);
                UASSERT_OBJ(refp || xrefp, exprp,
                            "Interfaces: Pin is not connected to a VarRef or VarXRef");
                m_cellp->addNextHere(ನೂತನ AstAliasScope{exprp->fileline(), lhsp, exprp});
            } ಅನ್ಯಥಾ {
                nodep->v3error("Assigned pin is neither input nor output");
            }
        }

        // We're done with the pin
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
    }

    // Save some time
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*) override {}
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Assign*) override {}
    ಶೂನ್ಯ visit(AstAlways*) override {}

    //--------------------
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit InstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~InstVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// Inst static function

ವರ್ಗ InstStatic final {
    InstStatic() = ಸ್ವಭಾವ;  // Static class

    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* extendOrSel(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, ಧ್ರುವ Ast_ಗಂಟು* cmpWidthp) {
        ಯದಿ (cmpWidthp->width() > rhsp->width()) {
            rhsp = (rhsp->isSigned() ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ನೂತನ AstExtendS{fl, rhsp})
                                     : static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ನೂತನ AstExtend{fl, rhsp}));
            // Need proper widthMin, which may differ from AstSel created above
            rhsp->dtypeFrom(cmpWidthp);
        } ಅನ್ಯಥಾ ಯದಿ (cmpWidthp->width() < rhsp->width()) {
            rhsp = ನೂತನ AstSel{fl, rhsp, 0, cmpWidthp->width()};
            // Need proper widthMin, which may differ from AstSel created above
            rhsp->dtypeFrom(cmpWidthp);
        }
        // else don't change dtype, as might be e.g. array of something
        ಪ್ರತ್ಯಾಗಮ rhsp;
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ AstAssignW* pinReconnectSimple(AstPin* pinp, AstCell* cellp, ತಾರ್ಕಿಕ forTristate,
                                          ತಾರ್ಕಿಕ alwaysCvt) {
        // If a pin connection is "simple" leave it as-is
        // Else create a intermediate wire to perform the interconnect
        // Return the new assignment, if one was made
        // Note this module calls cloneTree() via new AstVar
        AstVar* ಧ್ರುವ pinVarp = pinp->modVarp();
        ಯದಿ (!pinp->exprp()) {
            // No-connect, perhaps promote based on `unconnected_drive,
            // otherwise done
            ಯದಿ (pinVarp->direction() == VDirection::ಒಳಹರಿವು
                && cellp->modp()->unconnectedDrive().isSetTrue()) {
                pinp->exprp(ನೂತನ AstConst{pinp->fileline(), AstConst::All1{}});
            } ಅನ್ಯಥಾ ಯದಿ (pinVarp->direction() == VDirection::ಒಳಹರಿವು
                       && cellp->modp()->unconnectedDrive().isSetFalse()) {
                pinp->exprp(ನೂತನ AstConst{pinp->fileline(), AstConst::All0{}});
            } ಅನ್ಯಥಾ {
                ಪ್ರತ್ಯಾಗಮ nullptr;
            }
        }
        ಧ್ರುವ AstVarRef* ಧ್ರುವ connectRefp = VN_CAST(pinp->exprp(), VarRef);
        ಧ್ರುವ AstVarXRef* ಧ್ರುವ connectXRefp = VN_CAST(pinp->exprp(), VarXRef);
        ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ pinDTypep = pinVarp->dtypep()->skipRefp();
        ಧ್ರುವ AstBasicDType* ಧ್ರುವ pinBasicp = VN_CAST(pinDTypep, BasicDType);
        ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ connDTypep
            = connectRefp ? connectRefp->varp()->dtypep()->skipRefp() : nullptr;
        ಧ್ರುವ AstBasicDType* ಧ್ರುವ connBasicp = VN_CAST(connDTypep, BasicDType);
        AstAssignW* assignp = nullptr;
        //
        ಯದಿ (!alwaysCvt && connectRefp && connDTypep->same_ವೃಕ್ಷ(pinDTypep)
            && !connectRefp->varp()->isSc()) {  // Need the signal as a 'shell' to convert types
            // Done. Same data type
        } ಅನ್ಯಥಾ ಯದಿ (!alwaysCvt && connectRefp && connectRefp->varp()->isIfaceRef()) {
            // Done. Interface
        } ಅನ್ಯಥಾ ಯದಿ (!alwaysCvt && connectXRefp && connectXRefp->varp()
                   && connectXRefp->varp()->isIfaceRef()) {
        } ಅನ್ಯಥಾ ಯದಿ (!alwaysCvt && connBasicp && pinBasicp
                   && connBasicp->width() == pinBasicp->width()
                   && connBasicp->lo() == pinBasicp->lo()
                   && !connectRefp->varp()
                           ->isSc()  // Need the signal as a 'shell' to convert types
                   && connBasicp->width() == pinVarp->width()) {
            // Done. One to one interconnect won't need a temporary variable.
        } ಅನ್ಯಥಾ ಯದಿ (!alwaysCvt && !forTristate && VN_IS(pinp->exprp(), Const)) {
            // Done. Constant. Still check for driving an output, like below.
            V3Inst::checkOutputShort(pinp);
            ಯದಿ (!pinp->exprp()) ಪ್ರತ್ಯಾಗಮ nullptr;
        } ಅನ್ಯಥಾ {
            // Make a new temp wire
            // UINFOTREE(9, pinp, "", "in_pin");
            V3Inst::checkOutputShort(pinp);
            ಯದಿ (!pinp->exprp()) ಪ್ರತ್ಯಾಗಮ nullptr;
            // Simplify, so stuff like '{a[0], b[0]}[1]' produced during
            // instance array expansion are brought to normal 'a[0]'
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ pinexprp
                = V3Const::constifyEdit(VN_AS(pinp->exprp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)->unlinkFrBack());
            ಧ್ರುವ string newvarname
                = (string{pinVarp->isWritable() ? "__Vcellout" : "__Vcellinp"}
                   // Prevent name conflict if both tri & non-tri add signals
                   + (forTristate ? "t" : "") + "__" + cellp->name() + "__" + pinp->name());
            AstVar* ಧ್ರುವ newvarp
                = ನೂತನ AstVar{pinVarp->fileline(), VVarType::MODULETEMP, newvarname, pinVarp};
            // Important to add statement next to cell, in case there is a
            // generate with same named cell
            cellp->addNextHere(newvarp);
            ಯದಿ (pinVarp->isInout()) {
                pinVarp->v3fatalSrc("Unsupported: Inout connections to pins must be"
                                    " direct one-to-one connection (without any expression)");
                // V3Tristate should have cleared up before this point
            } ಅನ್ಯಥಾ ಯದಿ (pinVarp->isWritable()) {
                // See also V3Inst
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp = ನೂತನ AstVarRef{pinp->fileline(), newvarp, VAccess::READ};
                UINFO(5, "pinRecon width " << pinVarp->width() << " >? " << rhsp->width() << " >? "
                                           << pinexprp->width());
                rhsp = extendOrSel(pinp->fileline(), rhsp, pinVarp);
                pinp->exprp(ನೂತನ AstVarRef{newvarp->fileline(), newvarp, VAccess::WRITE});
                markContinuousLhs(pinexprp);
                ಯದಿ (VN_IS(pinexprp, ಗಂಟು_Stream)) {
                    assignp = ನೂತನ AstAssignW{pinp->fileline(), pinexprp, rhsp};
                    V3Width::streamAssignLowerEdit(assignp);
                } ಅನ್ಯಥಾ {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsSelp = extendOrSel(pinp->fileline(), rhsp, pinexprp);
                    // A reversed array is assigned by the left-to-right order of elements
                    // (IEEE 1800-2023 7.6)
                    ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ arrayp
                        = VN_CAST(pinexprp->dtypep()->skipRefp(), UnpackArrayDType);
                    ಯದಿ (arrayp && arrayp->isReverseOf(rhsSelp->dtypep())) {
                        rhsSelp = ನೂತನ AstCvtUnpackToUnpack{pinp->fileline(), rhsSelp,
                                                           pinexprp->dtypep()};
                    }
                    assignp = ನೂತನ AstAssignW{pinp->fileline(), pinexprp, rhsSelp};
                }
            } ಅನ್ಯಥಾ {
                // V3 width should have range/extended to make the widths correct
                newvarp->isContinuously(ಸತ್ಯ);
                // An input port driven from inside the cell by a clocking block
                // output flows out through the connection; V3Tristate split
                // pins resolve separately.
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp = nullptr;  // LHS of the interconnect assign
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp = nullptr;  // RHS of the interconnect assign
                ಯದಿ (forTristate || !pinVarp->isClockingDriven()) {
                    // Input-side wiring: temp fed from the connection
                    lhsp = ನೂತನ AstVarRef{pinp->fileline(), newvarp, VAccess::WRITE};
                    rhsp = pinexprp;
                } ಅನ್ಯಥಾ ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ drivenp = tryConnectionLhs(pinexprp)) {
                    // Reverse wiring: connection leaves fed from the temp.
                    // The original connection is consumed by the clone above.
                    VL_DO_DANGLING(pinexprp->delete_ವೃಕ್ಷ(), pinexprp);
                    markContinuousLhs(drivenp);
                    lhsp = drivenp;
                    rhsp = extendOrSel(pinp->fileline(),
                                       ನೂತನ AstVarRef{pinp->fileline(), newvarp, VAccess::READ},
                                       drivenp);
                } ಅನ್ಯಥಾ {
                    pinp->v3error("Unsupported: Clocking block output driving input port "
                                  "through non-decomposable connection "
                                  << pinp->prettyNameQ());
                    VL_DO_DANGLING(pinexprp->delete_ವೃಕ್ಷ(), pinexprp);
                    ಪ್ರತ್ಯಾಗಮ nullptr;
                }
                pinp->exprp(ನೂತನ AstVarRef{pinp->fileline(), newvarp, VAccess::READ});
                assignp = ನೂತನ AstAssignW{pinp->fileline(), lhsp, rhsp};
            }
            ಯದಿ (assignp) cellp->addNextHere(ನೂತನ AstAlways{assignp});
            // UINFOTREE(1, pinp, "", "out");
            // UINFOTREE(1, assignp, "", "aout");
        }
        ಪ್ರತ್ಯಾಗಮ assignp;
    }
};

//######################################################################
// Inst class functions

AstAssignW* V3Inst::pinReconnectSimple(AstPin* pinp, AstCell* cellp, ತಾರ್ಕಿಕ forTristate,
                                       ತಾರ್ಕಿಕ alwaysCvt) {
    ಪ್ರತ್ಯಾಗಮ InstStatic::pinReconnectSimple(pinp, cellp, forTristate, alwaysCvt);
}

ಶೂನ್ಯ V3Inst::checkOutputShort(ಧ್ರುವ AstPin* nodep) {
    ಯದಿ (nodep->modVarp()->direction() == VDirection::OUTPUT) {
        ಯದಿ (VN_IS(nodep->exprp(), Const) || VN_IS(nodep->exprp(), Extend)
            || (VN_IS(nodep->exprp(), Concat)
                && (VN_IS(VN_AS(nodep->exprp(), Concat)->lhsp(), Const)))) {
            // Uses v3warn for error, as might be found multiple times
            nodep->v3warn(E_PORTSHORT, "Output port is connected to a constant pin,"
                                       " electrical short");
            // Delete so we don't create a 'CONST = ...' assignment
            nodep->exprp()->unlinkFrBack()->delete_ವೃಕ್ಷ();
        }
    }
}

//######################################################################
// Inst class visitor

ಶೂನ್ಯ V3Inst::instAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { InstVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("inst", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
