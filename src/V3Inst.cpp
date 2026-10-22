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

ساکن خالی markContinuousLhs(Ast_گرہ* ثابت nodep) {
    nodep->foreach([](Ast_گرہ_Var_Ref* refp) {
        اگر (refp->access().isWriteOrRW()) refp->varp()->isContinuously(سچ);
    });
}

// Flip the access of the driven leaves of a pin connection expression so it
// can be used on the LHS of an assignment. Only variables and bit/part
// selects over variables are flipped; index and select operands stay
// read-only. Struct-member selects cannot appear here: V3Width lowers them to
// selects before V3Inst. Returns false if the connection cannot be driven
// (e.g. arithmetic), in which case the expression is left unmodified.
ساکن منطقی markConnectionLhs(Ast_گرہ_اظہاریہ* ثابت nodep) {
    اگر (Ast_گرہ_Var_Ref* ثابت refp = VN_CAST(nodep, گرہ_Var_Ref)) {
        اگر (!refp->access().isReadOnly()) واپس جھوٹ;
        refp->access(VAccess::WRITE);
        واپس سچ;
    }
    // Note AstSel (bit/part select) is not an AstNodeSel (array select);
    // both expose fromp() for the selected value.
    اگر (AstSel* ثابت selp = VN_CAST(nodep, Sel)) واپس markConnectionLhs(selp->fromp());
    اگر (Ast_گرہ_Sel* ثابت selp = VN_CAST(nodep, گرہ_Sel)) {
        واپس markConnectionLhs(selp->fromp());
    }
    اگر (AstConcat* ثابت concatp = VN_CAST(nodep, Concat)) {
        واپس markConnectionLhs(concatp->lhsp()) && markConnectionLhs(concatp->rhsp());
    }
    واپس جھوٹ;
}

// Prepare a pin connection expression as the LHS of the reverse (inside-out)
// assignment. Returns the flipped expression on success, nullptr if the
// connection cannot be driven. The input is left unmodified on failure.
ساکن Ast_گرہ_اظہاریہ* tryConnectionLhs(Ast_گرہ_اظہاریہ* ثابت nodep) {
    Ast_گرہ_اظہاریہ* ثابت clonep = nodep->clone_درخت(جھوٹ);
    اگر (markConnectionLhs(clonep)) واپس clonep;
    VL_DO_DANGLING(clonep->delete_درخت(), clonep);
    واپس خالی_اشاریہ;
}

//######################################################################
// Inst state, as a visitor of each AstNode

