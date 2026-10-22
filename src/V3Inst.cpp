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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Inst.h"

#অন্তর্ভাব "V3Const.h"
#অন্তর্ভাব "V3Control.h"
#অন্তর্ভাব "V3Width.h"

VL_DEFINE_DEBUG_FUNCTIONS;

স্থির শূন্য markContinuousLhs(Ast_গ্রন্থি* ধ্রুব nodep) {
    nodep->foreach([](Ast_গ্রন্থি_Var_Ref* refp) {
        যদি (refp->access().isWriteOrRW()) refp->varp()->isContinuously(সত্য);
    });
}

// Flip the access of the driven leaves of a pin connection expression so it
// can be used on the LHS of an assignment. Only variables and bit/part
// selects over variables are flipped; index and select operands stay
// read-only. Struct-member selects cannot appear here: V3Width lowers them to
// selects before V3Inst. Returns false if the connection cannot be driven
// (e.g. arithmetic), in which case the expression is left unmodified.
স্থির তার্কিক markConnectionLhs(Ast_গ্রন্থি_রাশিমালা* ধ্রুব nodep) {
    যদি (Ast_গ্রন্থি_Var_Ref* ধ্রুব refp = VN_CAST(nodep, গ্রন্থি_Var_Ref)) {
        যদি (!refp->access().isReadOnly()) প্রত্যাগম অসত্য;
        refp->access(VAccess::WRITE);
        প্রত্যাগম সত্য;
    }
    // Note AstSel (bit/part select) is not an AstNodeSel (array select);
    // both expose fromp() for the selected value.
    যদি (AstSel* ধ্রুব selp = VN_CAST(nodep, Sel)) প্রত্যাগম markConnectionLhs(selp->fromp());
    যদি (Ast_গ্রন্থি_Sel* ধ্রুব selp = VN_CAST(nodep, গ্রন্থি_Sel)) {
        প্রত্যাগম markConnectionLhs(selp->fromp());
    }
    যদি (AstConcat* ধ্রুব concatp = VN_CAST(nodep, Concat)) {
        প্রত্যাগম markConnectionLhs(concatp->lhsp()) && markConnectionLhs(concatp->rhsp());
    }
    প্রত্যাগম অসত্য;
}

// Prepare a pin connection expression as the LHS of the reverse (inside-out)
// assignment. Returns the flipped expression on success, nullptr if the
// connection cannot be driven. The input is left unmodified on failure.
স্থির Ast_গ্রন্থি_রাশিমালা* tryConnectionLhs(Ast_গ্রন্থি_রাশিমালা* ধ্রুব nodep) {
    Ast_গ্রন্থি_রাশিমালা* ধ্রুব clonep = nodep->clone_বৃক্ষ(অসত্য);
    যদি (markConnectionLhs(clonep)) প্রত্যাগম clonep;
    VL_DO_DANGLING(clonep->delete_বৃক্ষ(), clonep);
    প্রত্যাগম nullptr;
}

//######################################################################
// Inst state, as a visitor of each AstNode

