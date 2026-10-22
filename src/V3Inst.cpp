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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Inst.h"

#అన్తర్భావ "V3Const.h"
#అన్తర్భావ "V3Control.h"
#అన్తర్భావ "V3Width.h"

VL_DEFINE_DEBUG_FUNCTIONS;

స్థిర శూన్య markContinuousLhs(Ast_కణుపు* స్థిరరాశి nodep) {
    nodep->foreach([](Ast_కణుపు_Var_Ref* refp) {
        ఒకవేళ (refp->access().isWriteOrRW()) refp->varp()->isContinuously(నిజం);
    });
}

// Flip the access of the driven leaves of a pin connection expression so it
// can be used on the LHS of an assignment. Only variables and bit/part
// selects over variables are flipped; index and select operands stay
// read-only. Struct-member selects cannot appear here: V3Width lowers them to
// selects before V3Inst. Returns false if the connection cannot be driven
// (e.g. arithmetic), in which case the expression is left unmodified.
స్థిర తార్కిక markConnectionLhs(Ast_కణుపు_సమాసం* స్థిరరాశి nodep) {
    ఒకవేళ (Ast_కణుపు_Var_Ref* స్థిరరాశి refp = VN_CAST(nodep, కణుపు_Var_Ref)) {
        ఒకవేళ (!refp->access().isReadOnly()) తిరిగి అబద్ధం;
        refp->access(VAccess::WRITE);
        తిరిగి నిజం;
    }
    // Note AstSel (bit/part select) is not an AstNodeSel (array select);
    // both expose fromp() for the selected value.
    ఒకవేళ (AstSel* స్థిరరాశి selp = VN_CAST(nodep, Sel)) తిరిగి markConnectionLhs(selp->fromp());
    ఒకవేళ (Ast_కణుపు_Sel* స్థిరరాశి selp = VN_CAST(nodep, కణుపు_Sel)) {
        తిరిగి markConnectionLhs(selp->fromp());
    }
    ఒకవేళ (AstConcat* స్థిరరాశి concatp = VN_CAST(nodep, Concat)) {
        తిరిగి markConnectionLhs(concatp->lhsp()) && markConnectionLhs(concatp->rhsp());
    }
    తిరిగి అబద్ధం;
}

// Prepare a pin connection expression as the LHS of the reverse (inside-out)
// assignment. Returns the flipped expression on success, nullptr if the
// connection cannot be driven. The input is left unmodified on failure.
స్థిర Ast_కణుపు_సమాసం* tryConnectionLhs(Ast_కణుపు_సమాసం* స్థిరరాశి nodep) {
    Ast_కణుపు_సమాసం* స్థిరరాశి clonep = nodep->clone_వృక్షం(అబద్ధం);
    ఒకవేళ (markConnectionLhs(clonep)) తిరిగి clonep;
    VL_DO_DANGLING(clonep->delete_వృక్షం(), clonep);
    తిరిగి శూన్యసూచిక;
}

//######################################################################
// Inst state, as a visitor of each AstNode