جماعت InstVisitor final : عوامی VNVisitor {
    // NODE STATE
    // Cleared each Cell:
    //  AstPin::user1p()        -> bool.  True if created assignment already
    ثابت VNUser1_اندر_Use m_inuser1;

    // STATE
    AstCell* m_cellp = خالی_اشاریہ;  // Current cell

    // METHODS
    // If appropriate, add an AstAlias to connect the given Cell pin to the given expression.
    // Returns true if an alias was made, in which case there is nothing else to do for this pin.
    منطقی tryAliasPin(AstPin* nodep, Ast_گرہ_اظہاریہ* exprp) {
        AstVar* ثابت modVarp = nodep->modVarp();
        // An interface reference is aliased via AstAliasScope below, not as a variable
        اگر (modVarp->isIfaceRef()) واپس جھوٹ;
        // Only a whole variable can be aliased, anything else needs the assignment
        AstVarRef* ثابت refp = VN_CAST(exprp, VarRef);
        اگر (!refp) واپس جھوٹ;
        AstVar* ثابت اظہاریہ_Varp = refp->varp();
        // A virtual interface port connected to a real interface can't be an alias
        اگر (اظہاریہ_Varp->isIfaceRef()) واپس جھوٹ;
        // A ref port must become an alias
        اگر (!modVarp->direction().isRef()) {
            // V3FsmDetect recognizes the registers of an fsm_register_wrapper instance via
            // the assignments of its pins, so leave those as assignments
            Ast_گرہ_Module* ثابت cellModp = m_cellp->modp();
            اگر (V3Control::get_Fsm_رجسٹر_Wrapper(cellModp->origName())
                || V3Control::get_Fsm_رجسٹر_Wrapper(cellModp->prettyDehashOrigOrName())) {
                واپس جھوٹ;
            }
            // A virtual interface method call is dispatched at run time, so the body of an
            // interface reached that way must use the signals of the instance it is called
            // on, not those of whichever instance this connection happens to be made to
            اگر (ثابت AstIface* ثابت ifacep = VN_CAST(m_cellp->modp(), Iface)) {
                اگر (ifacep->hasVirtualRef()) واپس جھوٹ;
            }
            // Forced signals must keep their own storage, the two sides can be forced separately
            اگر (modVarp->isForced() || اظہاریہ_Varp->isForced()) واپس جھوٹ;
            // Same for public
            اگر (modVarp->isSigUserRWPublic() || اظہاریہ_Varp->isSigUserRWPublic()) واپس جھوٹ;
            // V3Tristate resolved the connected net already, and drives it from the
            // resolution it built for it, so a port merged into it would be driven by
            // the resolution of the instance as well
            اگر (اظہاریہ_Varp->isTristate()) واپس جھوٹ;
        }
        // They will become the same variable, so propagate file-line and attributes
        اظہاریہ_Varp->fileline()->modifyStateInherit(modVarp->fileline());
        modVarp->fileline()->modifyStateInherit(اظہاریہ_Varp->fileline());
        اظہاریہ_Varp->propagateAttrFrom(modVarp);
        modVarp->propagateAttrFrom(اظہاریہ_Varp);
        refp->access(VAccess::READWRITE);
        File_سطر* ثابت flp = exprp->fileline();
        Ast_گرہ_اظہاریہ* ثابت itemsp
            = نیا AstVarXRef{flp, modVarp, m_cellp->name(), VAccess::READWRITE};
        itemsp->جوڑو_اگلا(exprp);
        m_cellp->جوڑو_اگلا_یہاں(نیا AstAlias{flp, itemsp});
        واپس سچ;
    }

    // VISITORS
    خالی دورہ(AstCell* nodep) override {
        UINFO(4, "  CELL   " << nodep);
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        // VV*****  We reset user1p() on each cell!!!
        Ast_گرہ::user1_Clear_درخت();
        iterateChildren(nodep);
    }

    خالی دورہ(AstPin* nodep) override {
        // PIN(p,expr) -> ASSIGNW(VARXREF(p),expr)    (if sub's input)
        //            or  ASSIGNW(expr,VARXREF(p))    (if sub's output)
        UINFO(4, "   PIN  " << nodep);
        اگر (!nodep->user1()) {
            // Simplify it
            V3Inst::pinReconnectSimple(nodep, m_cellp, جھوٹ);
        }
        UINFOTREE(9, nodep, "", "Pin_oldb");
        اگر (!nodep->exprp()) واپس;  // No-connect
        V3Inst::checkOutputShort(nodep);
        اگر (!nodep->exprp()) واپس;  // Connection removed by checkOutputShort
        // Use user1p on the PIN to indicate we created an assign for this pin
        اگر (!nodep->user1_سیٹ_Once()) {
            // Make an ASSIGNW (expr, pin)
            Ast_گرہ_اظہاریہ* ثابت exprp = VN_AS(nodep->exprp(), گرہ_اظہاریہ)->clone_درخت(جھوٹ);
            UASSERT_OBJ(exprp->چوڑائی() == nodep->modVarp()->چوڑائی(), nodep,
                        "Width mismatch, should have been handled in pinReconnectSimple");
            اگر (nodep->modVarp()->isInout()) {
                nodep->v3fatalSrc("Unsupported: Verilator is a 2-state simulator");
            } ورنہ اگر (nodep->modVarp()->isWritable()) {
                اگر (!tryAliasPin(nodep, exprp)) {
                    Ast_گرہ_اظہاریہ* ثابت rhsp = نیا AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                             m_cellp->name(), VAccess::READ};
                    markContinuousLhs(exprp);
                    Ast_تفویض_W* ثابت assp = نیا Ast_تفویض_W{exprp->fileline(), exprp, rhsp};
                    m_cellp->جوڑو_اگلا_یہاں(نیا AstAlways{assp});
                }
            } ورنہ اگر (nodep->modVarp()->isNonOutput()) {
                اگر (!tryAliasPin(nodep, exprp)) {
                    // Don't bother moving constants now,
                    // we'll be pushing the const down to the cell soon enough.
                    AstVarXRef* ثابت lhsp = نیا AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                            m_cellp->name(), VAccess::WRITE};

                    markContinuousLhs(lhsp);
                    Ast_تفویض_W* ثابت assp = نیا Ast_تفویض_W{exprp->fileline(), lhsp, exprp};
                    m_cellp->جوڑو_اگلا_یہاں(نیا AstAlways{assp});
                    UINFOTREE(9, assp, "", "_new");
                }
            } ورنہ اگر (nodep->modVarp()->isIfaceRef()) {
                // Create an AstAliasScope for Vars to Cells so we can
                // link with their scope later
                Ast_گرہ_اظہاریہ* ثابت lhsp = نیا AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                         m_cellp->name(), VAccess::READ};
                ثابت AstVarRef* ثابت refp = VN_CAST(exprp, VarRef);
                ثابت AstVarXRef* ثابت xrefp = VN_CAST(exprp, VarXRef);
                UASSERT_OBJ(refp || xrefp, exprp,
                            "Interfaces: Pin is not connected to a VarRef or VarXRef");
                m_cellp->جوڑو_اگلا_یہاں(نیا AstAliasScope{exprp->fileline(), lhsp, exprp});
            } ورنہ {
                nodep->v3error("Assigned pin is neither input nor output");
            }
        }

        // We're done with the pin
        VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
    }

    // Save some time
    خالی دورہ(Ast_گرہ_اظہاریہ*) override {}
    خالی دورہ(Ast_گرہ_تفویض*) override {}
    خالی دورہ(AstAlways*) override {}

    //--------------------
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح InstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~InstVisitor() override = طےشدہ;
};

