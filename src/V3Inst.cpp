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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Inst.h"

#ضمن "V3Const.h"
#ضمن "V3Control.h"
#ضمن "V3Width.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ساكن فراغ markContinuousLhs(Ast_ڳنڍ* ثابت nodep) {
    nodep->foreach([](Ast_ڳنڍ_Var_Ref* refp) {
        إذا (refp->access().isWriteOrRW()) refp->varp()->isContinuously(سچ);
    });
}

// Flip the access of the driven leaves of a pin connection expression so it
// can be used on the LHS of an assignment. Only variables and bit/part
// selects over variables are flipped; index and select operands stay
// read-only. Struct-member selects cannot appear here: V3Width lowers them to
// selects before V3Inst. Returns false if the connection cannot be driven
// (e.g. arithmetic), in which case the expression is left unmodified.
ساكن منطقي markConnectionLhs(Ast_ڳنڍ_اظهار* ثابت nodep) {
    إذا (Ast_ڳنڍ_Var_Ref* ثابت refp = VN_CAST(nodep, ڳنڍ_Var_Ref)) {
        إذا (!refp->access().isReadOnly()) أرجع خطأ;
        refp->access(VAccess::WRITE);
        أرجع سچ;
    }
    // Note AstSel (bit/part select) is not an AstNodeSel (array select);
    // both expose fromp() for the selected value.
    إذا (AstSel* ثابت selp = VN_CAST(nodep, Sel)) أرجع markConnectionLhs(selp->fromp());
    إذا (Ast_ڳنڍ_Sel* ثابت selp = VN_CAST(nodep, ڳنڍ_Sel)) {
        أرجع markConnectionLhs(selp->fromp());
    }
    إذا (AstConcat* ثابت concatp = VN_CAST(nodep, Concat)) {
        أرجع markConnectionLhs(concatp->lhsp()) && markConnectionLhs(concatp->rhsp());
    }
    أرجع خطأ;
}

// Prepare a pin connection expression as the LHS of the reverse (inside-out)
// assignment. Returns the flipped expression on success, nullptr if the
// connection cannot be driven. The input is left unmodified on failure.
ساكن Ast_ڳنڍ_اظهار* tryConnectionLhs(Ast_ڳنڍ_اظهار* ثابت nodep) {
    Ast_ڳنڍ_اظهار* ثابت clonep = nodep->clone_وڻ(خطأ);
    إذا (markConnectionLhs(clonep)) أرجع clonep;
    VL_DO_DANGLING(clonep->delete_وڻ(), clonep);
    أرجع خالي_اشارو;
}

//######################################################################
// Inst state, as a visitor of each AstNode

