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

स्थिर शून्य markContinuousLhs(Ast_गाँठो* अचर nodep) {
    nodep->foreach([](Ast_गाँठो_Var_Ref* refp) {
        यदि (refp->access().isWriteOrRW()) refp->varp()->isContinuously(सत्य);
    });
}

// Flip the access of the driven leaves of a pin connection expression so it
// can be used on the LHS of an assignment. Only variables and bit/part
// selects over variables are flipped; index and select operands stay
// read-only. Struct-member selects cannot appear here: V3Width lowers them to
// selects before V3Inst. Returns false if the connection cannot be driven
// (e.g. arithmetic), in which case the expression is left unmodified.
स्थिर तार्किक markConnectionLhs(Ast_गाँठो_अभिव्यक्ति* अचर nodep) {
    यदि (Ast_गाँठो_Var_Ref* अचर refp = VN_CAST(nodep, गाँठो_Var_Ref)) {
        यदि (!refp->access().isReadOnly()) फर्काऊ असत्य;
        refp->access(VAccess::WRITE);
        फर्काऊ सत्य;
    }
    // Note AstSel (bit/part select) is not an AstNodeSel (array select);
    // both expose fromp() for the selected value.
    यदि (AstSel* अचर selp = VN_CAST(nodep, Sel)) फर्काऊ markConnectionLhs(selp->fromp());
    यदि (Ast_गाँठो_Sel* अचर selp = VN_CAST(nodep, गाँठो_Sel)) {
        फर्काऊ markConnectionLhs(selp->fromp());
    }
    यदि (AstConcat* अचर concatp = VN_CAST(nodep, Concat)) {
        फर्काऊ markConnectionLhs(concatp->lhsp()) && markConnectionLhs(concatp->rhsp());
    }
    फर्काऊ असत्य;
}

// Prepare a pin connection expression as the LHS of the reverse (inside-out)
// assignment. Returns the flipped expression on success, nullptr if the
// connection cannot be driven. The input is left unmodified on failure.
स्थिर Ast_गाँठो_अभिव्यक्ति* tryConnectionLhs(Ast_गाँठो_अभिव्यक्ति* अचर nodep) {
    Ast_गाँठो_अभिव्यक्ति* अचर clonep = nodep->clone_रूख(असत्य);
    यदि (markConnectionLhs(clonep)) फर्काऊ clonep;
    VL_DO_DANGLING(clonep->delete_रूख(), clonep);
    फर्काऊ nullptr;
}

//######################################################################
// Inst state, as a visitor of each AstNode

