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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Inst.h"

#அந்தர்பாவ "V3Const.h"
#அந்தர்பாவ "V3Control.h"
#அந்தர்பாவ "V3Width.h"

VL_DEFINE_DEBUG_FUNCTIONS;

நிலையான வெறுமை markContinuousLhs(Ast_முனை* மாறிலி nodep) {
    nodep->foreach([](Ast_முனை_Var_Ref* refp) {
        எனில் (refp->access().isWriteOrRW()) refp->varp()->isContinuously(உண்மை);
    });
}

// Flip the access of the driven leaves of a pin connection expression so it
// can be used on the LHS of an assignment. Only variables and bit/part
// selects over variables are flipped; index and select operands stay
// read-only. Struct-member selects cannot appear here: V3Width lowers them to
// selects before V3Inst. Returns false if the connection cannot be driven
// (e.g. arithmetic), in which case the expression is left unmodified.
நிலையான தருக்க markConnectionLhs(Ast_முனை_கோவை* மாறிலி nodep) {
    எனில் (Ast_முனை_Var_Ref* மாறிலி refp = VN_CAST(nodep, முனை_Var_Ref)) {
        எனில் (!refp->access().isReadOnly()) திரும்பு பொய்;
        refp->access(VAccess::WRITE);
        திரும்பு உண்மை;
    }
    // Note AstSel (bit/part select) is not an AstNodeSel (array select);
    // both expose fromp() for the selected value.
    எனில் (AstSel* மாறிலி selp = VN_CAST(nodep, Sel)) திரும்பு markConnectionLhs(selp->fromp());
    எனில் (Ast_முனை_Sel* மாறிலி selp = VN_CAST(nodep, முனை_Sel)) {
        திரும்பு markConnectionLhs(selp->fromp());
    }
    எனில் (AstConcat* மாறிலி concatp = VN_CAST(nodep, Concat)) {
        திரும்பு markConnectionLhs(concatp->lhsp()) && markConnectionLhs(concatp->rhsp());
    }
    திரும்பு பொய்;
}

// Prepare a pin connection expression as the LHS of the reverse (inside-out)
// assignment. Returns the flipped expression on success, nullptr if the
// connection cannot be driven. The input is left unmodified on failure.
நிலையான Ast_முனை_கோவை* tryConnectionLhs(Ast_முனை_கோவை* மாறிலி nodep) {
    Ast_முனை_கோவை* மாறிலி clonep = nodep->clone_மரம்(பொய்);
    எனில் (markConnectionLhs(clonep)) திரும்பு clonep;
    VL_DO_DANGLING(clonep->delete_மரம்(), clonep);
    திரும்பு nullptr;
}

//######################################################################
// Inst state, as a visitor of each AstNode

