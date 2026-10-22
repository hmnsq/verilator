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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Inst.h"

#समावेश "V3Const.h"
#समावेश "V3Control.h"
#समावेश "V3Width.h"

VL_DEFINE_DEBUG_FUNCTIONS;

अटल व्योम markContinuousLhs(Ast_गाँठ* स्थिर nodep) {
    nodep->foreach([](Ast_गाँठ_Var_Ref* refp) {
        अगर (refp->access().isWriteOrRW()) refp->varp()->isContinuously(true);
    });
}

// Flip the access of the driven leaves of a pin connection expression so it
// can be used on the LHS of an assignment. Only variables and bit/part
// selects over variables are flipped; index and select operands stay
// read-only. Struct-member selects cannot appear here: V3Width lowers them to
// selects before V3Inst. Returns false if the connection cannot be driven
// (e.g. arithmetic), in which case the expression is left unmodified.
अटल bool markConnectionLhs(Ast_गाँठ_व्यंजक* स्थिर nodep) {
    अगर (Ast_गाँठ_Var_Ref* स्थिर refp = VN_CAST(nodep, गाँठ_Var_Ref)) {
        अगर (!refp->access().isReadOnly()) वापस false;
        refp->access(VAccess::WRITE);
        वापस true;
    }
    // Note AstSel (bit/part select) is not an AstNodeSel (array select);
    // both expose fromp() for the selected value.
    अगर (AstSel* स्थिर selp = VN_CAST(nodep, Sel)) वापस markConnectionLhs(selp->fromp());
    अगर (Ast_गाँठ_Sel* स्थिर selp = VN_CAST(nodep, गाँठ_Sel)) {
        वापस markConnectionLhs(selp->fromp());
    }
    अगर (AstConcat* स्थिर concatp = VN_CAST(nodep, Concat)) {
        वापस markConnectionLhs(concatp->lhsp()) && markConnectionLhs(concatp->rhsp());
    }
    वापस false;
}

// Prepare a pin connection expression as the LHS of the reverse (inside-out)
// assignment. Returns the flipped expression on success, nullptr if the
// connection cannot be driven. The input is left unmodified on failure.
अटल Ast_गाँठ_व्यंजक* tryConnectionLhs(Ast_गाँठ_व्यंजक* स्थिर nodep) {
    Ast_गाँठ_व्यंजक* स्थिर clonep = nodep->clone_वृक्ष(false);
    अगर (markConnectionLhs(clonep)) वापस clonep;
    VL_DO_DANGLING(clonep->delete_वृक्ष(), clonep);
    वापस nullptr;
}

//######################################################################
// Inst state, as a visitor of each AstNode