বর্গ InstVisitor final : সার্বজনিক VNVisitor {
    // NODE STATE
    // Cleared each Cell:
    //  AstPin::user1p()        -> bool.  True if created assignment already
    ধ্রুব VNUser1InUse m_inuser1;

    // STATE
    AstCell* m_cellp = nullptr;  // Current cell

    // METHODS
    // If appropriate, add an AstAlias to connect the given Cell pin to the given expression.
    // Returns true if an alias was made, in which case there is nothing else to do for this pin.
    তার্কিক tryAliasPin(AstPin* nodep, Ast_গ্রন্থি_রাশিমালা* exprp) {
        AstVar* ধ্রুব modVarp = nodep->modVarp();
        // An interface reference is aliased via AstAliasScope below, not as a variable
        যদি (modVarp->isIfaceRef()) প্রত্যাগম অসত্য;
        // Only a whole variable can be aliased, anything else needs the assignment
        AstVarRef* ধ্রুব refp = VN_CAST(exprp, VarRef);
        যদি (!refp) প্রত্যাগম অসত্য;
        AstVar* ধ্রুব রাশিমালা_Varp = refp->varp();
        // A virtual interface port connected to a real interface can't be an alias
        যদি (রাশিমালা_Varp->isIfaceRef()) প্রত্যাগম অসত্য;
        // A ref port must become an alias
        যদি (!modVarp->direction().isRef()) {
            // V3FsmDetect recognizes the registers of an fsm_register_wrapper instance via
            // the assignments of its pins, so leave those as assignments
            Ast_গ্রন্থি_Module* ধ্রুব cellModp = m_cellp->modp();
            যদি (V3Control::getFsmRegisterWrapper(cellModp->origName())
                || V3Control::getFsmRegisterWrapper(cellModp->prettyDehashOrigOrName())) {
                প্রত্যাগম অসত্য;
            }
            // A virtual interface method call is dispatched at run time, so the body of an
            // interface reached that way must use the signals of the instance it is called
            // on, not those of whichever instance this connection happens to be made to
            যদি (ধ্রুব AstIface* ধ্রুব ifacep = VN_CAST(m_cellp->modp(), Iface)) {
                যদি (ifacep->hasVirtualRef()) প্রত্যাগম অসত্য;
            }
            // Forced signals must keep their own storage, the two sides can be forced separately
            যদি (modVarp->isForced() || রাশিমালা_Varp->isForced()) প্রত্যাগম অসত্য;
            // Same for public
            যদি (modVarp->isSigUserRWPublic() || রাশিমালা_Varp->isSigUserRWPublic()) প্রত্যাগম অসত্য;
            // V3Tristate resolved the connected net already, and drives it from the
            // resolution it built for it, so a port merged into it would be driven by
            // the resolution of the instance as well
            যদি (রাশিমালা_Varp->isTristate()) প্রত্যাগম অসত্য;
        }
        // They will become the same variable, so propagate file-line and attributes
        রাশিমালা_Varp->fileline()->modifyStateInherit(modVarp->fileline());
        modVarp->fileline()->modifyStateInherit(রাশিমালা_Varp->fileline());
        রাশিমালা_Varp->propagateAttrFrom(modVarp);
        modVarp->propagateAttrFrom(রাশিমালা_Varp);
        refp->access(VAccess::READWRITE);
        File_সারি* ধ্রুব flp = exprp->fileline();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব itemsp
            = নূতন AstVarXRef{flp, modVarp, m_cellp->name(), VAccess::READWRITE};
        itemsp->addNext(exprp);
        m_cellp->addNextHere(নূতন AstAlias{flp, itemsp});
        প্রত্যাগম সত্য;
    }

    // VISITORS
    শূন্য visit(AstCell* nodep) override {
        UINFO(4, "  CELL   " << nodep);
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        // VV*****  We reset user1p() on each cell!!!
        Ast_গ্রন্থি::user1_Clear_বৃক্ষ();
        iterateChildren(nodep);
    }

    শূন্য visit(AstPin* nodep) override {
        // PIN(p,expr) -> ASSIGNW(VARXREF(p),expr)    (if sub's input)
        //            or  ASSIGNW(expr,VARXREF(p))    (if sub's output)
        UINFO(4, "   PIN  " << nodep);
        যদি (!nodep->user1()) {
            // Simplify it
            V3Inst::pinReconnectSimple(nodep, m_cellp, অসত্য);
        }
        UINFOTREE(9, nodep, "", "Pin_oldb");
        যদি (!nodep->exprp()) প্রত্যাগম;  // No-connect
        V3Inst::checkOutputShort(nodep);
        যদি (!nodep->exprp()) প্রত্যাগম;  // Connection removed by checkOutputShort
        // Use user1p on the PIN to indicate we created an assign for this pin
        যদি (!nodep->user1SetOnce()) {
            // Make an ASSIGNW (expr, pin)
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp = VN_AS(nodep->exprp(), গ্রন্থি_রাশিমালা)->clone_বৃক্ষ(অসত্য);
            UASSERT_OBJ(exprp->width() == nodep->modVarp()->width(), nodep,
                        "Width mismatch, should have been handled in pinReconnectSimple");
            যদি (nodep->modVarp()->isInout()) {
                nodep->v3fatalSrc("Unsupported: Verilator is a 2-state simulator");
            } অন্যথা যদি (nodep->modVarp()->isWritable()) {
                যদি (!tryAliasPin(nodep, exprp)) {
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব rhsp = নূতন AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                             m_cellp->name(), VAccess::READ};
                    markContinuousLhs(exprp);
                    AstAssignW* ধ্রুব assp = নূতন AstAssignW{exprp->fileline(), exprp, rhsp};
                    m_cellp->addNextHere(নূতন AstAlways{assp});
                }
            } অন্যথা যদি (nodep->modVarp()->isNonOutput()) {
                যদি (!tryAliasPin(nodep, exprp)) {
                    // Don't bother moving constants now,
                    // we'll be pushing the const down to the cell soon enough.
                    AstVarXRef* ধ্রুব lhsp = নূতন AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                            m_cellp->name(), VAccess::WRITE};

                    markContinuousLhs(lhsp);
                    AstAssignW* ধ্রুব assp = নূতন AstAssignW{exprp->fileline(), lhsp, exprp};
                    m_cellp->addNextHere(নূতন AstAlways{assp});
                    UINFOTREE(9, assp, "", "_new");
                }
            } অন্যথা যদি (nodep->modVarp()->isIfaceRef()) {
                // Create an AstAliasScope for Vars to Cells so we can
                // link with their scope later
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব lhsp = নূতন AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                         m_cellp->name(), VAccess::READ};
                ধ্রুব AstVarRef* ধ্রুব refp = VN_CAST(exprp, VarRef);
                ধ্রুব AstVarXRef* ধ্রুব xrefp = VN_CAST(exprp, VarXRef);
                UASSERT_OBJ(refp || xrefp, exprp,
                            "Interfaces: Pin is not connected to a VarRef or VarXRef");
                m_cellp->addNextHere(নূতন AstAliasScope{exprp->fileline(), lhsp, exprp});
            } অন্যথা {
                nodep->v3error("Assigned pin is neither input nor output");
            }
        }

        // We're done with the pin
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
    }

    // Save some time
    শূন্য visit(Ast_গ্রন্থি_রাশিমালা*) override {}
    শূন্য visit(Ast_গ্রন্থি_Assign*) override {}
    শূন্য visit(AstAlways*) override {}

    //--------------------
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সার্বজনিক:
    // CONSTRUCTORS
    explicit InstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~InstVisitor() override = স্বভাব;
};

