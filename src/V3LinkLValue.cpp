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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3LinkLValue.h"

#ضمن "V3Task.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

صنف LinkLValueVisitor final : عام VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    منطقي m_setContinuously = خطأ;  // Set that var has some continuous assignment
    منطقي m_setForcedByCode = خطأ;  // Set that var is the target of an AstAssignForce/AstRelease
    منطقي m_setIfRand = خطأ;  // Update VarRefs if var declared as rand
    منطقي m_setStrengthSpecified = خطأ;  // Set that var has assignment with strength specified.
    منطقي m_inFunc = خطأ;  // Set if inside AstNodeFTask
    منطقي m_inInitialStatic = خطأ;  // Set if inside AstInitialStatic
    منطقي m_inInitialStaticStmt = خطأ;  // Set if inside AstInitialStaticStmt
    VAccess m_setRefLvalue;  // Set VarRefs to lvalues for pin assignments

    // VISITORS
    // Result handing
    فراغ visit(Ast_گرہ_Var_Ref* nodep) override {
        // VarRef: LValue its reference
        إذا (m_setIfRand && !(nodep->varp() && nodep->varp()->isRand())) أرجع;
        إذا (m_setRefLvalue != VAccess::NOCHANGE) nodep->access(m_setRefLvalue);
        // Linked via an input-only modport by V3LinkDot, before it was known if written
        إذا (AstVarXRef* ثابت xrefp = VN_CAST(nodep, VarXRef)) {
            إذا (xrefp->readOnlyModport()) {
                إذا (nodep->access().isWriteOrRW()) {
                    nodep->v3error(
                        "Attempt to drive input-only modport: " << nodep->prettyNameQ());
                }
                xrefp->readOnlyModport(خطأ);
            }
        }
        إذا (nodep->varp() && nodep->access().isWriteOrRW()) {
            إذا (nodep->varp()->isParam()) {
                // All parameters that did get constified happened before now
                // as V3LinkLValue runs after V3Param
                nodep->v3error("Storing to parameter variable "
                               << nodep->prettyNameQ()
                               << " in a context that is determined only at runtime");
            }
            إذا (m_setContinuously) {
                nodep->varp()->isContinuously(true);
                // Strength may only be specified in continuous assignment,
                // so it is needed to check only if m_setContinuously is true
                إذا (m_setStrengthSpecified) nodep->varp()->hasStrengthAssignment(true);
            }
            إذا (ثابت AstClockingItem* ثابت itemp
                = VN_CAST(nodep->varp()->backp(), ClockingItem)) {
                UINFO(5, "ClkOut " << nodep);
                إذا (itemp->outputp()) nodep->varp(itemp->outputp()->varp());
            }
            إذا (m_setForcedByCode) {
                nodep->varp()->setForcedByCode();
                // If a public signal is being forced in SystemVerilog and VPI
                // is enabled, mark it as forceable to ensure that the VPI
                // functions read the forced value correctly
                إذا (v3Global.opt.vpi()
                    && (nodep->varp()->isSigPublic() || nodep->varp()->isSigModPublic())) {
                    nodep->varp()->setForceable();
                }
            } وإلا إذا (!nodep->varp()->isFuncLocal() && nodep->varp()->isReadOnly()) {
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
    فراغ visit(AstPin* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        إذا (nodep->modVarp() && nodep->modVarp()->isWritable()) {
            // When the varref's were created, we didn't know the I/O state
            // Now that we do, and it's from a output, we know it's a lvalue
            m_setRefLvalue = VAccess::WRITE;
            iterateChildren(nodep);
            m_setRefLvalue = VAccess::NOCHANGE;
        } وإلا {
            iterateChildren(nodep);
        }
    }
    فراغ visit(Ast_گرہ_Assign* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        VL_RESTORER(m_setContinuously);
        VL_RESTORER(m_setStrengthSpecified);
        {
            m_setRefLvalue = VAccess::WRITE;
            m_setContinuously = VN_IS(nodep, AssignW);
            إذا (ثابت AstAssignW* ثابت assignwp = VN_CAST(nodep, AssignW)) {
                إذا (assignwp->strengthSpecp()) m_setStrengthSpecified = true;
            }
            {
                VL_RESTORER(m_setForcedByCode);
                m_setForcedByCode = VN_IS(nodep, AssignForce);
                iterateAndNextNull(nodep->lhsp());
            }
            m_setRefLvalue = VAccess::NOCHANGE;
            m_setContinuously = خطأ;
            m_setStrengthSpecified = خطأ;
            iterateAndNextNull(nodep->rhsp());
        }

        إذا ((m_inInitialStatic || m_inInitialStaticStmt) && m_inFunc) {
            ثابت منطقي rhsHasIO = nodep->rhsp()->exists([](ثابت Ast_گرہ_Var_Ref* ثابت refp) {
                // Exclude module I/O referenced from a function/task.
                أرجع refp->varp() && refp->varp()->isIO()
                       && refp->varp()->lifetime() != VLifetime::NONE;
            });
            إذا (rhsHasIO) {
                nodep->rhsp()->v3warn(E_UNSUPPORTED,
                                      "Static variable initializer\n"
                                          << nodep->rhsp()->warnMore()
                                          << "is dependent on function/task I/O variable");
            } وإلا {
                ثابت منطقي rhsHasAutomatic
                    = nodep->rhsp()->exists([](ثابت Ast_گرہ_Var_Ref* ثابت refp) {
                          أرجع refp->varp() && refp->varp()->lifetime().isAutomatic();
                      });
                إذا (rhsHasAutomatic) {
                    nodep->rhsp()->v3error("Static variable initializer\n"
                                           << nodep->rhsp()->warnMore()
                                           << "is dependent on automatic variable");
                }
            }
        }
    }
    فراغ visit(AstAlias* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::READWRITE;
        iterateChildren(nodep);
    }
    فراغ visit(AstInitialStatic* nodep) override {
        VL_RESTORER(m_inInitialStatic);
        m_inInitialStatic = true;
        iterateChildren(nodep);
    }
    فراغ visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitialStaticStmt);
        m_inInitialStaticStmt = true;
        iterateChildren(nodep);
    }
    فراغ visit(AstRelease* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        VL_RESTORER(m_setContinuously);
        VL_RESTORER(m_setForcedByCode);
        m_setRefLvalue = VAccess::WRITE;
        m_setContinuously = خطأ;
        m_setForcedByCode = true;
        iterateAndNextNull(nodep->lhsp());
    }
    فراغ visit(AstDeassign* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        VL_RESTORER(m_setContinuously);
        m_setRefLvalue = VAccess::WRITE;
        m_setContinuously = خطأ;
        iterateAndNextNull(nodep->lhsp());
    }
    فراغ visit(AstFireEvent* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->operandp());
    }
    فراغ visit(AstCastDynamic* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->fromp());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->چوٹی());
    }
    فراغ visit(AstFError* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->strp());
    }
    فراغ visit(AstFGetS* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->strgp());
    }
    فراغ visit(AstFRead* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->memp());
    }
    فراغ visit(AstFScanF* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->exprsp());
    }
    فراغ visit(AstFUngetC* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->rhsp());
    }
    فراغ visit(AstSScanF* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->exprsp());
    }
    فراغ visit(AstSysIgnore* nodep) override {
        // Can't know if lvalue or not; presume not
        iterateChildren(nodep);
    }
    فراغ visit(AstRand* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        إذا (!nodep->urandom()) m_setRefLvalue = VAccess::READWRITE;
        iterateAndNextNull(nodep->seedp());
    }
    فراغ visit(AstReadMem* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->memp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->filenamep());
        iterateAndNextNull(nodep->lsbp());
        iterateAndNextNull(nodep->msbp());
    }
    فراغ visit(AstTestPlusArgs* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->searchp());
    }
    فراغ visit(Ast_قدر_Plus_Args* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->searchp());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->outp());
    }
    فراغ visit(AstSFormat* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->lhsp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->fmtp());
    }
    فراغ visit(Ast_گرہ_Dist_Biop* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::READWRITE;
        iterateAndNextNull(nodep->lhsp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->rhsp());
    }
    فراغ visit(Ast_گرہ_Dist_Triop* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::READWRITE;
        iterateAndNextNull(nodep->lhsp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->rhsp());
        iterateAndNextNull(nodep->thsp());
    }
    // cppcheck-suppress constParameterPointer
    فراغ prepost_visit(Ast_گرہ_Uniop* nodep) {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->lhsp());
    }
    فراغ visit(AstPreInc* nodep) override { prepost_visit(nodep); }
    فراغ visit(AstPostInc* nodep) override { prepost_visit(nodep); }
    فراغ visit(AstPreDec* nodep) override { prepost_visit(nodep); }
    فراغ visit(AstPostDec* nodep) override { prepost_visit(nodep); }

    // Nodes that change LValue state
    فراغ visit(AstSel* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->fromp());
        // Only set lvalues on the from
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->lsbp());
    }
    فراغ visit(Ast_گرہ_Sel* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        // Only set lvalues on the from
        iterateAndNextNull(nodep->fromp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->bitp());
    }
    فراغ visit(AstCellArrayRef* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        // selp is not an lvalue
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->selp());
    }
    فراغ visit(Ast_گرہ_Pre_Sel* nodep) override {
        إذا (AstSelBit* ثابت selbitp = VN_CAST(nodep, SelBit)) selbitp->access(m_setRefLvalue);
        VL_RESTORER(m_setRefLvalue);
        // Only set lvalues on the from
        iterateAndNextNull(nodep->fromp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->rhsp());
        iterateAndNextNull(nodep->thsp());
    }
    فراغ visit(AstMemberSel* nodep) override {
        إذا (m_setRefLvalue != VAccess::NOCHANGE) {
            nodep->access(m_setRefLvalue);
            إذا (nodep->varp() && nodep->access().isWriteOrRW()) {
                إذا (ثابت AstClockingItem* ثابت itemp
                    = VN_CAST(nodep->varp()->backp(), ClockingItem)) {
                    UINFO(5, "ClkOut " << nodep);
                    إذا (itemp->outputp()) nodep->varp(itemp->outputp()->varp());
                }
            }
        } وإلا {
            // It is the only place where the access is set to member select nodes.
            // If it doesn't have to be set to WRITE, it means that it is READ.
            nodep->access(VAccess::READ);
        }
        iterateChildren(nodep);
    }
    فراغ visit(Ast_گرہ_FTask_Ref* nodep) override {
        ثابت Ast_گرہ_FTask* ثابت taskp = nodep->taskp();
        // We'll deal with mismatching pins later
        إذا (!taskp) أرجع;
        ثابت V3TaskConnects tconnects
            = V3Task::taskConnects(nodep, taskp->stmtsp(), nullptr, خطأ);
        لكل (ثابت auto& tconnect : tconnects) {
            ثابت AstVar* ثابت portp = tconnect.first;
            ثابت AstArg* ثابت argp = tconnect.second;
            إذا (!argp) استمر;
            Ast_گرہ_اظہاریہ* ثابت pinp = argp->exprp();
            إذا (!pinp) استمر;
            VL_RESTORER(m_setRefLvalue);
            m_setRefLvalue = portp->direction().pinAccess();
            iterate(pinp);
        }
        إذا (nodep->withp()) iterate(nodep->withp());
    }
    فراغ visit(AstConstraint* nodep) override {
        VL_RESTORER(m_setIfRand);
        m_setIfRand = true;
        iterateChildren(nodep);
    }
    فراغ visit(Ast_گرہ_FTask* nodep) override {
        VL_RESTORER(m_inFunc);
        m_inFunc = true;
        iterateChildren(nodep);
    }

    فراغ visit(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    LinkLValueVisitor(Ast_گرہ* nodep, VAccess start)
        : m_setRefLvalue{start} {
        iterate(nodep);
    }
    ~LinkLValueVisitor() override = افتراضي;
};

//######################################################################
// Link class functions

فراغ V3LinkLValue::linkLValue(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    { LinkLValueVisitor{nodep, VAccess::NOCHANGE}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("linklvalue", 0, dump_درخت_Either_Level() >= 6);
}
فراغ V3LinkLValue::linkLValueSet(Ast_گرہ* ثابت nodep, ثابت VAccess access) {
    // Called by later link functions when it is known a node needs
    // to be converted to a lvalue.
    UINFO(9, __FUNCTION__ << ": ");
    { LinkLValueVisitor{nodep, access}; }
}