తరగతి InstVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    // Cleared each Cell:
    //  AstPin::user1p()        -> bool.  True if created assignment already
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;

    // STATE
    AstCell* m_cellp = శూన్యసూచిక;  // Current cell

    // METHODS
    // If appropriate, add an AstAlias to connect the given Cell pin to the given expression.
    // Returns true if an alias was made, in which case there is nothing else to do for this pin.
    తార్కిక tryAliasPin(AstPin* nodep, Ast_కణుపు_సమాసం* exprp) {
        AstVar* స్థిరరాశి modVarp = nodep->modVarp();
        // An interface reference is aliased via AstAliasScope below, not as a variable
        ఒకవేళ (modVarp->isIfaceRef()) తిరిగి అబద్ధం;
        // Only a whole variable can be aliased, anything else needs the assignment
        AstVarRef* స్థిరరాశి refp = VN_CAST(exprp, VarRef);
        ఒకవేళ (!refp) తిరిగి అబద్ధం;
        AstVar* స్థిరరాశి సమాసం_Varp = refp->varp();
        // A virtual interface port connected to a real interface can't be an alias
        ఒకవేళ (సమాసం_Varp->isIfaceRef()) తిరిగి అబద్ధం;
        // A ref port must become an alias
        ఒకవేళ (!modVarp->direction().isRef()) {
            // V3FsmDetect recognizes the registers of an fsm_register_wrapper instance via
            // the assignments of its pins, so leave those as assignments
            Ast_కణుపు_Module* స్థిరరాశి cellModp = m_cellp->modp();
            ఒకవేళ (V3Control::get_Fsm_రిజిస్టరు_Wrapper(cellModp->origName())
                || V3Control::get_Fsm_రిజిస్టరు_Wrapper(cellModp->prettyDehashOrigOrName())) {
                తిరిగి అబద్ధం;
            }
            // A virtual interface method call is dispatched at run time, so the body of an
            // interface reached that way must use the signals of the instance it is called
            // on, not those of whichever instance this connection happens to be made to
            ఒకవేళ (స్థిరరాశి AstIface* స్థిరరాశి ifacep = VN_CAST(m_cellp->modp(), Iface)) {
                ఒకవేళ (ifacep->hasVirtualRef()) తిరిగి అబద్ధం;
            }
            // Forced signals must keep their own storage, the two sides can be forced separately
            ఒకవేళ (modVarp->isForced() || సమాసం_Varp->isForced()) తిరిగి అబద్ధం;
            // Same for public
            ఒకవేళ (modVarp->isSigUserRWPublic() || సమాసం_Varp->isSigUserRWPublic()) తిరిగి అబద్ధం;
            // V3Tristate resolved the connected net already, and drives it from the
            // resolution it built for it, so a port merged into it would be driven by
            // the resolution of the instance as well
            ఒకవేళ (సమాసం_Varp->isTristate()) తిరిగి అబద్ధం;
        }
        // They will become the same variable, so propagate file-line and attributes
        సమాసం_Varp->fileline()->modifyStateInherit(modVarp->fileline());
        modVarp->fileline()->modifyStateInherit(సమాసం_Varp->fileline());
        సమాసం_Varp->propagateAttrFrom(modVarp);
        modVarp->propagateAttrFrom(సమాసం_Varp);
        refp->access(VAccess::READWRITE);
        File_పంక్తి* స్థిరరాశి flp = exprp->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి itemsp
            = కొత్త AstVarXRef{flp, modVarp, m_cellp->name(), VAccess::READWRITE};
        itemsp->కూడు_తదుపరి(exprp);
        m_cellp->కూడు_తదుపరి_ఇక్కడ(కొత్త AstAlias{flp, itemsp});
        తిరిగి నిజం;
    }

    // VISITORS
    శూన్య సందర్శించు(AstCell* nodep) override {
        UINFO(4, "  CELL   " << nodep);
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        // VV*****  We reset user1p() on each cell!!!
        Ast_కణుపు::user1_Clear_వృక్షం();
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(AstPin* nodep) override {
        // PIN(p,expr) -> ASSIGNW(VARXREF(p),expr)    (if sub's input)
        //            or  ASSIGNW(expr,VARXREF(p))    (if sub's output)
        UINFO(4, "   PIN  " << nodep);
        ఒకవేళ (!nodep->user1()) {
            // Simplify it
            V3Inst::pinReconnectSimple(nodep, m_cellp, అబద్ధం);
        }
        UINFOTREE(9, nodep, "", "Pin_oldb");
        ఒకవేళ (!nodep->exprp()) తిరిగి;  // No-connect
        V3Inst::checkOutputShort(nodep);
        ఒకవేళ (!nodep->exprp()) తిరిగి;  // Connection removed by checkOutputShort
        // Use user1p on the PIN to indicate we created an assign for this pin
        ఒకవేళ (!nodep->user1_సమితి_Once()) {
            // Make an ASSIGNW (expr, pin)
            Ast_కణుపు_సమాసం* స్థిరరాశి exprp = VN_AS(nodep->exprp(), కణుపు_సమాసం)->clone_వృక్షం(అబద్ధం);
            UASSERT_OBJ(exprp->వెడల్పు() == nodep->modVarp()->వెడల్పు(), nodep,
                        "Width mismatch, should have been handled in pinReconnectSimple");
            ఒకవేళ (nodep->modVarp()->isInout()) {
                nodep->v3fatalSrc("Unsupported: Verilator is a 2-state simulator");
            } లేకపోతే ఒకవేళ (nodep->modVarp()->isWritable()) {
                ఒకవేళ (!tryAliasPin(nodep, exprp)) {
                    Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = కొత్త AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                             m_cellp->name(), VAccess::READ};
                    markContinuousLhs(exprp);
                    Ast_కేటాయించు_W* స్థిరరాశి assp = కొత్త Ast_కేటాయించు_W{exprp->fileline(), exprp, rhsp};
                    m_cellp->కూడు_తదుపరి_ఇక్కడ(కొత్త AstAlways{assp});
                }
            } లేకపోతే ఒకవేళ (nodep->modVarp()->isNonOutput()) {
                ఒకవేళ (!tryAliasPin(nodep, exprp)) {
                    // Don't bother moving constants now,
                    // we'll be pushing the const down to the cell soon enough.
                    AstVarXRef* స్థిరరాశి lhsp = కొత్త AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                            m_cellp->name(), VAccess::WRITE};

                    markContinuousLhs(lhsp);
                    Ast_కేటాయించు_W* స్థిరరాశి assp = కొత్త Ast_కేటాయించు_W{exprp->fileline(), lhsp, exprp};
                    m_cellp->కూడు_తదుపరి_ఇక్కడ(కొత్త AstAlways{assp});
                    UINFOTREE(9, assp, "", "_new");
                }
            } లేకపోతే ఒకవేళ (nodep->modVarp()->isIfaceRef()) {
                // Create an AstAliasScope for Vars to Cells so we can
                // link with their scope later
                Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = కొత్త AstVarXRef{exprp->fileline(), nodep->modVarp(),
                                                         m_cellp->name(), VAccess::READ};
                స్థిరరాశి AstVarRef* స్థిరరాశి refp = VN_CAST(exprp, VarRef);
                స్థిరరాశి AstVarXRef* స్థిరరాశి xrefp = VN_CAST(exprp, VarXRef);
                UASSERT_OBJ(refp || xrefp, exprp,
                            "Interfaces: Pin is not connected to a VarRef or VarXRef");
                m_cellp->కూడు_తదుపరి_ఇక్కడ(కొత్త AstAliasScope{exprp->fileline(), lhsp, exprp});
            } లేకపోతే {
                nodep->v3error("Assigned pin is neither input nor output");
            }
        }

        // We're done with the pin
        VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
    }

    // Save some time
    శూన్య సందర్శించు(Ast_కణుపు_సమాసం*) override {}
    శూన్య సందర్శించు(Ast_కణుపు_కేటాయించు*) override {}
    శూన్య సందర్శించు(AstAlways*) override {}

    //--------------------
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన InstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~InstVisitor() override = అప్రమేయ;
};

