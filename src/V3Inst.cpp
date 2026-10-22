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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Inst.h"

#شامل "V3Const.h"
#شامل "V3Control.h"
#شامل "V3Width.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ایستا تهی markContinuousLhs(Ast_گره* ثابت nodep) {
    nodep->foreach([](Ast_گره_Var_Ref* refp) {
        اگر (refp->access().isWriteOrRW()) refp->varp()->isContinuously(درست);
    });
}

// Flip the access of the driven leaves of a pin connection expression so it
// can be used on the LHS of an assignment. Only variables and bit/part
// selects over variables are flipped; index and select operands stay
// read-only. Struct-member selects cannot appear here: V3Width lowers them to
// selects before V3Inst. Returns false if the connection cannot be driven
// (e.g. arithmetic), in which case the expression is left unmodified.
ایستا منطقی markConnectionLhs(Ast_گره_عبارت* ثابت nodep) {
    اگر (Ast_گره_Var_Ref* ثابت refp = VN_CAST(nodep, گره_Var_Ref)) {
        اگر (!refp->access().isReadOnly()) بازگشت نادرست;
        refp->access(VAccess::WRITE);
        بازگشت درست;
    }
    // Note AstSel (bit/part select) is not an AstNodeSel (array select);
    // both expose fromp() for the selected value.
    اگر (AstSel* ثابت selp = VN_CAST(nodep, Sel)) بازگشت markConnectionLhs(selp->fromp());
    اگر (Ast_گره_Sel* ثابت selp = VN_CAST(nodep, گره_Sel)) {
        بازگشت markConnectionLhs(selp->fromp());
    }
    اگر (AstConcat* ثابت concatp = VN_CAST(nodep, Concat)) {
        بازگشت markConnectionLhs(concatp->lhsp()) && markConnectionLhs(concatp->rhsp());
    }
    بازگشت نادرست;
}

// Prepare a pin connection expression as the LHS of the reverse (inside-out)
// assignment. Returns the flipped expression on success, nullptr if the
// connection cannot be driven. The input is left unmodified on failure.
ایستا Ast_گره_عبارت* tryConnectionLhs(Ast_گره_عبارت* ثابت nodep) {
    Ast_گره_عبارت* ثابت clonep = nodep->clone_درخت(نادرست);
    اگر (markConnectionLhs(clonep)) بازگشت clonep;
    VL_DO_DANGLING(clonep->delete_درخت(), clonep);
    بازگشت nullptr;
}

//######################################################################
// Inst state, as a visitor of each AstNode

