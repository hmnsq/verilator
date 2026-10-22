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
    तार्किक m_setContinuously = असत्य;  // Set that var has some continuous assignment
    तार्किक m_setForcedByCode = असत्य;  // Set that var is the target of an AstAssignForce/AstRelease
    तार्किक m_setIfRand = असत्य;  // Update VarRefs if var declared as rand
    तार्किक m_setStrengthSpecified = असत्य;  // Set that var has assignment with strength specified.
    तार्किक m_inFunc = असत्य;  // Set if inside AstNodeFTask
    तार्किक m_inInitialStatic = असत्य;  // Set if inside AstInitialStatic
    तार्किक m_inInitialStaticStmt = असत्य;  // Set if inside AstInitialStaticStmt
    VAccess m_setRefLvalue;  // Set VarRefs to lvalues for pin assignments

    // VISITORS
    // Result handing
    शून्य visit(Ast_गाठ_Var_Ref* nodep) override {
        // VarRef: LValue its reference
        जर (m_setIfRand && !(nodep->varp() && nodep->varp()->isRand())) परत;
        जर (m_setRefLvalue != VAccess::NOCHANGE) nodep->access(m_setRefLvalue);
        // Linked via an input-only modport by V3LinkDot, before it was known if written
        जर (AstVarXRef* स्थिरांक xrefp = VN_CAST(nodep, VarXRef)) {
            जर (xrefp->readOnlyModport()) {
                जर (nodep->access().isWriteOrRW()) {
                    nodep->v3error(
                        "Attempt to drive input-only modport: " << nodep->prettyNameQ());
                }
                xrefp->readOnlyModport(असत्य);
            }
        }
        जर (nodep->varp() && nodep->access().isWriteOrRW()) {
            जर (nodep->varp()->isParam()) {
                // All parameters that did get constified happened before now
                // as V3LinkLValue runs after V3Param
                nodep->v3error("Storing to parameter variable "
                               << nodep->prettyNameQ()
                               << " in a context that is determined only at runtime");
            }
            जर (m_setContinuously) {
                nodep->varp()->isContinuously(सत्य);
                // Strength may only be specified in continuous assignment,
                // so it is needed to check only if m_setContinuously is true
                जर (m_setStrengthSpecified) nodep->varp()->hasStrengthAssignment(सत्य);
            }
            जर (स्थिरांक AstClockingItem* स्थिरांक itemp
                = VN_CAST(nodep->varp()->backp(), ClockingItem)) {
                UINFO(5, "ClkOut " << nodep);
                जर (itemp->outputp()) nodep->varp(itemp->outputp()->varp());
            }
            जर (m_setForcedByCode) {
                nodep->varp()->setForcedByCode();
                // If a public signal is being forced in SystemVerilog and VPI
                // is enabled, mark it as forceable to ensure that the VPI
                // functions read the forced value correctly
                जर (v3Global.opt.vpi()
                    && (nodep->varp()->isSigPublic() || nodep->varp()->isSigModPublic())) {
                    nodep->varp()->setForceable();
                }
            } अन्यथा जर (!nodep->varp()->isFuncLocal() && nodep->varp()->isReadOnly()) {
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
        जर (nodep->modVarp() && nodep->modVarp()->isWritable()) {
            // When the varref's were created, we didn't know the I/O state
            // Now that we do, and it's from a output, we know it's a lvalue
            m_setRefLvalue = VAccess::WRITE;
            iterateChildren(nodep);
            m_setRefLvalue = VAccess::NOCHANGE;
        } अन्यथा {
            iterateChildren(nodep);
        }
    }
    शून्य visit(Ast_गाठ_Assign* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        VL_RESTORER(m_setContinuously);
        VL_RESTORER(m_setStrengthSpecified);
        {
            m_setRefLvalue = VAccess::WRITE;
            m_setContinuously = VN_IS(nodep, AssignW);
            जर (स्थिरांक AstAssignW* स्थिरांक assignwp = VN_CAST(nodep, AssignW)) {
                जर (assignwp->strengthSpecp()) m_setStrengthSpecified = सत्य;
            }
            {
                VL_RESTORER(m_setForcedByCode);
                m_setForcedByCode = VN_IS(nodep, AssignForce);
                iterateAndNextNull(nodep->lhsp());
            }
            m_setRefLvalue = VAccess::NOCHANGE;
            m_setContinuously = असत्य;
            m_setStrengthSpecified = असत्य;
            iterateAndNextNull(nodep->rhsp());
        }

        जर ((m_inInitialStatic || m_inInitialStaticStmt) && m_inFunc) {
            स्थिरांक तार्किक rhsHasIO = nodep->rhsp()->exists([](स्थिरांक Ast_गाठ_Var_Ref* स्थिरांक refp) {
                // Exclude module I/O referenced from a function/task.
                परत refp->varp() && refp->varp()->isIO()
                       && refp->varp()->lifetime() != VLifetime::NONE;
            });
            जर (rhsHasIO) {
                nodep->rhsp()->v3warn(E_UNSUPPORTED,
                                      "Static variable initializer\n"
                                          << nodep->rhsp()->warnMore()
                                          << "is dependent on function/task I/O variable");
            } अन्यथा {
                स्थिरांक तार्किक rhsHasAutomatic
                    = nodep->rhsp()->exists([](स्थिरांक Ast_गाठ_Var_Ref* स्थिरांक refp) {
                          परत refp->varp() && refp->varp()->lifetime().isAutomatic();
                      });
                जर (rhsHasAutomatic) {
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
        m_inInitialStatic = सत्य;
        iterateChildren(nodep);
    }
    शून्य visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitialStaticStmt);
        m_inInitialStaticStmt = सत्य;
        iterateChildren(nodep);
    }
    शून्य visit(AstRelease* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        VL_RESTORER(m_setContinuously);
        VL_RESTORER(m_setForcedByCode);
        m_setRefLvalue = VAccess::WRITE;
        m_setContinuously = असत्य;
        m_setForcedByCode = सत्य;
        iterateAndNextNull(nodep->lhsp());
    }
    शून्य visit(AstDeassign* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        VL_RESTORER(m_setContinuously);
        m_setRefLvalue = VAccess::WRITE;
        m_setContinuously = असत्य;
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
        iterateAndNextNull(nodep->शीर्ष());
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
        जर (!nodep->urandom()) m_setRefLvalue = VAccess::READWRITE;
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
    शून्य visit(Ast_मूल्य_Plus_Args* nodep) override {
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
    शून्य visit(Ast_गाठ_Dist_Biop* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::READWRITE;
        iterateAndNextNull(nodep->lhsp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->rhsp());
    }
    शून्य visit(Ast_गाठ_Dist_Triop* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::READWRITE;
        iterateAndNextNull(nodep->lhsp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->rhsp());
        iterateAndNextNull(nodep->thsp());
    }
    // cppcheck-suppress constParameterPointer
    शून्य prepost_visit(Ast_गाठ_Uniop* nodep) {
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
    शून्य visit(Ast_गाठ_Sel* nodep) override {
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
    शून्य visit(Ast_गाठ_Pre_Sel* nodep) override {
        जर (AstSelBit* स्थिरांक selbitp = VN_CAST(nodep, SelBit)) selbitp->access(m_setRefLvalue);
        VL_RESTORER(m_setRefLvalue);
        // Only set lvalues on the from
        iterateAndNextNull(nodep->fromp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->rhsp());
        iterateAndNextNull(nodep->thsp());
    }
    शून्य visit(AstMemberSel* nodep) override {
        जर (m_setRefLvalue != VAccess::NOCHANGE) {
            nodep->access(m_setRefLvalue);
            जर (nodep->varp() && nodep->access().isWriteOrRW()) {
                जर (स्थिरांक AstClockingItem* स्थिरांक itemp
                    = VN_CAST(nodep->varp()->backp(), ClockingItem)) {
                    UINFO(5, "ClkOut " << nodep);
                    जर (itemp->outputp()) nodep->varp(itemp->outputp()->varp());
                }
            }
        } अन्यथा {
            // It is the only place where the access is set to member select nodes.
            // If it doesn't have to be set to WRITE, it means that it is READ.
            nodep->access(VAccess::READ);
        }
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाठ_FTask_Ref* nodep) override {
        स्थिरांक Ast_गाठ_FTask* स्थिरांक taskp = nodep->taskp();
        // We'll deal with mismatching pins later
        जर (!taskp) परत;
        स्थिरांक V3TaskConnects tconnects
            = V3Task::taskConnects(nodep, taskp->stmtsp(), nullptr, असत्य);
        साठी (स्थिरांक स्वतः& tconnect : tconnects) {
            स्थिरांक AstVar* स्थिरांक portp = tconnect.first;
            स्थिरांक AstArg* स्थिरांक argp = tconnect.second;
            जर (!argp) सुरूठेवा;
            Ast_गाठ_अभिव्यक्ती* स्थिरांक pinp = argp->exprp();
            जर (!pinp) सुरूठेवा;
            VL_RESTORER(m_setRefLvalue);
            m_setRefLvalue = portp->direction().pinAccess();
            iterate(pinp);
        }
        जर (nodep->withp()) iterate(nodep->withp());
    }
    शून्य visit(AstConstraint* nodep) override {
        VL_RESTORER(m_setIfRand);
        m_setIfRand = सत्य;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाठ_FTask* nodep) override {
        VL_RESTORER(m_inFunc);
        m_inFunc = सत्य;
        iterateChildren(nodep);
    }

    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    LinkLValueVisitor(Ast_गाठ* nodep, VAccess start)
        : m_setRefLvalue{start} {
        iterate(nodep);
    }
    ~LinkLValueVisitor() override = पूर्वनिर्धारित;
};

//######################################################################
// Link class functions

शून्य V3LinkLValue::linkLValue(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    { LinkLValueVisitor{nodep, VAccess::NOCHANGE}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("linklvalue", 0, dump_वृक्ष_Either_Level() >= 6);
}
शून्य V3LinkLValue::linkLValueSet(Ast_गाठ* स्थिरांक nodep, स्थिरांक VAccess access) {
    // Called by later link functions when it is known a node needs
    // to be converted to a lvalue.
    UINFO(9, __FUNCTION__ << ": ");
    { LinkLValueVisitor{nodep, access}; }
}
