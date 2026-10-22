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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Inst.h"

#അന്തര്ഭാവ "V3Const.h"
#അന്തര്ഭാവ "V3Control.h"
#അന്തര്ഭാവ "V3Width.h"

VL_DEFINE_DEBUG_FUNCTIONS;

സ്ഥിതം ശൂന്യം markContinuousLhs(Ast_കണ്ണി* സ്ഥിരം nodep) {
    nodep->foreach([](Ast_കണ്ണി_Var_Ref* refp) {
        എങ്കിൽ (refp->access().isWriteOrRW()) refp->varp()->isContinuously(സത്യം);
    });
}

// Flip the access of the driven leaves of a pin connection expression so it
// can be used on the LHS of an assignment. Only variables and bit/part
// selects over variables are flipped; index and select operands stay
// read-only. Struct-member selects cannot appear here: V3Width lowers them to
// selects before V3Inst. Returns false if the connection cannot be driven
// (e.g. arithmetic), in which case the expression is left unmodified.
സ്ഥിതം തർക്കം markConnectionLhs(Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം nodep) {
    എങ്കിൽ (Ast_കണ്ണി_Var_Ref* സ്ഥിരം refp = VN_CAST(nodep, കണ്ണി_Var_Ref)) {
        എങ്കിൽ (!refp->access().isReadOnly()) തിരികെ അസത്യം;
        refp->access(VAccess::WRITE);
        തിരികെ സത്യം;
    }
    // Note AstSel (bit/part select) is not an AstNodeSel (array select);
    // both expose fromp() for the selected value.
    എങ്കിൽ (AstSel* സ്ഥിരം selp = VN_CAST(nodep, Sel)) തിരികെ markConnectionLhs(selp->fromp());
    എങ്കിൽ (Ast_കണ്ണി_Sel* സ്ഥിരം selp = VN_CAST(nodep, കണ്ണി_Sel)) {
        തിരികെ markConnectionLhs(selp->fromp());
    }
    എങ്കിൽ (AstConcat* സ്ഥിരം concatp = VN_CAST(nodep, Concat)) {
        തിരികെ markConnectionLhs(concatp->lhsp()) && markConnectionLhs(concatp->rhsp());
    }
    തിരികെ അസത്യം;
}

// Prepare a pin connection expression as the LHS of the reverse (inside-out)
// assignment. Returns the flipped expression on success, nullptr if the
// connection cannot be driven. The input is left unmodified on failure.
സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* tryConnectionLhs(Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം nodep) {
    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം clonep = nodep->clone_വൃക്ഷം(അസത്യം);
    എങ്കിൽ (markConnectionLhs(clonep)) തിരികെ clonep;
    VL_DO_DANGLING(clonep->delete_വൃക്ഷം(), clonep);
    തിരികെ nullptr;
}

//######################################################################
// Inst state, as a visitor of each AstNode