رده InstVisitor final : عمومی VNVisitor {
    // NODE STATE
    // Cleared each Cell:
    //  AstPin::user1p()        -> bool.  True if created assignment already
    ثابت VNUser1InUse m_inuser1;

    // STATE
    AstCell* m_cellp = nullptr;  // Current cell

    // METHODS
    // If appropriate, add an AstAlias to connect the given Cell pin to the given expression.
    // Returns true if an alias was made, in which case there is nothing else to do for this pin.
    منطقی tryAliasPin(AstPin* nodep, Ast_گره_عبارت* exprp) {
        AstVar* ثابت modVarp = nodep->modVarp();
        // An interface reference is aliased via AstAliasScope below, not as a variable
        اگر (modVarp->isIfaceRef()) بازگشت نادرست;
        // Only a whole variable can be aliased, anything else needs the assignment
        AstVarRef* ثابت refp = VN_CAST(exprp, VarRef);
        اگر (!refp) بازگشت نادرست;
        AstVar* ثابت عبارت_Varp = refp->varp();
        // A virtual interface port connected to a real interface can't be an alias
        اگر (عبارت_Varp->isIfaceRef()) بازگشت نادرست;
        // A ref port must become an alias
        اگر (!modVarp->direction().isRef()) {
            // V3FsmDetect recognizes the registers of an fsm_register_wrapper instance via
            // the assignments of its pins, so leave those as assignments
            Ast_گره_Module* ثابت cellModp = m_cellp->modp();
            اگر (V3Control::getFsmRegisterWrapper(cellModp->origName())
                || V3Control::getFsmRegisterWrapper(cellModp->prettyDehashOrigOrName())) {
                بازگشت نادرست;
            }
            // A virtual interface method call is dispatched at run time, so the body of an
            // interface reached that way must use the signals of the instance it is called
            // on, not those of whichever instance this connection happens to be made to
            اگر (ثابت AstIface* ثابت ifacep = VN_CAST(m_cellp->modp(), Iface)) {
                اگر (ifacep->hasVirtualRef()) بازگشت نادرست;
            }
            // Forced signals must keep their own storage, the two sides can be forced separately
            اگر (modVarp->isForced() || عبارت_Varp->isForced()) بازگشت نادرست;
            // Same for public
            اگر (modVarp->isSigUserRWPublic() || عبارت_Varp->isSigUserRWPublic()) بازگشت نادرست;
            // V3Tristate resolved the connected net already, and drives it from the
            // resolution it built for it, so a port merged into it would be driven by
            // the resolution of the instance as well
            اگر (عبارت_Varp->isTristate()) بازگشت نادرست;
        }
        // They will become the same variable, so propagate file-line and attributes
        عبارت_Varp->fileline()->modifyStateInherit(modVarp->fileline());
        modVarp->fileline()->modifyStateInherit(عبارت_Varp->fileline());
        عبارت_Varp->propagateAttrFrom(modVarp);
        modVarp->propagateAttrFrom(عبارت_Varp);
        refp->access(VAccess::READWRITE);
        File_سطر* ثابت flp = exprp->fileline();
        Ast_گره_عبارت* ثابت itemsp
            = جدید AstVarXRef{flp, modVarp, m_cellp->name(), VAccess::READWRITE};
        itemsp->addNext(exprp);
        m_cellp->addNextHere(جدید AstAlias{flp, itemsp});
        بازگشت درست;
    }

    // VISITORS
    تهی visit(AstCell* nodep) override {
        UINFO(4, "  CELL   " << nodep);
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        // VV*****  We reset user1p() on each cell!!!
        Ast_گره::user1_Clear_درخت();
        iterateChildren(nodep);
    }

    تهی visit(AstPin* nodep) override {
        // PIN(p,expr) -> ASSIGNW(VARXREF(p),expr)    (if sub's input)
        //            or  ASSIGNW(expr,VARXREF(p))    (if sub's output)
        UINFO(4, "   PIN  " << nodep);
        اگر (!nodep->user1()) {
            // Simplify it
            V3Inst::pinReconnectSimple(nodep, m_cellp, نادرست);
        }
        UINFOTREE(9, nodep, "", "Pin_oldb");
        اگر (!nodep->exprp()) بازگشت;  // No-connect
        V3Inst::checkOutputShort(nodep);
        اگر (!nodep->exprp()) بازگشت;  // Connection removed by checkOutputShort
        // Use user1p on the PIN to indicate we created an assign for this pin
        اگر (!nodep->user1SetOnce()) {
            // Make an ASSIGNW (expr, pin)
            Ast_گره_عبارت* ثابت exprp = VN_AS(nodep->exprp(), گره_عبارت)->clone_درخت(نادرست);
            UASSERT_OBJ(exprp->width() == nodep->modVarp()->width(), nodep,
                        "Width mismatch, should have been handled in pinReconnectSimple");
            اگر (nodep->modVarp()->isInout()) {
                nodep->v3fatalSrc("Unsupported: Verilator is a 2-state simulator");
            } وگرنه اگر (nodep->modVarp()->isWritable()) {
                اگر (!tryAliasPin(nodep, exprp)) {
                    Ast_گره_عبارت* ثابت rhsp = جدید AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                             m_cellp->name(), VAccess::READ};
                    markContinuousLhs(exprp);
                    AstAssignW* ثابت assp = جدید AstAssignW{exprp->fileline(), exprp, rhsp};
                    m_cellp->addNextHere(جدید AstAlways{assp});
                }
            } وگرنه اگر (nodep->modVarp()->isNonOutput()) {
                اگر (!tryAliasPin(nodep, exprp)) {
                    // Don't bother moving constants now,
                    // we'll be pushing the const down to the cell soon enough.
                    AstVarXRef* ثابت lhsp = جدید AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                            m_cellp->name(), VAccess::WRITE};

                    markContinuousLhs(lhsp);
                    AstAssignW* ثابت assp = جدید AstAssignW{exprp->fileline(), lhsp, exprp};
                    m_cellp->addNextHere(جدید AstAlways{assp});
                    UINFOTREE(9, assp, "", "_new");
                }
            } وگرنه اگر (nodep->modVarp()->isIfaceRef()) {
                // Create an AstAliasScope for Vars to Cells so we can
                // link with their scope later
                Ast_گره_عبارت* ثابت lhsp = جدید AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                         m_cellp->name(), VAccess::READ};
                ثابت AstVarRef* ثابت refp = VN_CAST(exprp, VarRef);
                ثابت AstVarXRef* ثابت xrefp = VN_CAST(exprp, VarXRef);
                UASSERT_OBJ(refp || xrefp, exprp,
                            "Interfaces: Pin is not connected to a VarRef or VarXRef");
                m_cellp->addNextHere(جدید AstAliasScope{exprp->fileline(), lhsp, exprp});
            } وگرنه {
                nodep->v3error("Assigned pin is neither input nor output");
            }
        }

        // We're done with the pin
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
    }

    // Save some time
    تهی visit(Ast_گره_عبارت*) override {}
    تهی visit(Ast_گره_Assign*) override {}
    تهی visit(AstAlways*) override {}

    //--------------------
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit InstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~InstVisitor() override = پیشفرض;
};

