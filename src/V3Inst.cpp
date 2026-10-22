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

ساكن فراغ markContinuousLhs(AstNode* ثابت nodep) {
    nodep->foreach([](AstNodeVarRef* refp) {
        𐡄𐡍 (refp->access().isWriteOrRW()) refp->varp()->isContinuously(𐡒𐡔𐡈);
    });
}

// Flip the access of the driven leaves of a pin connection expression so it
// can be used on the LHS of an assignment. Only variables and bit/part
// selects over variables are flipped; index and select operands stay
// read-only. Struct-member selects cannot appear here: V3Width lowers them to
// selects before V3Inst. Returns false if the connection cannot be driven
// (e.g. arithmetic), in which case the expression is left unmodified.
ساكن منطقي markConnectionLhs(AstNodeExpr* ثابت nodep) {
    𐡄𐡍 (AstNodeVarRef* ثابت refp = VN_CAST(nodep, NodeVarRef)) {
        𐡄𐡍 (!refp->access().isReadOnly()) أرجع خطأ;
        refp->access(VAccess::WRITE);
        أرجع 𐡒𐡔𐡈;
    }
    // Note AstSel (bit/part select) is not an AstNodeSel (array select);
    // both expose fromp() for the selected value.
    𐡄𐡍 (AstSel* ثابت selp = VN_CAST(nodep, Sel)) أرجع markConnectionLhs(selp->fromp());
    𐡄𐡍 (AstNodeSel* ثابت selp = VN_CAST(nodep, NodeSel)) {
        أرجع markConnectionLhs(selp->fromp());
    }
    𐡄𐡍 (AstConcat* ثابت concatp = VN_CAST(nodep, Concat)) {
        أرجع markConnectionLhs(concatp->lhsp()) && markConnectionLhs(concatp->rhsp());
    }
    أرجع خطأ;
}

// Prepare a pin connection expression as the LHS of the reverse (inside-out)
// assignment. Returns the flipped expression on success, nullptr if the
// connection cannot be driven. The input is left unmodified on failure.
ساكن AstNodeExpr* tryConnectionLhs(AstNodeExpr* ثابت nodep) {
    AstNodeExpr* ثابت clonep = nodep->cloneTree(خطأ);
    𐡄𐡍 (markConnectionLhs(clonep)) أرجع clonep;
    VL_DO_DANGLING(clonep->deleteTree(), clonep);
    أرجع nullptr;
}

//######################################################################
// Inst state, as a visitor of each AstNode