वर्ग InstVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // Cleared each Cell:
    //  AstPin::user1p()        -> bool.  True if created assignment already
    अचर VNUser1InUse m_inuser1;

    // STATE
    AstCell* m_cellp = nullptr;  // Current cell

    // METHODS
    // If appropriate, add an AstAlias to connect the given Cell pin to the given expression.
    // Returns true if an alias was made, in which case there is nothing else to do for this pin.
    तार्किक tryAliasPin(AstPin* nodep, Ast_गाँठो_अभिव्यक्ति* exprp) {
        AstVar* अचर modVarp = nodep->modVarp();
        // An interface reference is aliased via AstAliasScope below, not as a variable
        यदि (modVarp->isIfaceRef()) फर्काऊ असत्य;
        // Only a whole variable can be aliased, anything else needs the assignment
        AstVarRef* अचर refp = VN_CAST(exprp, VarRef);
        यदि (!refp) फर्काऊ असत्य;
        AstVar* अचर अभिव्यक्ति_Varp = refp->varp();
        // A virtual interface port connected to a real interface can't be an alias
        यदि (अभिव्यक्ति_Varp->isIfaceRef()) फर्काऊ असत्य;
        // A ref port must become an alias
        यदि (!modVarp->direction().isRef()) {
            // V3FsmDetect recognizes the registers of an fsm_register_wrapper instance via
            // the assignments of its pins, so leave those as assignments
            Ast_गाँठो_Module* अचर cellModp = m_cellp->modp();
            यदि (V3Control::getFsmRegisterWrapper(cellModp->origName())
                || V3Control::getFsmRegisterWrapper(cellModp->prettyDehashOrigOrName())) {
                फर्काऊ असत्य;
            }
            // A virtual interface method call is dispatched at run time, so the body of an
            // interface reached that way must use the signals of the instance it is called
            // on, not those of whichever instance this connection happens to be made to
            यदि (अचर AstIface* अचर ifacep = VN_CAST(m_cellp->modp(), Iface)) {
                यदि (ifacep->hasVirtualRef()) फर्काऊ असत्य;
            }
            // Forced signals must keep their own storage, the two sides can be forced separately
            यदि (modVarp->isForced() || अभिव्यक्ति_Varp->isForced()) फर्काऊ असत्य;
            // Same for public
            यदि (modVarp->isSigUserRWPublic() || अभिव्यक्ति_Varp->isSigUserRWPublic()) फर्काऊ असत्य;
            // V3Tristate resolved the connected net already, and drives it from the
            // resolution it built for it, so a port merged into it would be driven by
            // the resolution of the instance as well
            यदि (अभिव्यक्ति_Varp->isTristate()) फर्काऊ असत्य;
        }
        // They will become the same variable, so propagate file-line and attributes
        अभिव्यक्ति_Varp->fileline()->modifyStateInherit(modVarp->fileline());
        modVarp->fileline()->modifyStateInherit(अभिव्यक्ति_Varp->fileline());
        अभिव्यक्ति_Varp->propagateAttrFrom(modVarp);
        modVarp->propagateAttrFrom(अभिव्यक्ति_Varp);
        refp->access(VAccess::READWRITE);
        File_पङ्क्ति* अचर flp = exprp->fileline();
        Ast_गाँठो_अभिव्यक्ति* अचर itemsp
            = नूतन AstVarXRef{flp, modVarp, m_cellp->name(), VAccess::READWRITE};
        itemsp->addNext(exprp);
        m_cellp->addNextHere(नूतन AstAlias{flp, itemsp});
        फर्काऊ सत्य;
    }

    // VISITORS
    शून्य visit(AstCell* nodep) override {
        UINFO(4, "  CELL   " << nodep);
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        // VV*****  We reset user1p() on each cell!!!
        Ast_गाँठो::user1_Clear_रूख();
        iterateChildren(nodep);
    }

    शून्य visit(AstPin* nodep) override {
        // PIN(p,expr) -> ASSIGNW(VARXREF(p),expr)    (if sub's input)
        //            or  ASSIGNW(expr,VARXREF(p))    (if sub's output)
        UINFO(4, "   PIN  " << nodep);
        यदि (!nodep->user1()) {
            // Simplify it
            V3Inst::pinReconnectSimple(nodep, m_cellp, असत्य);
        }
        UINFOTREE(9, nodep, "", "Pin_oldb");
        यदि (!nodep->exprp()) फर्काऊ;  // No-connect
        V3Inst::checkOutputShort(nodep);
        यदि (!nodep->exprp()) फर्काऊ;  // Connection removed by checkOutputShort
        // Use user1p on the PIN to indicate we created an assign for this pin
        यदि (!nodep->user1SetOnce()) {
            // Make an ASSIGNW (expr, pin)
            Ast_गाँठो_अभिव्यक्ति* अचर exprp = VN_AS(nodep->exprp(), गाँठो_अभिव्यक्ति)->clone_रूख(असत्य);
            UASSERT_OBJ(exprp->width() == nodep->modVarp()->width(), nodep,
                        "Width mismatch, should have been handled in pinReconnectSimple");
            यदि (nodep->modVarp()->isInout()) {
                nodep->v3fatalSrc("Unsupported: Verilator is a 2-state simulator");
            } अन्यथा यदि (nodep->modVarp()->isWritable()) {
                यदि (!tryAliasPin(nodep, exprp)) {
                    Ast_गाँठो_अभिव्यक्ति* अचर rhsp = नूतन AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                             m_cellp->name(), VAccess::READ};
                    markContinuousLhs(exprp);
                    AstAssignW* अचर assp = नूतन AstAssignW{exprp->fileline(), exprp, rhsp};
                    m_cellp->addNextHere(नूतन AstAlways{assp});
                }
            } अन्यथा यदि (nodep->modVarp()->isNonOutput()) {
                यदि (!tryAliasPin(nodep, exprp)) {
                    // Don't bother moving constants now,
                    // we'll be pushing the const down to the cell soon enough.
                    AstVarXRef* अचर lhsp = नूतन AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                            m_cellp->name(), VAccess::WRITE};

                    markContinuousLhs(lhsp);
                    AstAssignW* अचर assp = नूतन AstAssignW{exprp->fileline(), lhsp, exprp};
                    m_cellp->addNextHere(नूतन AstAlways{assp});
                    UINFOTREE(9, assp, "", "_new");
                }
            } अन्यथा यदि (nodep->modVarp()->isIfaceRef()) {
                // Create an AstAliasScope for Vars to Cells so we can
                // link with their scope later
                Ast_गाँठो_अभिव्यक्ति* अचर lhsp = नूतन AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                         m_cellp->name(), VAccess::READ};
                अचर AstVarRef* अचर refp = VN_CAST(exprp, VarRef);
                अचर AstVarXRef* अचर xrefp = VN_CAST(exprp, VarXRef);
                UASSERT_OBJ(refp || xrefp, exprp,
                            "Interfaces: Pin is not connected to a VarRef or VarXRef");
                m_cellp->addNextHere(नूतन AstAliasScope{exprp->fileline(), lhsp, exprp});
            } अन्यथा {
                nodep->v3error("Assigned pin is neither input nor output");
            }
        }

        // We're done with the pin
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_रूख(), nodep);
    }

    // Save some time
    शून्य visit(Ast_गाँठो_अभिव्यक्ति*) override {}
    शून्य visit(Ast_गाँठो_Assign*) override {}
    शून्य visit(AstAlways*) override {}

    //--------------------
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit InstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~InstVisitor() override = स्वभाव;
};