//######################################################################
// Inst static function

తరగతి InstStatic final {
    InstStatic() = అప్రమేయ;  // Static class

    స్థిర Ast_కణుపు_సమాసం* extendOrSel(File_పంక్తి* fl, Ast_కణుపు_సమాసం* rhsp, స్థిరరాశి Ast_కణుపు* cmpWidthp) {
        ఒకవేళ (cmpWidthp->వెడల్పు() > rhsp->వెడల్పు()) {
            rhsp = (rhsp->isSigned() ? static_cast<Ast_కణుపు_సమాసం*>(కొత్త AstExtendS{fl, rhsp})
                                     : static_cast<Ast_కణుపు_సమాసం*>(కొత్త AstExtend{fl, rhsp}));
            // Need proper widthMin, which may differ from AstSel created above
            rhsp->dtypeFrom(cmpWidthp);
        } లేకపోతే ఒకవేళ (cmpWidthp->వెడల్పు() < rhsp->వెడల్పు()) {
            rhsp = కొత్త AstSel{fl, rhsp, 0, cmpWidthp->వెడల్పు()};
            // Need proper widthMin, which may differ from AstSel created above
            rhsp->dtypeFrom(cmpWidthp);
        }
        // else don't change dtype, as might be e.g. array of something
        తిరిగి rhsp;
    }

బహిరంగ:
    స్థిర Ast_కేటాయించు_W* pinReconnectSimple(AstPin* pinp, AstCell* cellp, తార్కిక forTristate,
                                          తార్కిక alwaysCvt) {
        // If a pin connection is "simple" leave it as-is
        // Else create a intermediate wire to perform the interconnect
        // Return the new assignment, if one was made
        // Note this module calls cloneTree() via new AstVar
        AstVar* స్థిరరాశి pinVarp = pinp->modVarp();
        ఒకవేళ (!pinp->exprp()) {
            // No-connect, perhaps promote based on `unconnected_drive,
            // otherwise done
            ఒకవేళ (pinVarp->direction() == VDirection::ఆదానం
                && cellp->modp()->unconnectedDrive().is_సమితి_True()) {
                pinp->exprp(కొత్త AstConst{pinp->fileline(), AstConst::All1{}});
            } లేకపోతే ఒకవేళ (pinVarp->direction() == VDirection::ఆదానం
                       && cellp->modp()->unconnectedDrive().is_సమితి_False()) {
                pinp->exprp(కొత్త AstConst{pinp->fileline(), AstConst::All0{}});
            } లేకపోతే {
                తిరిగి శూన్యసూచిక;
            }
        }
        స్థిరరాశి AstVarRef* స్థిరరాశి connectRefp = VN_CAST(pinp->exprp(), VarRef);
        స్థిరరాశి AstVarXRef* స్థిరరాశి connectXRefp = VN_CAST(pinp->exprp(), VarXRef);
        స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి pinDTypep = pinVarp->dtypep()->skipRefp();
        స్థిరరాశి AstBasicDType* స్థిరరాశి pinBasicp = VN_CAST(pinDTypep, BasicDType);
        స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి connDTypep
            = connectRefp ? connectRefp->varp()->dtypep()->skipRefp() : శూన్యసూచిక;
        స్థిరరాశి AstBasicDType* స్థిరరాశి connBasicp = VN_CAST(connDTypep, BasicDType);
        Ast_కేటాయించు_W* assignp = శూన్యసూచిక;
        //
        ఒకవేళ (!alwaysCvt && connectRefp && connDTypep->same_వృక్షం(pinDTypep)
            && !connectRefp->varp()->isSc()) {  // Need the signal as a 'shell' to convert types
            // Done. Same data type
        } లేకపోతే ఒకవేళ (!alwaysCvt && connectRefp && connectRefp->varp()->isIfaceRef()) {
            // Done. Interface
        } లేకపోతే ఒకవేళ (!alwaysCvt && connectXRefp && connectXRefp->varp()
                   && connectXRefp->varp()->isIfaceRef()) {
        } లేకపోతే ఒకవేళ (!alwaysCvt && connBasicp && pinBasicp
                   && connBasicp->వెడల్పు() == pinBasicp->వెడల్పు()
                   && connBasicp->lo() == pinBasicp->lo()
                   && !connectRefp->varp()
                           ->isSc()  // Need the signal as a 'shell' to convert types
                   && connBasicp->వెడల్పు() == pinVarp->వెడల్పు()) {
            // Done. One to one interconnect won't need a temporary variable.
        } లేకపోతే ఒకవేళ (!alwaysCvt && !forTristate && VN_IS(pinp->exprp(), Const)) {
            // Done. Constant. Still check for driving an output, like below.
            V3Inst::checkOutputShort(pinp);
            ఒకవేళ (!pinp->exprp()) తిరిగి శూన్యసూచిక;
        } లేకపోతే {
            // Make a new temp wire
            // UINFOTREE(9, pinp, "", "in_pin");
            V3Inst::checkOutputShort(pinp);
            ఒకవేళ (!pinp->exprp()) తిరిగి శూన్యసూచిక;
            // Simplify, so stuff like '{a[0], b[0]}[1]' produced during
            // instance array expansion are brought to normal 'a[0]'
            Ast_కణుపు_సమాసం* స్థిరరాశి pinexprp
                = V3Const::constifyEdit(VN_AS(pinp->exprp(), కణుపు_సమాసం)->unlink_Fr_వెనుక());
            స్థిరరాశి string newvarname
                = (string{pinVarp->isWritable() ? "__Vcellout" : "__Vcellinp"}
                   // Prevent name conflict if both tri & non-tri add signals
                   + (forTristate ? "t" : "") + "__" + cellp->name() + "__" + pinp->name());
            AstVar* స్థిరరాశి newvarp
                = కొత్త AstVar{pinVarp->fileline(), VVarType::MODULETEMP, newvarname, pinVarp};
            // Important to add statement next to cell, in case there is a
            // generate with same named cell
            cellp->కూడు_తదుపరి_ఇక్కడ(newvarp);
            ఒకవేళ (pinVarp->isInout()) {
                pinVarp->v3fatalSrc("Unsupported: Inout connections to pins must be"
                                    " direct one-to-one connection (without any expression)");
                // V3Tristate should have cleared up before this point
            } లేకపోతే ఒకవేళ (pinVarp->isWritable()) {
                // See also V3Inst
                Ast_కణుపు_సమాసం* rhsp = కొత్త AstVarRef{pinp->fileline(), newvarp, VAccess::READ};
                UINFO(5, "pinRecon width " << pinVarp->వెడల్పు() << " >? " << rhsp->వెడల్పు() << " >? "
                                           << pinexprp->వెడల్పు());
                rhsp = extendOrSel(pinp->fileline(), rhsp, pinVarp);
                pinp->exprp(కొత్త AstVarRef{newvarp->fileline(), newvarp, VAccess::WRITE});
                markContinuousLhs(pinexprp);
                ఒకవేళ (VN_IS(pinexprp, కణుపు_ప్రవాహం)) {
                    assignp = కొత్త Ast_కేటాయించు_W{pinp->fileline(), pinexprp, rhsp};
                    V3_వెడల్పు::ప్రవాహం_కేటాయించు_క్రింది_Edit(assignp);
                } లేకపోతే {
                    Ast_కణుపు_సమాసం* rhsSelp = extendOrSel(pinp->fileline(), rhsp, pinexprp);
                    // A reversed array is assigned by the left-to-right order of elements
                    // (IEEE 1800-2023 7.6)
                    స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి arrayp
                        = VN_CAST(pinexprp->dtypep()->skipRefp(), UnpackArrayDType);
                    ఒకవేళ (arrayp && arrayp->is_తిరగేయి_యొక్క(rhsSelp->dtypep())) {
                        rhsSelp = కొత్త Ast_Cvt_Unpack_గమ్యం_Unpack{pinp->fileline(), rhsSelp,
                                                           pinexprp->dtypep()};
                    }
                    assignp = కొత్త Ast_కేటాయించు_W{pinp->fileline(), pinexprp, rhsSelp};
                }
            } లేకపోతే {
                // V3 width should have range/extended to make the widths correct
                newvarp->isContinuously(నిజం);
                // An input port driven from inside the cell by a clocking block
                // output flows out through the connection; V3Tristate split
                // pins resolve separately.
                Ast_కణుపు_సమాసం* lhsp = శూన్యసూచిక;  // LHS of the interconnect assign
                Ast_కణుపు_సమాసం* rhsp = శూన్యసూచిక;  // RHS of the interconnect assign
                ఒకవేళ (forTristate || !pinVarp->isClockingDriven()) {
                    // Input-side wiring: temp fed from the connection
                    lhsp = కొత్త AstVarRef{pinp->fileline(), newvarp, VAccess::WRITE};
                    rhsp = pinexprp;
                } లేకపోతే ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి drivenp = tryConnectionLhs(pinexprp)) {
                    // Reverse wiring: connection leaves fed from the temp.
                    // The original connection is consumed by the clone above.
                    VL_DO_DANGLING(pinexprp->delete_వృక్షం(), pinexprp);
                    markContinuousLhs(drivenp);
                    lhsp = drivenp;
                    rhsp = extendOrSel(pinp->fileline(),
                                       కొత్త AstVarRef{pinp->fileline(), newvarp, VAccess::READ},
                                       drivenp);
                } లేకపోతే {
                    pinp->v3error("Unsupported: Clocking block output driving input port "
                                  "through non-decomposable connection "
                                  << pinp->prettyNameQ());
                    VL_DO_DANGLING(pinexprp->delete_వృక్షం(), pinexprp);
                    తిరిగి శూన్యసూచిక;
                }
                pinp->exprp(కొత్త AstVarRef{pinp->fileline(), newvarp, VAccess::READ});
                assignp = కొత్త Ast_కేటాయించు_W{pinp->fileline(), lhsp, rhsp};
            }
            ఒకవేళ (assignp) cellp->కూడు_తదుపరి_ఇక్కడ(కొత్త AstAlways{assignp});
            // UINFOTREE(1, pinp, "", "out");
            // UINFOTREE(1, assignp, "", "aout");
        }
        తిరిగి assignp;
    }
};