صنف InstVisitor final : عام VNVisitor {
    // NODE STATE
    // Cleared each Cell:
    //  AstPin::user1p()        -> bool.  True if created assignment already
    ثابت VNUser1InUse m_inuser1;

    // STATE
    AstCell* m_cellp = nullptr;  // Current cell

    // METHODS
    // If appropriate, add an AstAlias to connect the given Cell pin to the given expression.
    // Returns true if an alias was made, in which case there is nothing else to do for this pin.
    منطقي tryAliasPin(AstPin* nodep, AstNodeExpr* exprp) {
        AstVar* ثابت modVarp = nodep->modVarp();
        // An interface reference is aliased via AstAliasScope below, not as a variable
        𐡄𐡍 (modVarp->isIfaceRef()) أرجع خطأ;
        // Only a whole variable can be aliased, anything else needs the assignment
        AstVarRef* ثابت refp = VN_CAST(exprp, VarRef);
        𐡄𐡍 (!refp) أرجع خطأ;
        AstVar* ثابت exprVarp = refp->varp();
        // A virtual interface port connected to a real interface can't be an alias
        𐡄𐡍 (exprVarp->isIfaceRef()) أرجع خطأ;
        // A ref port must become an alias
        𐡄𐡍 (!modVarp->direction().isRef()) {
            // V3FsmDetect recognizes the registers of an fsm_register_wrapper instance via
            // the assignments of its pins, so leave those as assignments
            AstNodeModule* ثابت cellModp = m_cellp->modp();
            𐡄𐡍 (V3Control::getFsmRegisterWrapper(cellModp->origName())
                || V3Control::getFsmRegisterWrapper(cellModp->prettyDehashOrigOrName())) {
                أرجع خطأ;
            }
            // A virtual interface method call is dispatched at run time, so the body of an
            // interface reached that way must use the signals of the instance it is called
            // on, not those of whichever instance this connection happens to be made to
            𐡄𐡍 (ثابت AstIface* ثابت ifacep = VN_CAST(m_cellp->modp(), Iface)) {
                𐡄𐡍 (ifacep->hasVirtualRef()) أرجع خطأ;
            }
            // Forced signals must keep their own storage, the two sides can be forced separately
            𐡄𐡍 (modVarp->isForced() || exprVarp->isForced()) أرجع خطأ;
            // Same for public
            𐡄𐡍 (modVarp->isSigUserRWPublic() || exprVarp->isSigUserRWPublic()) أرجع خطأ;
            // V3Tristate resolved the connected net already, and drives it from the
            // resolution it built for it, so a port merged into it would be driven by
            // the resolution of the instance as well
            𐡄𐡍 (exprVarp->isTristate()) أرجع خطأ;
        }
        // They will become the same variable, so propagate file-line and attributes
        exprVarp->fileline()->modifyStateInherit(modVarp->fileline());
        modVarp->fileline()->modifyStateInherit(exprVarp->fileline());
        exprVarp->propagateAttrFrom(modVarp);
        modVarp->propagateAttrFrom(exprVarp);
        refp->access(VAccess::READWRITE);
        FileLine* ثابت flp = exprp->fileline();
        AstNodeExpr* ثابت itemsp
            = جديد AstVarXRef{flp, modVarp, m_cellp->name(), VAccess::READWRITE};
        itemsp->addNext(exprp);
        m_cellp->addNextHere(جديد AstAlias{flp, itemsp});
        أرجع 𐡒𐡔𐡈;
    }

    // VISITORS
    فراغ visit(AstCell* nodep) override {
        UINFO(4, "  CELL   " << nodep);
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        // VV*****  We reset user1p() on each cell!!!
        AstNode::user1ClearTree();
        iterateChildren(nodep);
    }

    فراغ visit(AstPin* nodep) override {
        // PIN(p,expr) -> ASSIGNW(VARXREF(p),expr)    (if sub's input)
        //            or  ASSIGNW(expr,VARXREF(p))    (if sub's output)
        UINFO(4, "   PIN  " << nodep);
        𐡄𐡍 (!nodep->user1()) {
            // Simplify it
            V3Inst::pinReconnectSimple(nodep, m_cellp, خطأ);
        }
        UINFOTREE(9, nodep, "", "Pin_oldb");
        𐡄𐡍 (!nodep->exprp()) أرجع;  // No-connect
        V3Inst::checkOutputShort(nodep);
        𐡄𐡍 (!nodep->exprp()) أرجع;  // Connection removed by checkOutputShort
        // Use user1p on the PIN to indicate we created an assign for this pin
        𐡄𐡍 (!nodep->user1SetOnce()) {
            // Make an ASSIGNW (expr, pin)
            AstNodeExpr* ثابت exprp = VN_AS(nodep->exprp(), NodeExpr)->cloneTree(خطأ);
            UASSERT_OBJ(exprp->width() == nodep->modVarp()->width(), nodep,
                        "Width mismatch, should have been handled in pinReconnectSimple");
            𐡄𐡍 (nodep->modVarp()->isInout()) {
                nodep->v3fatalSrc("Unsupported: Verilator is a 2-state simulator");
            } وإلا 𐡄𐡍 (nodep->modVarp()->isWritable()) {
                𐡄𐡍 (!tryAliasPin(nodep, exprp)) {
                    AstNodeExpr* ثابت rhsp = جديد AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                             m_cellp->name(), VAccess::READ};
                    markContinuousLhs(exprp);
                    AstAssignW* ثابت assp = جديد AstAssignW{exprp->fileline(), exprp, rhsp};
                    m_cellp->addNextHere(جديد AstAlways{assp});
                }
            } وإلا 𐡄𐡍 (nodep->modVarp()->isNonOutput()) {
                𐡄𐡍 (!tryAliasPin(nodep, exprp)) {
                    // Don't bother moving constants now,
                    // we'll be pushing the const down to the cell soon enough.
                    AstVarXRef* ثابت lhsp = جديد AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                            m_cellp->name(), VAccess::WRITE};

                    markContinuousLhs(lhsp);
                    AstAssignW* ثابت assp = جديد AstAssignW{exprp->fileline(), lhsp, exprp};
                    m_cellp->addNextHere(جديد AstAlways{assp});
                    UINFOTREE(9, assp, "", "_new");
                }
            } وإلا 𐡄𐡍 (nodep->modVarp()->isIfaceRef()) {
                // Create an AstAliasScope for Vars to Cells so we can
                // link with their scope later
                AstNodeExpr* ثابت lhsp = جديد AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                         m_cellp->name(), VAccess::READ};
                ثابت AstVarRef* ثابت refp = VN_CAST(exprp, VarRef);
                ثابت AstVarXRef* ثابت xrefp = VN_CAST(exprp, VarXRef);
                UASSERT_OBJ(refp || xrefp, exprp,
                            "Interfaces: Pin is not connected to a VarRef or VarXRef");
                m_cellp->addNextHere(جديد AstAliasScope{exprp->fileline(), lhsp, exprp});
            } وإلا {
                nodep->v3error("Assigned pin is neither input nor output");
            }
        }

        // We're done with the pin
        VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
    }

    // Save some time
    فراغ visit(AstNodeExpr*) override {}
    فراغ visit(AstNodeAssign*) override {}
    فراغ visit(AstAlways*) override {}

    //--------------------
    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit InstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~InstVisitor() override = افتراضي;
};

