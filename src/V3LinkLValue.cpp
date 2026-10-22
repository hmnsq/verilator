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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3LinkLValue.h"

#અન્તર્ભાવ "V3Task.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

વર્ગ LinkLValueVisitor final : જાહેર VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    તાર્કિક m_setContinuously = ખોટું;  // Set that var has some continuous assignment
    તાર્કિક m_setForcedByCode = ખોટું;  // Set that var is the target of an AstAssignForce/AstRelease
    તાર્કિક m_setIfRand = ખોટું;  // Update VarRefs if var declared as rand
    તાર્કિક m_setStrengthSpecified = ખોટું;  // Set that var has assignment with strength specified.
    તાર્કિક m_inFunc = ખોટું;  // Set if inside AstNodeFTask
    તાર્કિક m_inInitialStatic = ખોટું;  // Set if inside AstInitialStatic
    તાર્કિક m_inInitialStaticStmt = ખોટું;  // Set if inside AstInitialStaticStmt
    VAccess m_setRefLvalue;  // Set VarRefs to lvalues for pin assignments

    // VISITORS
    // Result handing
    શૂન્ય visit(Ast_ગાંઠ_Var_Ref* nodep) override {
        // VarRef: LValue its reference
        જો (m_setIfRand && !(nodep->varp() && nodep->varp()->isRand())) પાછા;
        જો (m_setRefLvalue != VAccess::NOCHANGE) nodep->access(m_setRefLvalue);
        // Linked via an input-only modport by V3LinkDot, before it was known if written
        જો (AstVarXRef* અચલ xrefp = VN_CAST(nodep, VarXRef)) {
            જો (xrefp->readOnlyModport()) {
                જો (nodep->access().isWriteOrRW()) {
                    nodep->v3error(
                        "Attempt to drive input-only modport: " << nodep->prettyNameQ());
                }
                xrefp->readOnlyModport(ખોટું);
            }
        }
        જો (nodep->varp() && nodep->access().isWriteOrRW()) {
            જો (nodep->varp()->isParam()) {
                // All parameters that did get constified happened before now
                // as V3LinkLValue runs after V3Param
                nodep->v3error("Storing to parameter variable "
                               << nodep->prettyNameQ()
                               << " in a context that is determined only at runtime");
            }
            જો (m_setContinuously) {
                nodep->varp()->isContinuously(સાચું);
                // Strength may only be specified in continuous assignment,
                // so it is needed to check only if m_setContinuously is true
                જો (m_setStrengthSpecified) nodep->varp()->hasStrengthAssignment(સાચું);
            }
            જો (અચલ AstClockingItem* અચલ itemp
                = VN_CAST(nodep->varp()->backp(), ClockingItem)) {
                UINFO(5, "ClkOut " << nodep);
                જો (itemp->outputp()) nodep->varp(itemp->outputp()->varp());
            }
            જો (m_setForcedByCode) {
                nodep->varp()->setForcedByCode();
                // If a public signal is being forced in SystemVerilog and VPI
                // is enabled, mark it as forceable to ensure that the VPI
                // functions read the forced value correctly
                જો (v3Global.opt.vpi()
                    && (nodep->varp()->isSigPublic() || nodep->varp()->isSigModPublic())) {
                    nodep->varp()->setForceable();
                }
            } નહીંતર જો (!nodep->varp()->isFuncLocal() && nodep->varp()->isReadOnly()) {
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
    શૂન્ય visit(AstPin* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        જો (nodep->modVarp() && nodep->modVarp()->isWritable()) {
            // When the varref's were created, we didn't know the I/O state
            // Now that we do, and it's from a output, we know it's a lvalue
            m_setRefLvalue = VAccess::WRITE;
            iterateChildren(nodep);
            m_setRefLvalue = VAccess::NOCHANGE;
        } નહીંતર {
            iterateChildren(nodep);
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_Assign* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        VL_RESTORER(m_setContinuously);
        VL_RESTORER(m_setStrengthSpecified);
        {
            m_setRefLvalue = VAccess::WRITE;
            m_setContinuously = VN_IS(nodep, AssignW);
            જો (અચલ AstAssignW* અચલ assignwp = VN_CAST(nodep, AssignW)) {
                જો (assignwp->strengthSpecp()) m_setStrengthSpecified = સાચું;
            }
            {
                VL_RESTORER(m_setForcedByCode);
                m_setForcedByCode = VN_IS(nodep, AssignForce);
                iterateAndNextNull(nodep->lhsp());
            }
            m_setRefLvalue = VAccess::NOCHANGE;
            m_setContinuously = ખોટું;
            m_setStrengthSpecified = ખોટું;
            iterateAndNextNull(nodep->rhsp());
        }

        જો ((m_inInitialStatic || m_inInitialStaticStmt) && m_inFunc) {
            અચલ તાર્કિક rhsHasIO = nodep->rhsp()->exists([](અચલ Ast_ગાંઠ_Var_Ref* અચલ refp) {
                // Exclude module I/O referenced from a function/task.
                પાછા refp->varp() && refp->varp()->isIO()
                       && refp->varp()->lifetime() != VLifetime::NONE;
            });
            જો (rhsHasIO) {
                nodep->rhsp()->v3warn(E_UNSUPPORTED,
                                      "Static variable initializer\n"
                                          << nodep->rhsp()->warnMore()
                                          << "is dependent on function/task I/O variable");
            } નહીંતર {
                અચલ તાર્કિક rhsHasAutomatic
                    = nodep->rhsp()->exists([](અચલ Ast_ગાંઠ_Var_Ref* અચલ refp) {
                          પાછા refp->varp() && refp->varp()->lifetime().isAutomatic();
                      });
                જો (rhsHasAutomatic) {
                    nodep->rhsp()->v3error("Static variable initializer\n"
                                           << nodep->rhsp()->warnMore()
                                           << "is dependent on automatic variable");
                }
            }
        }
    }
    શૂન્ય visit(AstAlias* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::READWRITE;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstInitialStatic* nodep) override {
        VL_RESTORER(m_inInitialStatic);
        m_inInitialStatic = સાચું;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitialStaticStmt);
        m_inInitialStaticStmt = સાચું;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstRelease* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        VL_RESTORER(m_setContinuously);
        VL_RESTORER(m_setForcedByCode);
        m_setRefLvalue = VAccess::WRITE;
        m_setContinuously = ખોટું;
        m_setForcedByCode = સાચું;
        iterateAndNextNull(nodep->lhsp());
    }
    શૂન્ય visit(AstDeassign* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        VL_RESTORER(m_setContinuously);
        m_setRefLvalue = VAccess::WRITE;
        m_setContinuously = ખોટું;
        iterateAndNextNull(nodep->lhsp());
    }
    શૂન્ય visit(AstFireEvent* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->operandp());
    }
    શૂન્ય visit(AstCastDynamic* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->fromp());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->ટોચ());
    }
    શૂન્ય visit(AstFError* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->strp());
    }
    શૂન્ય visit(AstFGetS* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->strgp());
    }
    શૂન્ય visit(AstFRead* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->memp());
    }
    શૂન્ય visit(AstFScanF* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->exprsp());
    }
    શૂન્ય visit(AstFUngetC* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->rhsp());
    }
    શૂન્ય visit(AstSScanF* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->exprsp());
    }
    શૂન્ય visit(AstSysIgnore* nodep) override {
        // Can't know if lvalue or not; presume not
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstRand* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        જો (!nodep->urandom()) m_setRefLvalue = VAccess::READWRITE;
        iterateAndNextNull(nodep->seedp());
    }
    શૂન્ય visit(AstReadMem* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->memp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->filenamep());
        iterateAndNextNull(nodep->lsbp());
        iterateAndNextNull(nodep->msbp());
    }
    શૂન્ય visit(AstTestPlusArgs* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->searchp());
    }
    શૂન્ય visit(Ast_કિંમત_Plus_Args* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->searchp());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->outp());
    }
    શૂન્ય visit(AstSFormat* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->lhsp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->fmtp());
    }
    શૂન્ય visit(Ast_ગાંઠ_Dist_Biop* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::READWRITE;
        iterateAndNextNull(nodep->lhsp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->rhsp());
    }
    શૂન્ય visit(Ast_ગાંઠ_Dist_Triop* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::READWRITE;
        iterateAndNextNull(nodep->lhsp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->rhsp());
        iterateAndNextNull(nodep->thsp());
    }
    // cppcheck-suppress constParameterPointer
    શૂન્ય prepost_visit(Ast_ગાંઠ_Uniop* nodep) {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->lhsp());
    }
    શૂન્ય visit(AstPreInc* nodep) override { prepost_visit(nodep); }
    શૂન્ય visit(AstPostInc* nodep) override { prepost_visit(nodep); }
    શૂન્ય visit(AstPreDec* nodep) override { prepost_visit(nodep); }
    શૂન્ય visit(AstPostDec* nodep) override { prepost_visit(nodep); }

    // Nodes that change LValue state
    શૂન્ય visit(AstSel* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->fromp());
        // Only set lvalues on the from
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->lsbp());
    }
    શૂન્ય visit(Ast_ગાંઠ_Sel* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        // Only set lvalues on the from
        iterateAndNextNull(nodep->fromp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->bitp());
    }
    શૂન્ય visit(AstCellArrayRef* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        // selp is not an lvalue
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->selp());
    }
    શૂન્ય visit(Ast_ગાંઠ_Pre_Sel* nodep) override {
        જો (AstSelBit* અચલ selbitp = VN_CAST(nodep, SelBit)) selbitp->access(m_setRefLvalue);
        VL_RESTORER(m_setRefLvalue);
        // Only set lvalues on the from
        iterateAndNextNull(nodep->fromp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->rhsp());
        iterateAndNextNull(nodep->thsp());
    }
    શૂન્ય visit(AstMemberSel* nodep) override {
        જો (m_setRefLvalue != VAccess::NOCHANGE) {
            nodep->access(m_setRefLvalue);
            જો (nodep->varp() && nodep->access().isWriteOrRW()) {
                જો (અચલ AstClockingItem* અચલ itemp
                    = VN_CAST(nodep->varp()->backp(), ClockingItem)) {
                    UINFO(5, "ClkOut " << nodep);
                    જો (itemp->outputp()) nodep->varp(itemp->outputp()->varp());
                }
            }
        } નહીંતર {
            // It is the only place where the access is set to member select nodes.
            // If it doesn't have to be set to WRITE, it means that it is READ.
            nodep->access(VAccess::READ);
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask_Ref* nodep) override {
        અચલ Ast_ગાંઠ_FTask* અચલ taskp = nodep->taskp();
        // We'll deal with mismatching pins later
        જો (!taskp) પાછા;
        અચલ V3TaskConnects tconnects
            = V3Task::taskConnects(nodep, taskp->stmtsp(), nullptr, ખોટું);
        માટે (અચલ auto& tconnect : tconnects) {
            અચલ AstVar* અચલ portp = tconnect.first;
            અચલ AstArg* અચલ argp = tconnect.second;
            જો (!argp) ચાલુ;
            Ast_ગાંઠ_પદાવલિ* અચલ pinp = argp->exprp();
            જો (!pinp) ચાલુ;
            VL_RESTORER(m_setRefLvalue);
            m_setRefLvalue = portp->direction().pinAccess();
            iterate(pinp);
        }
        જો (nodep->withp()) iterate(nodep->withp());
    }
    શૂન્ય visit(AstConstraint* nodep) override {
        VL_RESTORER(m_setIfRand);
        m_setIfRand = સાચું;
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask* nodep) override {
        VL_RESTORER(m_inFunc);
        m_inFunc = સાચું;
        iterateChildren(nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    LinkLValueVisitor(Ast_ગાંઠ* nodep, VAccess start)
        : m_setRefLvalue{start} {
        iterate(nodep);
    }
    ~LinkLValueVisitor() override = મૂળભૂત;
};

//######################################################################
// Link class functions

શૂન્ય V3LinkLValue::linkLValue(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    { LinkLValueVisitor{nodep, VAccess::NOCHANGE}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("linklvalue", 0, dump_વૃક્ષ_Either_Level() >= 6);
}
શૂન્ય V3LinkLValue::linkLValueSet(Ast_ગાંઠ* અચલ nodep, અચલ VAccess access) {
    // Called by later link functions when it is known a node needs
    // to be converted to a lvalue.
    UINFO(9, __FUNCTION__ << ": ");
    { LinkLValueVisitor{nodep, access}; }
}