//######################################################################
// Inst class functions

Ast_కేటాయించు_W* V3Inst::pinReconnectSimple(AstPin* pinp, AstCell* cellp, తార్కిక forTristate,
                                       తార్కిక alwaysCvt) {
    తిరిగి InstStatic::pinReconnectSimple(pinp, cellp, forTristate, alwaysCvt);
}

శూన్య V3Inst::checkOutputShort(స్థిరరాశి AstPin* nodep) {
    ఒకవేళ (nodep->modVarp()->direction() == VDirection::OUTPUT) {
        ఒకవేళ (VN_IS(nodep->exprp(), Const) || VN_IS(nodep->exprp(), Extend)
            || (VN_IS(nodep->exprp(), Concat)
                && (VN_IS(VN_AS(nodep->exprp(), Concat)->lhsp(), Const)))) {
            // Uses v3warn for error, as might be found multiple times
            nodep->v3warn(E_PORTSHORT, "Output port is connected to a constant pin,"
                                       " electrical short");
            // Delete so we don't create a 'CONST = ...' assignment
            nodep->exprp()->unlink_Fr_వెనుక()->delete_వృక్షం();
        }
    }
}

//######################################################################
// Inst class visitor

శూన్య V3Inst::instAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { InstVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("inst", 0, dump_వృక్షం_Either_Level() >= 3);
}