//######################################################################
// Inst static function

جماعت InstStatic final {
    InstStatic() = طےشدہ;  // Static class

    ساکن Ast_گرہ_اظہاریہ* extendOrSel(File_سطر* fl, Ast_گرہ_اظہاریہ* rhsp, ثابت Ast_گرہ* cmpWidthp) {
        اگر (cmpWidthp->چوڑائی() > rhsp->چوڑائی()) {
            rhsp = (rhsp->isSigned() ? static_cast<Ast_گرہ_اظہاریہ*>(نیا AstExtendS{fl, rhsp})
                                     : static_cast<Ast_گرہ_اظہاریہ*>(نیا AstExtend{fl, rhsp}));
            // Need proper widthMin, which may differ from AstSel created above
            rhsp->dtypeFrom(cmpWidthp);
        } ورنہ اگر (cmpWidthp->چوڑائی() < rhsp->چوڑائی()) {
            rhsp = نیا AstSel{fl, rhsp, 0, cmpWidthp->چوڑائی()};
            // Need proper widthMin, which may differ from AstSel created above
            rhsp->dtypeFrom(cmpWidthp);
        }
        // else don't change dtype, as might be e.g. array of something
        واپس rhsp;
    }

عوامی:
    ساکن Ast_تفویض_W* pinReconnectSimple(AstPin* pinp, AstCell* cellp, منطقی forTristate,
                                          منطقی alwaysCvt) {
        // If a pin connection is "simple" leave it as-is
        // Else create a intermediate wire to perform the interconnect
        // Return the new assignment, if one was made
        // Note this module calls cloneTree() via new AstVar
        AstVar* ثابت pinVarp = pinp->modVarp();
        اگر (!pinp->exprp()) {
            // No-connect, perhaps promote based on `unconnected_drive,
            // otherwise done
            اگر (pinVarp->direction() == VDirection::داخلہ
                && cellp->modp()->unconnectedDrive().is_سیٹ_True()) {
                pinp->exprp(نیا AstConst{pinp->fileline(), AstConst::All1{}});
            } ورنہ اگر (pinVarp->direction() == VDirection::داخلہ
                       && cellp->modp()->unconnectedDrive().is_سیٹ_False()) {
                pinp->exprp(نیا AstConst{pinp->fileline(), AstConst::All0{}});
            } ورنہ {
                واپس خالی_اشاریہ;
            }
        }
        ثابت AstVarRef* ثابت connectRefp = VN_CAST(pinp->exprp(), VarRef);
        ثابت AstVarXRef* ثابت connectXRefp = VN_CAST(pinp->exprp(), VarXRef);
        ثابت Ast_گرہ_DType* ثابت pinDTypep = pinVarp->dtypep()->skipRefp();
        ثابت AstBasicDType* ثابت pinBasicp = VN_CAST(pinDTypep, BasicDType);
        ثابت Ast_گرہ_DType* ثابت connDTypep
            = connectRefp ? connectRefp->varp()->dtypep()->skipRefp() : خالی_اشاریہ;
        ثابت AstBasicDType* ثابت connBasicp = VN_CAST(connDTypep, BasicDType);
        Ast_تفویض_W* assignp = خالی_اشاریہ;
        //
        اگر (!alwaysCvt && connectRefp && connDTypep->same_درخت(pinDTypep)
            && !connectRefp->varp()->isSc()) {  // Need the signal as a 'shell' to convert types
            // Done. Same data type
        } ورنہ اگر (!alwaysCvt && connectRefp && connectRefp->varp()->isIfaceRef()) {
            // Done. Interface
        } ورنہ اگر (!alwaysCvt && connectXRefp && connectXRefp->varp()
                   && connectXRefp->varp()->isIfaceRef()) {
        } ورنہ اگر (!alwaysCvt && connBasicp && pinBasicp
                   && connBasicp->چوڑائی() == pinBasicp->چوڑائی()
                   && connBasicp->lo() == pinBasicp->lo()
                   && !connectRefp->varp()
                           ->isSc()  // Need the signal as a 'shell' to convert types
                   && connBasicp->چوڑائی() == pinVarp->چوڑائی()) {
            // Done. One to one interconnect won't need a temporary variable.
        } ورنہ اگر (!alwaysCvt && !forTristate && VN_IS(pinp->exprp(), Const)) {
            // Done. Constant. Still check for driving an output, like below.
            V3Inst::checkOutputShort(pinp);
            اگر (!pinp->exprp()) واپس خالی_اشاریہ;
        } ورنہ {
            // Make a new temp wire
            // UINFOTREE(9, pinp, "", "in_pin");
            V3Inst::checkOutputShort(pinp);
            اگر (!pinp->exprp()) واپس خالی_اشاریہ;
            // Simplify, so stuff like '{a[0], b[0]}[1]' produced during
            // instance array expansion are brought to normal 'a[0]'
            Ast_گرہ_اظہاریہ* ثابت pinexprp
                = V3Const::constifyEdit(VN_AS(pinp->exprp(), گرہ_اظہاریہ)->unlink_Fr_عقب());
            ثابت string newvarname
                = (string{pinVarp->isWritable() ? "__Vcellout" : "__Vcellinp"}
                   // Prevent name conflict if both tri & non-tri add signals
                   + (forTristate ? "t" : "") + "__" + cellp->name() + "__" + pinp->name());
            AstVar* ثابت newvarp
                = نیا AstVar{pinVarp->fileline(), VVarType::MODULETEMP, newvarname, pinVarp};
            // Important to add statement next to cell, in case there is a
            // generate with same named cell
            cellp->جوڑو_اگلا_یہاں(newvarp);
            اگر (pinVarp->isInout()) {
                pinVarp->v3fatalSrc("Unsupported: Inout connections to pins must be"
                                    " direct one-to-one connection (without any expression)");
                // V3Tristate should have cleared up before this point
            } ورنہ اگر (pinVarp->isWritable()) {
                // See also V3Inst
                Ast_گرہ_اظہاریہ* rhsp = نیا AstVarRef{pinp->fileline(), newvarp, VAccess::READ};
                UINFO(5, "pinRecon width " << pinVarp->چوڑائی() << " >? " << rhsp->چوڑائی() << " >? "
                                           << pinexprp->چوڑائی());
                rhsp = extendOrSel(pinp->fileline(), rhsp, pinVarp);
                pinp->exprp(نیا AstVarRef{newvarp->fileline(), newvarp, VAccess::WRITE});
                markContinuousLhs(pinexprp);
                اگر (VN_IS(pinexprp, گرہ_دھارا)) {
                    assignp = نیا Ast_تفویض_W{pinp->fileline(), pinexprp, rhsp};
                    V3_چوڑائی::دھارا_تفویض_زیریں_Edit(assignp);
                } ورنہ {
                    Ast_گرہ_اظہاریہ* rhsSelp = extendOrSel(pinp->fileline(), rhsp, pinexprp);
                    // A reversed array is assigned by the left-to-right order of elements
                    // (IEEE 1800-2023 7.6)
                    ثابت AstUnpackArrayDType* ثابت arrayp
                        = VN_CAST(pinexprp->dtypep()->skipRefp(), UnpackArrayDType);
                    اگر (arrayp && arrayp->is_الٹ_کا(rhsSelp->dtypep())) {
                        rhsSelp = نیا Ast_Cvt_Unpack_مقصد_Unpack{pinp->fileline(), rhsSelp,
                                                           pinexprp->dtypep()};
                    }
                    assignp = نیا Ast_تفویض_W{pinp->fileline(), pinexprp, rhsSelp};
                }
            } ورنہ {
                // V3 width should have range/extended to make the widths correct
                newvarp->isContinuously(سچ);
                // An input port driven from inside the cell by a clocking block
                // output flows out through the connection; V3Tristate split
                // pins resolve separately.
                Ast_گرہ_اظہاریہ* lhsp = خالی_اشاریہ;  // LHS of the interconnect assign
                Ast_گرہ_اظہاریہ* rhsp = خالی_اشاریہ;  // RHS of the interconnect assign
                اگر (forTristate || !pinVarp->isClockingDriven()) {
                    // Input-side wiring: temp fed from the connection
                    lhsp = نیا AstVarRef{pinp->fileline(), newvarp, VAccess::WRITE};
                    rhsp = pinexprp;
                } ورنہ اگر (Ast_گرہ_اظہاریہ* ثابت drivenp = tryConnectionLhs(pinexprp)) {
                    // Reverse wiring: connection leaves fed from the temp.
                    // The original connection is consumed by the clone above.
                    VL_DO_DANGLING(pinexprp->delete_درخت(), pinexprp);
                    markContinuousLhs(drivenp);
                    lhsp = drivenp;
                    rhsp = extendOrSel(pinp->fileline(),
                                       نیا AstVarRef{pinp->fileline(), newvarp, VAccess::READ},
                                       drivenp);
                } ورنہ {
                    pinp->v3error("Unsupported: Clocking block output driving input port "
                                  "through non-decomposable connection "
                                  << pinp->prettyNameQ());
                    VL_DO_DANGLING(pinexprp->delete_درخت(), pinexprp);
                    واپس خالی_اشاریہ;
                }
                pinp->exprp(نیا AstVarRef{pinp->fileline(), newvarp, VAccess::READ});
                assignp = نیا Ast_تفویض_W{pinp->fileline(), lhsp, rhsp};
            }
            اگر (assignp) cellp->جوڑو_اگلا_یہاں(نیا AstAlways{assignp});
            // UINFOTREE(1, pinp, "", "out");
            // UINFOTREE(1, assignp, "", "aout");
        }
        واپس assignp;
    }
};

//######################################################################
// Inst class functions

Ast_تفویض_W* V3Inst::pinReconnectSimple(AstPin* pinp, AstCell* cellp, منطقی forTristate,
                                       منطقی alwaysCvt) {
    واپس InstStatic::pinReconnectSimple(pinp, cellp, forTristate, alwaysCvt);
}

خالی V3Inst::checkOutputShort(ثابت AstPin* nodep) {
    اگر (nodep->modVarp()->direction() == VDirection::OUTPUT) {
        اگر (VN_IS(nodep->exprp(), Const) || VN_IS(nodep->exprp(), Extend)
            || (VN_IS(nodep->exprp(), Concat)
                && (VN_IS(VN_AS(nodep->exprp(), Concat)->lhsp(), Const)))) {
            // Uses v3warn for error, as might be found multiple times
            nodep->v3warn(E_PORTSHORT, "Output port is connected to a constant pin,"
                                       " electrical short");
            // Delete so we don't create a 'CONST = ...' assignment
            nodep->exprp()->unlink_Fr_عقب()->delete_درخت();
        }
    }
}

//######################################################################
// Inst class visitor

خالی V3Inst::instAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { InstVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("inst", 0, dump_درخت_Either_Level() >= 3);
}
