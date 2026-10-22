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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3LinkLValue.h"

#ଅନ୍ତର୍ଭାଵ "V3Task.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

ଶ୍ରେଣୀ LinkLValueVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    ତାର୍କିକ m_setContinuously = ମିଥ୍ୟା;  // Set that var has some continuous assignment
    ତାର୍କିକ m_setForcedByCode = ମିଥ୍ୟା;  // Set that var is the target of an AstAssignForce/AstRelease
    ତାର୍କିକ m_setIfRand = ମିଥ୍ୟା;  // Update VarRefs if var declared as rand
    ତାର୍କିକ m_setStrengthSpecified = ମିଥ୍ୟା;  // Set that var has assignment with strength specified.
    ତାର୍କିକ m_inFunc = ମିଥ୍ୟା;  // Set if inside AstNodeFTask
    ତାର୍କିକ m_inInitialStatic = ମିଥ୍ୟା;  // Set if inside AstInitialStatic
    ତାର୍କିକ m_inInitialStaticStmt = ମିଥ୍ୟା;  // Set if inside AstInitialStaticStmt
    VAccess m_setRefLvalue;  // Set VarRefs to lvalues for pin assignments

    // VISITORS
    // Result handing
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Var_Ref* nodep) override {
        // VarRef: LValue its reference
        ଯଦି (m_setIfRand && !(nodep->varp() && nodep->varp()->isRand())) ଫେରନ୍ତୁ;
        ଯଦି (m_setRefLvalue != VAccess::NOCHANGE) nodep->access(m_setRefLvalue);
        // Linked via an input-only modport by V3LinkDot, before it was known if written
        ଯଦି (AstVarXRef* ଅଚଳ xrefp = VN_CAST(nodep, VarXRef)) {
            ଯଦି (xrefp->readOnlyModport()) {
                ଯଦି (nodep->access().isWriteOrRW()) {
                    nodep->v3error(
                        "Attempt to drive input-only modport: " << nodep->prettyNameQ());
                }
                xrefp->readOnlyModport(ମିଥ୍ୟା);
            }
        }
        ଯଦି (nodep->varp() && nodep->access().isWriteOrRW()) {
            ଯଦି (nodep->varp()->isParam()) {
                // All parameters that did get constified happened before now
                // as V3LinkLValue runs after V3Param
                nodep->v3error("Storing to parameter variable "
                               << nodep->prettyNameQ()
                               << " in a context that is determined only at runtime");
            }
            ଯଦି (m_setContinuously) {
                nodep->varp()->isContinuously(ସତ୍ୟ);
                // Strength may only be specified in continuous assignment,
                // so it is needed to check only if m_setContinuously is true
                ଯଦି (m_setStrengthSpecified) nodep->varp()->hasStrengthAssignment(ସତ୍ୟ);
            }
            ଯଦି (ଅଚଳ AstClockingItem* ଅଚଳ itemp
                = VN_CAST(nodep->varp()->backp(), ClockingItem)) {
                UINFO(5, "ClkOut " << nodep);
                ଯଦି (itemp->outputp()) nodep->varp(itemp->outputp()->varp());
            }
            ଯଦି (m_setForcedByCode) {
                nodep->varp()->setForcedByCode();
                // If a public signal is being forced in SystemVerilog and VPI
                // is enabled, mark it as forceable to ensure that the VPI
                // functions read the forced value correctly
                ଯଦି (v3Global.opt.vpi()
                    && (nodep->varp()->isSigPublic() || nodep->varp()->isSigModPublic())) {
                    nodep->varp()->setForceable();
                }
            } ଅନ୍ୟଥା ଯଦି (!nodep->varp()->isFuncLocal() && nodep->varp()->isReadOnly()) {
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
    ଶୂନ୍ୟ visit(AstPin* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        ଯଦି (nodep->modVarp() && nodep->modVarp()->isWritable()) {
            // When the varref's were created, we didn't know the I/O state
            // Now that we do, and it's from a output, we know it's a lvalue
            m_setRefLvalue = VAccess::WRITE;
            iterateChildren(nodep);
            m_setRefLvalue = VAccess::NOCHANGE;
        } ଅନ୍ୟଥା {
            iterateChildren(nodep);
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Assign* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        VL_RESTORER(m_setContinuously);
        VL_RESTORER(m_setStrengthSpecified);
        {
            m_setRefLvalue = VAccess::WRITE;
            m_setContinuously = VN_IS(nodep, AssignW);
            ଯଦି (ଅଚଳ AstAssignW* ଅଚଳ assignwp = VN_CAST(nodep, AssignW)) {
                ଯଦି (assignwp->strengthSpecp()) m_setStrengthSpecified = ସତ୍ୟ;
            }
            {
                VL_RESTORER(m_setForcedByCode);
                m_setForcedByCode = VN_IS(nodep, AssignForce);
                iterateAndNextNull(nodep->lhsp());
            }
            m_setRefLvalue = VAccess::NOCHANGE;
            m_setContinuously = ମିଥ୍ୟା;
            m_setStrengthSpecified = ମିଥ୍ୟା;
            iterateAndNextNull(nodep->rhsp());
        }

        ଯଦି ((m_inInitialStatic || m_inInitialStaticStmt) && m_inFunc) {
            ଅଚଳ ତାର୍କିକ rhsHasIO = nodep->rhsp()->exists([](ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ refp) {
                // Exclude module I/O referenced from a function/task.
                ଫେରନ୍ତୁ refp->varp() && refp->varp()->isIO()
                       && refp->varp()->lifetime() != VLifetime::NONE;
            });
            ଯଦି (rhsHasIO) {
                nodep->rhsp()->v3warn(E_UNSUPPORTED,
                                      "Static variable initializer\n"
                                          << nodep->rhsp()->warnMore()
                                          << "is dependent on function/task I/O variable");
            } ଅନ୍ୟଥା {
                ଅଚଳ ତାର୍କିକ rhsHasAutomatic
                    = nodep->rhsp()->exists([](ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ refp) {
                          ଫେରନ୍ତୁ refp->varp() && refp->varp()->lifetime().isAutomatic();
                      });
                ଯଦି (rhsHasAutomatic) {
                    nodep->rhsp()->v3error("Static variable initializer\n"
                                           << nodep->rhsp()->warnMore()
                                           << "is dependent on automatic variable");
                }
            }
        }
    }
    ଶୂନ୍ୟ visit(AstAlias* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::READWRITE;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstInitialStatic* nodep) override {
        VL_RESTORER(m_inInitialStatic);
        m_inInitialStatic = ସତ୍ୟ;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitialStaticStmt);
        m_inInitialStaticStmt = ସତ୍ୟ;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstRelease* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        VL_RESTORER(m_setContinuously);
        VL_RESTORER(m_setForcedByCode);
        m_setRefLvalue = VAccess::WRITE;
        m_setContinuously = ମିଥ୍ୟା;
        m_setForcedByCode = ସତ୍ୟ;
        iterateAndNextNull(nodep->lhsp());
    }
    ଶୂନ୍ୟ visit(AstDeassign* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        VL_RESTORER(m_setContinuously);
        m_setRefLvalue = VAccess::WRITE;
        m_setContinuously = ମିଥ୍ୟା;
        iterateAndNextNull(nodep->lhsp());
    }
    ଶୂନ୍ୟ visit(AstFireEvent* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->operandp());
    }
    ଶୂନ୍ୟ visit(AstCastDynamic* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->fromp());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->ଶୀର୍ଷ());
    }
    ଶୂନ୍ୟ visit(AstFError* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->strp());
    }
    ଶୂନ୍ୟ visit(AstFGetS* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->strgp());
    }
    ଶୂନ୍ୟ visit(AstFRead* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->memp());
    }
    ଶୂନ୍ୟ visit(AstFScanF* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->exprsp());
    }
    ଶୂନ୍ୟ visit(AstFUngetC* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->rhsp());
    }
    ଶୂନ୍ୟ visit(AstSScanF* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->exprsp());
    }
    ଶୂନ୍ୟ visit(AstSysIgnore* nodep) override {
        // Can't know if lvalue or not; presume not
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstRand* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        ଯଦି (!nodep->urandom()) m_setRefLvalue = VAccess::READWRITE;
        iterateAndNextNull(nodep->seedp());
    }
    ଶୂନ୍ୟ visit(AstReadMem* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->memp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->filenamep());
        iterateAndNextNull(nodep->lsbp());
        iterateAndNextNull(nodep->msbp());
    }
    ଶୂନ୍ୟ visit(AstTestPlusArgs* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->searchp());
    }
    ଶୂନ୍ୟ visit(Ast_ମୂଲ୍ୟ_Plus_Args* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->searchp());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->outp());
    }
    ଶୂନ୍ୟ visit(AstSFormat* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->lhsp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->fmtp());
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Dist_Biop* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::READWRITE;
        iterateAndNextNull(nodep->lhsp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->rhsp());
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Dist_Triop* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::READWRITE;
        iterateAndNextNull(nodep->lhsp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->rhsp());
        iterateAndNextNull(nodep->thsp());
    }
    // cppcheck-suppress constParameterPointer
    ଶୂନ୍ୟ prepost_visit(Ast_ଗଣ୍ଠି_Uniop* nodep) {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->lhsp());
    }
    ଶୂନ୍ୟ visit(AstPreInc* nodep) override { prepost_visit(nodep); }
    ଶୂନ୍ୟ visit(AstPostInc* nodep) override { prepost_visit(nodep); }
    ଶୂନ୍ୟ visit(AstPreDec* nodep) override { prepost_visit(nodep); }
    ଶୂନ୍ୟ visit(AstPostDec* nodep) override { prepost_visit(nodep); }

    // Nodes that change LValue state
    ଶୂନ୍ୟ visit(AstSel* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->fromp());
        // Only set lvalues on the from
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->lsbp());
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Sel* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        // Only set lvalues on the from
        iterateAndNextNull(nodep->fromp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->bitp());
    }
    ଶୂନ୍ୟ visit(AstCellArrayRef* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        // selp is not an lvalue
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->selp());
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Pre_Sel* nodep) override {
        ଯଦି (AstSelBit* ଅଚଳ selbitp = VN_CAST(nodep, SelBit)) selbitp->access(m_setRefLvalue);
        VL_RESTORER(m_setRefLvalue);
        // Only set lvalues on the from
        iterateAndNextNull(nodep->fromp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->rhsp());
        iterateAndNextNull(nodep->thsp());
    }
    ଶୂନ୍ୟ visit(AstMemberSel* nodep) override {
        ଯଦି (m_setRefLvalue != VAccess::NOCHANGE) {
            nodep->access(m_setRefLvalue);
            ଯଦି (nodep->varp() && nodep->access().isWriteOrRW()) {
                ଯଦି (ଅଚଳ AstClockingItem* ଅଚଳ itemp
                    = VN_CAST(nodep->varp()->backp(), ClockingItem)) {
                    UINFO(5, "ClkOut " << nodep);
                    ଯଦି (itemp->outputp()) nodep->varp(itemp->outputp()->varp());
                }
            }
        } ଅନ୍ୟଥା {
            // It is the only place where the access is set to member select nodes.
            // If it doesn't have to be set to WRITE, it means that it is READ.
            nodep->access(VAccess::READ);
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask_Ref* nodep) override {
        ଅଚଳ Ast_ଗଣ୍ଠି_FTask* ଅଚଳ taskp = nodep->taskp();
        // We'll deal with mismatching pins later
        ଯଦି (!taskp) ଫେରନ୍ତୁ;
        ଅଚଳ V3TaskConnects tconnects
            = V3Task::taskConnects(nodep, taskp->stmtsp(), nullptr, ମିଥ୍ୟା);
        ପାଇଁ (ଅଚଳ auto& tconnect : tconnects) {
            ଅଚଳ AstVar* ଅଚଳ portp = tconnect.first;
            ଅଚଳ AstArg* ଅଚଳ argp = tconnect.second;
            ଯଦି (!argp) ଜାରି;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ pinp = argp->exprp();
            ଯଦି (!pinp) ଜାରି;
            VL_RESTORER(m_setRefLvalue);
            m_setRefLvalue = portp->direction().pinAccess();
            iterate(pinp);
        }
        ଯଦି (nodep->withp()) iterate(nodep->withp());
    }
    ଶୂନ୍ୟ visit(AstConstraint* nodep) override {
        VL_RESTORER(m_setIfRand);
        m_setIfRand = ସତ୍ୟ;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask* nodep) override {
        VL_RESTORER(m_inFunc);
        m_inFunc = ସତ୍ୟ;
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    LinkLValueVisitor(Ast_ଗଣ୍ଠି* nodep, VAccess start)
        : m_setRefLvalue{start} {
        iterate(nodep);
    }
    ~LinkLValueVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Link class functions

ଶୂନ୍ୟ V3LinkLValue::linkLValue(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    { LinkLValueVisitor{nodep, VAccess::NOCHANGE}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("linklvalue", 0, dump_ବୃକ୍ଷ_Either_Level() >= 6);
}
ଶୂନ୍ୟ V3LinkLValue::linkLValueSet(Ast_ଗଣ୍ଠି* ଅଚଳ nodep, ଅଚଳ VAccess access) {
    // Called by later link functions when it is known a node needs
    // to be converted to a lvalue.
    UINFO(9, __FUNCTION__ << ": ");
    { LinkLValueVisitor{nodep, access}; }
}