//######################################################################
// Inst static function

वर्ग InstStatic final {
    InstStatic() = स्वभाव;  // Static class

    स्थिर Ast_गाँठो_अभिव्यक्ति* extendOrSel(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* rhsp, अचर Ast_गाँठो* cmpWidthp) {
        यदि (cmpWidthp->width() > rhsp->width()) {
            rhsp = (rhsp->isSigned() ? static_cast<Ast_गाँठो_अभिव्यक्ति*>(नूतन AstExtendS{fl, rhsp})
                                     : static_cast<Ast_गाँठो_अभिव्यक्ति*>(नूतन AstExtend{fl, rhsp}));
            // Need proper widthMin, which may differ from AstSel created above
            rhsp->dtypeFrom(cmpWidthp);
        } अन्यथा यदि (cmpWidthp->width() < rhsp->width()) {
            rhsp = नूतन AstSel{fl, rhsp, 0, cmpWidthp->width()};
            // Need proper widthMin, which may differ from AstSel created above
            rhsp->dtypeFrom(cmpWidthp);
        }
        // else don't change dtype, as might be e.g. array of something
        फर्काऊ rhsp;
    }

सार्वजनिक:
    स्थिर AstAssignW* pinReconnectSimple(AstPin* pinp, AstCell* cellp, तार्किक forTristate,
                                          तार्किक alwaysCvt) {
        // If a pin connection is "simple" leave it as-is
        // Else create a intermediate wire to perform the interconnect
        // Return the new assignment, if one was made
        // Note this module calls cloneTree() via new AstVar
        AstVar* अचर pinVarp = pinp->modVarp();
        यदि (!pinp->exprp()) {
            // No-connect, perhaps promote based on `unconnected_drive,
            // otherwise done
            यदि (pinVarp->direction() == VDirection::आगत
                && cellp->modp()->unconnectedDrive().isSetTrue()) {
                pinp->exprp(नूतन AstConst{pinp->fileline(), AstConst::All1{}});
            } अन्यथा यदि (pinVarp->direction() == VDirection::आगत
                       && cellp->modp()->unconnectedDrive().isSetFalse()) {
                pinp->exprp(नूतन AstConst{pinp->fileline(), AstConst::All0{}});
            } अन्यथा {
                फर्काऊ nullptr;
            }
        }
        अचर AstVarRef* अचर connectRefp = VN_CAST(pinp->exprp(), VarRef);
        अचर AstVarXRef* अचर connectXRefp = VN_CAST(pinp->exprp(), VarXRef);
        अचर Ast_गाँठो_DType* अचर pinDTypep = pinVarp->dtypep()->skipRefp();
        अचर AstBasicDType* अचर pinBasicp = VN_CAST(pinDTypep, BasicDType);
        अचर Ast_गाँठो_DType* अचर connDTypep
            = connectRefp ? connectRefp->varp()->dtypep()->skipRefp() : nullptr;
        अचर AstBasicDType* अचर connBasicp = VN_CAST(connDTypep, BasicDType);
        AstAssignW* assignp = nullptr;
        //
        यदि (!alwaysCvt && connectRefp && connDTypep->same_रूख(pinDTypep)
            && !connectRefp->varp()->isSc()) {  // Need the signal as a 'shell' to convert types
            // Done. Same data type
        } अन्यथा यदि (!alwaysCvt && connectRefp && connectRefp->varp()->isIfaceRef()) {
            // Done. Interface
        } अन्यथा यदि (!alwaysCvt && connectXRefp && connectXRefp->varp()
                   && connectXRefp->varp()->isIfaceRef()) {
        } अन्यथा यदि (!alwaysCvt && connBasicp && pinBasicp
                   && connBasicp->width() == pinBasicp->width()
                   && connBasicp->lo() == pinBasicp->lo()
                   && !connectRefp->varp()
                           ->isSc()  // Need the signal as a 'shell' to convert types
                   && connBasicp->width() == pinVarp->width()) {
            // Done. One to one interconnect won't need a temporary variable.
        } अन्यथा यदि (!alwaysCvt && !forTristate && VN_IS(pinp->exprp(), Const)) {
            // Done. Constant. Still check for driving an output, like below.
            V3Inst::checkOutputShort(pinp);
            यदि (!pinp->exprp()) फर्काऊ nullptr;
        } अन्यथा {
            // Make a new temp wire
            // UINFOTREE(9, pinp, "", "in_pin");
            V3Inst::checkOutputShort(pinp);
            यदि (!pinp->exprp()) फर्काऊ nullptr;
            // Simplify, so stuff like '{a[0], b[0]}[1]' produced during
            // instance array expansion are brought to normal 'a[0]'
            Ast_गाँठो_अभिव्यक्ति* अचर pinexprp
                = V3Const::constifyEdit(VN_AS(pinp->exprp(), गाँठो_अभिव्यक्ति)->unlinkFrBack());
            अचर string newvarname
                = (string{pinVarp->isWritable() ? "__Vcellout" : "__Vcellinp"}
                   // Prevent name conflict if both tri & non-tri add signals
                   + (forTristate ? "t" : "") + "__" + cellp->name() + "__" + pinp->name());
            AstVar* अचर newvarp
                = नूतन AstVar{pinVarp->fileline(), VVarType::MODULETEMP, newvarname, pinVarp};
            // Important to add statement next to cell, in case there is a
            // generate with same named cell
            cellp->addNextHere(newvarp);
            यदि (pinVarp->isInout()) {
                pinVarp->v3fatalSrc("Unsupported: Inout connections to pins must be"
                                    " direct one-to-one connection (without any expression)");
                // V3Tristate should have cleared up before this point
            } अन्यथा यदि (pinVarp->isWritable()) {
                // See also V3Inst
                Ast_गाँठो_अभिव्यक्ति* rhsp = नूतन AstVarRef{pinp->fileline(), newvarp, VAccess::READ};
                UINFO(5, "pinRecon width " << pinVarp->width() << " >? " << rhsp->width() << " >? "
                                           << pinexprp->width());
                rhsp = extendOrSel(pinp->fileline(), rhsp, pinVarp);
                pinp->exprp(नूतन AstVarRef{newvarp->fileline(), newvarp, VAccess::WRITE});
                markContinuousLhs(pinexprp);
                यदि (VN_IS(pinexprp, गाँठो_Stream)) {
                    assignp = नूतन AstAssignW{pinp->fileline(), pinexprp, rhsp};
                    V3Width::streamAssignLowerEdit(assignp);
                } अन्यथा {
                    Ast_गाँठो_अभिव्यक्ति* rhsSelp = extendOrSel(pinp->fileline(), rhsp, pinexprp);
                    // A reversed array is assigned by the left-to-right order of elements
                    // (IEEE 1800-2023 7.6)
                    अचर AstUnpackArrayDType* अचर arrayp
                        = VN_CAST(pinexprp->dtypep()->skipRefp(), UnpackArrayDType);
                    यदि (arrayp && arrayp->isReverseOf(rhsSelp->dtypep())) {
                        rhsSelp = नूतन AstCvtUnpackToUnpack{pinp->fileline(), rhsSelp,
                                                           pinexprp->dtypep()};
                    }
                    assignp = नूतन AstAssignW{pinp->fileline(), pinexprp, rhsSelp};
                }
            } अन्यथा {
                // V3 width should have range/extended to make the widths correct
                newvarp->isContinuously(सत्य);
                // An input port driven from inside the cell by a clocking block
                // output flows out through the connection; V3Tristate split
                // pins resolve separately.
                Ast_गाँठो_अभिव्यक्ति* lhsp = nullptr;  // LHS of the interconnect assign
                Ast_गाँठो_अभिव्यक्ति* rhsp = nullptr;  // RHS of the interconnect assign
                यदि (forTristate || !pinVarp->isClockingDriven()) {
                    // Input-side wiring: temp fed from the connection
                    lhsp = नूतन AstVarRef{pinp->fileline(), newvarp, VAccess::WRITE};
                    rhsp = pinexprp;
                } अन्यथा यदि (Ast_गाँठो_अभिव्यक्ति* अचर drivenp = tryConnectionLhs(pinexprp)) {
                    // Reverse wiring: connection leaves fed from the temp.
                    // The original connection is consumed by the clone above.
                    VL_DO_DANGLING(pinexprp->delete_रूख(), pinexprp);
                    markContinuousLhs(drivenp);
                    lhsp = drivenp;
                    rhsp = extendOrSel(pinp->fileline(),
                                       नूतन AstVarRef{pinp->fileline(), newvarp, VAccess::READ},
                                       drivenp);
                } अन्यथा {
                    pinp->v3error("Unsupported: Clocking block output driving input port "
                                  "through non-decomposable connection "
                                  << pinp->prettyNameQ());
                    VL_DO_DANGLING(pinexprp->delete_रूख(), pinexprp);
                    फर्काऊ nullptr;
                }
                pinp->exprp(नूतन AstVarRef{pinp->fileline(), newvarp, VAccess::READ});
                assignp = नूतन AstAssignW{pinp->fileline(), lhsp, rhsp};
            }
            यदि (assignp) cellp->addNextHere(नूतन AstAlways{assignp});
            // UINFOTREE(1, pinp, "", "out");
            // UINFOTREE(1, assignp, "", "aout");
        }
        फर्काऊ assignp;
    }
};

//######################################################################
// Inst class functions

AstAssignW* V3Inst::pinReconnectSimple(AstPin* pinp, AstCell* cellp, तार्किक forTristate,
                                       तार्किक alwaysCvt) {
    फर्काऊ InstStatic::pinReconnectSimple(pinp, cellp, forTristate, alwaysCvt);
}

शून्य V3Inst::checkOutputShort(अचर AstPin* nodep) {
    यदि (nodep->modVarp()->direction() == VDirection::OUTPUT) {
        यदि (VN_IS(nodep->exprp(), Const) || VN_IS(nodep->exprp(), Extend)
            || (VN_IS(nodep->exprp(), Concat)
                && (VN_IS(VN_AS(nodep->exprp(), Concat)->lhsp(), Const)))) {
            // Uses v3warn for error, as might be found multiple times
            nodep->v3warn(E_PORTSHORT, "Output port is connected to a constant pin,"
                                       " electrical short");
            // Delete so we don't create a 'CONST = ...' assignment
            nodep->exprp()->unlinkFrBack()->delete_रूख();
        }
    }
}

//######################################################################
// Inst class visitor

शून्य V3Inst::instAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { InstVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_रूख("inst", 0, dump_रूख_Either_Level() >= 3);
}
