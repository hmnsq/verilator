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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Inst.h"

#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3Control.h"
#ಅನ್ತರ್ಭಾವ "V3Width.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ಸ್ಥಿರ ಶೂನ್ಯ markContinuousLhs(Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep) {
    nodep->foreach([](Ast_ಗಂಟು_Var_Ref* refp) {
        ಆದರೆ (refp->access().isWriteOrRW()) refp->varp()->isContinuously(ನಿಜ);
    });
}

// Flip the access of the driven leaves of a pin connection expression so it
// can be used on the LHS of an assignment. Only variables and bit/part
// selects over variables are flipped; index and select operands stay
// read-only. Struct-member selects cannot appear here: V3Width lowers them to
// selects before V3Inst. Returns false if the connection cannot be driven
// (e.g. arithmetic), in which case the expression is left unmodified.
ಸ್ಥಿರ ತಾರ್ಕಿಕ markConnectionLhs(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ nodep) {
    ಆದರೆ (Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep, ಗಂಟು_Var_Ref)) {
        ಆದರೆ (!refp->access().isReadOnly()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        refp->access(VAccess::WRITE);
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    // Note AstSel (bit/part select) is not an AstNodeSel (array select);
    // both expose fromp() for the selected value.
    ಆದರೆ (AstSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(nodep, Sel)) ಹಿಂತಿರುಗಿ markConnectionLhs(selp->fromp());
    ಆದರೆ (Ast_ಗಂಟು_Sel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(nodep, ಗಂಟು_Sel)) {
        ಹಿಂತಿರುಗಿ markConnectionLhs(selp->fromp());
    }
    ಆದರೆ (AstConcat* ಸ್ಥಿರಾಂಕ concatp = VN_CAST(nodep, Concat)) {
        ಹಿಂತಿರುಗಿ markConnectionLhs(concatp->lhsp()) && markConnectionLhs(concatp->rhsp());
    }
    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
}

// Prepare a pin connection expression as the LHS of the reverse (inside-out)
// assignment. Returns the flipped expression on success, nullptr if the
// connection cannot be driven. The input is left unmodified on failure.
ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* tryConnectionLhs(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ nodep) {
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ clonep = nodep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
    ಆದರೆ (markConnectionLhs(clonep)) ಹಿಂತಿರುಗಿ clonep;
    VL_DO_DANGLING(clonep->delete_ವೃಕ್ಷ(), clonep);
    ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
}

//######################################################################
// Inst state, as a visitor of each AstNode

ವರ್ಗ InstVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // Cleared each Cell:
    //  AstPin::user1p()        -> bool.  True if created assignment already
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;

    // STATE
    AstCell* m_cellp = ಶೂನ್ಯಸೂಚಕ;  // Current cell

    // METHODS
    // If appropriate, add an AstAlias to connect the given Cell pin to the given expression.
    // Returns true if an alias was made, in which case there is nothing else to do for this pin.
    ತಾರ್ಕಿಕ tryAliasPin(AstPin* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        AstVar* ಸ್ಥಿರಾಂಕ modVarp = nodep->modVarp();
        // An interface reference is aliased via AstAliasScope below, not as a variable
        ಆದರೆ (modVarp->isIfaceRef()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Only a whole variable can be aliased, anything else needs the assignment
        AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(exprp, VarRef);
        ಆದರೆ (!refp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        AstVar* ಸ್ಥಿರಾಂಕ ಅಭಿವ್ಯಕ್ತಿ_Varp = refp->varp();
        // A virtual interface port connected to a real interface can't be an alias
        ಆದರೆ (ಅಭಿವ್ಯಕ್ತಿ_Varp->isIfaceRef()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // A ref port must become an alias
        ಆದರೆ (!modVarp->direction().isRef()) {
            // V3FsmDetect recognizes the registers of an fsm_register_wrapper instance via
            // the assignments of its pins, so leave those as assignments
            Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ cellModp = m_cellp->modp();
            ಆದರೆ (V3Control::get_Fsm_ರಿಜಿಸ್ಟರ್_Wrapper(cellModp->origName())
                || V3Control::get_Fsm_ರಿಜಿಸ್ಟರ್_Wrapper(cellModp->prettyDehashOrigOrName())) {
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
            // A virtual interface method call is dispatched at run time, so the body of an
            // interface reached that way must use the signals of the instance it is called
            // on, not those of whichever instance this connection happens to be made to
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstIface* ಸ್ಥಿರಾಂಕ ifacep = VN_CAST(m_cellp->modp(), Iface)) {
                ಆದರೆ (ifacep->hasVirtualRef()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
            // Forced signals must keep their own storage, the two sides can be forced separately
            ಆದರೆ (modVarp->isForced() || ಅಭಿವ್ಯಕ್ತಿ_Varp->isForced()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            // Same for public
            ಆದರೆ (modVarp->isSigUserRWPublic() || ಅಭಿವ್ಯಕ್ತಿ_Varp->isSigUserRWPublic()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            // V3Tristate resolved the connected net already, and drives it from the
            // resolution it built for it, so a port merged into it would be driven by
            // the resolution of the instance as well
            ಆದರೆ (ಅಭಿವ್ಯಕ್ತಿ_Varp->isTristate()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        // They will become the same variable, so propagate file-line and attributes
        ಅಭಿವ್ಯಕ್ತಿ_Varp->fileline()->modifyStateInherit(modVarp->fileline());
        modVarp->fileline()->modifyStateInherit(ಅಭಿವ್ಯಕ್ತಿ_Varp->fileline());
        ಅಭಿವ್ಯಕ್ತಿ_Varp->propagateAttrFrom(modVarp);
        modVarp->propagateAttrFrom(ಅಭಿವ್ಯಕ್ತಿ_Varp);
        refp->access(VAccess::READWRITE);
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = exprp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ itemsp
            = ಹೊಸ AstVarXRef{flp, modVarp, m_cellp->name(), VAccess::READWRITE};
        itemsp->ಕೂಡಿಸು_ಮುಂದಿನ(exprp);
        m_cellp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(ಹೊಸ AstAlias{flp, itemsp});
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstCell* nodep) override {
        UINFO(4, "  CELL   " << nodep);
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        // VV*****  We reset user1p() on each cell!!!
        Ast_ಗಂಟು::user1_Clear_ವೃಕ್ಷ();
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstPin* nodep) override {
        // PIN(p,expr) -> ASSIGNW(VARXREF(p),expr)    (if sub's input)
        //            or  ASSIGNW(expr,VARXREF(p))    (if sub's output)
        UINFO(4, "   PIN  " << nodep);
        ಆದರೆ (!nodep->user1()) {
            // Simplify it
            V3Inst::pinReconnectSimple(nodep, m_cellp, ಸುಳ್ಳು);
        }
        UINFOTREE(9, nodep, "", "Pin_oldb");
        ಆದರೆ (!nodep->exprp()) ಹಿಂತಿರುಗಿ;  // No-connect
        V3Inst::checkOutputShort(nodep);
        ಆದರೆ (!nodep->exprp()) ಹಿಂತಿರುಗಿ;  // Connection removed by checkOutputShort
        // Use user1p on the PIN to indicate we created an assign for this pin
        ಆದರೆ (!nodep->user1_ಗಣ_Once()) {
            // Make an ASSIGNW (expr, pin)
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = VN_AS(nodep->exprp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
            UASSERT_OBJ(exprp->ಅಗಲ() == nodep->modVarp()->ಅಗಲ(), nodep,
                        "Width mismatch, should have been handled in pinReconnectSimple");
            ಆದರೆ (nodep->modVarp()->isInout()) {
                nodep->v3fatalSrc("Unsupported: Verilator is a 2-state simulator");
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->modVarp()->isWritable()) {
                ಆದರೆ (!tryAliasPin(nodep, exprp)) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = ಹೊಸ AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                             m_cellp->name(), VAccess::READ};
                    markContinuousLhs(exprp);
                    Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ assp = ಹೊಸ Ast_ನಿಯೋಜಿಸು_W{exprp->fileline(), exprp, rhsp};
                    m_cellp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(ಹೊಸ AstAlways{assp});
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->modVarp()->isNonOutput()) {
                ಆದರೆ (!tryAliasPin(nodep, exprp)) {
                    // Don't bother moving constants now,
                    // we'll be pushing the const down to the cell soon enough.
                    AstVarXRef* ಸ್ಥಿರಾಂಕ lhsp = ಹೊಸ AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                            m_cellp->name(), VAccess::WRITE};

                    markContinuousLhs(lhsp);
                    Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ assp = ಹೊಸ Ast_ನಿಯೋಜಿಸು_W{exprp->fileline(), lhsp, exprp};
                    m_cellp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(ಹೊಸ AstAlways{assp});
                    UINFOTREE(9, assp, "", "_new");
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->modVarp()->isIfaceRef()) {
                // Create an AstAliasScope for Vars to Cells so we can
                // link with their scope later
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = ಹೊಸ AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                         m_cellp->name(), VAccess::READ};
                ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(exprp, VarRef);
                ಸ್ಥಿರಾಂಕ AstVarXRef* ಸ್ಥಿರಾಂಕ xrefp = VN_CAST(exprp, VarXRef);
                UASSERT_OBJ(refp || xrefp, exprp,
                            "Interfaces: Pin is not connected to a VarRef or VarXRef");
                m_cellp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(ಹೊಸ AstAliasScope{exprp->fileline(), lhsp, exprp});
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3error("Assigned pin is neither input nor output");
            }
        }

        // We're done with the pin
        VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
    }

    // Save some time
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*) override {}
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ನಿಯೋಜಿಸು*) override {}
    ಶೂನ್ಯ ಭೇಟಿ(AstAlways*) override {}

    //--------------------
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ InstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~InstVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Inst static function

ವರ್ಗ InstStatic final {
    InstStatic() = ಪೂರ್ವನಿಯೋಜಿತ;  // Static class

    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* extendOrSel(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* cmpWidthp) {
        ಆದರೆ (cmpWidthp->ಅಗಲ() > rhsp->ಅಗಲ()) {
            rhsp = (rhsp->isSigned() ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ಹೊಸ AstExtendS{fl, rhsp})
                                     : static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ಹೊಸ AstExtend{fl, rhsp}));
            // Need proper widthMin, which may differ from AstSel created above
            rhsp->dtypeFrom(cmpWidthp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (cmpWidthp->ಅಗಲ() < rhsp->ಅಗಲ()) {
            rhsp = ಹೊಸ AstSel{fl, rhsp, 0, cmpWidthp->ಅಗಲ()};
            // Need proper widthMin, which may differ from AstSel created above
            rhsp->dtypeFrom(cmpWidthp);
        }
        // else don't change dtype, as might be e.g. array of something
        ಹಿಂತಿರುಗಿ rhsp;
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ Ast_ನಿಯೋಜಿಸು_W* pinReconnectSimple(AstPin* pinp, AstCell* cellp, ತಾರ್ಕಿಕ forTristate,
                                          ತಾರ್ಕಿಕ alwaysCvt) {
        // If a pin connection is "simple" leave it as-is
        // Else create a intermediate wire to perform the interconnect
        // Return the new assignment, if one was made
        // Note this module calls cloneTree() via new AstVar
        AstVar* ಸ್ಥಿರಾಂಕ pinVarp = pinp->modVarp();
        ಆದರೆ (!pinp->exprp()) {
            // No-connect, perhaps promote based on `unconnected_drive,
            // otherwise done
            ಆದರೆ (pinVarp->direction() == VDirection::ಒಳಹರಿವು
                && cellp->modp()->unconnectedDrive().is_ಗಣ_True()) {
                pinp->exprp(ಹೊಸ AstConst{pinp->fileline(), AstConst::All1{}});
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (pinVarp->direction() == VDirection::ಒಳಹರಿವು
                       && cellp->modp()->unconnectedDrive().is_ಗಣ_False()) {
                pinp->exprp(ಹೊಸ AstConst{pinp->fileline(), AstConst::All0{}});
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
            }
        }
        ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ connectRefp = VN_CAST(pinp->exprp(), VarRef);
        ಸ್ಥಿರಾಂಕ AstVarXRef* ಸ್ಥಿರಾಂಕ connectXRefp = VN_CAST(pinp->exprp(), VarXRef);
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ pinDTypep = pinVarp->dtypep()->skipRefp();
        ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ pinBasicp = VN_CAST(pinDTypep, BasicDType);
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ connDTypep
            = connectRefp ? connectRefp->varp()->dtypep()->skipRefp() : ಶೂನ್ಯಸೂಚಕ;
        ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ connBasicp = VN_CAST(connDTypep, BasicDType);
        Ast_ನಿಯೋಜಿಸು_W* assignp = ಶೂನ್ಯಸೂಚಕ;
        //
        ಆದರೆ (!alwaysCvt && connectRefp && connDTypep->same_ವೃಕ್ಷ(pinDTypep)
            && !connectRefp->varp()->isSc()) {  // Need the signal as a 'shell' to convert types
            // Done. Same data type
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!alwaysCvt && connectRefp && connectRefp->varp()->isIfaceRef()) {
            // Done. Interface
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!alwaysCvt && connectXRefp && connectXRefp->varp()
                   && connectXRefp->varp()->isIfaceRef()) {
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!alwaysCvt && connBasicp && pinBasicp
                   && connBasicp->ಅಗಲ() == pinBasicp->ಅಗಲ()
                   && connBasicp->lo() == pinBasicp->lo()
                   && !connectRefp->varp()
                           ->isSc()  // Need the signal as a 'shell' to convert types
                   && connBasicp->ಅಗಲ() == pinVarp->ಅಗಲ()) {
            // Done. One to one interconnect won't need a temporary variable.
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!alwaysCvt && !forTristate && VN_IS(pinp->exprp(), Const)) {
            // Done. Constant. Still check for driving an output, like below.
            V3Inst::checkOutputShort(pinp);
            ಆದರೆ (!pinp->exprp()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Make a new temp wire
            // UINFOTREE(9, pinp, "", "in_pin");
            V3Inst::checkOutputShort(pinp);
            ಆದರೆ (!pinp->exprp()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
            // Simplify, so stuff like '{a[0], b[0]}[1]' produced during
            // instance array expansion are brought to normal 'a[0]'
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ pinexprp
                = V3Const::constifyEdit(VN_AS(pinp->exprp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)->unlink_Fr_ಹಿಂಭಾಗ());
            ಸ್ಥಿರಾಂಕ string newvarname
                = (string{pinVarp->isWritable() ? "__Vcellout" : "__Vcellinp"}
                   // Prevent name conflict if both tri & non-tri add signals
                   + (forTristate ? "t" : "") + "__" + cellp->name() + "__" + pinp->name());
            AstVar* ಸ್ಥಿರಾಂಕ newvarp
                = ಹೊಸ AstVar{pinVarp->fileline(), VVarType::MODULETEMP, newvarname, pinVarp};
            // Important to add statement next to cell, in case there is a
            // generate with same named cell
            cellp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(newvarp);
            ಆದರೆ (pinVarp->isInout()) {
                pinVarp->v3fatalSrc("Unsupported: Inout connections to pins must be"
                                    " direct one-to-one connection (without any expression)");
                // V3Tristate should have cleared up before this point
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (pinVarp->isWritable()) {
                // See also V3Inst
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp = ಹೊಸ AstVarRef{pinp->fileline(), newvarp, VAccess::READ};
                UINFO(5, "pinRecon width " << pinVarp->ಅಗಲ() << " >? " << rhsp->ಅಗಲ() << " >? "
                                           << pinexprp->ಅಗಲ());
                rhsp = extendOrSel(pinp->fileline(), rhsp, pinVarp);
                pinp->exprp(ಹೊಸ AstVarRef{newvarp->fileline(), newvarp, VAccess::WRITE});
                markContinuousLhs(pinexprp);
                ಆದರೆ (VN_IS(pinexprp, ಗಂಟು_ಹರಿವು)) {
                    assignp = ಹೊಸ Ast_ನಿಯೋಜಿಸು_W{pinp->fileline(), pinexprp, rhsp};
                    V3_ಅಗಲ::ಹರಿವು_ನಿಯೋಜಿಸು_ಅಧೋ_Edit(assignp);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsSelp = extendOrSel(pinp->fileline(), rhsp, pinexprp);
                    // A reversed array is assigned by the left-to-right order of elements
                    // (IEEE 1800-2023 7.6)
                    ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ arrayp
                        = VN_CAST(pinexprp->dtypep()->skipRefp(), UnpackArrayDType);
                    ಆದರೆ (arrayp && arrayp->is_ಹಿಮ್ಮುಖ_ದ(rhsSelp->dtypep())) {
                        rhsSelp = ಹೊಸ Ast_Cvt_Unpack_ಗಮ್ಯ_Unpack{pinp->fileline(), rhsSelp,
                                                           pinexprp->dtypep()};
                    }
                    assignp = ಹೊಸ Ast_ನಿಯೋಜಿಸು_W{pinp->fileline(), pinexprp, rhsSelp};
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                // V3 width should have range/extended to make the widths correct
                newvarp->isContinuously(ನಿಜ);
                // An input port driven from inside the cell by a clocking block
                // output flows out through the connection; V3Tristate split
                // pins resolve separately.
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp = ಶೂನ್ಯಸೂಚಕ;  // LHS of the interconnect assign
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp = ಶೂನ್ಯಸೂಚಕ;  // RHS of the interconnect assign
                ಆದರೆ (forTristate || !pinVarp->isClockingDriven()) {
                    // Input-side wiring: temp fed from the connection
                    lhsp = ಹೊಸ AstVarRef{pinp->fileline(), newvarp, VAccess::WRITE};
                    rhsp = pinexprp;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ drivenp = tryConnectionLhs(pinexprp)) {
                    // Reverse wiring: connection leaves fed from the temp.
                    // The original connection is consumed by the clone above.
                    VL_DO_DANGLING(pinexprp->delete_ವೃಕ್ಷ(), pinexprp);
                    markContinuousLhs(drivenp);
                    lhsp = drivenp;
                    rhsp = extendOrSel(pinp->fileline(),
                                       ಹೊಸ AstVarRef{pinp->fileline(), newvarp, VAccess::READ},
                                       drivenp);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    pinp->v3error("Unsupported: Clocking block output driving input port "
                                  "through non-decomposable connection "
                                  << pinp->prettyNameQ());
                    VL_DO_DANGLING(pinexprp->delete_ವೃಕ್ಷ(), pinexprp);
                    ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
                }
                pinp->exprp(ಹೊಸ AstVarRef{pinp->fileline(), newvarp, VAccess::READ});
                assignp = ಹೊಸ Ast_ನಿಯೋಜಿಸು_W{pinp->fileline(), lhsp, rhsp};
            }
            ಆದರೆ (assignp) cellp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(ಹೊಸ AstAlways{assignp});
            // UINFOTREE(1, pinp, "", "out");
            // UINFOTREE(1, assignp, "", "aout");
        }
        ಹಿಂತಿರುಗಿ assignp;
    }
};

//######################################################################
// Inst class functions

Ast_ನಿಯೋಜಿಸು_W* V3Inst::pinReconnectSimple(AstPin* pinp, AstCell* cellp, ತಾರ್ಕಿಕ forTristate,
                                       ತಾರ್ಕಿಕ alwaysCvt) {
    ಹಿಂತಿರುಗಿ InstStatic::pinReconnectSimple(pinp, cellp, forTristate, alwaysCvt);
}

ಶೂನ್ಯ V3Inst::checkOutputShort(ಸ್ಥಿರಾಂಕ AstPin* nodep) {
    ಆದರೆ (nodep->modVarp()->direction() == VDirection::OUTPUT) {
        ಆದರೆ (VN_IS(nodep->exprp(), Const) || VN_IS(nodep->exprp(), Extend)
            || (VN_IS(nodep->exprp(), Concat)
                && (VN_IS(VN_AS(nodep->exprp(), Concat)->lhsp(), Const)))) {
            // Uses v3warn for error, as might be found multiple times
            nodep->v3warn(E_PORTSHORT, "Output port is connected to a constant pin,"
                                       " electrical short");
            // Delete so we don't create a 'CONST = ...' assignment
            nodep->exprp()->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ();
        }
    }
}

//######################################################################
// Inst class visitor

ಶೂನ್ಯ V3Inst::instAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { InstVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("inst", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