வகுப்பு InstVisitor final : பொது VNVisitor {
    // NODE STATE
    // Cleared each Cell:
    //  AstPin::user1p()        -> bool.  True if created assignment already
    மாறிலி VNUser1InUse m_inuser1;

    // STATE
    AstCell* m_cellp = nullptr;  // Current cell

    // METHODS
    // If appropriate, add an AstAlias to connect the given Cell pin to the given expression.
    // Returns true if an alias was made, in which case there is nothing else to do for this pin.
    தருக்க tryAliasPin(AstPin* nodep, Ast_முனை_கோவை* exprp) {
        AstVar* மாறிலி modVarp = nodep->modVarp();
        // An interface reference is aliased via AstAliasScope below, not as a variable
        எனில் (modVarp->isIfaceRef()) திரும்பு பொய்;
        // Only a whole variable can be aliased, anything else needs the assignment
        AstVarRef* மாறிலி refp = VN_CAST(exprp, VarRef);
        எனில் (!refp) திரும்பு பொய்;
        AstVar* மாறிலி கோவை_Varp = refp->varp();
        // A virtual interface port connected to a real interface can't be an alias
        எனில் (கோவை_Varp->isIfaceRef()) திரும்பு பொய்;
        // A ref port must become an alias
        எனில் (!modVarp->direction().isRef()) {
            // V3FsmDetect recognizes the registers of an fsm_register_wrapper instance via
            // the assignments of its pins, so leave those as assignments
            Ast_முனை_Module* மாறிலி cellModp = m_cellp->modp();
            எனில் (V3Control::getFsmRegisterWrapper(cellModp->origName())
                || V3Control::getFsmRegisterWrapper(cellModp->prettyDehashOrigOrName())) {
                திரும்பு பொய்;
            }
            // A virtual interface method call is dispatched at run time, so the body of an
            // interface reached that way must use the signals of the instance it is called
            // on, not those of whichever instance this connection happens to be made to
            எனில் (மாறிலி AstIface* மாறிலி ifacep = VN_CAST(m_cellp->modp(), Iface)) {
                எனில் (ifacep->hasVirtualRef()) திரும்பு பொய்;
            }
            // Forced signals must keep their own storage, the two sides can be forced separately
            எனில் (modVarp->isForced() || கோவை_Varp->isForced()) திரும்பு பொய்;
            // Same for public
            எனில் (modVarp->isSigUserRWPublic() || கோவை_Varp->isSigUserRWPublic()) திரும்பு பொய்;
            // V3Tristate resolved the connected net already, and drives it from the
            // resolution it built for it, so a port merged into it would be driven by
            // the resolution of the instance as well
            எனில் (கோவை_Varp->isTristate()) திரும்பு பொய்;
        }
        // They will become the same variable, so propagate file-line and attributes
        கோவை_Varp->fileline()->modifyStateInherit(modVarp->fileline());
        modVarp->fileline()->modifyStateInherit(கோவை_Varp->fileline());
        கோவை_Varp->propagateAttrFrom(modVarp);
        modVarp->propagateAttrFrom(கோவை_Varp);
        refp->access(VAccess::READWRITE);
        File_வரி* மாறிலி flp = exprp->fileline();
        Ast_முனை_கோவை* மாறிலி itemsp
            = புதிய AstVarXRef{flp, modVarp, m_cellp->name(), VAccess::READWRITE};
        itemsp->addNext(exprp);
        m_cellp->addNextHere(புதிய AstAlias{flp, itemsp});
        திரும்பு உண்மை;
    }

    // VISITORS
    வெறுமை visit(AstCell* nodep) override {
        UINFO(4, "  CELL   " << nodep);
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        // VV*****  We reset user1p() on each cell!!!
        Ast_முனை::user1_Clear_மரம்();
        iterateChildren(nodep);
    }

    வெறுமை visit(AstPin* nodep) override {
        // PIN(p,expr) -> ASSIGNW(VARXREF(p),expr)    (if sub's input)
        //            or  ASSIGNW(expr,VARXREF(p))    (if sub's output)
        UINFO(4, "   PIN  " << nodep);
        எனில் (!nodep->user1()) {
            // Simplify it
            V3Inst::pinReconnectSimple(nodep, m_cellp, பொய்);
        }
        UINFOTREE(9, nodep, "", "Pin_oldb");
        எனில் (!nodep->exprp()) திரும்பு;  // No-connect
        V3Inst::checkOutputShort(nodep);
        எனில் (!nodep->exprp()) திரும்பு;  // Connection removed by checkOutputShort
        // Use user1p on the PIN to indicate we created an assign for this pin
        எனில் (!nodep->user1SetOnce()) {
            // Make an ASSIGNW (expr, pin)
            Ast_முனை_கோவை* மாறிலி exprp = VN_AS(nodep->exprp(), முனை_கோவை)->clone_மரம்(பொய்);
            UASSERT_OBJ(exprp->width() == nodep->modVarp()->width(), nodep,
                        "Width mismatch, should have been handled in pinReconnectSimple");
            எனில் (nodep->modVarp()->isInout()) {
                nodep->v3fatalSrc("Unsupported: Verilator is a 2-state simulator");
            } இல்லையெனில் எனில் (nodep->modVarp()->isWritable()) {
                எனில் (!tryAliasPin(nodep, exprp)) {
                    Ast_முனை_கோவை* மாறிலி rhsp = புதிய AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                             m_cellp->name(), VAccess::READ};
                    markContinuousLhs(exprp);
                    AstAssignW* மாறிலி assp = புதிய AstAssignW{exprp->fileline(), exprp, rhsp};
                    m_cellp->addNextHere(புதிய AstAlways{assp});
                }
            } இல்லையெனில் எனில் (nodep->modVarp()->isNonOutput()) {
                எனில் (!tryAliasPin(nodep, exprp)) {
                    // Don't bother moving constants now,
                    // we'll be pushing the const down to the cell soon enough.
                    AstVarXRef* மாறிலி lhsp = புதிய AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                            m_cellp->name(), VAccess::WRITE};

                    markContinuousLhs(lhsp);
                    AstAssignW* மாறிலி assp = புதிய AstAssignW{exprp->fileline(), lhsp, exprp};
                    m_cellp->addNextHere(புதிய AstAlways{assp});
                    UINFOTREE(9, assp, "", "_new");
                }
            } இல்லையெனில் எனில் (nodep->modVarp()->isIfaceRef()) {
                // Create an AstAliasScope for Vars to Cells so we can
                // link with their scope later
                Ast_முனை_கோவை* மாறிலி lhsp = புதிய AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                         m_cellp->name(), VAccess::READ};
                மாறிலி AstVarRef* மாறிலி refp = VN_CAST(exprp, VarRef);
                மாறிலி AstVarXRef* மாறிலி xrefp = VN_CAST(exprp, VarXRef);
                UASSERT_OBJ(refp || xrefp, exprp,
                            "Interfaces: Pin is not connected to a VarRef or VarXRef");
                m_cellp->addNextHere(புதிய AstAliasScope{exprp->fileline(), lhsp, exprp});
            } இல்லையெனில் {
                nodep->v3error("Assigned pin is neither input nor output");
            }
        }

        // We're done with the pin
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
    }

    // Save some time
    வெறுமை visit(Ast_முனை_கோவை*) override {}
    வெறுமை visit(Ast_முனை_Assign*) override {}
    வெறுமை visit(AstAlways*) override {}

    //--------------------
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    explicit InstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~InstVisitor() override = இயல்பு;
};