//######################################################################
// Inst static function

صنف InstStatic final {
    InstStatic() = افتراضي;  // Static class

    ساكن AstNodeExpr* extendOrSel(FileLine* fl, AstNodeExpr* rhsp, ثابت AstNode* cmpWidthp) {
        𐡄𐡍 (cmpWidthp->width() > rhsp->width()) {
            rhsp = (rhsp->isSigned() ? static_cast<AstNodeExpr*>(جديد AstExtendS{fl, rhsp})
                                     : static_cast<AstNodeExpr*>(جديد AstExtend{fl, rhsp}));
            // Need proper widthMin, which may differ from AstSel created above
            rhsp->dtypeFrom(cmpWidthp);
        } وإلا 𐡄𐡍 (cmpWidthp->width() < rhsp->width()) {
            rhsp = جديد AstSel{fl, rhsp, 0, cmpWidthp->width()};
            // Need proper widthMin, which may differ from AstSel created above
            rhsp->dtypeFrom(cmpWidthp);
        }
        // else don't change dtype, as might be e.g. array of something
        أرجع rhsp;
    }

عام:
    ساكن AstAssignW* pinReconnectSimple(AstPin* pinp, AstCell* cellp, منطقي forTristate,
                                          منطقي alwaysCvt) {
        // If a pin connection is "simple" leave it as-is
        // Else create a intermediate wire to perform the interconnect
        // Return the new assignment, if one was made
        // Note this module calls cloneTree() via new AstVar
        AstVar* ثابت pinVarp = pinp->modVarp();
        𐡄𐡍 (!pinp->exprp()) {
            // No-connect, perhaps promote based on `unconnected_drive,
            // otherwise done
            𐡄𐡍 (pinVarp->direction() == VDirection::INPUT
                && cellp->modp()->unconnectedDrive().isSetTrue()) {
                pinp->exprp(جديد AstConst{pinp->fileline(), AstConst::All1{}});
            } وإلا 𐡄𐡍 (pinVarp->direction() == VDirection::INPUT
                       && cellp->modp()->unconnectedDrive().isSetFalse()) {
                pinp->exprp(جديد AstConst{pinp->fileline(), AstConst::All0{}});
            } وإلا {
                أرجع nullptr;
            }
        }
        ثابت AstVarRef* ثابت connectRefp = VN_CAST(pinp->exprp(), VarRef);
        ثابت AstVarXRef* ثابت connectXRefp = VN_CAST(pinp->exprp(), VarXRef);
        ثابت AstNodeDType* ثابت pinDTypep = pinVarp->dtypep()->skipRefp();
        ثابت AstBasicDType* ثابت pinBasicp = VN_CAST(pinDTypep, BasicDType);
        ثابت AstNodeDType* ثابت connDTypep
            = connectRefp ? connectRefp->varp()->dtypep()->skipRefp() : nullptr;
        ثابت AstBasicDType* ثابت connBasicp = VN_CAST(connDTypep, BasicDType);
        AstAssignW* assignp = nullptr;
        //
        𐡄𐡍 (!alwaysCvt && connectRefp && connDTypep->sameTree(pinDTypep)
            && !connectRefp->varp()->isSc()) {  // Need the signal as a 'shell' to convert types
            // Done. Same data type
        } وإلا 𐡄𐡍 (!alwaysCvt && connectRefp && connectRefp->varp()->isIfaceRef()) {
            // Done. Interface
        } وإلا 𐡄𐡍 (!alwaysCvt && connectXRefp && connectXRefp->varp()
                   && connectXRefp->varp()->isIfaceRef()) {
        } وإلا 𐡄𐡍 (!alwaysCvt && connBasicp && pinBasicp
                   && connBasicp->width() == pinBasicp->width()
                   && connBasicp->lo() == pinBasicp->lo()
                   && !connectRefp->varp()
                           ->isSc()  // Need the signal as a 'shell' to convert types
                   && connBasicp->width() == pinVarp->width()) {
            // Done. One to one interconnect won't need a temporary variable.
        } وإلا 𐡄𐡍 (!alwaysCvt && !forTristate && VN_IS(pinp->exprp(), Const)) {
            // Done. Constant. Still check for driving an output, like below.
            V3Inst::checkOutputShort(pinp);
            𐡄𐡍 (!pinp->exprp()) أرجع nullptr;
        } وإلا {
            // Make a new temp wire
            // UINFOTREE(9, pinp, "", "in_pin");
            V3Inst::checkOutputShort(pinp);
            𐡄𐡍 (!pinp->exprp()) أرجع nullptr;
            // Simplify, so stuff like '{a[0], b[0]}[1]' produced during
            // instance array expansion are brought to normal 'a[0]'
            AstNodeExpr* ثابت pinexprp
                = V3Const::constifyEdit(VN_AS(pinp->exprp(), NodeExpr)->unlinkFrBack());
            ثابت string newvarname
                = (string{pinVarp->isWritable() ? "__Vcellout" : "__Vcellinp"}
                   // Prevent name conflict if both tri & non-tri add signals
                   + (forTristate ? "t" : "") + "__" + cellp->name() + "__" + pinp->name());
            AstVar* ثابت newvarp
                = جديد AstVar{pinVarp->fileline(), VVarType::MODULETEMP, newvarname, pinVarp};
            // Important to add statement next to cell, in case there is a
            // generate with same named cell
            cellp->addNextHere(newvarp);
            𐡄𐡍 (pinVarp->isInout()) {
                pinVarp->v3fatalSrc("Unsupported: Inout connections to pins must be"
                                    " direct one-to-one connection (without any expression)");
                // V3Tristate should have cleared up before this point
            } وإلا 𐡄𐡍 (pinVarp->isWritable()) {
                // See also V3Inst
                AstNodeExpr* rhsp = جديد AstVarRef{pinp->fileline(), newvarp, VAccess::READ};
                UINFO(5, "pinRecon width " << pinVarp->width() << " >? " << rhsp->width() << " >? "
                                           << pinexprp->width());
                rhsp = extendOrSel(pinp->fileline(), rhsp, pinVarp);
                pinp->exprp(جديد AstVarRef{newvarp->fileline(), newvarp, VAccess::WRITE});
                markContinuousLhs(pinexprp);
                𐡄𐡍 (VN_IS(pinexprp, NodeStream)) {
                    assignp = جديد AstAssignW{pinp->fileline(), pinexprp, rhsp};
                    V3Width::streamAssignLowerEdit(assignp);
                } وإلا {
                    AstNodeExpr* rhsSelp = extendOrSel(pinp->fileline(), rhsp, pinexprp);
                    // A reversed array is assigned by the left-to-right order of elements
                    // (IEEE 1800-2023 7.6)
                    ثابت AstUnpackArrayDType* ثابت arrayp
                        = VN_CAST(pinexprp->dtypep()->skipRefp(), UnpackArrayDType);
                    𐡄𐡍 (arrayp && arrayp->isReverseOf(rhsSelp->dtypep())) {
                        rhsSelp = جديد AstCvtUnpackToUnpack{pinp->fileline(), rhsSelp,
                                                           pinexprp->dtypep()};
                    }
                    assignp = جديد AstAssignW{pinp->fileline(), pinexprp, rhsSelp};
                }
            } وإلا {
                // V3 width should have range/extended to make the widths correct
                newvarp->isContinuously(𐡒𐡔𐡈);
                // An input port driven from inside the cell by a clocking block
                // output flows out through the connection; V3Tristate split
                // pins resolve separately.
                AstNodeExpr* lhsp = nullptr;  // LHS of the interconnect assign
                AstNodeExpr* rhsp = nullptr;  // RHS of the interconnect assign
                𐡄𐡍 (forTristate || !pinVarp->isClockingDriven()) {
                    // Input-side wiring: temp fed from the connection
                    lhsp = جديد AstVarRef{pinp->fileline(), newvarp, VAccess::WRITE};
                    rhsp = pinexprp;
                } وإلا 𐡄𐡍 (AstNodeExpr* ثابت drivenp = tryConnectionLhs(pinexprp)) {
                    // Reverse wiring: connection leaves fed from the temp.
                    // The original connection is consumed by the clone above.
                    VL_DO_DANGLING(pinexprp->deleteTree(), pinexprp);
                    markContinuousLhs(drivenp);
                    lhsp = drivenp;
                    rhsp = extendOrSel(pinp->fileline(),
                                       جديد AstVarRef{pinp->fileline(), newvarp, VAccess::READ},
                                       drivenp);
                } وإلا {
                    pinp->v3error("Unsupported: Clocking block output driving input port "
                                  "through non-decomposable connection "
                                  << pinp->prettyNameQ());
                    VL_DO_DANGLING(pinexprp->deleteTree(), pinexprp);
                    أرجع nullptr;
                }
                pinp->exprp(جديد AstVarRef{pinp->fileline(), newvarp, VAccess::READ});
                assignp = جديد AstAssignW{pinp->fileline(), lhsp, rhsp};
            }
            𐡄𐡍 (assignp) cellp->addNextHere(جديد AstAlways{assignp});
            // UINFOTREE(1, pinp, "", "out");
            // UINFOTREE(1, assignp, "", "aout");
        }
        أرجع assignp;
    }
};