വർഗ്ഗം InstVisitor final : പൊതു VNVisitor {
    // NODE STATE
    // Cleared each Cell:
    //  AstPin::user1p()        -> bool.  True if created assignment already
    സ്ഥിരം VNUser1InUse m_inuser1;

    // STATE
    AstCell* m_cellp = nullptr;  // Current cell

    // METHODS
    // If appropriate, add an AstAlias to connect the given Cell pin to the given expression.
    // Returns true if an alias was made, in which case there is nothing else to do for this pin.
    തർക്കം tryAliasPin(AstPin* nodep, Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        AstVar* സ്ഥിരം modVarp = nodep->modVarp();
        // An interface reference is aliased via AstAliasScope below, not as a variable
        എങ്കിൽ (modVarp->isIfaceRef()) തിരികെ അസത്യം;
        // Only a whole variable can be aliased, anything else needs the assignment
        AstVarRef* സ്ഥിരം refp = VN_CAST(exprp, VarRef);
        എങ്കിൽ (!refp) തിരികെ അസത്യം;
        AstVar* സ്ഥിരം വ്യഞ്ജകം_Varp = refp->varp();
        // A virtual interface port connected to a real interface can't be an alias
        എങ്കിൽ (വ്യഞ്ജകം_Varp->isIfaceRef()) തിരികെ അസത്യം;
        // A ref port must become an alias
        എങ്കിൽ (!modVarp->direction().isRef()) {
            // V3FsmDetect recognizes the registers of an fsm_register_wrapper instance via
            // the assignments of its pins, so leave those as assignments
            Ast_കണ്ണി_Module* സ്ഥിരം cellModp = m_cellp->modp();
            എങ്കിൽ (V3Control::getFsmRegisterWrapper(cellModp->origName())
                || V3Control::getFsmRegisterWrapper(cellModp->prettyDehashOrigOrName())) {
                തിരികെ അസത്യം;
            }
            // A virtual interface method call is dispatched at run time, so the body of an
            // interface reached that way must use the signals of the instance it is called
            // on, not those of whichever instance this connection happens to be made to
            എങ്കിൽ (സ്ഥിരം AstIface* സ്ഥിരം ifacep = VN_CAST(m_cellp->modp(), Iface)) {
                എങ്കിൽ (ifacep->hasVirtualRef()) തിരികെ അസത്യം;
            }
            // Forced signals must keep their own storage, the two sides can be forced separately
            എങ്കിൽ (modVarp->isForced() || വ്യഞ്ജകം_Varp->isForced()) തിരികെ അസത്യം;
            // Same for public
            എങ്കിൽ (modVarp->isSigUserRWPublic() || വ്യഞ്ജകം_Varp->isSigUserRWPublic()) തിരികെ അസത്യം;
            // V3Tristate resolved the connected net already, and drives it from the
            // resolution it built for it, so a port merged into it would be driven by
            // the resolution of the instance as well
            എങ്കിൽ (വ്യഞ്ജകം_Varp->isTristate()) തിരികെ അസത്യം;
        }
        // They will become the same variable, so propagate file-line and attributes
        വ്യഞ്ജകം_Varp->fileline()->modifyStateInherit(modVarp->fileline());
        modVarp->fileline()->modifyStateInherit(വ്യഞ്ജകം_Varp->fileline());
        വ്യഞ്ജകം_Varp->propagateAttrFrom(modVarp);
        modVarp->propagateAttrFrom(വ്യഞ്ജകം_Varp);
        refp->access(VAccess::READWRITE);
        File_വരി* സ്ഥിരം flp = exprp->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം itemsp
            = പുതിയ AstVarXRef{flp, modVarp, m_cellp->name(), VAccess::READWRITE};
        itemsp->addNext(exprp);
        m_cellp->addNextHere(പുതിയ AstAlias{flp, itemsp});
        തിരികെ സത്യം;
    }

    // VISITORS
    ശൂന്യം visit(AstCell* nodep) override {
        UINFO(4, "  CELL   " << nodep);
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        // VV*****  We reset user1p() on each cell!!!
        Ast_കണ്ണി::user1_Clear_വൃക്ഷം();
        iterateChildren(nodep);
    }

    ശൂന്യം visit(AstPin* nodep) override {
        // PIN(p,expr) -> ASSIGNW(VARXREF(p),expr)    (if sub's input)
        //            or  ASSIGNW(expr,VARXREF(p))    (if sub's output)
        UINFO(4, "   PIN  " << nodep);
        എങ്കിൽ (!nodep->user1()) {
            // Simplify it
            V3Inst::pinReconnectSimple(nodep, m_cellp, അസത്യം);
        }
        UINFOTREE(9, nodep, "", "Pin_oldb");
        എങ്കിൽ (!nodep->exprp()) തിരികെ;  // No-connect
        V3Inst::checkOutputShort(nodep);
        എങ്കിൽ (!nodep->exprp()) തിരികെ;  // Connection removed by checkOutputShort
        // Use user1p on the PIN to indicate we created an assign for this pin
        എങ്കിൽ (!nodep->user1SetOnce()) {
            // Make an ASSIGNW (expr, pin)
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = VN_AS(nodep->exprp(), കണ്ണി_വ്യഞ്ജകം)->clone_വൃക്ഷം(അസത്യം);
            UASSERT_OBJ(exprp->width() == nodep->modVarp()->width(), nodep,
                        "Width mismatch, should have been handled in pinReconnectSimple");
            എങ്കിൽ (nodep->modVarp()->isInout()) {
                nodep->v3fatalSrc("Unsupported: Verilator is a 2-state simulator");
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->modVarp()->isWritable()) {
                എങ്കിൽ (!tryAliasPin(nodep, exprp)) {
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = പുതിയ AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                             m_cellp->name(), VAccess::READ};
                    markContinuousLhs(exprp);
                    AstAssignW* സ്ഥിരം assp = പുതിയ AstAssignW{exprp->fileline(), exprp, rhsp};
                    m_cellp->addNextHere(പുതിയ AstAlways{assp});
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->modVarp()->isNonOutput()) {
                എങ്കിൽ (!tryAliasPin(nodep, exprp)) {
                    // Don't bother moving constants now,
                    // we'll be pushing the const down to the cell soon enough.
                    AstVarXRef* സ്ഥിരം lhsp = പുതിയ AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                            m_cellp->name(), VAccess::WRITE};

                    markContinuousLhs(lhsp);
                    AstAssignW* സ്ഥിരം assp = പുതിയ AstAssignW{exprp->fileline(), lhsp, exprp};
                    m_cellp->addNextHere(പുതിയ AstAlways{assp});
                    UINFOTREE(9, assp, "", "_new");
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->modVarp()->isIfaceRef()) {
                // Create an AstAliasScope for Vars to Cells so we can
                // link with their scope later
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = പുതിയ AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                         m_cellp->name(), VAccess::READ};
                സ്ഥിരം AstVarRef* സ്ഥിരം refp = VN_CAST(exprp, VarRef);
                സ്ഥിരം AstVarXRef* സ്ഥിരം xrefp = VN_CAST(exprp, VarXRef);
                UASSERT_OBJ(refp || xrefp, exprp,
                            "Interfaces: Pin is not connected to a VarRef or VarXRef");
                m_cellp->addNextHere(പുതിയ AstAliasScope{exprp->fileline(), lhsp, exprp});
            } അല്ലെങ്കിൽ {
                nodep->v3error("Assigned pin is neither input nor output");
            }
        }

        // We're done with the pin
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
    }

    // Save some time
    ശൂന്യം visit(Ast_കണ്ണി_വ്യഞ്ജകം*) override {}
    ശൂന്യം visit(Ast_കണ്ണി_Assign*) override {}
    ശൂന്യം visit(AstAlways*) override {}

    //--------------------
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit InstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~InstVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Inst static function