صنف InstVisitor final : عام VNVisitor {
    // NODE STATE
    // Cleared each Cell:
    //  AstPin::user1p()        -> bool.  True if created assignment already
    ثابت VNUser1_اندريون_Use m_inuser1;

    // STATE
    AstCell* m_cellp = خالي_اشارو;  // Current cell

    // METHODS
    // If appropriate, add an AstAlias to connect the given Cell pin to the given expression.
    // Returns true if an alias was made, in which case there is nothing else to do for this pin.
    منطقي tryAliasPin(AstPin* nodep, Ast_ڳنڍ_اظهار* exprp) {
        AstVar* ثابت modVarp = nodep->modVarp();
        // An interface reference is aliased via AstAliasScope below, not as a variable
        إذا (modVarp->isIfaceRef()) أرجع خطأ;
        // Only a whole variable can be aliased, anything else needs the assignment
        AstVarRef* ثابت refp = VN_CAST(exprp, VarRef);
        إذا (!refp) أرجع خطأ;
        AstVar* ثابت اظهار_Varp = refp->varp();
        // A virtual interface port connected to a real interface can't be an alias
        إذا (اظهار_Varp->isIfaceRef()) أرجع خطأ;
        // A ref port must become an alias
        إذا (!modVarp->direction().isRef()) {
            // V3FsmDetect recognizes the registers of an fsm_register_wrapper instance via
            // the assignments of its pins, so leave those as assignments
            Ast_ڳنڍ_Module* ثابت cellModp = m_cellp->modp();
            إذا (V3Control::get_Fsm_رجسٽر_Wrapper(cellModp->origName())
                || V3Control::get_Fsm_رجسٽر_Wrapper(cellModp->prettyDehashOrigOrName())) {
                أرجع خطأ;
            }
            // A virtual interface method call is dispatched at run time, so the body of an
            // interface reached that way must use the signals of the instance it is called
            // on, not those of whichever instance this connection happens to be made to
            إذا (ثابت AstIface* ثابت ifacep = VN_CAST(m_cellp->modp(), Iface)) {
                إذا (ifacep->hasVirtualRef()) أرجع خطأ;
            }
            // Forced signals must keep their own storage, the two sides can be forced separately
            إذا (modVarp->isForced() || اظهار_Varp->isForced()) أرجع خطأ;
            // Same for public
            إذا (modVarp->isSigUserRWPublic() || اظهار_Varp->isSigUserRWPublic()) أرجع خطأ;
            // V3Tristate resolved the connected net already, and drives it from the
            // resolution it built for it, so a port merged into it would be driven by
            // the resolution of the instance as well
            إذا (اظهار_Varp->isTristate()) أرجع خطأ;
        }
        // They will become the same variable, so propagate file-line and attributes
        اظهار_Varp->fileline()->modifyStateInherit(modVarp->fileline());
        modVarp->fileline()->modifyStateInherit(اظهار_Varp->fileline());
        اظهار_Varp->propagateAttrFrom(modVarp);
        modVarp->propagateAttrFrom(اظهار_Varp);
        refp->access(VAccess::READWRITE);
        File_سٽ* ثابت flp = exprp->fileline();
        Ast_ڳنڍ_اظهار* ثابت itemsp
            = جديد AstVarXRef{flp, modVarp, m_cellp->name(), VAccess::READWRITE};
        itemsp->جوڙيو_ايندڙ(exprp);
        m_cellp->جوڙيو_ايندڙ_هتي(جديد AstAlias{flp, itemsp});
        أرجع سچ;
    }

    // VISITORS
    فراغ گهمو(AstCell* nodep) override {
        UINFO(4, "  CELL   " << nodep);
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        // VV*****  We reset user1p() on each cell!!!
        Ast_ڳنڍ::user1_Clear_وڻ();
        iterateChildren(nodep);
    }

    فراغ گهمو(AstPin* nodep) override {
        // PIN(p,expr) -> ASSIGNW(VARXREF(p),expr)    (if sub's input)
        //            or  ASSIGNW(expr,VARXREF(p))    (if sub's output)
        UINFO(4, "   PIN  " << nodep);
        إذا (!nodep->user1()) {
            // Simplify it
            V3Inst::pinReconnectSimple(nodep, m_cellp, خطأ);
        }
        UINFOTREE(9, nodep, "", "Pin_oldb");
        إذا (!nodep->exprp()) أرجع;  // No-connect
        V3Inst::checkOutputShort(nodep);
        إذا (!nodep->exprp()) أرجع;  // Connection removed by checkOutputShort
        // Use user1p on the PIN to indicate we created an assign for this pin
        إذا (!nodep->user1_سيٽ_Once()) {
            // Make an ASSIGNW (expr, pin)
            Ast_ڳنڍ_اظهار* ثابت exprp = VN_AS(nodep->exprp(), ڳنڍ_اظهار)->clone_وڻ(خطأ);
            UASSERT_OBJ(exprp->ويڪر() == nodep->modVarp()->ويڪر(), nodep,
                        "Width mismatch, should have been handled in pinReconnectSimple");
            إذا (nodep->modVarp()->isInout()) {
                nodep->v3fatalSrc("Unsupported: Verilator is a 2-state simulator");
            } وإلا إذا (nodep->modVarp()->isWritable()) {
                إذا (!tryAliasPin(nodep, exprp)) {
                    Ast_ڳنڍ_اظهار* ثابت rhsp = جديد AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                             m_cellp->name(), VAccess::READ};
                    markContinuousLhs(exprp);
                    Ast_سونپيو_W* ثابت assp = جديد Ast_سونپيو_W{exprp->fileline(), exprp, rhsp};
                    m_cellp->جوڙيو_ايندڙ_هتي(جديد AstAlways{assp});
                }
            } وإلا إذا (nodep->modVarp()->isNonOutput()) {
                إذا (!tryAliasPin(nodep, exprp)) {
                    // Don't bother moving constants now,
                    // we'll be pushing the const down to the cell soon enough.
                    AstVarXRef* ثابت lhsp = جديد AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                            m_cellp->name(), VAccess::WRITE};

                    markContinuousLhs(lhsp);
                    Ast_سونپيو_W* ثابت assp = جديد Ast_سونپيو_W{exprp->fileline(), lhsp, exprp};
                    m_cellp->جوڙيو_ايندڙ_هتي(جديد AstAlways{assp});
                    UINFOTREE(9, assp, "", "_new");
                }
            } وإلا إذا (nodep->modVarp()->isIfaceRef()) {
                // Create an AstAliasScope for Vars to Cells so we can
                // link with their scope later
                Ast_ڳنڍ_اظهار* ثابت lhsp = جديد AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                         m_cellp->name(), VAccess::READ};
                ثابت AstVarRef* ثابت refp = VN_CAST(exprp, VarRef);
                ثابت AstVarXRef* ثابت xrefp = VN_CAST(exprp, VarXRef);
                UASSERT_OBJ(refp || xrefp, exprp,
                            "Interfaces: Pin is not connected to a VarRef or VarXRef");
                m_cellp->جوڙيو_ايندڙ_هتي(جديد AstAliasScope{exprp->fileline(), lhsp, exprp});
            } وإلا {
                nodep->v3error("Assigned pin is neither input nor output");
            }
        }

        // We're done with the pin
        VL_DO_DANGLING(nodep->unlink_Fr_پٺ()->delete_وڻ(), nodep);
    }

    // Save some time
    فراغ گهمو(Ast_ڳنڍ_اظهار*) override {}
    فراغ گهمو(Ast_ڳنڍ_سونپيو*) override {}
    فراغ گهمو(AstAlways*) override {}

    //--------------------
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح InstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~InstVisitor() override = افتراضي;
};