//######################################################################
// Inst class functions

AstAssignW* V3Inst::pinReconnectSimple(AstPin* pinp, AstCell* cellp, منطقي forTristate,
                                       منطقي alwaysCvt) {
    أرجع InstStatic::pinReconnectSimple(pinp, cellp, forTristate, alwaysCvt);
}

فراغ V3Inst::checkOutputShort(ثابت AstPin* nodep) {
    𐡄𐡍 (nodep->modVarp()->direction() == VDirection::OUTPUT) {
        𐡄𐡍 (VN_IS(nodep->exprp(), Const) || VN_IS(nodep->exprp(), Extend)
            || (VN_IS(nodep->exprp(), Concat)
                && (VN_IS(VN_AS(nodep->exprp(), Concat)->lhsp(), Const)))) {
            // Uses v3warn for error, as might be found multiple times
            nodep->v3warn(E_PORTSHORT, "Output port is connected to a constant pin,"
                                       " electrical short");
            // Delete so we don't create a 'CONST = ...' assignment
            nodep->exprp()->unlinkFrBack()->deleteTree();
        }
    }
}

//######################################################################
// Inst class visitor

فراغ V3Inst::instAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { InstVisitor{nodep}; }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("inst", 0, dumpTreeEitherLevel() >= 3);
}