//######################################################################
// Inst static function

வகுப்பு InstStatic final {
    InstStatic() = இயல்பு;  // Static class

    நிலையான Ast_முனை_கோவை* extendOrSel(File_வரி* fl, Ast_முனை_கோவை* rhsp, மாறிலி Ast_முனை* cmpWidthp) {
        எனில் (cmpWidthp->width() > rhsp->width()) {
            rhsp = (rhsp->isSigned() ? static_cast<Ast_முனை_கோவை*>(புதிய AstExtendS{fl, rhsp})
                                     : static_cast<Ast_முனை_கோவை*>(புதிய AstExtend{fl, rhsp}));
            // Need proper widthMin, which may differ from AstSel created above
            rhsp->dtypeFrom(cmpWidthp);
        } இல்லையெனில் எனில் (cmpWidthp->width() < rhsp->width()) {
            rhsp = புதிய AstSel{fl, rhsp, 0, cmpWidthp->width()};
            // Need proper widthMin, which may differ from AstSel created above
            rhsp->dtypeFrom(cmpWidthp);
        }
        // else don't change dtype, as might be e.g. array of something
        திரும்பு rhsp;
    }

பொது:
    நிலையான AstAssignW* pinReconnectSimple(AstPin* pinp, AstCell* cellp, தருக்க forTristate,
                                          தருக்க alwaysCvt) {
        // If a pin connection is "simple" leave it as-is
        // Else create a intermediate wire to perform the interconnect
        // Return the new assignment, if one was made
        // Note this module calls cloneTree() via new AstVar
        AstVar* மாறிலி pinVarp = pinp->modVarp();
        எனில் (!pinp->exprp()) {
            // No-connect, perhaps promote based on `unconnected_drive,
            // otherwise done
            எனில் (pinVarp->direction() == VDirection::உள்ளீடு
                && cellp->modp()->unconnectedDrive().isSetTrue()) {
                pinp->exprp(புதிய AstConst{pinp->fileline(), AstConst::All1{}});
            } இல்லையெனில் எனில் (pinVarp->direction() == VDirection::உள்ளீடு
                       && cellp->modp()->unconnectedDrive().isSetFalse()) {
                pinp->exprp(புதிய AstConst{pinp->fileline(), AstConst::All0{}});
            } இல்லையெனில் {
                திரும்பு nullptr;
            }
        }
        மாறிலி AstVarRef* மாறிலி connectRefp = VN_CAST(pinp->exprp(), VarRef);
        மாறிலி AstVarXRef* மாறிலி connectXRefp = VN_CAST(pinp->exprp(), VarXRef);
        மாறிலி Ast_முனை_DType* மாறிலி pinDTypep = pinVarp->dtypep()->skipRefp();
        மாறிலி AstBasicDType* மாறிலி pinBasicp = VN_CAST(pinDTypep, BasicDType);
        மாறிலி Ast_முனை_DType* மாறிலி connDTypep
            = connectRefp ? connectRefp->varp()->dtypep()->skipRefp() : nullptr;
        மாறிலி AstBasicDType* மாறிலி connBasicp = VN_CAST(connDTypep, BasicDType);
        AstAssignW* assignp = nullptr;
        //
        எனில் (!alwaysCvt && connectRefp && connDTypep->same_மரம்(pinDTypep)
            && !connectRefp->varp()->isSc()) {  // Need the signal as a 'shell' to convert types
            // Done. Same data type
        } இல்லையெனில் எனில் (!alwaysCvt && connectRefp && connectRefp->varp()->isIfaceRef()) {
            // Done. Interface
        } இல்லையெனில் எனில் (!alwaysCvt && connectXRefp && connectXRefp->varp()
                   && connectXRefp->varp()->isIfaceRef()) {
        } இல்லையெனில் எனில் (!alwaysCvt && connBasicp && pinBasicp
                   && connBasicp->width() == pinBasicp->width()
                   && connBasicp->lo() == pinBasicp->lo()
                   && !connectRefp->varp()
                           ->isSc()  // Need the signal as a 'shell' to convert types
                   && connBasicp->width() == pinVarp->width()) {
            // Done. One to one interconnect won't need a temporary variable.
        } இல்லையெனில் எனில் (!alwaysCvt && !forTristate && VN_IS(pinp->exprp(), Const)) {
            // Done. Constant. Still check for driving an output, like below.
            V3Inst::checkOutputShort(pinp);
            எனில் (!pinp->exprp()) திரும்பு nullptr;
        } இல்லையெனில் {
            // Make a new temp wire
            // UINFOTREE(9, pinp, "", "in_pin");
            V3Inst::checkOutputShort(pinp);
            எனில் (!pinp->exprp()) திரும்பு nullptr;
            // Simplify, so stuff like '{a[0], b[0]}[1]' produced during
            // instance array expansion are brought to normal 'a[0]'
            Ast_முனை_கோவை* மாறிலி pinexprp
                = V3Const::constifyEdit(VN_AS(pinp->exprp(), முனை_கோவை)->unlinkFrBack());
            மாறிலி string newvarname
                = (string{pinVarp->isWritable() ? "__Vcellout" : "__Vcellinp"}
                   // Prevent name conflict if both tri & non-tri add signals
                   + (forTristate ? "t" : "") + "__" + cellp->name() + "__" + pinp->name());
            AstVar* மாறிலி newvarp
                = புதிய AstVar{pinVarp->fileline(), VVarType::MODULETEMP, newvarname, pinVarp};
            // Important to add statement next to cell, in case there is a
            // generate with same named cell
            cellp->addNextHere(newvarp);
            எனில் (pinVarp->isInout()) {
                pinVarp->v3fatalSrc("Unsupported: Inout connections to pins must be"
                                    " direct one-to-one connection (without any expression)");
                // V3Tristate should have cleared up before this point
            } இல்லையெனில் எனில் (pinVarp->isWritable()) {
                // See also V3Inst
                Ast_முனை_கோவை* rhsp = புதிய AstVarRef{pinp->fileline(), newvarp, VAccess::READ};
                UINFO(5, "pinRecon width " << pinVarp->width() << " >? " << rhsp->width() << " >? "
                                           << pinexprp->width());
                rhsp = extendOrSel(pinp->fileline(), rhsp, pinVarp);
                pinp->exprp(புதிய AstVarRef{newvarp->fileline(), newvarp, VAccess::WRITE});
                markContinuousLhs(pinexprp);
                எனில் (VN_IS(pinexprp, முனை_Stream)) {
                    assignp = புதிய AstAssignW{pinp->fileline(), pinexprp, rhsp};
                    V3Width::streamAssignLowerEdit(assignp);
                } இல்லையெனில் {
                    Ast_முனை_கோவை* rhsSelp = extendOrSel(pinp->fileline(), rhsp, pinexprp);
                    // A reversed array is assigned by the left-to-right order of elements
                    // (IEEE 1800-2023 7.6)
                    மாறிலி AstUnpackArrayDType* மாறிலி arrayp
                        = VN_CAST(pinexprp->dtypep()->skipRefp(), UnpackArrayDType);
                    எனில் (arrayp && arrayp->isReverseOf(rhsSelp->dtypep())) {
                        rhsSelp = புதிய AstCvtUnpackToUnpack{pinp->fileline(), rhsSelp,
                                                           pinexprp->dtypep()};
                    }
                    assignp = புதிய AstAssignW{pinp->fileline(), pinexprp, rhsSelp};
                }
            } இல்லையெனில் {
                // V3 width should have range/extended to make the widths correct
                newvarp->isContinuously(உண்மை);
                // An input port driven from inside the cell by a clocking block
                // output flows out through the connection; V3Tristate split
                // pins resolve separately.
                Ast_முனை_கோவை* lhsp = nullptr;  // LHS of the interconnect assign
                Ast_முனை_கோவை* rhsp = nullptr;  // RHS of the interconnect assign
                எனில் (forTristate || !pinVarp->isClockingDriven()) {
                    // Input-side wiring: temp fed from the connection
                    lhsp = புதிய AstVarRef{pinp->fileline(), newvarp, VAccess::WRITE};
                    rhsp = pinexprp;
                } இல்லையெனில் எனில் (Ast_முனை_கோவை* மாறிலி drivenp = tryConnectionLhs(pinexprp)) {
                    // Reverse wiring: connection leaves fed from the temp.
                    // The original connection is consumed by the clone above.
                    VL_DO_DANGLING(pinexprp->delete_மரம்(), pinexprp);
                    markContinuousLhs(drivenp);
                    lhsp = drivenp;
                    rhsp = extendOrSel(pinp->fileline(),
                                       புதிய AstVarRef{pinp->fileline(), newvarp, VAccess::READ},
                                       drivenp);
                } இல்லையெனில் {
                    pinp->v3error("Unsupported: Clocking block output driving input port "
                                  "through non-decomposable connection "
                                  << pinp->prettyNameQ());
                    VL_DO_DANGLING(pinexprp->delete_மரம்(), pinexprp);
                    திரும்பு nullptr;
                }
                pinp->exprp(புதிய AstVarRef{pinp->fileline(), newvarp, VAccess::READ});
                assignp = புதிய AstAssignW{pinp->fileline(), lhsp, rhsp};
            }
            எனில் (assignp) cellp->addNextHere(புதிய AstAlways{assignp});
            // UINFOTREE(1, pinp, "", "out");
            // UINFOTREE(1, assignp, "", "aout");
        }
        திரும்பு assignp;
    }
};

//######################################################################
// Inst class functions

AstAssignW* V3Inst::pinReconnectSimple(AstPin* pinp, AstCell* cellp, தருக்க forTristate,
                                       தருக்க alwaysCvt) {
    திரும்பு InstStatic::pinReconnectSimple(pinp, cellp, forTristate, alwaysCvt);
}

வெறுமை V3Inst::checkOutputShort(மாறிலி AstPin* nodep) {
    எனில் (nodep->modVarp()->direction() == VDirection::OUTPUT) {
        எனில் (VN_IS(nodep->exprp(), Const) || VN_IS(nodep->exprp(), Extend)
            || (VN_IS(nodep->exprp(), Concat)
                && (VN_IS(VN_AS(nodep->exprp(), Concat)->lhsp(), Const)))) {
            // Uses v3warn for error, as might be found multiple times
            nodep->v3warn(E_PORTSHORT, "Output port is connected to a constant pin,"
                                       " electrical short");
            // Delete so we don't create a 'CONST = ...' assignment
            nodep->exprp()->unlinkFrBack()->delete_மரம்();
        }
    }
}

//######################################################################
// Inst class visitor

வெறுமை V3Inst::instAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { InstVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("inst", 0, dump_மரம்_Either_Level() >= 3);
}
