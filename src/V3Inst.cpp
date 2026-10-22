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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Inst.h"

#અન્તર્ભાવ "V3Const.h"
#અન્તર્ભાવ "V3Control.h"
#અન્તર્ભાવ "V3Width.h"

VL_DEFINE_DEBUG_FUNCTIONS;

સ્થિર શૂન્ય markContinuousLhs(Ast_ગાંઠ* અચલ nodep) {
    nodep->foreach([](Ast_ગાંઠ_Var_Ref* refp) {
        જો (refp->access().isWriteOrRW()) refp->varp()->isContinuously(સાચું);
    });
}

// Flip the access of the driven leaves of a pin connection expression so it
// can be used on the LHS of an assignment. Only variables and bit/part
// selects over variables are flipped; index and select operands stay
// read-only. Struct-member selects cannot appear here: V3Width lowers them to
// selects before V3Inst. Returns false if the connection cannot be driven
// (e.g. arithmetic), in which case the expression is left unmodified.
સ્થિર તાર્કિક markConnectionLhs(Ast_ગાંઠ_પદાવલિ* અચલ nodep) {
    જો (Ast_ગાંઠ_Var_Ref* અચલ refp = VN_CAST(nodep, ગાંઠ_Var_Ref)) {
        જો (!refp->access().isReadOnly()) પાછા ખોટું;
        refp->access(VAccess::WRITE);
        પાછા સાચું;
    }
    // Note AstSel (bit/part select) is not an AstNodeSel (array select);
    // both expose fromp() for the selected value.
    જો (AstSel* અચલ selp = VN_CAST(nodep, Sel)) પાછા markConnectionLhs(selp->fromp());
    જો (Ast_ગાંઠ_Sel* અચલ selp = VN_CAST(nodep, ગાંઠ_Sel)) {
        પાછા markConnectionLhs(selp->fromp());
    }
    જો (AstConcat* અચલ concatp = VN_CAST(nodep, Concat)) {
        પાછા markConnectionLhs(concatp->lhsp()) && markConnectionLhs(concatp->rhsp());
    }
    પાછા ખોટું;
}

// Prepare a pin connection expression as the LHS of the reverse (inside-out)
// assignment. Returns the flipped expression on success, nullptr if the
// connection cannot be driven. The input is left unmodified on failure.
સ્થિર Ast_ગાંઠ_પદાવલિ* tryConnectionLhs(Ast_ગાંઠ_પદાવલિ* અચલ nodep) {
    Ast_ગાંઠ_પદાવલિ* અચલ clonep = nodep->clone_વૃક્ષ(ખોટું);
    જો (markConnectionLhs(clonep)) પાછા clonep;
    VL_DO_DANGLING(clonep->delete_વૃક્ષ(), clonep);
    પાછા nullptr;
}

//######################################################################
// Inst state, as a visitor of each AstNode

