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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Inst.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Const.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Control.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Width.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ਸਥਿਰ ਖਾਲੀ markContinuousLhs(Ast_ਗੰਢ* ਅਚਲ nodep) {
    nodep->foreach([](Ast_ਗੰਢ_Var_Ref* refp) {
        ਜੇ (refp->access().isWriteOrRW()) refp->varp()->isContinuously(ਸੱਚ);
    });
}

// Flip the access of the driven leaves of a pin connection expression so it
// can be used on the LHS of an assignment. Only variables and bit/part
// selects over variables are flipped; index and select operands stay
// read-only. Struct-member selects cannot appear here: V3Width lowers them to
// selects before V3Inst. Returns false if the connection cannot be driven
// (e.g. arithmetic), in which case the expression is left unmodified.
ਸਥਿਰ ਤਾਰਕਿਕ markConnectionLhs(Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ nodep) {
    ਜੇ (Ast_ਗੰਢ_Var_Ref* ਅਚਲ refp = VN_CAST(nodep, ਗੰਢ_Var_Ref)) {
        ਜੇ (!refp->access().isReadOnly()) ਵਾਪਸ ਝੂਠ;
        refp->access(VAccess::WRITE);
        ਵਾਪਸ ਸੱਚ;
    }
    // Note AstSel (bit/part select) is not an AstNodeSel (array select);
    // both expose fromp() for the selected value.
    ਜੇ (AstSel* ਅਚਲ selp = VN_CAST(nodep, Sel)) ਵਾਪਸ markConnectionLhs(selp->fromp());
    ਜੇ (Ast_ਗੰਢ_Sel* ਅਚਲ selp = VN_CAST(nodep, ਗੰਢ_Sel)) {
        ਵਾਪਸ markConnectionLhs(selp->fromp());
    }
    ਜੇ (AstConcat* ਅਚਲ concatp = VN_CAST(nodep, Concat)) {
        ਵਾਪਸ markConnectionLhs(concatp->lhsp()) && markConnectionLhs(concatp->rhsp());
    }
    ਵਾਪਸ ਝੂਠ;
}

// Prepare a pin connection expression as the LHS of the reverse (inside-out)
// assignment. Returns the flipped expression on success, nullptr if the
// connection cannot be driven. The input is left unmodified on failure.
ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* tryConnectionLhs(Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ nodep) {
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ clonep = nodep->clone_ਰੁੱਖ(ਝੂਠ);
    ਜੇ (markConnectionLhs(clonep)) ਵਾਪਸ clonep;
    VL_DO_DANGLING(clonep->delete_ਰੁੱਖ(), clonep);
    ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
}

//######################################################################
// Inst state, as a visitor of each AstNode