श्रेणी InstVisitor final : खुला VNVisitor {
    // NODE STATE
    // Cleared each Cell:
    //  AstPin::user1p()        -> bool.  True if created assignment already
    स्थिर VNUser1InUse m_inuser1;

    // STATE
    AstCell* m_cellp = nullptr;  // Current cell

    // METHODS
    // If appropriate, add an AstAlias to connect the given Cell pin to the given expression.
    // Returns true if an alias was made, in which case there is nothing else to do for this pin.
    bool tryAliasPin(AstPin* nodep, Ast_गाँठ_व्यंजक* exprp) {
        AstVar* स्थिर modVarp = nodep->modVarp();
        // An interface reference is aliased via AstAliasScope below, not as a variable
        अगर (modVarp->isIfaceRef()) वापस false;
        // Only a whole variable can be aliased, anything else needs the assignment
        AstVarRef* स्थिर refp = VN_CAST(exprp, VarRef);
        अगर (!refp) वापस false;
        AstVar* स्थिर व्यंजक_Varp = refp->varp();
        // A virtual interface port connected to a real interface can't be an alias
        अगर (व्यंजक_Varp->isIfaceRef()) वापस false;
        // A ref port must become an alias
        अगर (!modVarp->direction().isRef()) {
            // V3FsmDetect recognizes the registers of an fsm_register_wrapper instance via
            // the assignments of its pins, so leave those as assignments
            Ast_गाँठ_Module* स्थिर cellModp = m_cellp->modp();
            अगर (V3Control::getFsmRegisterWrapper(cellModp->origName())
                || V3Control::getFsmRegisterWrapper(cellModp->prettyDehashOrigOrName())) {
                वापस false;
            }
            // A virtual interface method call is dispatched at run time, so the body of an
            // interface reached that way must use the signals of the instance it is called
            // on, not those of whichever instance this connection happens to be made to
            अगर (स्थिर AstIface* स्थिर ifacep = VN_CAST(m_cellp->modp(), Iface)) {
                अगर (ifacep->hasVirtualRef()) वापस false;
            }
            // Forced signals must keep their own storage, the two sides can be forced separately
            अगर (modVarp->isForced() || व्यंजक_Varp->isForced()) वापस false;
            // Same for public
            अगर (modVarp->isSigUserRWPublic() || व्यंजक_Varp->isSigUserRWPublic()) वापस false;
            // V3Tristate resolved the connected net already, and drives it from the
            // resolution it built for it, so a port merged into it would be driven by
            // the resolution of the instance as well
            अगर (व्यंजक_Varp->isTristate()) वापस false;
        }
        // They will become the same variable, so propagate file-line and attributes
        व्यंजक_Varp->fileline()->modifyStateInherit(modVarp->fileline());
        modVarp->fileline()->modifyStateInherit(व्यंजक_Varp->fileline());
        व्यंजक_Varp->propagateAttrFrom(modVarp);
        modVarp->propagateAttrFrom(व्यंजक_Varp);
        refp->access(VAccess::READWRITE);
        File_पंक्ति* स्थिर flp = exprp->fileline();
        Ast_गाँठ_व्यंजक* स्थिर itemsp
            = नया AstVarXRef{flp, modVarp, m_cellp->name(), VAccess::READWRITE};
        itemsp->add_अग्र(exprp);
        m_cellp->add_अग्र_Here(नया AstAlias{flp, itemsp});
        वापस true;
    }

    // VISITORS
    व्योम visit(AstCell* nodep) override {
        UINFO(4, "  CELL   " << nodep);
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        // VV*****  We reset user1p() on each cell!!!
        Ast_गाँठ::user1_Clear_वृक्ष();
        iterateChildren(nodep);
    }

    व्योम visit(AstPin* nodep) override {
        // PIN(p,expr) -> ASSIGNW(VARXREF(p),expr)    (if sub's input)
        //            or  ASSIGNW(expr,VARXREF(p))    (if sub's output)
        UINFO(4, "   PIN  " << nodep);
        अगर (!nodep->user1()) {
            // Simplify it
            V3Inst::pinReconnectSimple(nodep, m_cellp, false);
        }
        UINFOTREE(9, nodep, "", "Pin_oldb");
        अगर (!nodep->exprp()) वापस;  // No-connect
        V3Inst::checkOutputShort(nodep);
        अगर (!nodep->exprp()) वापस;  // Connection removed by checkOutputShort
        // Use user1p on the PIN to indicate we created an assign for this pin
        अगर (!nodep->user1SetOnce()) {
            // Make an ASSIGNW (expr, pin)
            Ast_गाँठ_व्यंजक* स्थिर exprp = VN_AS(nodep->exprp(), गाँठ_व्यंजक)->clone_वृक्ष(false);
            UASSERT_OBJ(exprp->width() == nodep->modVarp()->width(), nodep,
                        "Width mismatch, should have been handled in pinReconnectSimple");
            अगर (nodep->modVarp()->isInout()) {
                nodep->v3fatalSrc("Unsupported: Verilator is a 2-state simulator");
            } अन्यथा अगर (nodep->modVarp()->isWritable()) {
                अगर (!tryAliasPin(nodep, exprp)) {
                    Ast_गाँठ_व्यंजक* स्थिर rhsp = नया AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                             m_cellp->name(), VAccess::READ};
                    markContinuousLhs(exprp);
                    AstAssignW* स्थिर assp = नया AstAssignW{exprp->fileline(), exprp, rhsp};
                    m_cellp->add_अग्र_Here(नया AstAlways{assp});
                }
            } अन्यथा अगर (nodep->modVarp()->isNonOutput()) {
                अगर (!tryAliasPin(nodep, exprp)) {
                    // Don't bother moving constants now,
                    // we'll be pushing the const down to the cell soon enough.
                    AstVarXRef* स्थिर lhsp = नया AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                            m_cellp->name(), VAccess::WRITE};

                    markContinuousLhs(lhsp);
                    AstAssignW* स्थिर assp = नया AstAssignW{exprp->fileline(), lhsp, exprp};
                    m_cellp->add_अग्र_Here(नया AstAlways{assp});
                    UINFOTREE(9, assp, "", "_new");
                }
            } अन्यथा अगर (nodep->modVarp()->isIfaceRef()) {
                // Create an AstAliasScope for Vars to Cells so we can
                // link with their scope later
                Ast_गाँठ_व्यंजक* स्थिर lhsp = नया AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                         m_cellp->name(), VAccess::READ};
                स्थिर AstVarRef* स्थिर refp = VN_CAST(exprp, VarRef);
                स्थिर AstVarXRef* स्थिर xrefp = VN_CAST(exprp, VarXRef);
                UASSERT_OBJ(refp || xrefp, exprp,
                            "Interfaces: Pin is not connected to a VarRef or VarXRef");
                m_cellp->add_अग्र_Here(नया AstAliasScope{exprp->fileline(), lhsp, exprp});
            } अन्यथा {
                nodep->v3error("Assigned pin is neither input nor output");
            }
        }

        // We're done with the pin
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
    }

    // Save some time
    व्योम visit(Ast_गाँठ_व्यंजक*) override {}
    व्योम visit(Ast_गाँठ_Assign*) override {}
    व्योम visit(AstAlways*) override {}

    //--------------------
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    // CONSTRUCTORS
    explicit InstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~InstVisitor() override = शेष;
};