വർഗ്ഗം InstStatic final {
    InstStatic() = സ്ഥിരസ്ഥിതി;  // Static class

    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* extendOrSel(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp, സ്ഥിരം Ast_കണ്ണി* cmpWidthp) {
        എങ്കിൽ (cmpWidthp->width() > rhsp->width()) {
            rhsp = (rhsp->isSigned() ? static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(പുതിയ AstExtendS{fl, rhsp})
                                     : static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(പുതിയ AstExtend{fl, rhsp}));
            // Need proper widthMin, which may differ from AstSel created above
            rhsp->dtypeFrom(cmpWidthp);
        } അല്ലെങ്കിൽ എങ്കിൽ (cmpWidthp->width() < rhsp->width()) {
            rhsp = പുതിയ AstSel{fl, rhsp, 0, cmpWidthp->width()};
            // Need proper widthMin, which may differ from AstSel created above
            rhsp->dtypeFrom(cmpWidthp);
        }
        // else don't change dtype, as might be e.g. array of something
        തിരികെ rhsp;
    }

പൊതു:
    സ്ഥിതം AstAssignW* pinReconnectSimple(AstPin* pinp, AstCell* cellp, തർക്കം forTristate,
                                          തർക്കം alwaysCvt) {
        // If a pin connection is "simple" leave it as-is
        // Else create a intermediate wire to perform the interconnect
        // Return the new assignment, if one was made
        // Note this module calls cloneTree() via new AstVar
        AstVar* സ്ഥിരം pinVarp = pinp->modVarp();
        എങ്കിൽ (!pinp->exprp()) {
            // No-connect, perhaps promote based on `unconnected_drive,
            // otherwise done
            എങ്കിൽ (pinVarp->direction() == VDirection::നിവേശം
                && cellp->modp()->unconnectedDrive().isSetTrue()) {
                pinp->exprp(പുതിയ AstConst{pinp->fileline(), AstConst::All1{}});
            } അല്ലെങ്കിൽ എങ്കിൽ (pinVarp->direction() == VDirection::നിവേശം
                       && cellp->modp()->unconnectedDrive().isSetFalse()) {
                pinp->exprp(പുതിയ AstConst{pinp->fileline(), AstConst::All0{}});
            } അല്ലെങ്കിൽ {
                തിരികെ nullptr;
            }
        }
        സ്ഥിരം AstVarRef* സ്ഥിരം connectRefp = VN_CAST(pinp->exprp(), VarRef);
        സ്ഥിരം AstVarXRef* സ്ഥിരം connectXRefp = VN_CAST(pinp->exprp(), VarXRef);
        സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം pinDTypep = pinVarp->dtypep()->skipRefp();
        സ്ഥിരം AstBasicDType* സ്ഥിരം pinBasicp = VN_CAST(pinDTypep, BasicDType);
        സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം connDTypep
            = connectRefp ? connectRefp->varp()->dtypep()->skipRefp() : nullptr;
        സ്ഥിരം AstBasicDType* സ്ഥിരം connBasicp = VN_CAST(connDTypep, BasicDType);
        AstAssignW* assignp = nullptr;
        //
        എങ്കിൽ (!alwaysCvt && connectRefp && connDTypep->same_വൃക്ഷം(pinDTypep)
            && !connectRefp->varp()->isSc()) {  // Need the signal as a 'shell' to convert types
            // Done. Same data type
        } അല്ലെങ്കിൽ എങ്കിൽ (!alwaysCvt && connectRefp && connectRefp->varp()->isIfaceRef()) {
            // Done. Interface
        } അല്ലെങ്കിൽ എങ്കിൽ (!alwaysCvt && connectXRefp && connectXRefp->varp()
                   && connectXRefp->varp()->isIfaceRef()) {
        } അല്ലെങ്കിൽ എങ്കിൽ (!alwaysCvt && connBasicp && pinBasicp
                   && connBasicp->width() == pinBasicp->width()
                   && connBasicp->lo() == pinBasicp->lo()
                   && !connectRefp->varp()
                           ->isSc()  // Need the signal as a 'shell' to convert types
                   && connBasicp->width() == pinVarp->width()) {
            // Done. One to one interconnect won't need a temporary variable.
        } അല്ലെങ്കിൽ എങ്കിൽ (!alwaysCvt && !forTristate && VN_IS(pinp->exprp(), Const)) {
            // Done. Constant. Still check for driving an output, like below.
            V3Inst::checkOutputShort(pinp);
            എങ്കിൽ (!pinp->exprp()) തിരികെ nullptr;
        } അല്ലെങ്കിൽ {
            // Make a new temp wire
            // UINFOTREE(9, pinp, "", "in_pin");
            V3Inst::checkOutputShort(pinp);
            എങ്കിൽ (!pinp->exprp()) തിരികെ nullptr;
            // Simplify, so stuff like '{a[0], b[0]}[1]' produced during
            // instance array expansion are brought to normal 'a[0]'
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം pinexprp
                = V3Const::constifyEdit(VN_AS(pinp->exprp(), കണ്ണി_വ്യഞ്ജകം)->unlinkFrBack());
            സ്ഥിരം string newvarname
                = (string{pinVarp->isWritable() ? "__Vcellout" : "__Vcellinp"}
                   // Prevent name conflict if both tri & non-tri add signals
                   + (forTristate ? "t" : "") + "__" + cellp->name() + "__" + pinp->name());
            AstVar* സ്ഥിരം newvarp
                = പുതിയ AstVar{pinVarp->fileline(), VVarType::MODULETEMP, newvarname, pinVarp};
            // Important to add statement next to cell, in case there is a
            // generate with same named cell
            cellp->addNextHere(newvarp);
            എങ്കിൽ (pinVarp->isInout()) {
                pinVarp->v3fatalSrc("Unsupported: Inout connections to pins must be"
                                    " direct one-to-one connection (without any expression)");
                // V3Tristate should have cleared up before this point
            } അല്ലെങ്കിൽ എങ്കിൽ (pinVarp->isWritable()) {
                // See also V3Inst
                Ast_കണ്ണി_വ്യഞ്ജകം* rhsp = പുതിയ AstVarRef{pinp->fileline(), newvarp, VAccess::READ};
                UINFO(5, "pinRecon width " << pinVarp->width() << " >? " << rhsp->width() << " >? "
                                           << pinexprp->width());
                rhsp = extendOrSel(pinp->fileline(), rhsp, pinVarp);
                pinp->exprp(പുതിയ AstVarRef{newvarp->fileline(), newvarp, VAccess::WRITE});
                markContinuousLhs(pinexprp);
                എങ്കിൽ (VN_IS(pinexprp, കണ്ണി_Stream)) {
                    assignp = പുതിയ AstAssignW{pinp->fileline(), pinexprp, rhsp};
                    V3Width::streamAssignLowerEdit(assignp);
                } അല്ലെങ്കിൽ {
                    Ast_കണ്ണി_വ്യഞ്ജകം* rhsSelp = extendOrSel(pinp->fileline(), rhsp, pinexprp);
                    // A reversed array is assigned by the left-to-right order of elements
                    // (IEEE 1800-2023 7.6)
                    സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം arrayp
                        = VN_CAST(pinexprp->dtypep()->skipRefp(), UnpackArrayDType);
                    എങ്കിൽ (arrayp && arrayp->isReverseOf(rhsSelp->dtypep())) {
                        rhsSelp = പുതിയ AstCvtUnpackToUnpack{pinp->fileline(), rhsSelp,
                                                           pinexprp->dtypep()};
                    }
                    assignp = പുതിയ AstAssignW{pinp->fileline(), pinexprp, rhsSelp};
                }
            } അല്ലെങ്കിൽ {
                // V3 width should have range/extended to make the widths correct
                newvarp->isContinuously(സത്യം);
                // An input port driven from inside the cell by a clocking block
                // output flows out through the connection; V3Tristate split
                // pins resolve separately.
                Ast_കണ്ണി_വ്യഞ്ജകം* lhsp = nullptr;  // LHS of the interconnect assign
                Ast_കണ്ണി_വ്യഞ്ജകം* rhsp = nullptr;  // RHS of the interconnect assign
                എങ്കിൽ (forTristate || !pinVarp->isClockingDriven()) {
                    // Input-side wiring: temp fed from the connection
                    lhsp = പുതിയ AstVarRef{pinp->fileline(), newvarp, VAccess::WRITE};
                    rhsp = pinexprp;
                } അല്ലെങ്കിൽ എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം drivenp = tryConnectionLhs(pinexprp)) {
                    // Reverse wiring: connection leaves fed from the temp.
                    // The original connection is consumed by the clone above.
                    VL_DO_DANGLING(pinexprp->delete_വൃക്ഷം(), pinexprp);
                    markContinuousLhs(drivenp);
                    lhsp = drivenp;
                    rhsp = extendOrSel(pinp->fileline(),
                                       പുതിയ AstVarRef{pinp->fileline(), newvarp, VAccess::READ},
                                       drivenp);
                } അല്ലെങ്കിൽ {
                    pinp->v3error("Unsupported: Clocking block output driving input port "
                                  "through non-decomposable connection "
                                  << pinp->prettyNameQ());
                    VL_DO_DANGLING(pinexprp->delete_വൃക്ഷം(), pinexprp);
                    തിരികെ nullptr;
                }
                pinp->exprp(പുതിയ AstVarRef{pinp->fileline(), newvarp, VAccess::READ});
                assignp = പുതിയ AstAssignW{pinp->fileline(), lhsp, rhsp};
            }
            എങ്കിൽ (assignp) cellp->addNextHere(പുതിയ AstAlways{assignp});
            // UINFOTREE(1, pinp, "", "out");
            // UINFOTREE(1, assignp, "", "aout");
        }
        തിരികെ assignp;
    }
};

//######################################################################
// Inst class functions

AstAssignW* V3Inst::pinReconnectSimple(AstPin* pinp, AstCell* cellp, തർക്കം forTristate,
                                       തർക്കം alwaysCvt) {
    തിരികെ InstStatic::pinReconnectSimple(pinp, cellp, forTristate, alwaysCvt);
}

ശൂന്യം V3Inst::checkOutputShort(സ്ഥിരം AstPin* nodep) {
    എങ്കിൽ (nodep->modVarp()->direction() == VDirection::OUTPUT) {
        എങ്കിൽ (VN_IS(nodep->exprp(), Const) || VN_IS(nodep->exprp(), Extend)
            || (VN_IS(nodep->exprp(), Concat)
                && (VN_IS(VN_AS(nodep->exprp(), Concat)->lhsp(), Const)))) {
            // Uses v3warn for error, as might be found multiple times
            nodep->v3warn(E_PORTSHORT, "Output port is connected to a constant pin,"
                                       " electrical short");
            // Delete so we don't create a 'CONST = ...' assignment
            nodep->exprp()->unlinkFrBack()->delete_വൃക്ഷം();
        }
    }
}

//######################################################################
// Inst class visitor

ശൂന്യം V3Inst::instAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { InstVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("inst", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