ਵਰਗ InstVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    // Cleared each Cell:
    //  AstPin::user1p()        -> bool.  True if created assignment already
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;

    // STATE
    AstCell* m_cellp = ਖਾਲੀ_ਸੂਚਕ;  // Current cell

    // METHODS
    // If appropriate, add an AstAlias to connect the given Cell pin to the given expression.
    // Returns true if an alias was made, in which case there is nothing else to do for this pin.
    ਤਾਰਕਿਕ tryAliasPin(AstPin* nodep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        AstVar* ਅਚਲ modVarp = nodep->modVarp();
        // An interface reference is aliased via AstAliasScope below, not as a variable
        ਜੇ (modVarp->isIfaceRef()) ਵਾਪਸ ਝੂਠ;
        // Only a whole variable can be aliased, anything else needs the assignment
        AstVarRef* ਅਚਲ refp = VN_CAST(exprp, VarRef);
        ਜੇ (!refp) ਵਾਪਸ ਝੂਠ;
        AstVar* ਅਚਲ ਵਿਅੰਜਕ_Varp = refp->varp();
        // A virtual interface port connected to a real interface can't be an alias
        ਜੇ (ਵਿਅੰਜਕ_Varp->isIfaceRef()) ਵਾਪਸ ਝੂਠ;
        // A ref port must become an alias
        ਜੇ (!modVarp->direction().isRef()) {
            // V3FsmDetect recognizes the registers of an fsm_register_wrapper instance via
            // the assignments of its pins, so leave those as assignments
            Ast_ਗੰਢ_Module* ਅਚਲ cellModp = m_cellp->modp();
            ਜੇ (V3Control::get_Fsm_ਰਜਿਸਟਰ_Wrapper(cellModp->origName())
                || V3Control::get_Fsm_ਰਜਿਸਟਰ_Wrapper(cellModp->prettyDehashOrigOrName())) {
                ਵਾਪਸ ਝੂਠ;
            }
            // A virtual interface method call is dispatched at run time, so the body of an
            // interface reached that way must use the signals of the instance it is called
            // on, not those of whichever instance this connection happens to be made to
            ਜੇ (ਅਚਲ AstIface* ਅਚਲ ifacep = VN_CAST(m_cellp->modp(), Iface)) {
                ਜੇ (ifacep->hasVirtualRef()) ਵਾਪਸ ਝੂਠ;
            }
            // Forced signals must keep their own storage, the two sides can be forced separately
            ਜੇ (modVarp->isForced() || ਵਿਅੰਜਕ_Varp->isForced()) ਵਾਪਸ ਝੂਠ;
            // Same for public
            ਜੇ (modVarp->isSigUserRWPublic() || ਵਿਅੰਜਕ_Varp->isSigUserRWPublic()) ਵਾਪਸ ਝੂਠ;
            // V3Tristate resolved the connected net already, and drives it from the
            // resolution it built for it, so a port merged into it would be driven by
            // the resolution of the instance as well
            ਜੇ (ਵਿਅੰਜਕ_Varp->isTristate()) ਵਾਪਸ ਝੂਠ;
        }
        // They will become the same variable, so propagate file-line and attributes
        ਵਿਅੰਜਕ_Varp->fileline()->modifyStateInherit(modVarp->fileline());
        modVarp->fileline()->modifyStateInherit(ਵਿਅੰਜਕ_Varp->fileline());
        ਵਿਅੰਜਕ_Varp->propagateAttrFrom(modVarp);
        modVarp->propagateAttrFrom(ਵਿਅੰਜਕ_Varp);
        refp->access(VAccess::READWRITE);
        File_ਸਤਰ* ਅਚਲ flp = exprp->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ itemsp
            = ਨਵਾਂ AstVarXRef{flp, modVarp, m_cellp->name(), VAccess::READWRITE};
        itemsp->ਜੋੜੋ_ਅਗਲਾ(exprp);
        m_cellp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(ਨਵਾਂ AstAlias{flp, itemsp});
        ਵਾਪਸ ਸੱਚ;
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstCell* nodep) override {
        UINFO(4, "  CELL   " << nodep);
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        // VV*****  We reset user1p() on each cell!!!
        Ast_ਗੰਢ::user1_Clear_ਰੁੱਖ();
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstPin* nodep) override {
        // PIN(p,expr) -> ASSIGNW(VARXREF(p),expr)    (if sub's input)
        //            or  ASSIGNW(expr,VARXREF(p))    (if sub's output)
        UINFO(4, "   PIN  " << nodep);
        ਜੇ (!nodep->user1()) {
            // Simplify it
            V3Inst::pinReconnectSimple(nodep, m_cellp, ਝੂਠ);
        }
        UINFOTREE(9, nodep, "", "Pin_oldb");
        ਜੇ (!nodep->exprp()) ਵਾਪਸ;  // No-connect
        V3Inst::checkOutputShort(nodep);
        ਜੇ (!nodep->exprp()) ਵਾਪਸ;  // Connection removed by checkOutputShort
        // Use user1p on the PIN to indicate we created an assign for this pin
        ਜੇ (!nodep->user1_ਸੈੱਟ_Once()) {
            // Make an ASSIGNW (expr, pin)
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = VN_AS(nodep->exprp(), ਗੰਢ_ਵਿਅੰਜਕ)->clone_ਰੁੱਖ(ਝੂਠ);
            UASSERT_OBJ(exprp->ਚੌੜਾਈ() == nodep->modVarp()->ਚੌੜਾਈ(), nodep,
                        "Width mismatch, should have been handled in pinReconnectSimple");
            ਜੇ (nodep->modVarp()->isInout()) {
                nodep->v3fatalSrc("Unsupported: Verilator is a 2-state simulator");
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->modVarp()->isWritable()) {
                ਜੇ (!tryAliasPin(nodep, exprp)) {
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = ਨਵਾਂ AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                             m_cellp->name(), VAccess::READ};
                    markContinuousLhs(exprp);
                    Ast_ਸੌਂਪੋ_W* ਅਚਲ assp = ਨਵਾਂ Ast_ਸੌਂਪੋ_W{exprp->fileline(), exprp, rhsp};
                    m_cellp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(ਨਵਾਂ AstAlways{assp});
                }
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->modVarp()->isNonOutput()) {
                ਜੇ (!tryAliasPin(nodep, exprp)) {
                    // Don't bother moving constants now,
                    // we'll be pushing the const down to the cell soon enough.
                    AstVarXRef* ਅਚਲ lhsp = ਨਵਾਂ AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                            m_cellp->name(), VAccess::WRITE};

                    markContinuousLhs(lhsp);
                    Ast_ਸੌਂਪੋ_W* ਅਚਲ assp = ਨਵਾਂ Ast_ਸੌਂਪੋ_W{exprp->fileline(), lhsp, exprp};
                    m_cellp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(ਨਵਾਂ AstAlways{assp});
                    UINFOTREE(9, assp, "", "_new");
                }
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->modVarp()->isIfaceRef()) {
                // Create an AstAliasScope for Vars to Cells so we can
                // link with their scope later
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = ਨਵਾਂ AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                         m_cellp->name(), VAccess::READ};
                ਅਚਲ AstVarRef* ਅਚਲ refp = VN_CAST(exprp, VarRef);
                ਅਚਲ AstVarXRef* ਅਚਲ xrefp = VN_CAST(exprp, VarXRef);
                UASSERT_OBJ(refp || xrefp, exprp,
                            "Interfaces: Pin is not connected to a VarRef or VarXRef");
                m_cellp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(ਨਵਾਂ AstAliasScope{exprp->fileline(), lhsp, exprp});
            } ਨਹੀਂਤਾਂ {
                nodep->v3error("Assigned pin is neither input nor output");
            }
        }

        // We're done with the pin
        VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
    }

    // Save some time
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਵਿਅੰਜਕ*) override {}
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਸੌਂਪੋ*) override {}
    ਖਾਲੀ ਦੇਖੋ(AstAlways*) override {}

    //--------------------
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ InstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~InstVisitor() override = ਮੂਲ;
};