//######################################################################
// Inst static function

श्रेणी InstStatic final {
    InstStatic() = शेष;  // Static class

    अटल Ast_गाँठ_व्यंजक* extendOrSel(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* rhsp, स्थिर Ast_गाँठ* cmpWidthp) {
        अगर (cmpWidthp->width() > rhsp->width()) {
            rhsp = (rhsp->isSigned() ? static_cast<Ast_गाँठ_व्यंजक*>(नया AstExtendS{fl, rhsp})
                                     : static_cast<Ast_गाँठ_व्यंजक*>(नया AstExtend{fl, rhsp}));
            // Need proper widthMin, which may differ from AstSel created above
            rhsp->dtypeFrom(cmpWidthp);
        } अन्यथा अगर (cmpWidthp->width() < rhsp->width()) {
            rhsp = नया AstSel{fl, rhsp, 0, cmpWidthp->width()};
            // Need proper widthMin, which may differ from AstSel created above
            rhsp->dtypeFrom(cmpWidthp);
        }
        // else don't change dtype, as might be e.g. array of something
        वापस rhsp;
    }

खुला:
    अटल AstAssignW* pinReconnectSimple(AstPin* pinp, AstCell* cellp, bool forTristate,
                                          bool alwaysCvt) {
        // If a pin connection is "simple" leave it as-is
        // Else create a intermediate wire to perform the interconnect
        // Return the new assignment, if one was made
        // Note this module calls cloneTree() via new AstVar
        AstVar* स्थिर pinVarp = pinp->modVarp();
        अगर (!pinp->exprp()) {
            // No-connect, perhaps promote based on `unconnected_drive,
            // otherwise done
            अगर (pinVarp->direction() == VDirection::निवेश
                && cellp->modp()->unconnectedDrive().isSetTrue()) {
                pinp->exprp(नया AstConst{pinp->fileline(), AstConst::All1{}});
            } अन्यथा अगर (pinVarp->direction() == VDirection::निवेश
                       && cellp->modp()->unconnectedDrive().isSetFalse()) {
                pinp->exprp(नया AstConst{pinp->fileline(), AstConst::All0{}});
            } अन्यथा {
                वापस nullptr;
            }
        }
        स्थिर AstVarRef* स्थिर connectRefp = VN_CAST(pinp->exprp(), VarRef);
        स्थिर AstVarXRef* स्थिर connectXRefp = VN_CAST(pinp->exprp(), VarXRef);
        स्थिर Ast_गाँठ_DType* स्थिर pinDTypep = pinVarp->dtypep()->skipRefp();
        स्थिर AstBasicDType* स्थिर pinBasicp = VN_CAST(pinDTypep, BasicDType);
        स्थिर Ast_गाँठ_DType* स्थिर connDTypep
            = connectRefp ? connectRefp->varp()->dtypep()->skipRefp() : nullptr;
        स्थिर AstBasicDType* स्थिर connBasicp = VN_CAST(connDTypep, BasicDType);
        AstAssignW* assignp = nullptr;
        //
        अगर (!alwaysCvt && connectRefp && connDTypep->same_वृक्ष(pinDTypep)
            && !connectRefp->varp()->isSc()) {  // Need the signal as a 'shell' to convert types
            // Done. Same data type
        } अन्यथा अगर (!alwaysCvt && connectRefp && connectRefp->varp()->isIfaceRef()) {
            // Done. Interface
        } अन्यथा अगर (!alwaysCvt && connectXRefp && connectXRefp->varp()
                   && connectXRefp->varp()->isIfaceRef()) {
        } अन्यथा अगर (!alwaysCvt && connBasicp && pinBasicp
                   && connBasicp->width() == pinBasicp->width()
                   && connBasicp->lo() == pinBasicp->lo()
                   && !connectRefp->varp()
                           ->isSc()  // Need the signal as a 'shell' to convert types
                   && connBasicp->width() == pinVarp->width()) {
            // Done. One to one interconnect won't need a temporary variable.
        } अन्यथा अगर (!alwaysCvt && !forTristate && VN_IS(pinp->exprp(), Const)) {
            // Done. Constant. Still check for driving an output, like below.
            V3Inst::checkOutputShort(pinp);
            अगर (!pinp->exprp()) वापस nullptr;
        } अन्यथा {
            // Make a new temp wire
            // UINFOTREE(9, pinp, "", "in_pin");
            V3Inst::checkOutputShort(pinp);
            अगर (!pinp->exprp()) वापस nullptr;
            // Simplify, so stuff like '{a[0], b[0]}[1]' produced during
            // instance array expansion are brought to normal 'a[0]'
            Ast_गाँठ_व्यंजक* स्थिर pinexprp
                = V3Const::constifyEdit(VN_AS(pinp->exprp(), गाँठ_व्यंजक)->unlinkFrBack());
            स्थिर string newvarname
                = (string{pinVarp->isWritable() ? "__Vcellout" : "__Vcellinp"}
                   // Prevent name conflict if both tri & non-tri add signals
                   + (forTristate ? "t" : "") + "__" + cellp->name() + "__" + pinp->name());
            AstVar* स्थिर newvarp
                = नया AstVar{pinVarp->fileline(), VVarType::MODULETEMP, newvarname, pinVarp};
            // Important to add statement next to cell, in case there is a
            // generate with same named cell
            cellp->add_अग्र_Here(newvarp);
            अगर (pinVarp->isInout()) {
                pinVarp->v3fatalSrc("Unsupported: Inout connections to pins must be"
                                    " direct one-to-one connection (without any expression)");
                // V3Tristate should have cleared up before this point
            } अन्यथा अगर (pinVarp->isWritable()) {
                // See also V3Inst
                Ast_गाँठ_व्यंजक* rhsp = नया AstVarRef{pinp->fileline(), newvarp, VAccess::READ};
                UINFO(5, "pinRecon width " << pinVarp->width() << " >? " << rhsp->width() << " >? "
                                           << pinexprp->width());
                rhsp = extendOrSel(pinp->fileline(), rhsp, pinVarp);
                pinp->exprp(नया AstVarRef{newvarp->fileline(), newvarp, VAccess::WRITE});
                markContinuousLhs(pinexprp);
                अगर (VN_IS(pinexprp, गाँठ_Stream)) {
                    assignp = नया AstAssignW{pinp->fileline(), pinexprp, rhsp};
                    V3Width::streamAssignLowerEdit(assignp);
                } अन्यथा {
                    Ast_गाँठ_व्यंजक* rhsSelp = extendOrSel(pinp->fileline(), rhsp, pinexprp);
                    // A reversed array is assigned by the left-to-right order of elements
                    // (IEEE 1800-2023 7.6)
                    स्थिर AstUnpackArrayDType* स्थिर arrayp
                        = VN_CAST(pinexprp->dtypep()->skipRefp(), UnpackArrayDType);
                    अगर (arrayp && arrayp->isReverseOf(rhsSelp->dtypep())) {
                        rhsSelp = नया AstCvtUnpackToUnpack{pinp->fileline(), rhsSelp,
                                                           pinexprp->dtypep()};
                    }
                    assignp = नया AstAssignW{pinp->fileline(), pinexprp, rhsSelp};
                }
            } अन्यथा {
                // V3 width should have range/extended to make the widths correct
                newvarp->isContinuously(true);
                // An input port driven from inside the cell by a clocking block
                // output flows out through the connection; V3Tristate split
                // pins resolve separately.
                Ast_गाँठ_व्यंजक* lhsp = nullptr;  // LHS of the interconnect assign
                Ast_गाँठ_व्यंजक* rhsp = nullptr;  // RHS of the interconnect assign
                अगर (forTristate || !pinVarp->isClockingDriven()) {
                    // Input-side wiring: temp fed from the connection
                    lhsp = नया AstVarRef{pinp->fileline(), newvarp, VAccess::WRITE};
                    rhsp = pinexprp;
                } अन्यथा अगर (Ast_गाँठ_व्यंजक* स्थिर drivenp = tryConnectionLhs(pinexprp)) {
                    // Reverse wiring: connection leaves fed from the temp.
                    // The original connection is consumed by the clone above.
                    VL_DO_DANGLING(pinexprp->delete_वृक्ष(), pinexprp);
                    markContinuousLhs(drivenp);
                    lhsp = drivenp;
                    rhsp = extendOrSel(pinp->fileline(),
                                       नया AstVarRef{pinp->fileline(), newvarp, VAccess::READ},
                                       drivenp);
                } अन्यथा {
                    pinp->v3error("Unsupported: Clocking block output driving input port "
                                  "through non-decomposable connection "
                                  << pinp->prettyNameQ());
                    VL_DO_DANGLING(pinexprp->delete_वृक्ष(), pinexprp);
                    वापस nullptr;
                }
                pinp->exprp(नया AstVarRef{pinp->fileline(), newvarp, VAccess::READ});
                assignp = नया AstAssignW{pinp->fileline(), lhsp, rhsp};
            }
            अगर (assignp) cellp->add_अग्र_Here(नया AstAlways{assignp});
            // UINFOTREE(1, pinp, "", "out");
            // UINFOTREE(1, assignp, "", "aout");
        }
        वापस assignp;
    }
};

//######################################################################
// Inst class functions

AstAssignW* V3Inst::pinReconnectSimple(AstPin* pinp, AstCell* cellp, bool forTristate,
                                       bool alwaysCvt) {
    वापस InstStatic::pinReconnectSimple(pinp, cellp, forTristate, alwaysCvt);
}

व्योम V3Inst::checkOutputShort(स्थिर AstPin* nodep) {
    अगर (nodep->modVarp()->direction() == VDirection::OUTPUT) {
        अगर (VN_IS(nodep->exprp(), Const) || VN_IS(nodep->exprp(), Extend)
            || (VN_IS(nodep->exprp(), Concat)
                && (VN_IS(VN_AS(nodep->exprp(), Concat)->lhsp(), Const)))) {
            // Uses v3warn for error, as might be found multiple times
            nodep->v3warn(E_PORTSHORT, "Output port is connected to a constant pin,"
                                       " electrical short");
            // Delete so we don't create a 'CONST = ...' assignment
            nodep->exprp()->unlinkFrBack()->delete_वृक्ष();
        }
    }
}

//######################################################################
// Inst class visitor

व्योम V3Inst::instAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { InstVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("inst", 0, dump_वृक्ष_Either_Level() >= 3);
}
