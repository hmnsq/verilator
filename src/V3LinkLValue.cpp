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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3LinkLValue.h"

#شامل "V3Task.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

رده LinkLValueVisitor final : عمومی VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    منطقی m_setContinuously = نادرست;  // Set that var has some continuous assignment
    منطقی m_setForcedByCode = نادرست;  // Set that var is the target of an AstAssignForce/AstRelease
    منطقی m_setIfRand = نادرست;  // Update VarRefs if var declared as rand
    منطقی m_setStrengthSpecified = نادرست;  // Set that var has assignment with strength specified.
    منطقی m_inFunc = نادرست;  // Set if inside AstNodeFTask
    منطقی m_inInitialStatic = نادرست;  // Set if inside AstInitialStatic
    منطقی m_inInitialStaticStmt = نادرست;  // Set if inside AstInitialStaticStmt
    VAccess m_setRefLvalue;  // Set VarRefs to lvalues for pin assignments

    // VISITORS
    // Result handing
    تهی visit(Ast_گره_Var_Ref* nodep) override {
        // VarRef: LValue its reference
        اگر (m_setIfRand && !(nodep->varp() && nodep->varp()->isRand())) بازگشت;
        اگر (m_setRefLvalue != VAccess::NOCHANGE) nodep->access(m_setRefLvalue);
        // Linked via an input-only modport by V3LinkDot, before it was known if written
        اگر (AstVarXRef* ثابت xrefp = VN_CAST(nodep, VarXRef)) {
            اگر (xrefp->readOnlyModport()) {
                اگر (nodep->access().isWriteOrRW()) {
                    nodep->v3error(
                        "Attempt to drive input-only modport: " << nodep->prettyNameQ());
                }
                xrefp->readOnlyModport(نادرست);
            }
        }
        اگر (nodep->varp() && nodep->access().isWriteOrRW()) {
            اگر (nodep->varp()->isParam()) {
                // All parameters that did get constified happened before now
                // as V3LinkLValue runs after V3Param
                nodep->v3error("Storing to parameter variable "
                               << nodep->prettyNameQ()
                               << " in a context that is determined only at runtime");
            }
            اگر (m_setContinuously) {
                nodep->varp()->isContinuously(درست);
                // Strength may only be specified in continuous assignment,
                // so it is needed to check only if m_setContinuously is true
                اگر (m_setStrengthSpecified) nodep->varp()->hasStrengthAssignment(درست);
            }
            اگر (ثابت AstClockingItem* ثابت itemp
                = VN_CAST(nodep->varp()->backp(), ClockingItem)) {
                UINFO(5, "ClkOut " << nodep);
                اگر (itemp->outputp()) nodep->varp(itemp->outputp()->varp());
            }
            اگر (m_setForcedByCode) {
                nodep->varp()->setForcedByCode();
                // If a public signal is being forced in SystemVerilog and VPI
                // is enabled, mark it as forceable to ensure that the VPI
                // functions read the forced value correctly
                اگر (v3Global.opt.vpi()
                    && (nodep->varp()->isSigPublic() || nodep->varp()->isSigModPublic())) {
                    nodep->varp()->setForceable();
                }
            } وگرنه اگر (!nodep->varp()->isFuncLocal() && nodep->varp()->isReadOnly()) {
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
    تهی visit(AstPin* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        اگر (nodep->modVarp() && nodep->modVarp()->isWritable()) {
            // When the varref's were created, we didn't know the I/O state
            // Now that we do, and it's from a output, we know it's a lvalue
            m_setRefLvalue = VAccess::WRITE;
            iterateChildren(nodep);
            m_setRefLvalue = VAccess::NOCHANGE;
        } وگرنه {
            iterateChildren(nodep);
        }
    }
    تهی visit(Ast_گره_Assign* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        VL_RESTORER(m_setContinuously);
        VL_RESTORER(m_setStrengthSpecified);
        {
            m_setRefLvalue = VAccess::WRITE;
            m_setContinuously = VN_IS(nodep, AssignW);
            اگر (ثابت AstAssignW* ثابت assignwp = VN_CAST(nodep, AssignW)) {
                اگر (assignwp->strengthSpecp()) m_setStrengthSpecified = درست;
            }
            {
                VL_RESTORER(m_setForcedByCode);
                m_setForcedByCode = VN_IS(nodep, AssignForce);
                iterateAndNextNull(nodep->lhsp());
            }
            m_setRefLvalue = VAccess::NOCHANGE;
            m_setContinuously = نادرست;
            m_setStrengthSpecified = نادرست;
            iterateAndNextNull(nodep->rhsp());
        }

        اگر ((m_inInitialStatic || m_inInitialStaticStmt) && m_inFunc) {
            ثابت منطقی rhsHasIO = nodep->rhsp()->exists([](ثابت Ast_گره_Var_Ref* ثابت refp) {
                // Exclude module I/O referenced from a function/task.
                بازگشت refp->varp() && refp->varp()->isIO()
                       && refp->varp()->lifetime() != VLifetime::NONE;
            });
            اگر (rhsHasIO) {
                nodep->rhsp()->v3warn(E_UNSUPPORTED,
                                      "Static variable initializer\n"
                                          << nodep->rhsp()->warnMore()
                                          << "is dependent on function/task I/O variable");
            } وگرنه {
                ثابت منطقی rhsHasAutomatic
                    = nodep->rhsp()->exists([](ثابت Ast_گره_Var_Ref* ثابت refp) {
                          بازگشت refp->varp() && refp->varp()->lifetime().isAutomatic();
                      });
                اگر (rhsHasAutomatic) {
                    nodep->rhsp()->v3error("Static variable initializer\n"
                                           << nodep->rhsp()->warnMore()
                                           << "is dependent on automatic variable");
                }
            }
        }
    }
    تهی visit(AstAlias* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::READWRITE;
        iterateChildren(nodep);
    }
    تهی visit(AstInitialStatic* nodep) override {
        VL_RESTORER(m_inInitialStatic);
        m_inInitialStatic = درست;
        iterateChildren(nodep);
    }
    تهی visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitialStaticStmt);
        m_inInitialStaticStmt = درست;
        iterateChildren(nodep);
    }
    تهی visit(AstRelease* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        VL_RESTORER(m_setContinuously);
        VL_RESTORER(m_setForcedByCode);
        m_setRefLvalue = VAccess::WRITE;
        m_setContinuously = نادرست;
        m_setForcedByCode = درست;
        iterateAndNextNull(nodep->lhsp());
    }
    تهی visit(AstDeassign* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        VL_RESTORER(m_setContinuously);
        m_setRefLvalue = VAccess::WRITE;
        m_setContinuously = نادرست;
        iterateAndNextNull(nodep->lhsp());
    }
    تهی visit(AstFireEvent* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->operandp());
    }
    تهی visit(AstCastDynamic* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->fromp());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->بالاترین());
    }
    تهی visit(AstFError* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->strp());
    }
    تهی visit(AstFGetS* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->strgp());
    }
    تهی visit(AstFRead* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->memp());
    }
    تهی visit(AstFScanF* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->exprsp());
    }
    تهی visit(AstFUngetC* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->filep());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->rhsp());
    }
    تهی visit(AstSScanF* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->exprsp());
    }
    تهی visit(AstSysIgnore* nodep) override {
        // Can't know if lvalue or not; presume not
        iterateChildren(nodep);
    }
    تهی visit(AstRand* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        اگر (!nodep->urandom()) m_setRefLvalue = VAccess::READWRITE;
        iterateAndNextNull(nodep->seedp());
    }
    تهی visit(AstReadMem* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->memp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->filenamep());
        iterateAndNextNull(nodep->lsbp());
        iterateAndNextNull(nodep->msbp());
    }
    تهی visit(AstTestPlusArgs* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->searchp());
    }
    تهی visit(Ast_مقدار_Plus_Args* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->searchp());
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->outp());
    }
    تهی visit(AstSFormat* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->lhsp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->fmtp());
    }
    تهی visit(Ast_گره_Dist_Biop* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::READWRITE;
        iterateAndNextNull(nodep->lhsp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->rhsp());
    }
    تهی visit(Ast_گره_Dist_Triop* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::READWRITE;
        iterateAndNextNull(nodep->lhsp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->rhsp());
        iterateAndNextNull(nodep->thsp());
    }
    // cppcheck-suppress constParameterPointer
    تهی prepost_visit(Ast_گره_Uniop* nodep) {
        VL_RESTORER(m_setRefLvalue);
        m_setRefLvalue = VAccess::WRITE;
        iterateAndNextNull(nodep->lhsp());
    }
    تهی visit(AstPreInc* nodep) override { prepost_visit(nodep); }
    تهی visit(AstPostInc* nodep) override { prepost_visit(nodep); }
    تهی visit(AstPreDec* nodep) override { prepost_visit(nodep); }
    تهی visit(AstPostDec* nodep) override { prepost_visit(nodep); }

    // Nodes that change LValue state
    تهی visit(AstSel* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        iterateAndNextNull(nodep->fromp());
        // Only set lvalues on the from
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->lsbp());
    }
    تهی visit(Ast_گره_Sel* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        // Only set lvalues on the from
        iterateAndNextNull(nodep->fromp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->bitp());
    }
    تهی visit(AstCellArrayRef* nodep) override {
        VL_RESTORER(m_setRefLvalue);
        // selp is not an lvalue
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->selp());
    }
    تهی visit(Ast_گره_Pre_Sel* nodep) override {
        اگر (AstSelBit* ثابت selbitp = VN_CAST(nodep, SelBit)) selbitp->access(m_setRefLvalue);
        VL_RESTORER(m_setRefLvalue);
        // Only set lvalues on the from
        iterateAndNextNull(nodep->fromp());
        m_setRefLvalue = VAccess::NOCHANGE;
        iterateAndNextNull(nodep->rhsp());
        iterateAndNextNull(nodep->thsp());
    }
    تهی visit(AstMemberSel* nodep) override {
        اگر (m_setRefLvalue != VAccess::NOCHANGE) {
            nodep->access(m_setRefLvalue);
            اگر (nodep->varp() && nodep->access().isWriteOrRW()) {
                اگر (ثابت AstClockingItem* ثابت itemp
                    = VN_CAST(nodep->varp()->backp(), ClockingItem)) {
                    UINFO(5, "ClkOut " << nodep);
                    اگر (itemp->outputp()) nodep->varp(itemp->outputp()->varp());
                }
            }
        } وگرنه {
            // It is the only place where the access is set to member select nodes.
            // If it doesn't have to be set to WRITE, it means that it is READ.
            nodep->access(VAccess::READ);
        }
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره_FTask_Ref* nodep) override {
        ثابت Ast_گره_FTask* ثابت taskp = nodep->taskp();
        // We'll deal with mismatching pins later
        اگر (!taskp) بازگشت;
        ثابت V3TaskConnects tconnects
            = V3Task::taskConnects(nodep, taskp->stmtsp(), nullptr, نادرست);
        برای (ثابت auto& tconnect : tconnects) {
            ثابت AstVar* ثابت portp = tconnect.first;
            ثابت AstArg* ثابت argp = tconnect.second;
            اگر (!argp) ادامه;
            Ast_گره_عبارت* ثابت pinp = argp->exprp();
            اگر (!pinp) ادامه;
            VL_RESTORER(m_setRefLvalue);
            m_setRefLvalue = portp->direction().pinAccess();
            iterate(pinp);
        }
        اگر (nodep->withp()) iterate(nodep->withp());
    }
    تهی visit(AstConstraint* nodep) override {
        VL_RESTORER(m_setIfRand);
        m_setIfRand = درست;
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره_FTask* nodep) override {
        VL_RESTORER(m_inFunc);
        m_inFunc = درست;
        iterateChildren(nodep);
    }

    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    LinkLValueVisitor(Ast_گره* nodep, VAccess start)
        : m_setRefLvalue{start} {
        iterate(nodep);
    }
    ~LinkLValueVisitor() override = پیشفرض;
};

//######################################################################
// Link class functions

تهی V3LinkLValue::linkLValue(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    { LinkLValueVisitor{nodep, VAccess::NOCHANGE}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("linklvalue", 0, dump_درخت_Either_Level() >= 6);
}
تهی V3LinkLValue::linkLValueSet(Ast_گره* ثابت nodep, ثابت VAccess access) {
    // Called by later link functions when it is known a node needs
    // to be converted to a lvalue.
    UINFO(9, __FUNCTION__ << ": ");
    { LinkLValueVisitor{nodep, access}; }
}
