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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3LinkLValue.h"

#अन्तर्भाव "V3Task.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

वग्ग LinkLValueVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    तार्किक m_setContinuously = मिच्छा;  // Set that var has some continuous assignment
    तार्किक m_setForcedByCode = मिच्छा;  // Set that var is the target of an AstAssignForce/AstRelease
    तार्किक m_setIfRand = मिच्छा;  // Update VarRefs if var declared as rand
    तार्किक m_setStrengthSpecified = मिच्छा;  // Set that var has assignment with strength specified.
    तार्किक m_inFunc = मिच्छा;  // Set if inside AstNodeFTask
    तार्किक m_inInitialStatic = मिच्छा;  // Set if inside AstInitialStatic
    तार्किक m_inInitialStaticStmt = मिच्छा;  // Set if inside AstInitialStaticStmt
    VAccess m_setRefLvalue;  // Set VarRefs to lvalues for pin assignments

    // VISITORS
    // Result handing
    शून्य visit(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        // VarRef: LValue its reference
        सचे (m_setIfRand && !(nodep->varp() && nodep->varp()->isRand())) पटिनिवत्त;
        सचे (m_setRefLvalue != VAccess::NOCHANGE) nodep->access(m_setRefLvalue);
        // Linked via an input-only modport by V3LinkDot, before it was known if written
        सचे (AstVarXRef* ध्रुव xrefp = VN_CAST(nodep, VarXRef)) {
            सचे (xrefp->readOnlyModport()) {
                सचे (nodep->access().isWriteOrRW()) {
                    nodep->v3error(
                        "Attempt to drive input-only modport: " << nodep->prettyNameQ());
                }
                xrefp->readOnlyModport(मिच्छा);
            }
        }
        सचे (nodep->varp() && nodep->access().isWriteOrRW()) {
            सचे (nodep->varp()->isParam()) {
                // All parameters that did get constified happened before now
                // as V3LinkLValue runs after V3Param
                nodep->v3error("Storing to parameter variable "
                               << nodep->prettyNameQ()
                               << " in a context that is determined only at runtime");
            }
            सचे (m_setContinuously) {
                nodep->varp()->isContinuously(सच्च);
                // Strength may only be specified in continuous assignment,
                // so it is needed to check only if m_setContinuously is true
                सचे (m_setStrengthSpecified) nodep->varp()->hasStrengthAssignment(सच्च);
            }
            सचे (ध्रुव AstClockingItem* ध्रुव itemp
                = VN_CAST(nodep->varp()->backp(), ClockingItem)) {
                UINFO(5, "ClkOut " << nodep);
                सचे (itemp->outputp()) nodep->varp(itemp->outputp()->varp());
            }
            सचे (m_setForcedByCode) {
                nodep->varp()->setForcedByCode();
                // If a public signal is being forced in SystemVerilog and VPI
                // is enabled, mark it as forceable to ensure that the VPI
                // functions read the forced value correctly
                सचे (v3Global.opt.vpi()
                    && (nodep->varp()->isSigPublic() || nodep->varp()->isSigModPublic())) {
                    nodep->varp()->setForceable();
                }
            } अञ्ञथा सचे (!nodep->varp()->isFuncLocal() && nodep->varp()->isReadOnly()) {
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
    शून्य visit(AstPin* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        सचे (nodep->modVarp() && nodep->modVarp()->isWritable()) {
            // When the varref's were created, we didn't know the I/O state
            // Now that we do, and it's from a output, we know it's a lvalue
            m_setRefLvalue = VAccess::WRITE;
            iterateChildren(nodep);
            m_setRefLvalue = VAccess::NOCHANGE;
        } अञ्ञथा {
            iterateChildren(nodep);
        }
    }
    शून्य visit(Ast_ग्रन्थिः_Assign* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        VL_RESTORER(m_setContinuously);
        VL_RESTORER(m_setStrengthSpecified);
        {
            m_setRefLvalue = VAccess::WRITE;
            m_setContinuously = VN_IS(nodep, AssignW);
            सचे (ध्रुव AstAssignW* ध्रुव assignwp = VN_CAST(nodep, AssignW)) {
                सचे (assignwp->strengthSpecp()) m_setStrengthSpecified = सच्च;
            }
            {
                VL_RESTORER(m_setForcedByCode);
                m_setForcedByCode = VN_IS(nodep, AssignForce);
                iterateAndNextNull(nodep->lhsp());
            }
            m_setRefLvalue = VAccess::NOCHANGE;
            m_setContinuously = मिच्छा;
            m_setStrengthSpecified = मिच्छा;
            iterateAndNextNull(nodep->rhsp());
        }

        सचे ((m_inInitialStatic || m_inInitialStaticStmt) && m_inFunc) {
            ध्रुव तार्किक rhsHasIO = nodep->rhsp()->exists([](ध्रुव Ast_ग्रन्थिः_Var_Ref* ध्रुव refp) {
                // Exclude module I/O referenced from a function/task.
                पटिनिवत्त refp->varp() && refp->varp()->isIO()
                       && refp->varp()->lifetime() != VLifetime::NONE;
            });
            सचे (rhsHasIO) {
                nodep->rhsp()->v3warn(E_UNSUPPORTED,
                                      "Static variable initializer\n"
                                          << nodep->rhsp()->warnMore()
                                          << "is dependent on function/task I/O variable");
            } अञ्ञथा {
                ध्रुव तार्किक rhsHasAutomatic
                    = nodep->rhsp()->exists([](ध्रुव Ast_ग्रन्थिः_Var_Ref* ध्रुव refp) {
                          पटिनिवत्त refp->varp() && refp->varp()->lifetime().isAutomatic();
                      });
                सचे (rhsHasAutomatic) {
                    nodep->rhsp()->v3error("Static variable initializer\n"
                                           << nodep->rhsp()->warnMore()
                                           << "is dependent on automatic variable");
                }
            }
        }
    }
    शून्य visit(AstAlias* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::READWRITE;
        iterateChildren(nodep);
    }
    शून्य visit(AstInitialStatic* nodep) override {
        VL_RESTORER(m_inInitialStatic);
        m_inInitialStatic = सच्च;
        iterateChildren(nodep);
    }
    शून्य visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitialStaticStmt);
        m_inInitialStaticStmt = सच्च;
        iterateChildren(nodep);
    }
    शून्य visit(AstRelease* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        VL_RESTORER(m_setContinuously);
        VL_RESTORER(m_setForcedByCode);
        m_setRefLvalue = VAccess::WRITE;
        m_setContinuously = मिच्छा;
        m_setForcedByCode = सच्च;
        iterateAndNextNull(nodep->lhsp());
    }
    शून्य visit(AstDeassign* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        VL_RESTORER(m_setContinuously);
        m_setRefLvalue = VAccess::WRITE;
        m_setContinuously = मिच्छा;
        iterateAndNextNull(nodep->lhsp());
    }
    शून्य visit(AstFireEvent* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->operandp());
    }
    शून्य visit(AstCastDynamic* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->fromp());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->शिखरम्());
    }
    शून्य visit(AstFError* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->strp());
    }
    शून्य visit(AstFGetS* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->strgp());
    }
    शून्य visit(AstFRead* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->memp());
    }
    शून्य visit(AstFScanF* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->exprsp());
    }
    शून्य visit(AstFUngetC* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->rhsp());
    }
    शून्य visit(AstSScanF* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->exprsp());
    }
    शून्य visit(AstSysIgnore* nodep) override {
        // Can't know if lvalue or not; presume not
        iterateChildren(nodep);
    }
    शून्य visit(AstRand* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        सचे (!nodep->urandom()) m_setRefLvalue = VAccess::READWRITE;
        iterateAndNextNull(nodep->seedp());
    }
    शून्य visit(AstReadMem* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->memp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->filenamep());
        iterateAndNextNull(nodep->lsbp());
        iterateAndNextNull(nodep->msbp());
    }
    शून्य visit(AstTestPlusArgs* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->searchp());
    }
    शून्य visit(Ast_मानम्_Plus_Args* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->searchp());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->outp());
    }
    शून्य visit(AstSFormat* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->lhsp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->fmtp());
    }
    शून्य visit(Ast_ग्रन्थिः_Dist_Biop* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::READWRITE;
        iterateAndNextNull(nodep->lhsp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->rhsp());
    }
    शून्य visit(Ast_ग्रन्थिः_Dist_Triop* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::READWRITE;
        iterateAndNextNull(nodep->lhsp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->rhsp());
        iterateAndNextNull(nodep->thsp());
    }
    // cppcheck-suppress constParameterPointer
    शून्य prepost_visit(Ast_ग्रन्थिः_Uniop* nodep) {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->lhsp());
    }
    शून्य visit(AstPreInc* nodep) override { prepost_visit(nodep); }
    शून्य visit(AstPostInc* nodep) override { prepost_visit(nodep); }
    शून्य visit(AstPreDec* nodep) override { prepost_visit(nodep); }
    शून्य visit(AstPostDec* nodep) override { prepost_visit(nodep); }

    // Nodes that change LValue state
    शून्य visit(AstSel* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->fromp());
        // Only set lvalues on the from
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->lsbp());
    }
    शून्य visit(Ast_ग्रन्थिः_Sel* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        // Only set lvalues on the from
        iterateAndNextNull(nodep->fromp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->bitp());
    }
    शून्य visit(AstCellArrayRef* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        // selp is not an lvalue
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->selp());
    }
    शून्य visit(Ast_ग्रन्थिः_Pre_Sel* nodep) override {
        सचे (AstSelBit* ध्रुव selbitp = VN_CAST(nodep, SelBit)) selbitp->access(m_setRefLvalue);
        VL_RESTORER(m_setRefLvalue);
        // Only set lvalues on the from
        iterateAndNextNull(nodep->fromp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->rhsp());
        iterateAndNextNull(nodep->thsp());
    }
    शून्य visit(AstMemberSel* nodep) override {
        सचे (m_setRefLvalue != VAccess::NOCHANGE) {
            nodep->access(m_setRefLvalue);
            सचे (nodep->varp() && nodep->access().isWriteOrRW()) {
                सचे (ध्रुव AstClockingItem* ध्रुव itemp
                    = VN_CAST(nodep->varp()->backp(), ClockingItem)) {
                    UINFO(5, "ClkOut " << nodep);
                    सचे (itemp->outputp()) nodep->varp(itemp->outputp()->varp());
                }
            }
        } अञ्ञथा {
            // It is the only place where the access is set to member select nodes.
            // If it doesn't have to be set to WRITE, it means that it is READ.
            nodep->access(VAccess::READ);
        }
        iterateChildren(nodep);
    }
    शून्य visit(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        ध्रुव Ast_ग्रन्थिः_FTask* ध्रुव taskp = nodep->taskp();
        // We'll deal with mismatching pins later
        सचे (!taskp) पटिनिवत्त;
        ध्रुव V3TaskConnects tconnects
            = V3Task::taskConnects(nodep, taskp->stmtsp(), nullptr, मिच्छा);
        अत्थाय (ध्रुव auto& tconnect : tconnects) {
            ध्रुव AstVar* ध्रुव portp = tconnect.first;
            ध्रुव AstArg* ध्रुव argp = tconnect.second;
            सचे (!argp) अनुवर्त;
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव pinp = argp->exprp();
            सचे (!pinp) अनुवर्त;
            VL_RESTORER(m_setRefLvalue);
            m_setRefLvalue = portp->direction().pinAccess();
            iterate(pinp);
        }
        सचे (nodep->withp()) iterate(nodep->withp());
    }
    शून्य visit(AstConstraint* nodep) override {
        VL_RESTORER(m_setIfRand);
        m_setIfRand = सच्च;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_ग्रन्थिः_FTask* nodep) override {
        VL_RESTORER(m_inFunc);
        m_inFunc = सच्च;
        iterateChildren(nodep);
    }

    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    LinkLValueVisitor(Ast_ग्रन्थिः* nodep, VAccess start)
        : m_setRefLvalue{start} {
        iterate(nodep);
    }
    ~LinkLValueVisitor() override = स्वभाव;
};

//######################################################################
// Link class functions

शून्य V3LinkLValue::linkLValue(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    { LinkLValueVisitor{nodep, VAccess::NOCHANGE}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("linklvalue", 0, dump_वृक्षः_Either_Level() >= 6);
}
शून्य V3LinkLValue::linkLValueSet(Ast_ग्रन्थिः* ध्रुव nodep, ध्रुव VAccess access) {
    // Called by later link functions when it is known a node needs
    // to be converted to a lvalue.
    UINFO(9, __FUNCTION__ << ": ");
    { LinkLValueVisitor{nodep, access}; }
}