//######################################################################
// Inst static function

ਵਰਗ InstStatic final {
    InstStatic() = ਮੂਲ;  // Static class

    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* extendOrSel(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp, ਅਚਲ Ast_ਗੰਢ* cmpWidthp) {
        ਜੇ (cmpWidthp->ਚੌੜਾਈ() > rhsp->ਚੌੜਾਈ()) {
            rhsp = (rhsp->isSigned() ? static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(ਨਵਾਂ AstExtendS{fl, rhsp})
                                     : static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(ਨਵਾਂ AstExtend{fl, rhsp}));
            // Need proper widthMin, which may differ from AstSel created above
            rhsp->dtypeFrom(cmpWidthp);
        } ਨਹੀਂਤਾਂ ਜੇ (cmpWidthp->ਚੌੜਾਈ() < rhsp->ਚੌੜਾਈ()) {
            rhsp = ਨਵਾਂ AstSel{fl, rhsp, 0, cmpWidthp->ਚੌੜਾਈ()};
            // Need proper widthMin, which may differ from AstSel created above
            rhsp->dtypeFrom(cmpWidthp);
        }
        // else don't change dtype, as might be e.g. array of something
        ਵਾਪਸ rhsp;
    }

ਜਨਤਕ:
    ਸਥਿਰ Ast_ਸੌਂਪੋ_W* pinReconnectSimple(AstPin* pinp, AstCell* cellp, ਤਾਰਕਿਕ forTristate,
                                          ਤਾਰਕਿਕ alwaysCvt) {
        // If a pin connection is "simple" leave it as-is
        // Else create a intermediate wire to perform the interconnect
        // Return the new assignment, if one was made
        // Note this module calls cloneTree() via new AstVar
        AstVar* ਅਚਲ pinVarp = pinp->modVarp();
        ਜੇ (!pinp->exprp()) {
            // No-connect, perhaps promote based on `unconnected_drive,
            // otherwise done
            ਜੇ (pinVarp->direction() == VDirection::ਨਿਵੇਸ਼
                && cellp->modp()->unconnectedDrive().is_ਸੈੱਟ_True()) {
                pinp->exprp(ਨਵਾਂ AstConst{pinp->fileline(), AstConst::All1{}});
            } ਨਹੀਂਤਾਂ ਜੇ (pinVarp->direction() == VDirection::ਨਿਵੇਸ਼
                       && cellp->modp()->unconnectedDrive().is_ਸੈੱਟ_False()) {
                pinp->exprp(ਨਵਾਂ AstConst{pinp->fileline(), AstConst::All0{}});
            } ਨਹੀਂਤਾਂ {
                ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
            }
        }
        ਅਚਲ AstVarRef* ਅਚਲ connectRefp = VN_CAST(pinp->exprp(), VarRef);
        ਅਚਲ AstVarXRef* ਅਚਲ connectXRefp = VN_CAST(pinp->exprp(), VarXRef);
        ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ pinDTypep = pinVarp->dtypep()->skipRefp();
        ਅਚਲ AstBasicDType* ਅਚਲ pinBasicp = VN_CAST(pinDTypep, BasicDType);
        ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ connDTypep
            = connectRefp ? connectRefp->varp()->dtypep()->skipRefp() : ਖਾਲੀ_ਸੂਚਕ;
        ਅਚਲ AstBasicDType* ਅਚਲ connBasicp = VN_CAST(connDTypep, BasicDType);
        Ast_ਸੌਂਪੋ_W* assignp = ਖਾਲੀ_ਸੂਚਕ;
        //
        ਜੇ (!alwaysCvt && connectRefp && connDTypep->same_ਰੁੱਖ(pinDTypep)
            && !connectRefp->varp()->isSc()) {  // Need the signal as a 'shell' to convert types
            // Done. Same data type
        } ਨਹੀਂਤਾਂ ਜੇ (!alwaysCvt && connectRefp && connectRefp->varp()->isIfaceRef()) {
            // Done. Interface
        } ਨਹੀਂਤਾਂ ਜੇ (!alwaysCvt && connectXRefp && connectXRefp->varp()
                   && connectXRefp->varp()->isIfaceRef()) {
        } ਨਹੀਂਤਾਂ ਜੇ (!alwaysCvt && connBasicp && pinBasicp
                   && connBasicp->ਚੌੜਾਈ() == pinBasicp->ਚੌੜਾਈ()
                   && connBasicp->lo() == pinBasicp->lo()
                   && !connectRefp->varp()
                           ->isSc()  // Need the signal as a 'shell' to convert types
                   && connBasicp->ਚੌੜਾਈ() == pinVarp->ਚੌੜਾਈ()) {
            // Done. One to one interconnect won't need a temporary variable.
        } ਨਹੀਂਤਾਂ ਜੇ (!alwaysCvt && !forTristate && VN_IS(pinp->exprp(), Const)) {
            // Done. Constant. Still check for driving an output, like below.
            V3Inst::checkOutputShort(pinp);
            ਜੇ (!pinp->exprp()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        } ਨਹੀਂਤਾਂ {
            // Make a new temp wire
            // UINFOTREE(9, pinp, "", "in_pin");
            V3Inst::checkOutputShort(pinp);
            ਜੇ (!pinp->exprp()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
            // Simplify, so stuff like '{a[0], b[0]}[1]' produced during
            // instance array expansion are brought to normal 'a[0]'
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ pinexprp
                = V3Const::constifyEdit(VN_AS(pinp->exprp(), ਗੰਢ_ਵਿਅੰਜਕ)->unlink_Fr_ਪਿੱਠ());
            ਅਚਲ string newvarname
                = (string{pinVarp->isWritable() ? "__Vcellout" : "__Vcellinp"}
                   // Prevent name conflict if both tri & non-tri add signals
                   + (forTristate ? "t" : "") + "__" + cellp->name() + "__" + pinp->name());
            AstVar* ਅਚਲ newvarp
                = ਨਵਾਂ AstVar{pinVarp->fileline(), VVarType::MODULETEMP, newvarname, pinVarp};
            // Important to add statement next to cell, in case there is a
            // generate with same named cell
            cellp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(newvarp);
            ਜੇ (pinVarp->isInout()) {
                pinVarp->v3fatalSrc("Unsupported: Inout connections to pins must be"
                                    " direct one-to-one connection (without any expression)");
                // V3Tristate should have cleared up before this point
            } ਨਹੀਂਤਾਂ ਜੇ (pinVarp->isWritable()) {
                // See also V3Inst
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp = ਨਵਾਂ AstVarRef{pinp->fileline(), newvarp, VAccess::READ};
                UINFO(5, "pinRecon width " << pinVarp->ਚੌੜਾਈ() << " >? " << rhsp->ਚੌੜਾਈ() << " >? "
                                           << pinexprp->ਚੌੜਾਈ());
                rhsp = extendOrSel(pinp->fileline(), rhsp, pinVarp);
                pinp->exprp(ਨਵਾਂ AstVarRef{newvarp->fileline(), newvarp, VAccess::WRITE});
                markContinuousLhs(pinexprp);
                ਜੇ (VN_IS(pinexprp, ਗੰਢ_ਧਾਰਾ)) {
                    assignp = ਨਵਾਂ Ast_ਸੌਂਪੋ_W{pinp->fileline(), pinexprp, rhsp};
                    V3_ਚੌੜਾਈ::ਧਾਰਾ_ਸੌਂਪੋ_ਹੇਠਲਾ_Edit(assignp);
                } ਨਹੀਂਤਾਂ {
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsSelp = extendOrSel(pinp->fileline(), rhsp, pinexprp);
                    // A reversed array is assigned by the left-to-right order of elements
                    // (IEEE 1800-2023 7.6)
                    ਅਚਲ AstUnpackArrayDType* ਅਚਲ arrayp
                        = VN_CAST(pinexprp->dtypep()->skipRefp(), UnpackArrayDType);
                    ਜੇ (arrayp && arrayp->is_ਉਲਟ_ਦਾ(rhsSelp->dtypep())) {
                        rhsSelp = ਨਵਾਂ Ast_Cvt_Unpack_ਮੰਜ਼ਿਲ_Unpack{pinp->fileline(), rhsSelp,
                                                           pinexprp->dtypep()};
                    }
                    assignp = ਨਵਾਂ Ast_ਸੌਂਪੋ_W{pinp->fileline(), pinexprp, rhsSelp};
                }
            } ਨਹੀਂਤਾਂ {
                // V3 width should have range/extended to make the widths correct
                newvarp->isContinuously(ਸੱਚ);
                // An input port driven from inside the cell by a clocking block
                // output flows out through the connection; V3Tristate split
                // pins resolve separately.
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp = ਖਾਲੀ_ਸੂਚਕ;  // LHS of the interconnect assign
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp = ਖਾਲੀ_ਸੂਚਕ;  // RHS of the interconnect assign
                ਜੇ (forTristate || !pinVarp->isClockingDriven()) {
                    // Input-side wiring: temp fed from the connection
                    lhsp = ਨਵਾਂ AstVarRef{pinp->fileline(), newvarp, VAccess::WRITE};
                    rhsp = pinexprp;
                } ਨਹੀਂਤਾਂ ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ drivenp = tryConnectionLhs(pinexprp)) {
                    // Reverse wiring: connection leaves fed from the temp.
                    // The original connection is consumed by the clone above.
                    VL_DO_DANGLING(pinexprp->delete_ਰੁੱਖ(), pinexprp);
                    markContinuousLhs(drivenp);
                    lhsp = drivenp;
                    rhsp = extendOrSel(pinp->fileline(),
                                       ਨਵਾਂ AstVarRef{pinp->fileline(), newvarp, VAccess::READ},
                                       drivenp);
                } ਨਹੀਂਤਾਂ {
                    pinp->v3error("Unsupported: Clocking block output driving input port "
                                  "through non-decomposable connection "
                                  << pinp->prettyNameQ());
                    VL_DO_DANGLING(pinexprp->delete_ਰੁੱਖ(), pinexprp);
                    ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
                }
                pinp->exprp(ਨਵਾਂ AstVarRef{pinp->fileline(), newvarp, VAccess::READ});
                assignp = ਨਵਾਂ Ast_ਸੌਂਪੋ_W{pinp->fileline(), lhsp, rhsp};
            }
            ਜੇ (assignp) cellp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(ਨਵਾਂ AstAlways{assignp});
            // UINFOTREE(1, pinp, "", "out");
            // UINFOTREE(1, assignp, "", "aout");
        }
        ਵਾਪਸ assignp;
    }
};

//######################################################################
// Inst class functions

Ast_ਸੌਂਪੋ_W* V3Inst::pinReconnectSimple(AstPin* pinp, AstCell* cellp, ਤਾਰਕਿਕ forTristate,
                                       ਤਾਰਕਿਕ alwaysCvt) {
    ਵਾਪਸ InstStatic::pinReconnectSimple(pinp, cellp, forTristate, alwaysCvt);
}

ਖਾਲੀ V3Inst::checkOutputShort(ਅਚਲ AstPin* nodep) {
    ਜੇ (nodep->modVarp()->direction() == VDirection::OUTPUT) {
        ਜੇ (VN_IS(nodep->exprp(), Const) || VN_IS(nodep->exprp(), Extend)
            || (VN_IS(nodep->exprp(), Concat)
                && (VN_IS(VN_AS(nodep->exprp(), Concat)->lhsp(), Const)))) {
            // Uses v3warn for error, as might be found multiple times
            nodep->v3warn(E_PORTSHORT, "Output port is connected to a constant pin,"
                                       " electrical short");
            // Delete so we don't create a 'CONST = ...' assignment
            nodep->exprp()->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ();
        }
    }
}

//######################################################################
// Inst class visitor

ਖਾਲੀ V3Inst::instAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { InstVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("inst", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
