// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: LValue module/signal name references
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
// LinkLValue TRANSFORMATIONS:
//      Top-down traversal
//          Set lvalue() attributes on appropriate VARREFs.
//*************************************************************************

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3LinkLValue.h"

#അന്തര്ഭാവ "V3Task.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

വർഗ്ഗം LinkLValueVisitor final : പൊതു VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    തർക്കം m_setContinuously = അസത്യം;  // Set that var has some continuous assignment
    തർക്കം m_setForcedByCode = അസത്യം;  // Set that var is the target of an AstAssignForce/AstRelease
    തർക്കം m_setIfRand = അസത്യം;  // Update VarRefs if var declared as rand
    തർക്കം m_setStrengthSpecified = അസത്യം;  // Set that var has assignment with strength specified.
    തർക്കം m_inFunc = അസത്യം;  // Set if inside AstNodeFTask
    തർക്കം m_inInitialStatic = അസത്യം;  // Set if inside AstInitialStatic
    തർക്കം m_inInitialStaticStmt = അസത്യം;  // Set if inside AstInitialStaticStmt
    VAccess m_setRefLvalue;  // Set VarRefs to lvalues for pin assignments

    // VISITORS
    // Result handing
    ശൂന്യം visit(Ast_കണ്ണി_Var_Ref* nodep) override {
        // VarRef: LValue its reference
        എങ്കിൽ (m_setIfRand && !(nodep->varp() && nodep->varp()->isRand())) തിരികെ;
        എങ്കിൽ (m_setRefLvalue != VAccess::NOCHANGE) nodep->access(m_setRefLvalue);
        // Linked via an input-only modport by V3LinkDot, before it was known if written
        എങ്കിൽ (AstVarXRef* സ്ഥിരം xrefp = VN_CAST(nodep, VarXRef)) {
            എങ്കിൽ (xrefp->readOnlyModport()) {
                എങ്കിൽ (nodep->access().isWriteOrRW()) {
                    nodep->v3error(
                        "Attempt to drive input-only modport: " << nodep->prettyNameQ());
                }
                xrefp->readOnlyModport(അസത്യം);
            }
        }
        എങ്കിൽ (nodep->varp() && nodep->access().isWriteOrRW()) {
            എങ്കിൽ (nodep->varp()->isParam()) {
                // All parameters that did get constified happened before now
                // as V3LinkLValue runs after V3Param
                nodep->v3error("Storing to parameter variable "
                               << nodep->prettyNameQ()
                               << " in a context that is determined only at runtime");
            }
            എങ്കിൽ (m_setContinuously) {
                nodep->varp()->isContinuously(സത്യം);
                // Strength may only be specified in continuous assignment,
                // so it is needed to check only if m_setContinuously is true
                എങ്കിൽ (m_setStrengthSpecified) nodep->varp()->hasStrengthAssignment(സത്യം);
            }
            എങ്കിൽ (സ്ഥിരം AstClockingItem* സ്ഥിരം itemp
                = VN_CAST(nodep->varp()->backp(), ClockingItem)) {
                UINFO(5, "ClkOut " << nodep);
                എങ്കിൽ (itemp->outputp()) nodep->varp(itemp->outputp()->varp());
            }
            എങ്കിൽ (m_setForcedByCode) {
                nodep->varp()->setForcedByCode();
                // If a public signal is being forced in SystemVerilog and VPI
                // is enabled, mark it as forceable to ensure that the VPI
                // functions read the forced value correctly
                എങ്കിൽ (v3Global.opt.vpi()
                    && (nodep->varp()->isSigPublic() || nodep->varp()->isSigModPublic())) {
                    nodep->varp()->setForceable();
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (!nodep->varp()->isFuncLocal() && nodep->varp()->isReadOnly()) {
                // This is allowed with IEEE 1800-2009 module input with default value.
                // the checking now happens in V3Width::visit(AstNodeVarRef*)
                // If you were to check here, it would fail on module inputs with default value,
                // because Inputs are isReadOnly()=true, and we don't yet have visibility into
                // it being an Initial style procedure.
            }
        }
        iterateChildren(nodep);
    }

    // Nodes that start propagating down lvalues
    ശൂന്യം visit(AstPin* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        എങ്കിൽ (nodep->modVarp() && nodep->modVarp()->isWritable()) {
            // When the varref's were created, we didn't know the I/O state
            // Now that we do, and it's from a output, we know it's a lvalue
            m_setRefLvalue = VAccess::WRITE;
            iterateChildren(nodep);
            m_setRefLvalue = VAccess::NOCHANGE;
        } അല്ലെങ്കിൽ {
            iterateChildren(nodep);
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Assign* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        VL_RESTORER(m_setContinuously);
        VL_RESTORER(m_setStrengthSpecified);
        {
            m_setRefLvalue = VAccess::WRITE;
            m_setContinuously = VN_IS(nodep, AssignW);
            എങ്കിൽ (സ്ഥിരം AstAssignW* സ്ഥിരം assignwp = VN_CAST(nodep, AssignW)) {
                എങ്കിൽ (assignwp->strengthSpecp()) m_setStrengthSpecified = സത്യം;
            }
            {
                VL_RESTORER(m_setForcedByCode);
                m_setForcedByCode = VN_IS(nodep, AssignForce);
                iterateAndNextNull(nodep->lhsp());
            }
            m_setRefLvalue = VAccess::NOCHANGE;
            m_setContinuously = അസത്യം;
            m_setStrengthSpecified = അസത്യം;
            iterateAndNextNull(nodep->rhsp());
        }

        എങ്കിൽ ((m_inInitialStatic || m_inInitialStaticStmt) && m_inFunc) {
            സ്ഥിരം തർക്കം rhsHasIO = nodep->rhsp()->exists([](സ്ഥിരം Ast_കണ്ണി_Var_Ref* സ്ഥിരം refp) {
                // Exclude module I/O referenced from a function/task.
                തിരികെ refp->varp() && refp->varp()->isIO()
                       && refp->varp()->lifetime() != VLifetime::NONE;
            });
            എങ്കിൽ (rhsHasIO) {
                nodep->rhsp()->v3warn(E_UNSUPPORTED,
                                      "Static variable initializer\n"
                                          << nodep->rhsp()->warnMore()
                                          << "is dependent on function/task I/O variable");
            } അല്ലെങ്കിൽ {
                സ്ഥിരം തർക്കം rhsHasAutomatic
                    = nodep->rhsp()->exists([](സ്ഥിരം Ast_കണ്ണി_Var_Ref* സ്ഥിരം refp) {
                          തിരികെ refp->varp() && refp->varp()->lifetime().isAutomatic();
                      });
                എങ്കിൽ (rhsHasAutomatic) {
                    nodep->rhsp()->v3error("Static variable initializer\n"
                                           << nodep->rhsp()->warnMore()
                                           << "is dependent on automatic variable");
                }
            }
        }
    }
    ശൂന്യം visit(AstAlias* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::READWRITE;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstInitialStatic* nodep) override {
        VL_RESTORER(m_inInitialStatic);
        m_inInitialStatic = സത്യം;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitialStaticStmt);
        m_inInitialStaticStmt = സത്യം;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstRelease* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        VL_RESTORER(m_setContinuously);
        VL_RESTORER(m_setForcedByCode);
        m_setRefLvalue = VAccess::WRITE;
        m_setContinuously = അസത്യം;
        m_setForcedByCode = സത്യം;
        iterateAndNextNull(nodep->lhsp());
    }
    ശൂന്യം visit(AstDeassign* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        VL_RESTORER(m_setContinuously);
        m_setRefLvalue = VAccess::WRITE;
        m_setContinuously = അസത്യം;
        iterateAndNextNull(nodep->lhsp());
    }
    ശൂന്യം visit(AstFireEvent* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->operandp());
    }
    ശൂന്യം visit(AstCastDynamic* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->fromp());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->മുകൾ());
    }
    ശൂന്യം visit(AstFError* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->strp());
    }
    ശൂന്യം visit(AstFGetS* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->strgp());
    }
    ശൂന്യം visit(AstFRead* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->memp());
    }
    ശൂന്യം visit(AstFScanF* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->exprsp());
    }
    ശൂന്യം visit(AstFUngetC* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->rhsp());
    }
    ശൂന്യം visit(AstSScanF* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->exprsp());
    }
    ശൂന്യം visit(AstSysIgnore* nodep) override {
        // Can't know if lvalue or not; presume not
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstRand* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        എങ്കിൽ (!nodep->urandom()) m_setRefLvalue = VAccess::READWRITE;
        iterateAndNextNull(nodep->seedp());
    }
    ശൂന്യം visit(AstReadMem* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->memp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->filenamep());
        iterateAndNextNull(nodep->lsbp());
        iterateAndNextNull(nodep->msbp());
    }
    ശൂന്യം visit(AstTestPlusArgs* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->searchp());
    }
    ശൂന്യം visit(Ast_മൂല്യം_Plus_Args* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->searchp());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->outp());
    }
    ശൂന്യം visit(AstSFormat* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->lhsp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->fmtp());
    }
    ശൂന്യം visit(Ast_കണ്ണി_Dist_Biop* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::READWRITE;
        iterateAndNextNull(nodep->lhsp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->rhsp());
    }
    ശൂന്യം visit(Ast_കണ്ണി_Dist_Triop* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::READWRITE;
        iterateAndNextNull(nodep->lhsp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->rhsp());
        iterateAndNextNull(nodep->thsp());
    }
    // cppcheck-suppress constParameterPointer
    ശൂന്യം prepost_visit(Ast_കണ്ണി_Uniop* nodep) {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->lhsp());
    }
    ശൂന്യം visit(AstPreInc* nodep) override { prepost_visit(nodep); }
    ശൂന്യം visit(AstPostInc* nodep) override { prepost_visit(nodep); }
    ശൂന്യം visit(AstPreDec* nodep) override { prepost_visit(nodep); }
    ശൂന്യം visit(AstPostDec* nodep) override { prepost_visit(nodep); }

    // Nodes that change LValue state
    ശൂന്യം visit(AstSel* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->fromp());
        // Only set lvalues on the from
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->lsbp());
    }
    ശൂന്യം visit(Ast_കണ്ണി_Sel* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        // Only set lvalues on the from
        iterateAndNextNull(nodep->fromp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->bitp());
    }
    ശൂന്യം visit(AstCellArrayRef* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        // selp is not an lvalue
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->selp());
    }
    ശൂന്യം visit(Ast_കണ്ണി_Pre_Sel* nodep) override {
        എങ്കിൽ (AstSelBit* സ്ഥിരം selbitp = VN_CAST(nodep, SelBit)) selbitp->access(m_setRefLvalue);
        VL_RESTORER(m_setRefLvalue);
        // Only set lvalues on the from
        iterateAndNextNull(nodep->fromp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->rhsp());
        iterateAndNextNull(nodep->thsp());
    }
    ശൂന്യം visit(AstMemberSel* nodep) override {
        എങ്കിൽ (m_setRefLvalue != VAccess::NOCHANGE) {
            nodep->access(m_setRefLvalue);
            എങ്കിൽ (nodep->varp() && nodep->access().isWriteOrRW()) {
                എങ്കിൽ (സ്ഥിരം AstClockingItem* സ്ഥിരം itemp
                    = VN_CAST(nodep->varp()->backp(), ClockingItem)) {
                    UINFO(5, "ClkOut " << nodep);
                    എങ്കിൽ (itemp->outputp()) nodep->varp(itemp->outputp()->varp());
                }
            }
        } അല്ലെങ്കിൽ {
            // It is the only place where the access is set to member select nodes.
            // If it doesn't have to be set to WRITE, it means that it is READ.
            nodep->access(VAccess::READ);
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask_Ref* nodep) override {
        സ്ഥിരം Ast_കണ്ണി_FTask* സ്ഥിരം taskp = nodep->taskp();
        // We'll deal with mismatching pins later
        എങ്കിൽ (!taskp) തിരികെ;
        സ്ഥിരം V3TaskConnects tconnects
            = V3Task::taskConnects(nodep, taskp->stmtsp(), nullptr, അസത്യം);
        വേണ്ടി (സ്ഥിരം auto& tconnect : tconnects) {
            സ്ഥിരം AstVar* സ്ഥിരം portp = tconnect.first;
            സ്ഥിരം AstArg* സ്ഥിരം argp = tconnect.second;
            എങ്കിൽ (!argp) തുടരുക;
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം pinp = argp->exprp();
            എങ്കിൽ (!pinp) തുടരുക;
            VL_RESTORER(m_setRefLvalue);
            m_setRefLvalue = portp->direction().pinAccess();
            iterate(pinp);
        }
        എങ്കിൽ (nodep->withp()) iterate(nodep->withp());
    }
    ശൂന്യം visit(AstConstraint* nodep) override {
        VL_RESTORER(m_setIfRand);
        m_setIfRand = സത്യം;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask* nodep) override {
        VL_RESTORER(m_inFunc);
        m_inFunc = സത്യം;
        iterateChildren(nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    LinkLValueVisitor(Ast_കണ്ണി* nodep, VAccess start)
        : m_setRefLvalue{start} {
        iterate(nodep);
    }
    ~LinkLValueVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Link class functions

ശൂന്യം V3LinkLValue::linkLValue(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    { LinkLValueVisitor{nodep, VAccess::NOCHANGE}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("linklvalue", 0, dump_വൃക്ഷം_Either_Level() >= 6);
}
ശൂന്യം V3LinkLValue::linkLValueSet(Ast_കണ്ണി* സ്ഥിരം nodep, സ്ഥിരം VAccess access) {
    // Called by later link functions when it is known a node needs
    // to be converted to a lvalue.
    UINFO(9, __FUNCTION__ << ": ");
    { LinkLValueVisitor{nodep, access}; }
}
