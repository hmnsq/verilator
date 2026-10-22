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

वर्ग LinkLValueVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    तार्किक m_समुच्चयः_Continuously = असत्य;  // Set that var has some continuous assignment
    तार्किक m_समुच्चयः_Forced_By_संहिता = असत्य;  // Set that var is the target of an AstAssignForce/AstRelease
    तार्किक m_समुच्चयः_If_Rand = असत्य;  // Update VarRefs if var declared as rand
    तार्किक m_समुच्चयः_Strength_Specified = असत्य;  // Set that var has assignment with strength specified.
    तार्किक m_अन्तर्_Func = असत्य;  // Set if inside AstNodeFTask
    तार्किक m_अन्तर्_Initial_Static = असत्य;  // Set if inside AstInitialStatic
    तार्किक m_अन्तर्_Initial_Static_Stmt = असत्य;  // Set if inside AstInitialStaticStmt
    VAccess m_समुच्चयः_Ref_Lvalue;  // Set VarRefs to lvalues for pin assignments

    // VISITORS
    // Result handing
    शून्य दर्शय(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        // VarRef: LValue its reference
        यदि (m_समुच्चयः_If_Rand && !(nodep->varp() && nodep->varp()->isRand())) प्रत्यागम;
        यदि (m_समुच्चयः_Ref_Lvalue != VAccess::NOCHANGE) nodep->access(m_समुच्चयः_Ref_Lvalue);
        // Linked via an input-only modport by V3LinkDot, before it was known if written
        यदि (AstVarXRef* ध्रुव xrefp = VN_CAST(nodep, VarXRef)) {
            यदि (xrefp->readOnlyModport()) {
                यदि (nodep->access().isWriteOrRW()) {
                    nodep->v3error(
                        "Attempt to drive input-only modport: " << nodep->prettyNameQ());
                }
                xrefp->readOnlyModport(असत्य);
            }
        }
        यदि (nodep->varp() && nodep->access().isWriteOrRW()) {
            यदि (nodep->varp()->isParam()) {
                // All parameters that did get constified happened before now
                // as V3LinkLValue runs after V3Param
                nodep->v3error("Storing to parameter variable "
                               << nodep->prettyNameQ()
                               << " in a context that is determined only at runtime");
            }
            यदि (m_समुच्चयः_Continuously) {
                nodep->varp()->isContinuously(सत्य);
                // Strength may only be specified in continuous assignment,
                // so it is needed to check only if m_setContinuously is true
                यदि (m_समुच्चयः_Strength_Specified) nodep->varp()->hasStrengthAssignment(सत्य);
            }
            यदि (ध्रुव AstClockingItem* ध्रुव itemp
                = VN_CAST(nodep->varp()->backp(), ClockingItem)) {
                UINFO(5, "ClkOut " << nodep);
                यदि (itemp->outputp()) nodep->varp(itemp->outputp()->varp());
            }
            यदि (m_समुच्चयः_Forced_By_संहिता) {
                nodep->varp()->समुच्चयः_Forced_By_संहिता();
                // If a public signal is being forced in SystemVerilog and VPI
                // is enabled, mark it as forceable to ensure that the VPI
                // functions read the forced value correctly
                यदि (v3Global.opt.vpi()
                    && (nodep->varp()->is_Sig_लौकिकम्() || nodep->varp()->is_Sig_Mod_लौकिकम्())) {
                    nodep->varp()->समुच्चयः_Forceable();
                }
            } अन्यथा यदि (!nodep->varp()->isFuncLocal() && nodep->varp()->isReadOnly()) {
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
    शून्य दर्शय(AstPin* nodep) override {
        VL_RESTORER(m_समुच्चयः_Ref_Lvalue);
        यदि (nodep->modVarp() && nodep->modVarp()->isWritable()) {
            // When the varref's were created, we didn't know the I/O state
            // Now that we do, and it's from a output, we know it's a lvalue
            m_समुच्चयः_Ref_Lvalue = VAccess::WRITE;
            iterateChildren(nodep);
            m_समुच्चयः_Ref_Lvalue = VAccess::NOCHANGE;
        } अन्यथा {
            iterateChildren(nodep);
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_नियोजय* nodep) override {
        VL_RESTORER(m_समुच्चयः_Ref_Lvalue);
        VL_RESTORER(m_समुच्चयः_Continuously);
        VL_RESTORER(m_समुच्चयः_Strength_Specified);
        {
            m_समुच्चयः_Ref_Lvalue = VAccess::WRITE;
            m_समुच्चयः_Continuously = VN_IS(nodep, नियोजय_W);
            यदि (ध्रुव Ast_नियोजय_W* ध्रुव assignwp = VN_CAST(nodep, नियोजय_W)) {
                यदि (assignwp->strengthSpecp()) m_समुच्चयः_Strength_Specified = सत्य;
            }
            {
                VL_RESTORER(m_समुच्चयः_Forced_By_संहिता);
                m_समुच्चयः_Forced_By_संहिता = VN_IS(nodep, नियोजय_Force);
                iterate_And_अग्र_Null(nodep->lhsp());
            }
            m_समुच्चयः_Ref_Lvalue = VAccess::NOCHANGE;
            m_समुच्चयः_Continuously = असत्य;
            m_समुच्चयः_Strength_Specified = असत्य;
            iterate_And_अग्र_Null(nodep->rhsp());
        }

        यदि ((m_अन्तर्_Initial_Static || m_अन्तर्_Initial_Static_Stmt) && m_अन्तर्_Func) {
            ध्रुव तार्किक rhsHasIO = nodep->rhsp()->exists([](ध्रुव Ast_ग्रन्थिः_Var_Ref* ध्रुव refp) {
                // Exclude module I/O referenced from a function/task.
                प्रत्यागम refp->varp() && refp->varp()->isIO()
                       && refp->varp()->lifetime() != VLifetime::NONE;
            });
            यदि (rhsHasIO) {
                nodep->rhsp()->v3warn(E_UNSUPPORTED,
                                      "Static variable initializer\n"
                                          << nodep->rhsp()->warn_अधिकम्()
                                          << "is dependent on function/task I/O variable");
            } अन्यथा {
                ध्रुव तार्किक rhsHasAutomatic
                    = nodep->rhsp()->exists([](ध्रुव Ast_ग्रन्थिः_Var_Ref* ध्रुव refp) {
                          प्रत्यागम refp->varp() && refp->varp()->lifetime().isAutomatic();
                      });
                यदि (rhsHasAutomatic) {
                    nodep->rhsp()->v3error("Static variable initializer\n"
                                           << nodep->rhsp()->warn_अधिकम्()
                                           << "is dependent on automatic variable");
                }
            }
        }
    }
    शून्य दर्शय(AstAlias* nodep) override {
        VL_RESTORER(m_समुच्चयः_Ref_Lvalue);
        m_समुच्चयः_Ref_Lvalue = VAccess::READWRITE;
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstInitialStatic* nodep) override {
        VL_RESTORER(m_अन्तर्_Initial_Static);
        m_अन्तर्_Initial_Static = सत्य;
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_अन्तर्_Initial_Static_Stmt);
        m_अन्तर्_Initial_Static_Stmt = सत्य;
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstRelease* nodep) override {
        VL_RESTORER(m_समुच्चयः_Ref_Lvalue);
        VL_RESTORER(m_समुच्चयः_Continuously);
        VL_RESTORER(m_समुच्चयः_Forced_By_संहिता);
        m_समुच्चयः_Ref_Lvalue = VAccess::WRITE;
        m_समुच्चयः_Continuously = असत्य;
        m_समुच्चयः_Forced_By_संहिता = सत्य;
        iterate_And_अग्र_Null(nodep->lhsp());
    }
    शून्य दर्शय(AstDeassign* nodep) override {
        VL_RESTORER(m_समुच्चयः_Ref_Lvalue);
        VL_RESTORER(m_समुच्चयः_Continuously);
        m_समुच्चयः_Ref_Lvalue = VAccess::WRITE;
        m_समुच्चयः_Continuously = असत्य;
        iterate_And_अग्र_Null(nodep->lhsp());
    }
    शून्य दर्शय(AstFireEvent* nodep) override {
        VL_RESTORER(m_समुच्चयः_Ref_Lvalue);
        m_समुच्चयः_Ref_Lvalue = VAccess::WRITE;
        iterate_And_अग्र_Null(nodep->operandp());
    }
    शून्य दर्शय(AstCastDynamic* nodep) override {
        VL_RESTORER(m_समुच्चयः_Ref_Lvalue);
        m_समुच्चयः_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_अग्र_Null(nodep->fromp());
        m_समुच्चयः_Ref_Lvalue = VAccess::WRITE;
        iterate_And_अग्र_Null(nodep->शिखरम्());
    }
    शून्य दर्शय(AstFError* nodep) override {
        VL_RESTORER(m_समुच्चयः_Ref_Lvalue);
        iterate_And_अग्र_Null(nodep->filep());
        m_समुच्चयः_Ref_Lvalue = VAccess::WRITE;
        iterate_And_अग्र_Null(nodep->strp());
    }
    शून्य दर्शय(AstFGetS* nodep) override {
        VL_RESTORER(m_समुच्चयः_Ref_Lvalue);
        iterate_And_अग्र_Null(nodep->filep());
        m_समुच्चयः_Ref_Lvalue = VAccess::WRITE;
        iterate_And_अग्र_Null(nodep->strgp());
    }
    शून्य दर्शय(AstFRead* nodep) override {
        VL_RESTORER(m_समुच्चयः_Ref_Lvalue);
        iterate_And_अग्र_Null(nodep->filep());
        m_समुच्चयः_Ref_Lvalue = VAccess::WRITE;
        iterate_And_अग्र_Null(nodep->memp());
    }
    शून्य दर्शय(AstFScanF* nodep) override {
        VL_RESTORER(m_समुच्चयः_Ref_Lvalue);
        iterate_And_अग्र_Null(nodep->filep());
        m_समुच्चयः_Ref_Lvalue = VAccess::WRITE;
        iterate_And_अग्र_Null(nodep->exprsp());
    }
    शून्य दर्शय(AstFUngetC* nodep) override {
        VL_RESTORER(m_समुच्चयः_Ref_Lvalue);
        iterate_And_अग्र_Null(nodep->filep());
        m_समुच्चयः_Ref_Lvalue = VAccess::WRITE;
        iterate_And_अग्र_Null(nodep->rhsp());
    }
    शून्य दर्शय(AstSScanF* nodep) override {
        VL_RESTORER(m_समुच्चयः_Ref_Lvalue);
        m_समुच्चयः_Ref_Lvalue = VAccess::WRITE;
        iterate_And_अग्र_Null(nodep->exprsp());
    }
    शून्य दर्शय(AstSysIgnore* nodep) override {
        // Can't know if lvalue or not; presume not
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstRand* nodep) override {
        VL_RESTORER(m_समुच्चयः_Ref_Lvalue);
        यदि (!nodep->urandom()) m_समुच्चयः_Ref_Lvalue = VAccess::READWRITE;
        iterate_And_अग्र_Null(nodep->seedp());
    }
    शून्य दर्शय(AstReadMem* nodep) override {
        VL_RESTORER(m_समुच्चयः_Ref_Lvalue);
        m_समुच्चयः_Ref_Lvalue = VAccess::WRITE;
        iterate_And_अग्र_Null(nodep->memp());
        m_समुच्चयः_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_अग्र_Null(nodep->filenamep());
        iterate_And_अग्र_Null(nodep->lsbp());
        iterate_And_अग्र_Null(nodep->msbp());
    }
    शून्य दर्शय(Ast_Test_Plus_तर्काः* nodep) override {
        VL_RESTORER(m_समुच्चयः_Ref_Lvalue);
        m_समुच्चयः_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_अग्र_Null(nodep->searchp());
    }
    शून्य दर्शय(Ast_मानम्_Plus_तर्काः* nodep) override {
        VL_RESTORER(m_समुच्चयः_Ref_Lvalue);
        m_समुच्चयः_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_अग्र_Null(nodep->searchp());
        m_समुच्चयः_Ref_Lvalue = VAccess::WRITE;
        iterate_And_अग्र_Null(nodep->outp());
    }
    शून्य दर्शय(AstSFormat* nodep) override {
        VL_RESTORER(m_समुच्चयः_Ref_Lvalue);
        m_समुच्चयः_Ref_Lvalue = VAccess::WRITE;
        iterate_And_अग्र_Null(nodep->lhsp());
        m_समुच्चयः_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_अग्र_Null(nodep->fmtp());
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Dist_Biop* nodep) override {
        VL_RESTORER(m_समुच्चयः_Ref_Lvalue);
        m_समुच्चयः_Ref_Lvalue = VAccess::READWRITE;
        iterate_And_अग्र_Null(nodep->lhsp());
        m_समुच्चयः_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_अग्र_Null(nodep->rhsp());
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Dist_Triop* nodep) override {
        VL_RESTORER(m_समुच्चयः_Ref_Lvalue);
        m_समुच्चयः_Ref_Lvalue = VAccess::READWRITE;
        iterate_And_अग्र_Null(nodep->lhsp());
        m_समुच्चयः_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_अग्र_Null(nodep->rhsp());
        iterate_And_अग्र_Null(nodep->thsp());
    }
    // cppcheck-suppress constParameterPointer
    शून्य prepost_दर्शय(Ast_ग्रन्थिः_Uniop* nodep) {
        VL_RESTORER(m_समुच्चयः_Ref_Lvalue);
        m_समुच्चयः_Ref_Lvalue = VAccess::WRITE;
        iterate_And_अग्र_Null(nodep->lhsp());
    }
    शून्य दर्शय(AstPreInc* nodep) override { prepost_दर्शय(nodep); }
    शून्य दर्शय(AstPostInc* nodep) override { prepost_दर्शय(nodep); }
    शून्य दर्शय(AstPreDec* nodep) override { prepost_दर्शय(nodep); }
    शून्य दर्शय(AstPostDec* nodep) override { prepost_दर्शय(nodep); }

    // Nodes that change LValue state
    शून्य दर्शय(AstSel* nodep) override {
        VL_RESTORER(m_समुच्चयः_Ref_Lvalue);
        iterate_And_अग्र_Null(nodep->fromp());
        // Only set lvalues on the from
        m_समुच्चयः_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_अग्र_Null(nodep->lsbp());
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Sel* nodep) override {
        VL_RESTORER(m_समुच्चयः_Ref_Lvalue);
        // Only set lvalues on the from
        iterate_And_अग्र_Null(nodep->fromp());
        m_समुच्चयः_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_अग्र_Null(nodep->bitp());
    }
    शून्य दर्शय(AstCellArrayRef* nodep) override {
        VL_RESTORER(m_समुच्चयः_Ref_Lvalue);
        // selp is not an lvalue
        m_समुच्चयः_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_अग्र_Null(nodep->selp());
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Pre_Sel* nodep) override {
        यदि (AstSelBit* ध्रुव selbitp = VN_CAST(nodep, SelBit)) selbitp->access(m_समुच्चयः_Ref_Lvalue);
        VL_RESTORER(m_समुच्चयः_Ref_Lvalue);
        // Only set lvalues on the from
        iterate_And_अग्र_Null(nodep->fromp());
        m_समुच्चयः_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_अग्र_Null(nodep->rhsp());
        iterate_And_अग्र_Null(nodep->thsp());
    }
    शून्य दर्शय(AstMemberSel* nodep) override {
        यदि (m_समुच्चयः_Ref_Lvalue != VAccess::NOCHANGE) {
            nodep->access(m_समुच्चयः_Ref_Lvalue);
            यदि (nodep->varp() && nodep->access().isWriteOrRW()) {
                यदि (ध्रुव AstClockingItem* ध्रुव itemp
                    = VN_CAST(nodep->varp()->backp(), ClockingItem)) {
                    UINFO(5, "ClkOut " << nodep);
                    यदि (itemp->outputp()) nodep->varp(itemp->outputp()->varp());
                }
            }
        } अन्यथा {
            // It is the only place where the access is set to member select nodes.
            // If it doesn't have to be set to WRITE, it means that it is READ.
            nodep->access(VAccess::READ);
        }
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        ध्रुव Ast_ग्रन्थिः_FTask* ध्रुव taskp = nodep->taskp();
        // We'll deal with mismatching pins later
        यदि (!taskp) प्रत्यागम;
        ध्रुव V3TaskConnects tconnects
            = V3Task::taskConnects(nodep, taskp->stmtsp(), शून्यसूचकः, असत्य);
        प्रति (ध्रुव स्वचालित& tconnect : tconnects) {
            ध्रुव AstVar* ध्रुव portp = tconnect.first;
            ध्रुव AstArg* ध्रुव argp = tconnect.second;
            यदि (!argp) अनुवर्त;
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव pinp = argp->exprp();
            यदि (!pinp) अनुवर्त;
            VL_RESTORER(m_समुच्चयः_Ref_Lvalue);
            m_समुच्चयः_Ref_Lvalue = portp->direction().pinAccess();
            iterate(pinp);
        }
        यदि (nodep->withp()) iterate(nodep->withp());
    }
    शून्य दर्शय(AstConstraint* nodep) override {
        VL_RESTORER(m_समुच्चयः_If_Rand);
        m_समुच्चयः_If_Rand = सत्य;
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask* nodep) override {
        VL_RESTORER(m_अन्तर्_Func);
        m_अन्तर्_Func = सत्य;
        iterateChildren(nodep);
    }

    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    LinkLValueVisitor(Ast_ग्रन्थिः* nodep, VAccess start)
        : m_समुच्चयः_Ref_Lvalue{start} {
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
शून्य V3LinkLValue::link_LValue_समुच्चयः(Ast_ग्रन्थिः* ध्रुव nodep, ध्रुव VAccess access) {
    // Called by later link functions when it is known a node needs
    // to be converted to a lvalue.
    UINFO(9, __FUNCTION__ << ": ");
    { LinkLValueVisitor{nodep, access}; }
}