//######################################################################
// Inst static function

বর্গ InstStatic final {
    InstStatic() = স্বভাব;  // Static class

    স্থির Ast_গ্রন্থি_রাশিমালা* extendOrSel(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* rhsp, ধ্রুব Ast_গ্রন্থি* cmpWidthp) {
        যদি (cmpWidthp->width() > rhsp->width()) {
            rhsp = (rhsp->isSigned() ? static_cast<Ast_গ্রন্থি_রাশিমালা*>(নূতন AstExtendS{fl, rhsp})
                                     : static_cast<Ast_গ্রন্থি_রাশিমালা*>(নূতন AstExtend{fl, rhsp}));
            // Need proper widthMin, which may differ from AstSel created above
            rhsp->dtypeFrom(cmpWidthp);
        } অন্যথা যদি (cmpWidthp->width() < rhsp->width()) {
            rhsp = নূতন AstSel{fl, rhsp, 0, cmpWidthp->width()};
            // Need proper widthMin, which may differ from AstSel created above
            rhsp->dtypeFrom(cmpWidthp);
        }
        // else don't change dtype, as might be e.g. array of something
        প্রত্যাগম rhsp;
    }

সার্বজনিক:
    স্থির AstAssignW* pinReconnectSimple(AstPin* pinp, AstCell* cellp, তার্কিক forTristate,
                                          তার্কিক alwaysCvt) {
        // If a pin connection is "simple" leave it as-is
        // Else create a intermediate wire to perform the interconnect
        // Return the new assignment, if one was made
        // Note this module calls cloneTree() via new AstVar
        AstVar* ধ্রুব pinVarp = pinp->modVarp();
        যদি (!pinp->exprp()) {
            // No-connect, perhaps promote based on `unconnected_drive,
            // otherwise done
            যদি (pinVarp->direction() == VDirection::প্রবেশ
                && cellp->modp()->unconnectedDrive().isSetTrue()) {
                pinp->exprp(নূতন AstConst{pinp->fileline(), AstConst::All1{}});
            } অন্যথা যদি (pinVarp->direction() == VDirection::প্রবেশ
                       && cellp->modp()->unconnectedDrive().isSetFalse()) {
                pinp->exprp(নূতন AstConst{pinp->fileline(), AstConst::All0{}});
            } অন্যথা {
                প্রত্যাগম nullptr;
            }
        }
        ধ্রুব AstVarRef* ধ্রুব connectRefp = VN_CAST(pinp->exprp(), VarRef);
        ধ্রুব AstVarXRef* ধ্রুব connectXRefp = VN_CAST(pinp->exprp(), VarXRef);
        ধ্রুব Ast_গ্রন্থি_DType* ধ্রুব pinDTypep = pinVarp->dtypep()->skipRefp();
        ধ্রুব AstBasicDType* ধ্রুব pinBasicp = VN_CAST(pinDTypep, BasicDType);
        ধ্রুব Ast_গ্রন্থি_DType* ধ্রুব connDTypep
            = connectRefp ? connectRefp->varp()->dtypep()->skipRefp() : nullptr;
        ধ্রুব AstBasicDType* ধ্রুব connBasicp = VN_CAST(connDTypep, BasicDType);
        AstAssignW* assignp = nullptr;
        //
        যদি (!alwaysCvt && connectRefp && connDTypep->same_বৃক্ষ(pinDTypep)
            && !connectRefp->varp()->isSc()) {  // Need the signal as a 'shell' to convert types
            // Done. Same data type
        } অন্যথা যদি (!alwaysCvt && connectRefp && connectRefp->varp()->isIfaceRef()) {
            // Done. Interface
        } অন্যথা যদি (!alwaysCvt && connectXRefp && connectXRefp->varp()
                   && connectXRefp->varp()->isIfaceRef()) {
        } অন্যথা যদি (!alwaysCvt && connBasicp && pinBasicp
                   && connBasicp->width() == pinBasicp->width()
                   && connBasicp->lo() == pinBasicp->lo()
                   && !connectRefp->varp()
                           ->isSc()  // Need the signal as a 'shell' to convert types
                   && connBasicp->width() == pinVarp->width()) {
            // Done. One to one interconnect won't need a temporary variable.
        } অন্যথা যদি (!alwaysCvt && !forTristate && VN_IS(pinp->exprp(), Const)) {
            // Done. Constant. Still check for driving an output, like below.
            V3Inst::checkOutputShort(pinp);
            যদি (!pinp->exprp()) প্রত্যাগম nullptr;
        } অন্যথা {
            // Make a new temp wire
            // UINFOTREE(9, pinp, "", "in_pin");
            V3Inst::checkOutputShort(pinp);
            যদি (!pinp->exprp()) প্রত্যাগম nullptr;
            // Simplify, so stuff like '{a[0], b[0]}[1]' produced during
            // instance array expansion are brought to normal 'a[0]'
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব pinexprp
                = V3Const::constifyEdit(VN_AS(pinp->exprp(), গ্রন্থি_রাশিমালা)->unlinkFrBack());
            ধ্রুব string newvarname
                = (string{pinVarp->isWritable() ? "__Vcellout" : "__Vcellinp"}
                   // Prevent name conflict if both tri & non-tri add signals
                   + (forTristate ? "t" : "") + "__" + cellp->name() + "__" + pinp->name());
            AstVar* ধ্রুব newvarp
                = নূতন AstVar{pinVarp->fileline(), VVarType::MODULETEMP, newvarname, pinVarp};
            // Important to add statement next to cell, in case there is a
            // generate with same named cell
            cellp->addNextHere(newvarp);
            যদি (pinVarp->isInout()) {
                pinVarp->v3fatalSrc("Unsupported: Inout connections to pins must be"
                                    " direct one-to-one connection (without any expression)");
                // V3Tristate should have cleared up before this point
            } অন্যথা যদি (pinVarp->isWritable()) {
                // See also V3Inst
                Ast_গ্রন্থি_রাশিমালা* rhsp = নূতন AstVarRef{pinp->fileline(), newvarp, VAccess::READ};
                UINFO(5, "pinRecon width " << pinVarp->width() << " >? " << rhsp->width() << " >? "
                                           << pinexprp->width());
                rhsp = extendOrSel(pinp->fileline(), rhsp, pinVarp);
                pinp->exprp(নূতন AstVarRef{newvarp->fileline(), newvarp, VAccess::WRITE});
                markContinuousLhs(pinexprp);
                যদি (VN_IS(pinexprp, গ্রন্থি_Stream)) {
                    assignp = নূতন AstAssignW{pinp->fileline(), pinexprp, rhsp};
                    V3Width::streamAssignLowerEdit(assignp);
                } অন্যথা {
                    Ast_গ্রন্থি_রাশিমালা* rhsSelp = extendOrSel(pinp->fileline(), rhsp, pinexprp);
                    // A reversed array is assigned by the left-to-right order of elements
                    // (IEEE 1800-2023 7.6)
                    ধ্রুব AstUnpackArrayDType* ধ্রুব arrayp
                        = VN_CAST(pinexprp->dtypep()->skipRefp(), UnpackArrayDType);
                    যদি (arrayp && arrayp->isReverseOf(rhsSelp->dtypep())) {
                        rhsSelp = নূতন AstCvtUnpackToUnpack{pinp->fileline(), rhsSelp,
                                                           pinexprp->dtypep()};
                    }
                    assignp = নূতন AstAssignW{pinp->fileline(), pinexprp, rhsSelp};
                }
            } অন্যথা {
                // V3 width should have range/extended to make the widths correct
                newvarp->isContinuously(সত্য);
                // An input port driven from inside the cell by a clocking block
                // output flows out through the connection; V3Tristate split
                // pins resolve separately.
                Ast_গ্রন্থি_রাশিমালা* lhsp = nullptr;  // LHS of the interconnect assign
                Ast_গ্রন্থি_রাশিমালা* rhsp = nullptr;  // RHS of the interconnect assign
                যদি (forTristate || !pinVarp->isClockingDriven()) {
                    // Input-side wiring: temp fed from the connection
                    lhsp = নূতন AstVarRef{pinp->fileline(), newvarp, VAccess::WRITE};
                    rhsp = pinexprp;
                } অন্যথা যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুব drivenp = tryConnectionLhs(pinexprp)) {
                    // Reverse wiring: connection leaves fed from the temp.
                    // The original connection is consumed by the clone above.
                    VL_DO_DANGLING(pinexprp->delete_বৃক্ষ(), pinexprp);
                    markContinuousLhs(drivenp);
                    lhsp = drivenp;
                    rhsp = extendOrSel(pinp->fileline(),
                                       নূতন AstVarRef{pinp->fileline(), newvarp, VAccess::READ},
                                       drivenp);
                } অন্যথা {
                    pinp->v3error("Unsupported: Clocking block output driving input port "
                                  "through non-decomposable connection "
                                  << pinp->prettyNameQ());
                    VL_DO_DANGLING(pinexprp->delete_বৃক্ষ(), pinexprp);
                    প্রত্যাগম nullptr;
                }
                pinp->exprp(নূতন AstVarRef{pinp->fileline(), newvarp, VAccess::READ});
                assignp = নূতন AstAssignW{pinp->fileline(), lhsp, rhsp};
            }
            যদি (assignp) cellp->addNextHere(নূতন AstAlways{assignp});
            // UINFOTREE(1, pinp, "", "out");
            // UINFOTREE(1, assignp, "", "aout");
        }
        প্রত্যাগম assignp;
    }
};

//######################################################################
// Inst class functions

AstAssignW* V3Inst::pinReconnectSimple(AstPin* pinp, AstCell* cellp, তার্কিক forTristate,
                                       তার্কিক alwaysCvt) {
    প্রত্যাগম InstStatic::pinReconnectSimple(pinp, cellp, forTristate, alwaysCvt);
}

শূন্য V3Inst::checkOutputShort(ধ্রুব AstPin* nodep) {
    যদি (nodep->modVarp()->direction() == VDirection::OUTPUT) {
        যদি (VN_IS(nodep->exprp(), Const) || VN_IS(nodep->exprp(), Extend)
            || (VN_IS(nodep->exprp(), Concat)
                && (VN_IS(VN_AS(nodep->exprp(), Concat)->lhsp(), Const)))) {
            // Uses v3warn for error, as might be found multiple times
            nodep->v3warn(E_PORTSHORT, "Output port is connected to a constant pin,"
                                       " electrical short");
            // Delete so we don't create a 'CONST = ...' assignment
            nodep->exprp()->unlinkFrBack()->delete_বৃক্ষ();
        }
    }
}

//######################################################################
// Inst class visitor

শূন্য V3Inst::instAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { InstVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("inst", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
