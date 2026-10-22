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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3LinkLValue.h"

#অন্তর্ভাব "V3Task.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

শ্রেণী LinkLValueVisitor final : সর্বজনীন VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    যৌক্তিক m_setContinuously = মিথ্যা;  // Set that var has some continuous assignment
    যৌক্তিক m_setForcedByCode = মিথ্যা;  // Set that var is the target of an AstAssignForce/AstRelease
    যৌক্তিক m_setIfRand = মিথ্যা;  // Update VarRefs if var declared as rand
    যৌক্তিক m_setStrengthSpecified = মিথ্যা;  // Set that var has assignment with strength specified.
    যৌক্তিক m_inFunc = মিথ্যা;  // Set if inside AstNodeFTask
    যৌক্তিক m_inInitialStatic = মিথ্যা;  // Set if inside AstInitialStatic
    যৌক্তিক m_inInitialStaticStmt = মিথ্যা;  // Set if inside AstInitialStaticStmt
    VAccess m_setRefLvalue;  // Set VarRefs to lvalues for pin assignments

    // VISITORS
    // Result handing
    শূন্য visit(Ast_গ্রন্থি_Var_Ref* nodep) override {
        // VarRef: LValue its reference
        যদি (m_setIfRand && !(nodep->varp() && nodep->varp()->isRand())) ফেরত;
        যদি (m_setRefLvalue != VAccess::NOCHANGE) nodep->access(m_setRefLvalue);
        // Linked via an input-only modport by V3LinkDot, before it was known if written
        যদি (AstVarXRef* ধ্রুবক xrefp = VN_CAST(nodep, VarXRef)) {
            যদি (xrefp->readOnlyModport()) {
                যদি (nodep->access().isWriteOrRW()) {
                    nodep->v3error(
                        "Attempt to drive input-only modport: " << nodep->prettyNameQ());
                }
                xrefp->readOnlyModport(মিথ্যা);
            }
        }
        যদি (nodep->varp() && nodep->access().isWriteOrRW()) {
            যদি (nodep->varp()->isParam()) {
                // All parameters that did get constified happened before now
                // as V3LinkLValue runs after V3Param
                nodep->v3error("Storing to parameter variable "
                               << nodep->prettyNameQ()
                               << " in a context that is determined only at runtime");
            }
            যদি (m_setContinuously) {
                nodep->varp()->isContinuously(সত্য);
                // Strength may only be specified in continuous assignment,
                // so it is needed to check only if m_setContinuously is true
                যদি (m_setStrengthSpecified) nodep->varp()->hasStrengthAssignment(সত্য);
            }
            যদি (ধ্রুবক AstClockingItem* ধ্রুবক itemp
                = VN_CAST(nodep->varp()->backp(), ClockingItem)) {
                UINFO(5, "ClkOut " << nodep);
                যদি (itemp->outputp()) nodep->varp(itemp->outputp()->varp());
            }
            যদি (m_setForcedByCode) {
                nodep->varp()->setForcedByCode();
                // If a public signal is being forced in SystemVerilog and VPI
                // is enabled, mark it as forceable to ensure that the VPI
                // functions read the forced value correctly
                যদি (v3Global.opt.vpi()
                    && (nodep->varp()->isSigPublic() || nodep->varp()->isSigModPublic())) {
                    nodep->varp()->setForceable();
                }
            } অন্যথা যদি (!nodep->varp()->isFuncLocal() && nodep->varp()->isReadOnly()) {
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
    শূন্য visit(AstPin* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        যদি (nodep->modVarp() && nodep->modVarp()->isWritable()) {
            // When the varref's were created, we didn't know the I/O state
            // Now that we do, and it's from a output, we know it's a lvalue
            m_setRefLvalue = VAccess::WRITE;
            iterateChildren(nodep);
            m_setRefLvalue = VAccess::NOCHANGE;
        } অন্যথা {
            iterateChildren(nodep);
        }
    }
    শূন্য visit(Ast_গ্রন্থি_Assign* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        VL_RESTORER(m_setContinuously);
        VL_RESTORER(m_setStrengthSpecified);
        {
            m_setRefLvalue = VAccess::WRITE;
            m_setContinuously = VN_IS(nodep, AssignW);
            যদি (ধ্রুবক AstAssignW* ধ্রুবক assignwp = VN_CAST(nodep, AssignW)) {
                যদি (assignwp->strengthSpecp()) m_setStrengthSpecified = সত্য;
            }
            {
                VL_RESTORER(m_setForcedByCode);
                m_setForcedByCode = VN_IS(nodep, AssignForce);
                iterate_And_পরবর্তী_Null(nodep->lhsp());
            }
            m_setRefLvalue = VAccess::NOCHANGE;
            m_setContinuously = মিথ্যা;
            m_setStrengthSpecified = মিথ্যা;
            iterate_And_পরবর্তী_Null(nodep->rhsp());
        }

        যদি ((m_inInitialStatic || m_inInitialStaticStmt) && m_inFunc) {
            ধ্রুবক যৌক্তিক rhsHasIO = nodep->rhsp()->exists([](ধ্রুবক Ast_গ্রন্থি_Var_Ref* ধ্রুবক refp) {
                // Exclude module I/O referenced from a function/task.
                ফেরত refp->varp() && refp->varp()->isIO()
                       && refp->varp()->lifetime() != VLifetime::NONE;
            });
            যদি (rhsHasIO) {
                nodep->rhsp()->v3warn(E_UNSUPPORTED,
                                      "Static variable initializer\n"
                                          << nodep->rhsp()->warnMore()
                                          << "is dependent on function/task I/O variable");
            } অন্যথা {
                ধ্রুবক যৌক্তিক rhsHasAutomatic
                    = nodep->rhsp()->exists([](ধ্রুবক Ast_গ্রন্থি_Var_Ref* ধ্রুবক refp) {
                          ফেরত refp->varp() && refp->varp()->lifetime().isAutomatic();
                      });
                যদি (rhsHasAutomatic) {
                    nodep->rhsp()->v3error("Static variable initializer\n"
                                           << nodep->rhsp()->warnMore()
                                           << "is dependent on automatic variable");
                }
            }
        }
    }
    শূন্য visit(AstAlias* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::READWRITE;
        iterateChildren(nodep);
    }
    শূন্য visit(AstInitialStatic* nodep) override {
        VL_RESTORER(m_inInitialStatic);
        m_inInitialStatic = সত্য;
        iterateChildren(nodep);
    }
    শূন্য visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitialStaticStmt);
        m_inInitialStaticStmt = সত্য;
        iterateChildren(nodep);
    }
    শূন্য visit(AstRelease* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        VL_RESTORER(m_setContinuously);
        VL_RESTORER(m_setForcedByCode);
        m_setRefLvalue = VAccess::WRITE;
        m_setContinuously = মিথ্যা;
        m_setForcedByCode = সত্য;
        iterate_And_পরবর্তী_Null(nodep->lhsp());
    }
    শূন্য visit(AstDeassign* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        VL_RESTORER(m_setContinuously);
        m_setRefLvalue = VAccess::WRITE;
        m_setContinuously = মিথ্যা;
        iterate_And_পরবর্তী_Null(nodep->lhsp());
    }
    শূন্য visit(AstFireEvent* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterate_And_পরবর্তী_Null(nodep->operandp());
    }
    শূন্য visit(AstCastDynamic* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::NOCHANGE;
        iterate_And_পরবর্তী_Null(nodep->fromp());
        m_setRefLvalue = VAccess::WRITE;
        iterate_And_পরবর্তী_Null(nodep->শীর্ষ());
    }
    শূন্য visit(AstFError* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterate_And_পরবর্তী_Null(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterate_And_পরবর্তী_Null(nodep->strp());
    }
    শূন্য visit(AstFGetS* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterate_And_পরবর্তী_Null(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterate_And_পরবর্তী_Null(nodep->strgp());
    }
    শূন্য visit(AstFRead* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterate_And_পরবর্তী_Null(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterate_And_পরবর্তী_Null(nodep->memp());
    }
    শূন্য visit(AstFScanF* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterate_And_পরবর্তী_Null(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterate_And_পরবর্তী_Null(nodep->exprsp());
    }
    শূন্য visit(AstFUngetC* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterate_And_পরবর্তী_Null(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterate_And_পরবর্তী_Null(nodep->rhsp());
    }
    শূন্য visit(AstSScanF* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterate_And_পরবর্তী_Null(nodep->exprsp());
    }
    শূন্য visit(AstSysIgnore* nodep) override {
        // Can't know if lvalue or not; presume not
        iterateChildren(nodep);
    }
    শূন্য visit(AstRand* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        যদি (!nodep->urandom()) m_setRefLvalue = VAccess::READWRITE;
        iterate_And_পরবর্তী_Null(nodep->seedp());
    }
    শূন্য visit(AstReadMem* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterate_And_পরবর্তী_Null(nodep->memp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterate_And_পরবর্তী_Null(nodep->filenamep());
        iterate_And_পরবর্তী_Null(nodep->lsbp());
        iterate_And_পরবর্তী_Null(nodep->msbp());
    }
    শূন্য visit(AstTestPlusArgs* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::NOCHANGE;
        iterate_And_পরবর্তী_Null(nodep->searchp());
    }
    শূন্য visit(Ast_মান_Plus_Args* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::NOCHANGE;
        iterate_And_পরবর্তী_Null(nodep->searchp());
        m_setRefLvalue = VAccess::WRITE;
        iterate_And_পরবর্তী_Null(nodep->outp());
    }
    শূন্য visit(AstSFormat* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterate_And_পরবর্তী_Null(nodep->lhsp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterate_And_পরবর্তী_Null(nodep->fmtp());
    }
    শূন্য visit(Ast_গ্রন্থি_Dist_Biop* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::READWRITE;
        iterate_And_পরবর্তী_Null(nodep->lhsp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterate_And_পরবর্তী_Null(nodep->rhsp());
    }
    শূন্য visit(Ast_গ্রন্থি_Dist_Triop* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::READWRITE;
        iterate_And_পরবর্তী_Null(nodep->lhsp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterate_And_পরবর্তী_Null(nodep->rhsp());
        iterate_And_পরবর্তী_Null(nodep->thsp());
    }
    // cppcheck-suppress constParameterPointer
    শূন্য prepost_visit(Ast_গ্রন্থি_Uniop* nodep) {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterate_And_পরবর্তী_Null(nodep->lhsp());
    }
    শূন্য visit(AstPreInc* nodep) override { prepost_visit(nodep); }
    শূন্য visit(AstPostInc* nodep) override { prepost_visit(nodep); }
    শূন্য visit(AstPreDec* nodep) override { prepost_visit(nodep); }
    শূন্য visit(AstPostDec* nodep) override { prepost_visit(nodep); }

    // Nodes that change LValue state
    শূন্য visit(AstSel* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterate_And_পরবর্তী_Null(nodep->fromp());
        // Only set lvalues on the from
        m_setRefLvalue = VAccess::NOCHANGE;
        iterate_And_পরবর্তী_Null(nodep->lsbp());
    }
    শূন্য visit(Ast_গ্রন্থি_Sel* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        // Only set lvalues on the from
        iterate_And_পরবর্তী_Null(nodep->fromp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterate_And_পরবর্তী_Null(nodep->bitp());
    }
    শূন্য visit(AstCellArrayRef* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        // selp is not an lvalue
        m_setRefLvalue = VAccess::NOCHANGE;
        iterate_And_পরবর্তী_Null(nodep->selp());
    }
    শূন্য visit(Ast_গ্রন্থি_Pre_Sel* nodep) override {
        যদি (AstSelBit* ধ্রুবক selbitp = VN_CAST(nodep, SelBit)) selbitp->access(m_setRefLvalue);
        VL_RESTORER(m_setRefLvalue);
        // Only set lvalues on the from
        iterate_And_পরবর্তী_Null(nodep->fromp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterate_And_পরবর্তী_Null(nodep->rhsp());
        iterate_And_পরবর্তী_Null(nodep->thsp());
    }
    শূন্য visit(AstMemberSel* nodep) override {
        যদি (m_setRefLvalue != VAccess::NOCHANGE) {
            nodep->access(m_setRefLvalue);
            যদি (nodep->varp() && nodep->access().isWriteOrRW()) {
                যদি (ধ্রুবক AstClockingItem* ধ্রুবক itemp
                    = VN_CAST(nodep->varp()->backp(), ClockingItem)) {
                    UINFO(5, "ClkOut " << nodep);
                    যদি (itemp->outputp()) nodep->varp(itemp->outputp()->varp());
                }
            }
        } অন্যথা {
            // It is the only place where the access is set to member select nodes.
            // If it doesn't have to be set to WRITE, it means that it is READ.
            nodep->access(VAccess::READ);
        }
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_FTask_Ref* nodep) override {
        ধ্রুবক Ast_গ্রন্থি_FTask* ধ্রুবক taskp = nodep->taskp();
        // We'll deal with mismatching pins later
        যদি (!taskp) ফেরত;
        ধ্রুবক V3TaskConnects tconnects
            = V3Task::taskConnects(nodep, taskp->stmtsp(), nullptr, মিথ্যা);
        জন্য (ধ্রুবক auto& tconnect : tconnects) {
            ধ্রুবক AstVar* ধ্রুবক portp = tconnect.first;
            ধ্রুবক AstArg* ধ্রুবক argp = tconnect.second;
            যদি (!argp) চালু;
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক pinp = argp->exprp();
            যদি (!pinp) চালু;
            VL_RESTORER(m_setRefLvalue);
            m_setRefLvalue = portp->direction().pinAccess();
            iterate(pinp);
        }
        যদি (nodep->withp()) iterate(nodep->withp());
    }
    শূন্য visit(AstConstraint* nodep) override {
        VL_RESTORER(m_setIfRand);
        m_setIfRand = সত্য;
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_FTask* nodep) override {
        VL_RESTORER(m_inFunc);
        m_inFunc = সত্য;
        iterateChildren(nodep);
    }

    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সর্বজনীন:
    // CONSTRUCTORS
    LinkLValueVisitor(Ast_গ্রন্থি* nodep, VAccess start)
        : m_setRefLvalue{start} {
        iterate(nodep);
    }
    ~LinkLValueVisitor() override = পূর্বনির্ধারিত;
};

//######################################################################
// Link class functions

শূন্য V3LinkLValue::linkLValue(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    { LinkLValueVisitor{nodep, VAccess::NOCHANGE}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("linklvalue", 0, dump_বৃক্ষ_Either_Level() >= 6);
}
শূন্য V3LinkLValue::linkLValueSet(Ast_গ্রন্থি* ধ্রুবক nodep, ধ্রুবক VAccess access) {
    // Called by later link functions when it is known a node needs
    // to be converted to a lvalue.
    UINFO(9, __FUNCTION__ << ": ");
    { LinkLValueVisitor{nodep, access}; }
}