//######################################################################
// Inst static function

صنف InstStatic final {
    InstStatic() = افتراضي;  // Static class

    ساكن Ast_ڳنڍ_اظهار* extendOrSel(File_سٽ* fl, Ast_ڳنڍ_اظهار* rhsp, ثابت Ast_ڳنڍ* cmpWidthp) {
        إذا (cmpWidthp->ويڪر() > rhsp->ويڪر()) {
            rhsp = (rhsp->isSigned() ? static_cast<Ast_ڳنڍ_اظهار*>(جديد AstExtendS{fl, rhsp})
                                     : static_cast<Ast_ڳنڍ_اظهار*>(جديد AstExtend{fl, rhsp}));
            // Need proper widthMin, which may differ from AstSel created above
            rhsp->dtypeFrom(cmpWidthp);
        } وإلا إذا (cmpWidthp->ويڪر() < rhsp->ويڪر()) {
            rhsp = جديد AstSel{fl, rhsp, 0, cmpWidthp->ويڪر()};
            // Need proper widthMin, which may differ from AstSel created above
            rhsp->dtypeFrom(cmpWidthp);
        }
        // else don't change dtype, as might be e.g. array of something
        أرجع rhsp;
    }

عام:
    ساكن Ast_سونپيو_W* pinReconnectSimple(AstPin* pinp, AstCell* cellp, منطقي forTristate,
                                          منطقي alwaysCvt) {
        // If a pin connection is "simple" leave it as-is
        // Else create a intermediate wire to perform the interconnect
        // Return the new assignment, if one was made
        // Note this module calls cloneTree() via new AstVar
        AstVar* ثابت pinVarp = pinp->modVarp();
        إذا (!pinp->exprp()) {
            // No-connect, perhaps promote based on `unconnected_drive,
            // otherwise done
            إذا (pinVarp->direction() == VDirection::داخلا
                && cellp->modp()->unconnectedDrive().is_سيٽ_True()) {
                pinp->exprp(جديد AstConst{pinp->fileline(), AstConst::All1{}});
            } وإلا إذا (pinVarp->direction() == VDirection::داخلا
                       && cellp->modp()->unconnectedDrive().is_سيٽ_False()) {
                pinp->exprp(جديد AstConst{pinp->fileline(), AstConst::All0{}});
            } وإلا {
                أرجع خالي_اشارو;
            }
        }
        ثابت AstVarRef* ثابت connectRefp = VN_CAST(pinp->exprp(), VarRef);
        ثابت AstVarXRef* ثابت connectXRefp = VN_CAST(pinp->exprp(), VarXRef);
        ثابت Ast_ڳنڍ_DType* ثابت pinDTypep = pinVarp->dtypep()->skipRefp();
        ثابت AstBasicDType* ثابت pinBasicp = VN_CAST(pinDTypep, BasicDType);
        ثابت Ast_ڳنڍ_DType* ثابت connDTypep
            = connectRefp ? connectRefp->varp()->dtypep()->skipRefp() : خالي_اشارو;
        ثابت AstBasicDType* ثابت connBasicp = VN_CAST(connDTypep, BasicDType);
        Ast_سونپيو_W* assignp = خالي_اشارو;
        //
        إذا (!alwaysCvt && connectRefp && connDTypep->same_وڻ(pinDTypep)
            && !connectRefp->varp()->isSc()) {  // Need the signal as a 'shell' to convert types
            // Done. Same data type
        } وإلا إذا (!alwaysCvt && connectRefp && connectRefp->varp()->isIfaceRef()) {
            // Done. Interface
        } وإلا إذا (!alwaysCvt && connectXRefp && connectXRefp->varp()
                   && connectXRefp->varp()->isIfaceRef()) {
        } وإلا إذا (!alwaysCvt && connBasicp && pinBasicp
                   && connBasicp->ويڪر() == pinBasicp->ويڪر()
                   && connBasicp->lo() == pinBasicp->lo()
                   && !connectRefp->varp()
                           ->isSc()  // Need the signal as a 'shell' to convert types
                   && connBasicp->ويڪر() == pinVarp->ويڪر()) {
            // Done. One to one interconnect won't need a temporary variable.
        } وإلا إذا (!alwaysCvt && !forTristate && VN_IS(pinp->exprp(), Const)) {
            // Done. Constant. Still check for driving an output, like below.
            V3Inst::checkOutputShort(pinp);
            إذا (!pinp->exprp()) أرجع خالي_اشارو;
        } وإلا {
            // Make a new temp wire
            // UINFOTREE(9, pinp, "", "in_pin");
            V3Inst::checkOutputShort(pinp);
            إذا (!pinp->exprp()) أرجع خالي_اشارو;
            // Simplify, so stuff like '{a[0], b[0]}[1]' produced during
            // instance array expansion are brought to normal 'a[0]'
            Ast_ڳنڍ_اظهار* ثابت pinexprp
                = V3Const::constifyEdit(VN_AS(pinp->exprp(), ڳنڍ_اظهار)->unlink_Fr_پٺ());
            ثابت string newvarname
                = (string{pinVarp->isWritable() ? "__Vcellout" : "__Vcellinp"}
                   // Prevent name conflict if both tri & non-tri add signals
                   + (forTristate ? "t" : "") + "__" + cellp->name() + "__" + pinp->name());
            AstVar* ثابت newvarp
                = جديد AstVar{pinVarp->fileline(), VVarType::MODULETEMP, newvarname, pinVarp};
            // Important to add statement next to cell, in case there is a
            // generate with same named cell
            cellp->جوڙيو_ايندڙ_هتي(newvarp);
            إذا (pinVarp->isInout()) {
                pinVarp->v3fatalSrc("Unsupported: Inout connections to pins must be"
                                    " direct one-to-one connection (without any expression)");
                // V3Tristate should have cleared up before this point
            } وإلا إذا (pinVarp->isWritable()) {
                // See also V3Inst
                Ast_ڳنڍ_اظهار* rhsp = جديد AstVarRef{pinp->fileline(), newvarp, VAccess::READ};
                UINFO(5, "pinRecon width " << pinVarp->ويڪر() << " >? " << rhsp->ويڪر() << " >? "
                                           << pinexprp->ويڪر());
                rhsp = extendOrSel(pinp->fileline(), rhsp, pinVarp);
                pinp->exprp(جديد AstVarRef{newvarp->fileline(), newvarp, VAccess::WRITE});
                markContinuousLhs(pinexprp);
                إذا (VN_IS(pinexprp, ڳنڍ_وهڪرو)) {
                    assignp = جديد Ast_سونپيو_W{pinp->fileline(), pinexprp, rhsp};
                    V3_ويڪر::وهڪرو_سونپيو_هيٺاهون_Edit(assignp);
                } وإلا {
                    Ast_ڳنڍ_اظهار* rhsSelp = extendOrSel(pinp->fileline(), rhsp, pinexprp);
                    // A reversed array is assigned by the left-to-right order of elements
                    // (IEEE 1800-2023 7.6)
                    ثابت AstUnpackArrayDType* ثابت arrayp
                        = VN_CAST(pinexprp->dtypep()->skipRefp(), UnpackArrayDType);
                    إذا (arrayp && arrayp->is_ابتو_جو(rhsSelp->dtypep())) {
                        rhsSelp = جديد Ast_Cvt_Unpack_منزل_Unpack{pinp->fileline(), rhsSelp,
                                                           pinexprp->dtypep()};
                    }
                    assignp = جديد Ast_سونپيو_W{pinp->fileline(), pinexprp, rhsSelp};
                }
            } وإلا {
                // V3 width should have range/extended to make the widths correct
                newvarp->isContinuously(سچ);
                // An input port driven from inside the cell by a clocking block
                // output flows out through the connection; V3Tristate split
                // pins resolve separately.
                Ast_ڳنڍ_اظهار* lhsp = خالي_اشارو;  // LHS of the interconnect assign
                Ast_ڳنڍ_اظهار* rhsp = خالي_اشارو;  // RHS of the interconnect assign
                إذا (forTristate || !pinVarp->isClockingDriven()) {
                    // Input-side wiring: temp fed from the connection
                    lhsp = جديد AstVarRef{pinp->fileline(), newvarp, VAccess::WRITE};
                    rhsp = pinexprp;
                } وإلا إذا (Ast_ڳنڍ_اظهار* ثابت drivenp = tryConnectionLhs(pinexprp)) {
                    // Reverse wiring: connection leaves fed from the temp.
                    // The original connection is consumed by the clone above.
                    VL_DO_DANGLING(pinexprp->delete_وڻ(), pinexprp);
                    markContinuousLhs(drivenp);
                    lhsp = drivenp;
                    rhsp = extendOrSel(pinp->fileline(),
                                       جديد AstVarRef{pinp->fileline(), newvarp, VAccess::READ},
                                       drivenp);
                } وإلا {
                    pinp->v3error("Unsupported: Clocking block output driving input port "
                                  "through non-decomposable connection "
                                  << pinp->prettyNameQ());
                    VL_DO_DANGLING(pinexprp->delete_وڻ(), pinexprp);
                    أرجع خالي_اشارو;
                }
                pinp->exprp(جديد AstVarRef{pinp->fileline(), newvarp, VAccess::READ});
                assignp = جديد Ast_سونپيو_W{pinp->fileline(), lhsp, rhsp};
            }
            إذا (assignp) cellp->جوڙيو_ايندڙ_هتي(جديد AstAlways{assignp});
            // UINFOTREE(1, pinp, "", "out");
            // UINFOTREE(1, assignp, "", "aout");
        }
        أرجع assignp;
    }
};

//######################################################################
// Inst class functions

Ast_سونپيو_W* V3Inst::pinReconnectSimple(AstPin* pinp, AstCell* cellp, منطقي forTristate,
                                       منطقي alwaysCvt) {
    أرجع InstStatic::pinReconnectSimple(pinp, cellp, forTristate, alwaysCvt);
}

فراغ V3Inst::checkOutputShort(ثابت AstPin* nodep) {
    إذا (nodep->modVarp()->direction() == VDirection::OUTPUT) {
        إذا (VN_IS(nodep->exprp(), Const) || VN_IS(nodep->exprp(), Extend)
            || (VN_IS(nodep->exprp(), Concat)
                && (VN_IS(VN_AS(nodep->exprp(), Concat)->lhsp(), Const)))) {
            // Uses v3warn for error, as might be found multiple times
            nodep->v3warn(E_PORTSHORT, "Output port is connected to a constant pin,"
                                       " electrical short");
            // Delete so we don't create a 'CONST = ...' assignment
            nodep->exprp()->unlink_Fr_پٺ()->delete_وڻ();
        }
    }
}

//######################################################################
// Inst class visitor

فراغ V3Inst::instAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { InstVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_وڻ("inst", 0, dump_وڻ_Either_Level() >= 3);
}