વર્ગ InstVisitor final : જાહેર VNVisitor {
    // NODE STATE
    // Cleared each Cell:
    //  AstPin::user1p()        -> bool.  True if created assignment already
    અચલ VNUser1InUse m_inuser1;

    // STATE
    AstCell* m_cellp = nullptr;  // Current cell

    // METHODS
    // If appropriate, add an AstAlias to connect the given Cell pin to the given expression.
    // Returns true if an alias was made, in which case there is nothing else to do for this pin.
    તાર્કિક tryAliasPin(AstPin* nodep, Ast_ગાંઠ_પદાવલિ* exprp) {
        AstVar* અચલ modVarp = nodep->modVarp();
        // An interface reference is aliased via AstAliasScope below, not as a variable
        જો (modVarp->isIfaceRef()) પાછા ખોટું;
        // Only a whole variable can be aliased, anything else needs the assignment
        AstVarRef* અચલ refp = VN_CAST(exprp, VarRef);
        જો (!refp) પાછા ખોટું;
        AstVar* અચલ પદાવલિ_Varp = refp->varp();
        // A virtual interface port connected to a real interface can't be an alias
        જો (પદાવલિ_Varp->isIfaceRef()) પાછા ખોટું;
        // A ref port must become an alias
        જો (!modVarp->direction().isRef()) {
            // V3FsmDetect recognizes the registers of an fsm_register_wrapper instance via
            // the assignments of its pins, so leave those as assignments
            Ast_ગાંઠ_Module* અચલ cellModp = m_cellp->modp();
            જો (V3Control::getFsmRegisterWrapper(cellModp->origName())
                || V3Control::getFsmRegisterWrapper(cellModp->prettyDehashOrigOrName())) {
                પાછા ખોટું;
            }
            // A virtual interface method call is dispatched at run time, so the body of an
            // interface reached that way must use the signals of the instance it is called
            // on, not those of whichever instance this connection happens to be made to
            જો (અચલ AstIface* અચલ ifacep = VN_CAST(m_cellp->modp(), Iface)) {
                જો (ifacep->hasVirtualRef()) પાછા ખોટું;
            }
            // Forced signals must keep their own storage, the two sides can be forced separately
            જો (modVarp->isForced() || પદાવલિ_Varp->isForced()) પાછા ખોટું;
            // Same for public
            જો (modVarp->isSigUserRWPublic() || પદાવલિ_Varp->isSigUserRWPublic()) પાછા ખોટું;
            // V3Tristate resolved the connected net already, and drives it from the
            // resolution it built for it, so a port merged into it would be driven by
            // the resolution of the instance as well
            જો (પદાવલિ_Varp->isTristate()) પાછા ખોટું;
        }
        // They will become the same variable, so propagate file-line and attributes
        પદાવલિ_Varp->fileline()->modifyStateInherit(modVarp->fileline());
        modVarp->fileline()->modifyStateInherit(પદાવલિ_Varp->fileline());
        પદાવલિ_Varp->propagateAttrFrom(modVarp);
        modVarp->propagateAttrFrom(પદાવલિ_Varp);
        refp->access(VAccess::READWRITE);
        File_લીટી* અચલ flp = exprp->fileline();
        Ast_ગાંઠ_પદાવલિ* અચલ itemsp
            = નવું AstVarXRef{flp, modVarp, m_cellp->name(), VAccess::READWRITE};
        itemsp->addNext(exprp);
        m_cellp->addNextHere(નવું AstAlias{flp, itemsp});
        પાછા સાચું;
    }

    // VISITORS
    શૂન્ય visit(AstCell* nodep) override {
        UINFO(4, "  CELL   " << nodep);
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        // VV*****  We reset user1p() on each cell!!!
        Ast_ગાંઠ::user1_Clear_વૃક્ષ();
        iterateChildren(nodep);
    }

    શૂન્ય visit(AstPin* nodep) override {
        // PIN(p,expr) -> ASSIGNW(VARXREF(p),expr)    (if sub's input)
        //            or  ASSIGNW(expr,VARXREF(p))    (if sub's output)
        UINFO(4, "   PIN  " << nodep);
        જો (!nodep->user1()) {
            // Simplify it
            V3Inst::pinReconnectSimple(nodep, m_cellp, ખોટું);
        }
        UINFOTREE(9, nodep, "", "Pin_oldb");
        જો (!nodep->exprp()) પાછા;  // No-connect
        V3Inst::checkOutputShort(nodep);
        જો (!nodep->exprp()) પાછા;  // Connection removed by checkOutputShort
        // Use user1p on the PIN to indicate we created an assign for this pin
        જો (!nodep->user1SetOnce()) {
            // Make an ASSIGNW (expr, pin)
            Ast_ગાંઠ_પદાવલિ* અચલ exprp = VN_AS(nodep->exprp(), ગાંઠ_પદાવલિ)->clone_વૃક્ષ(ખોટું);
            UASSERT_OBJ(exprp->width() == nodep->modVarp()->width(), nodep,
                        "Width mismatch, should have been handled in pinReconnectSimple");
            જો (nodep->modVarp()->isInout()) {
                nodep->v3fatalSrc("Unsupported: Verilator is a 2-state simulator");
            } નહીંતર જો (nodep->modVarp()->isWritable()) {
                જો (!tryAliasPin(nodep, exprp)) {
                    Ast_ગાંઠ_પદાવલિ* અચલ rhsp = નવું AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                             m_cellp->name(), VAccess::READ};
                    markContinuousLhs(exprp);
                    AstAssignW* અચલ assp = નવું AstAssignW{exprp->fileline(), exprp, rhsp};
                    m_cellp->addNextHere(નવું AstAlways{assp});
                }
            } નહીંતર જો (nodep->modVarp()->isNonOutput()) {
                જો (!tryAliasPin(nodep, exprp)) {
                    // Don't bother moving constants now,
                    // we'll be pushing the const down to the cell soon enough.
                    AstVarXRef* અચલ lhsp = નવું AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                            m_cellp->name(), VAccess::WRITE};

                    markContinuousLhs(lhsp);
                    AstAssignW* અચલ assp = નવું AstAssignW{exprp->fileline(), lhsp, exprp};
                    m_cellp->addNextHere(નવું AstAlways{assp});
                    UINFOTREE(9, assp, "", "_new");
                }
            } નહીંતર જો (nodep->modVarp()->isIfaceRef()) {
                // Create an AstAliasScope for Vars to Cells so we can
                // link with their scope later
                Ast_ગાંઠ_પદાવલિ* અચલ lhsp = નવું AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                         m_cellp->name(), VAccess::READ};
                અચલ AstVarRef* અચલ refp = VN_CAST(exprp, VarRef);
                અચલ AstVarXRef* અચલ xrefp = VN_CAST(exprp, VarXRef);
                UASSERT_OBJ(refp || xrefp, exprp,
                            "Interfaces: Pin is not connected to a VarRef or VarXRef");
                m_cellp->addNextHere(નવું AstAliasScope{exprp->fileline(), lhsp, exprp});
            } નહીંતર {
                nodep->v3error("Assigned pin is neither input nor output");
            }
        }

        // We're done with the pin
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
    }

    // Save some time
    શૂન્ય visit(Ast_ગાંઠ_પદાવલિ*) override {}
    શૂન્ય visit(Ast_ગાંઠ_Assign*) override {}
    શૂન્ય visit(AstAlways*) override {}

    //--------------------
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit InstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~InstVisitor() override = મૂળભૂત;
};