//######################################################################
// Inst static function

رده InstStatic final {
    InstStatic() = پیشفرض;  // Static class

    ایستا Ast_گره_عبارت* extendOrSel(File_سطر* fl, Ast_گره_عبارت* rhsp, ثابت Ast_گره* cmpWidthp) {
        اگر (cmpWidthp->width() > rhsp->width()) {
            rhsp = (rhsp->isSigned() ? static_cast<Ast_گره_عبارت*>(جدید AstExtendS{fl, rhsp})
                                     : static_cast<Ast_گره_عبارت*>(جدید AstExtend{fl, rhsp}));
            // Need proper widthMin, which may differ from AstSel created above
            rhsp->dtypeFrom(cmpWidthp);
        } وگرنه اگر (cmpWidthp->width() < rhsp->width()) {
            rhsp = جدید AstSel{fl, rhsp, 0, cmpWidthp->width()};
            // Need proper widthMin, which may differ from AstSel created above
            rhsp->dtypeFrom(cmpWidthp);
        }
        // else don't change dtype, as might be e.g. array of something
        بازگشت rhsp;
    }

عمومی:
    ایستا AstAssignW* pinReconnectSimple(AstPin* pinp, AstCell* cellp, منطقی forTristate,
                                          منطقی alwaysCvt) {
        // If a pin connection is "simple" leave it as-is
        // Else create a intermediate wire to perform the interconnect
        // Return the new assignment, if one was made
        // Note this module calls cloneTree() via new AstVar
        AstVar* ثابت pinVarp = pinp->modVarp();
        اگر (!pinp->exprp()) {
            // No-connect, perhaps promote based on `unconnected_drive,
            // otherwise done
            اگر (pinVarp->direction() == VDirection::درون_داد
                && cellp->modp()->unconnectedDrive().isSetTrue()) {
                pinp->exprp(جدید AstConst{pinp->fileline(), AstConst::All1{}});
            } وگرنه اگر (pinVarp->direction() == VDirection::درون_داد
                       && cellp->modp()->unconnectedDrive().isSetFalse()) {
                pinp->exprp(جدید AstConst{pinp->fileline(), AstConst::All0{}});
            } وگرنه {
                بازگشت nullptr;
            }
        }
        ثابت AstVarRef* ثابت connectRefp = VN_CAST(pinp->exprp(), VarRef);
        ثابت AstVarXRef* ثابت connectXRefp = VN_CAST(pinp->exprp(), VarXRef);
        ثابت Ast_گره_DType* ثابت pinDTypep = pinVarp->dtypep()->skipRefp();
        ثابت AstBasicDType* ثابت pinBasicp = VN_CAST(pinDTypep, BasicDType);
        ثابت Ast_گره_DType* ثابت connDTypep
            = connectRefp ? connectRefp->varp()->dtypep()->skipRefp() : nullptr;
        ثابت AstBasicDType* ثابت connBasicp = VN_CAST(connDTypep, BasicDType);
        AstAssignW* assignp = nullptr;
        //
        اگر (!alwaysCvt && connectRefp && connDTypep->same_درخت(pinDTypep)
            && !connectRefp->varp()->isSc()) {  // Need the signal as a 'shell' to convert types
            // Done. Same data type
        } وگرنه اگر (!alwaysCvt && connectRefp && connectRefp->varp()->isIfaceRef()) {
            // Done. Interface
        } وگرنه اگر (!alwaysCvt && connectXRefp && connectXRefp->varp()
                   && connectXRefp->varp()->isIfaceRef()) {
        } وگرنه اگر (!alwaysCvt && connBasicp && pinBasicp
                   && connBasicp->width() == pinBasicp->width()
                   && connBasicp->lo() == pinBasicp->lo()
                   && !connectRefp->varp()
                           ->isSc()  // Need the signal as a 'shell' to convert types
                   && connBasicp->width() == pinVarp->width()) {
            // Done. One to one interconnect won't need a temporary variable.
        } وگرنه اگر (!alwaysCvt && !forTristate && VN_IS(pinp->exprp(), Const)) {
            // Done. Constant. Still check for driving an output, like below.
            V3Inst::checkOutputShort(pinp);
            اگر (!pinp->exprp()) بازگشت nullptr;
        } وگرنه {
            // Make a new temp wire
            // UINFOTREE(9, pinp, "", "in_pin");
            V3Inst::checkOutputShort(pinp);
            اگر (!pinp->exprp()) بازگشت nullptr;
            // Simplify, so stuff like '{a[0], b[0]}[1]' produced during
            // instance array expansion are brought to normal 'a[0]'
            Ast_گره_عبارت* ثابت pinexprp
                = V3Const::constifyEdit(VN_AS(pinp->exprp(), گره_عبارت)->unlinkFrBack());
            ثابت string newvarname
                = (string{pinVarp->isWritable() ? "__Vcellout" : "__Vcellinp"}
                   // Prevent name conflict if both tri & non-tri add signals
                   + (forTristate ? "t" : "") + "__" + cellp->name() + "__" + pinp->name());
            AstVar* ثابت newvarp
                = جدید AstVar{pinVarp->fileline(), VVarType::MODULETEMP, newvarname, pinVarp};
            // Important to add statement next to cell, in case there is a
            // generate with same named cell
            cellp->addNextHere(newvarp);
            اگر (pinVarp->isInout()) {
                pinVarp->v3fatalSrc("Unsupported: Inout connections to pins must be"
                                    " direct one-to-one connection (without any expression)");
                // V3Tristate should have cleared up before this point
            } وگرنه اگر (pinVarp->isWritable()) {
                // See also V3Inst
                Ast_گره_عبارت* rhsp = جدید AstVarRef{pinp->fileline(), newvarp, VAccess::READ};
                UINFO(5, "pinRecon width " << pinVarp->width() << " >? " << rhsp->width() << " >? "
                                           << pinexprp->width());
                rhsp = extendOrSel(pinp->fileline(), rhsp, pinVarp);
                pinp->exprp(جدید AstVarRef{newvarp->fileline(), newvarp, VAccess::WRITE});
                markContinuousLhs(pinexprp);
                اگر (VN_IS(pinexprp, گره_Stream)) {
                    assignp = جدید AstAssignW{pinp->fileline(), pinexprp, rhsp};
                    V3Width::streamAssignLowerEdit(assignp);
                } وگرنه {
                    Ast_گره_عبارت* rhsSelp = extendOrSel(pinp->fileline(), rhsp, pinexprp);
                    // A reversed array is assigned by the left-to-right order of elements
                    // (IEEE 1800-2023 7.6)
                    ثابت AstUnpackArrayDType* ثابت arrayp
                        = VN_CAST(pinexprp->dtypep()->skipRefp(), UnpackArrayDType);
                    اگر (arrayp && arrayp->isReverseOf(rhsSelp->dtypep())) {
                        rhsSelp = جدید AstCvtUnpackToUnpack{pinp->fileline(), rhsSelp,
                                                           pinexprp->dtypep()};
                    }
                    assignp = جدید AstAssignW{pinp->fileline(), pinexprp, rhsSelp};
                }
            } وگرنه {
                // V3 width should have range/extended to make the widths correct
                newvarp->isContinuously(درست);
                // An input port driven from inside the cell by a clocking block
                // output flows out through the connection; V3Tristate split
                // pins resolve separately.
                Ast_گره_عبارت* lhsp = nullptr;  // LHS of the interconnect assign
                Ast_گره_عبارت* rhsp = nullptr;  // RHS of the interconnect assign
                اگر (forTristate || !pinVarp->isClockingDriven()) {
                    // Input-side wiring: temp fed from the connection
                    lhsp = جدید AstVarRef{pinp->fileline(), newvarp, VAccess::WRITE};
                    rhsp = pinexprp;
                } وگرنه اگر (Ast_گره_عبارت* ثابت drivenp = tryConnectionLhs(pinexprp)) {
                    // Reverse wiring: connection leaves fed from the temp.
                    // The original connection is consumed by the clone above.
                    VL_DO_DANGLING(pinexprp->delete_درخت(), pinexprp);
                    markContinuousLhs(drivenp);
                    lhsp = drivenp;
                    rhsp = extendOrSel(pinp->fileline(),
                                       جدید AstVarRef{pinp->fileline(), newvarp, VAccess::READ},
                                       drivenp);
                } وگرنه {
                    pinp->v3error("Unsupported: Clocking block output driving input port "
                                  "through non-decomposable connection "
                                  << pinp->prettyNameQ());
                    VL_DO_DANGLING(pinexprp->delete_درخت(), pinexprp);
                    بازگشت nullptr;
                }
                pinp->exprp(جدید AstVarRef{pinp->fileline(), newvarp, VAccess::READ});
                assignp = جدید AstAssignW{pinp->fileline(), lhsp, rhsp};
            }
            اگر (assignp) cellp->addNextHere(جدید AstAlways{assignp});
            // UINFOTREE(1, pinp, "", "out");
            // UINFOTREE(1, assignp, "", "aout");
        }
        بازگشت assignp;
    }
};

//######################################################################
// Inst class functions

AstAssignW* V3Inst::pinReconnectSimple(AstPin* pinp, AstCell* cellp, منطقی forTristate,
                                       منطقی alwaysCvt) {
    بازگشت InstStatic::pinReconnectSimple(pinp, cellp, forTristate, alwaysCvt);
}

تهی V3Inst::checkOutputShort(ثابت AstPin* nodep) {
    اگر (nodep->modVarp()->direction() == VDirection::OUTPUT) {
        اگر (VN_IS(nodep->exprp(), Const) || VN_IS(nodep->exprp(), Extend)
            || (VN_IS(nodep->exprp(), Concat)
                && (VN_IS(VN_AS(nodep->exprp(), Concat)->lhsp(), Const)))) {
            // Uses v3warn for error, as might be found multiple times
            nodep->v3warn(E_PORTSHORT, "Output port is connected to a constant pin,"
                                       " electrical short");
            // Delete so we don't create a 'CONST = ...' assignment
            nodep->exprp()->unlinkFrBack()->delete_درخت();
        }
    }
}

//######################################################################
// Inst class visitor

تهی V3Inst::instAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { InstVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("inst", 0, dump_درخت_Either_Level() >= 3);
}