//######################################################################
// Inst static function

વર્ગ InstStatic final {
    InstStatic() = મૂળભૂત;  // Static class

    સ્થિર Ast_ગાંઠ_પદાવલિ* extendOrSel(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* rhsp, અચલ Ast_ગાંઠ* cmpWidthp) {
        જો (cmpWidthp->width() > rhsp->width()) {
            rhsp = (rhsp->isSigned() ? static_cast<Ast_ગાંઠ_પદાવલિ*>(નવું AstExtendS{fl, rhsp})
                                     : static_cast<Ast_ગાંઠ_પદાવલિ*>(નવું AstExtend{fl, rhsp}));
            // Need proper widthMin, which may differ from AstSel created above
            rhsp->dtypeFrom(cmpWidthp);
        } નહીંતર જો (cmpWidthp->width() < rhsp->width()) {
            rhsp = નવું AstSel{fl, rhsp, 0, cmpWidthp->width()};
            // Need proper widthMin, which may differ from AstSel created above
            rhsp->dtypeFrom(cmpWidthp);
        }
        // else don't change dtype, as might be e.g. array of something
        પાછા rhsp;
    }

જાહેર:
    સ્થિર AstAssignW* pinReconnectSimple(AstPin* pinp, AstCell* cellp, તાર્કિક forTristate,
                                          તાર્કિક alwaysCvt) {
        // If a pin connection is "simple" leave it as-is
        // Else create a intermediate wire to perform the interconnect
        // Return the new assignment, if one was made
        // Note this module calls cloneTree() via new AstVar
        AstVar* અચલ pinVarp = pinp->modVarp();
        જો (!pinp->exprp()) {
            // No-connect, perhaps promote based on `unconnected_drive,
            // otherwise done
            જો (pinVarp->direction() == VDirection::આગત
                && cellp->modp()->unconnectedDrive().isSetTrue()) {
                pinp->exprp(નવું AstConst{pinp->fileline(), AstConst::All1{}});
            } નહીંતર જો (pinVarp->direction() == VDirection::આગત
                       && cellp->modp()->unconnectedDrive().isSetFalse()) {
                pinp->exprp(નવું AstConst{pinp->fileline(), AstConst::All0{}});
            } નહીંતર {
                પાછા nullptr;
            }
        }
        અચલ AstVarRef* અચલ connectRefp = VN_CAST(pinp->exprp(), VarRef);
        અચલ AstVarXRef* અચલ connectXRefp = VN_CAST(pinp->exprp(), VarXRef);
        અચલ Ast_ગાંઠ_DType* અચલ pinDTypep = pinVarp->dtypep()->skipRefp();
        અચલ AstBasicDType* અચલ pinBasicp = VN_CAST(pinDTypep, BasicDType);
        અચલ Ast_ગાંઠ_DType* અચલ connDTypep
            = connectRefp ? connectRefp->varp()->dtypep()->skipRefp() : nullptr;
        અચલ AstBasicDType* અચલ connBasicp = VN_CAST(connDTypep, BasicDType);
        AstAssignW* assignp = nullptr;
        //
        જો (!alwaysCvt && connectRefp && connDTypep->same_વૃક્ષ(pinDTypep)
            && !connectRefp->varp()->isSc()) {  // Need the signal as a 'shell' to convert types
            // Done. Same data type
        } નહીંતર જો (!alwaysCvt && connectRefp && connectRefp->varp()->isIfaceRef()) {
            // Done. Interface
        } નહીંતર જો (!alwaysCvt && connectXRefp && connectXRefp->varp()
                   && connectXRefp->varp()->isIfaceRef()) {
        } નહીંતર જો (!alwaysCvt && connBasicp && pinBasicp
                   && connBasicp->width() == pinBasicp->width()
                   && connBasicp->lo() == pinBasicp->lo()
                   && !connectRefp->varp()
                           ->isSc()  // Need the signal as a 'shell' to convert types
                   && connBasicp->width() == pinVarp->width()) {
            // Done. One to one interconnect won't need a temporary variable.
        } નહીંતર જો (!alwaysCvt && !forTristate && VN_IS(pinp->exprp(), Const)) {
            // Done. Constant. Still check for driving an output, like below.
            V3Inst::checkOutputShort(pinp);
            જો (!pinp->exprp()) પાછા nullptr;
        } નહીંતર {
            // Make a new temp wire
            // UINFOTREE(9, pinp, "", "in_pin");
            V3Inst::checkOutputShort(pinp);
            જો (!pinp->exprp()) પાછા nullptr;
            // Simplify, so stuff like '{a[0], b[0]}[1]' produced during
            // instance array expansion are brought to normal 'a[0]'
            Ast_ગાંઠ_પદાવલિ* અચલ pinexprp
                = V3Const::constifyEdit(VN_AS(pinp->exprp(), ગાંઠ_પદાવલિ)->unlinkFrBack());
            અચલ string newvarname
                = (string{pinVarp->isWritable() ? "__Vcellout" : "__Vcellinp"}
                   // Prevent name conflict if both tri & non-tri add signals
                   + (forTristate ? "t" : "") + "__" + cellp->name() + "__" + pinp->name());
            AstVar* અચલ newvarp
                = નવું AstVar{pinVarp->fileline(), VVarType::MODULETEMP, newvarname, pinVarp};
            // Important to add statement next to cell, in case there is a
            // generate with same named cell
            cellp->addNextHere(newvarp);
            જો (pinVarp->isInout()) {
                pinVarp->v3fatalSrc("Unsupported: Inout connections to pins must be"
                                    " direct one-to-one connection (without any expression)");
                // V3Tristate should have cleared up before this point
            } નહીંતર જો (pinVarp->isWritable()) {
                // See also V3Inst
                Ast_ગાંઠ_પદાવલિ* rhsp = નવું AstVarRef{pinp->fileline(), newvarp, VAccess::READ};
                UINFO(5, "pinRecon width " << pinVarp->width() << " >? " << rhsp->width() << " >? "
                                           << pinexprp->width());
                rhsp = extendOrSel(pinp->fileline(), rhsp, pinVarp);
                pinp->exprp(નવું AstVarRef{newvarp->fileline(), newvarp, VAccess::WRITE});
                markContinuousLhs(pinexprp);
                જો (VN_IS(pinexprp, ગાંઠ_Stream)) {
                    assignp = નવું AstAssignW{pinp->fileline(), pinexprp, rhsp};
                    V3Width::streamAssignLowerEdit(assignp);
                } નહીંતર {
                    Ast_ગાંઠ_પદાવલિ* rhsSelp = extendOrSel(pinp->fileline(), rhsp, pinexprp);
                    // A reversed array is assigned by the left-to-right order of elements
                    // (IEEE 1800-2023 7.6)
                    અચલ AstUnpackArrayDType* અચલ arrayp
                        = VN_CAST(pinexprp->dtypep()->skipRefp(), UnpackArrayDType);
                    જો (arrayp && arrayp->isReverseOf(rhsSelp->dtypep())) {
                        rhsSelp = નવું AstCvtUnpackToUnpack{pinp->fileline(), rhsSelp,
                                                           pinexprp->dtypep()};
                    }
                    assignp = નવું AstAssignW{pinp->fileline(), pinexprp, rhsSelp};
                }
            } નહીંતર {
                // V3 width should have range/extended to make the widths correct
                newvarp->isContinuously(સાચું);
                // An input port driven from inside the cell by a clocking block
                // output flows out through the connection; V3Tristate split
                // pins resolve separately.
                Ast_ગાંઠ_પદાવલિ* lhsp = nullptr;  // LHS of the interconnect assign
                Ast_ગાંઠ_પદાવલિ* rhsp = nullptr;  // RHS of the interconnect assign
                જો (forTristate || !pinVarp->isClockingDriven()) {
                    // Input-side wiring: temp fed from the connection
                    lhsp = નવું AstVarRef{pinp->fileline(), newvarp, VAccess::WRITE};
                    rhsp = pinexprp;
                } નહીંતર જો (Ast_ગાંઠ_પદાવલિ* અચલ drivenp = tryConnectionLhs(pinexprp)) {
                    // Reverse wiring: connection leaves fed from the temp.
                    // The original connection is consumed by the clone above.
                    VL_DO_DANGLING(pinexprp->delete_વૃક્ષ(), pinexprp);
                    markContinuousLhs(drivenp);
                    lhsp = drivenp;
                    rhsp = extendOrSel(pinp->fileline(),
                                       નવું AstVarRef{pinp->fileline(), newvarp, VAccess::READ},
                                       drivenp);
                } નહીંતર {
                    pinp->v3error("Unsupported: Clocking block output driving input port "
                                  "through non-decomposable connection "
                                  << pinp->prettyNameQ());
                    VL_DO_DANGLING(pinexprp->delete_વૃક્ષ(), pinexprp);
                    પાછા nullptr;
                }
                pinp->exprp(નવું AstVarRef{pinp->fileline(), newvarp, VAccess::READ});
                assignp = નવું AstAssignW{pinp->fileline(), lhsp, rhsp};
            }
            જો (assignp) cellp->addNextHere(નવું AstAlways{assignp});
            // UINFOTREE(1, pinp, "", "out");
            // UINFOTREE(1, assignp, "", "aout");
        }
        પાછા assignp;
    }
};

//######################################################################
// Inst class functions

AstAssignW* V3Inst::pinReconnectSimple(AstPin* pinp, AstCell* cellp, તાર્કિક forTristate,
                                       તાર્કિક alwaysCvt) {
    પાછા InstStatic::pinReconnectSimple(pinp, cellp, forTristate, alwaysCvt);
}

શૂન્ય V3Inst::checkOutputShort(અચલ AstPin* nodep) {
    જો (nodep->modVarp()->direction() == VDirection::OUTPUT) {
        જો (VN_IS(nodep->exprp(), Const) || VN_IS(nodep->exprp(), Extend)
            || (VN_IS(nodep->exprp(), Concat)
                && (VN_IS(VN_AS(nodep->exprp(), Concat)->lhsp(), Const)))) {
            // Uses v3warn for error, as might be found multiple times
            nodep->v3warn(E_PORTSHORT, "Output port is connected to a constant pin,"
                                       " electrical short");
            // Delete so we don't create a 'CONST = ...' assignment
            nodep->exprp()->unlinkFrBack()->delete_વૃક્ષ();
        }
    }
}

//######################################################################
// Inst class visitor

શૂન્ય V3Inst::instAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { InstVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("inst", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
