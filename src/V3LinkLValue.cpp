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
    منطقي m_مجموعه_Continuously = خطأ;  // Set that var has some continuous assignment
    منطقي m_مجموعه_Forced_By_کد = خطأ;  // Set that var is the target of an AstAssignForce/AstRelease
    منطقي m_مجموعه_If_Rand = خطأ;  // Update VarRefs if var declared as rand
    منطقي m_مجموعه_Strength_Specified = خطأ;  // Set that var has assignment with strength specified.
    منطقي m_درون_Func = خطأ;  // Set if inside AstNodeFTask
    منطقي m_درون_Initial_Static = خطأ;  // Set if inside AstInitialStatic
    منطقي m_درون_Initial_Static_Stmt = خطأ;  // Set if inside AstInitialStaticStmt
    VAccess m_مجموعه_Ref_Lvalue;  // Set VarRefs to lvalues for pin assignments

    // VISITORS
    // Result handing
    فراغ بازدید(Ast_گره_Var_Ref* nodep) override {
        // VarRef: LValue its reference
        إذا (m_مجموعه_If_Rand && !(nodep->varp() && nodep->varp()->isRand())) أرجع;
        إذا (m_مجموعه_Ref_Lvalue != VAccess::NOCHANGE) nodep->access(m_مجموعه_Ref_Lvalue);
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
            إذا (m_مجموعه_Continuously) {
                nodep->varp()->isContinuously(درست);
                // Strength may only be specified in continuous assignment,
                // so it is needed to check only if m_setContinuously is true
                إذا (m_مجموعه_Strength_Specified) nodep->varp()->hasStrengthAssignment(درست);
            }
            إذا (ثابت Ast_Clocking_قلم* ثابت itemp
                = VN_CAST(nodep->varp()->backp(), Clocking_قلم)) {
                UINFO(5, "ClkOut " << nodep);
                إذا (itemp->outputp()) nodep->varp(itemp->outputp()->varp());
            }
            إذا (m_مجموعه_Forced_By_کد) {
                nodep->varp()->مجموعه_Forced_By_کد();
                // If a public signal is being forced in SystemVerilog and VPI
                // is enabled, mark it as forceable to ensure that the VPI
                // functions read the forced value correctly
                إذا (v3Global.opt.vpi()
                    && (nodep->varp()->is_Sig_عمومی() || nodep->varp()->is_Sig_Mod_عمومی())) {
                    nodep->varp()->مجموعه_Forceable();
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
    فراغ بازدید(AstPin* nodep) override {
        VL_RESTORER(m_مجموعه_Ref_Lvalue);
        إذا (nodep->modVarp() && nodep->modVarp()->isWritable()) {
            // When the varref's were created, we didn't know the I/O state
            // Now that we do, and it's from a output, we know it's a lvalue
            m_مجموعه_Ref_Lvalue = VAccess::WRITE;
            iterateChildren(nodep);
            m_مجموعه_Ref_Lvalue = VAccess::NOCHANGE;
        } وإلا {
            iterateChildren(nodep);
        }
    }
    فراغ بازدید(Ast_گره_تخصیص_بده* nodep) override {
        VL_RESTORER(m_مجموعه_Ref_Lvalue);
        VL_RESTORER(m_مجموعه_Continuously);
        VL_RESTORER(m_مجموعه_Strength_Specified);
        {
            m_مجموعه_Ref_Lvalue = VAccess::WRITE;
            m_مجموعه_Continuously = VN_IS(nodep, تخصیص_بده_W);
            إذا (ثابت Ast_تخصیص_بده_W* ثابت assignwp = VN_CAST(nodep, تخصیص_بده_W)) {
                إذا (assignwp->strengthSpecp()) m_مجموعه_Strength_Specified = درست;
            }
            {
                VL_RESTORER(m_مجموعه_Forced_By_کد);
                m_مجموعه_Forced_By_کد = VN_IS(nodep, تخصیص_بده_Force);
                iterate_And_بعدی_Null(nodep->lhsp());
            }
            m_مجموعه_Ref_Lvalue = VAccess::NOCHANGE;
            m_مجموعه_Continuously = خطأ;
            m_مجموعه_Strength_Specified = خطأ;
            iterate_And_بعدی_Null(nodep->rhsp());
        }

        إذا ((m_درون_Initial_Static || m_درون_Initial_Static_Stmt) && m_درون_Func) {
            ثابت منطقي rhsHasIO = nodep->rhsp()->exists([](ثابت Ast_گره_Var_Ref* ثابت refp) {
                // Exclude module I/O referenced from a function/task.
                أرجع refp->varp() && refp->varp()->isIO()
                       && refp->varp()->lifetime() != VLifetime::NONE;
            });
            إذا (rhsHasIO) {
                nodep->rhsp()->v3warn(E_UNSUPPORTED,
                                      "Static variable initializer\n"
                                          << nodep->rhsp()->warn_بیشتر()
                                          << "is dependent on function/task I/O variable");
            } وإلا {
                ثابت منطقي rhsHasAutomatic
                    = nodep->rhsp()->exists([](ثابت Ast_گره_Var_Ref* ثابت refp) {
                          أرجع refp->varp() && refp->varp()->lifetime().isAutomatic();
                      });
                إذا (rhsHasAutomatic) {
                    nodep->rhsp()->v3error("Static variable initializer\n"
                                           << nodep->rhsp()->warn_بیشتر()
                                           << "is dependent on automatic variable");
                }
            }
        }
    }
    فراغ بازدید(AstAlias* nodep) override {
        VL_RESTORER(m_مجموعه_Ref_Lvalue);
        m_مجموعه_Ref_Lvalue = VAccess::READWRITE;
        iterateChildren(nodep);
    }
    فراغ بازدید(AstInitialStatic* nodep) override {
        VL_RESTORER(m_درون_Initial_Static);
        m_درون_Initial_Static = درست;
        iterateChildren(nodep);
    }
    فراغ بازدید(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_درون_Initial_Static_Stmt);
        m_درون_Initial_Static_Stmt = درست;
        iterateChildren(nodep);
    }
    فراغ بازدید(AstRelease* nodep) override {
        VL_RESTORER(m_مجموعه_Ref_Lvalue);
        VL_RESTORER(m_مجموعه_Continuously);
        VL_RESTORER(m_مجموعه_Forced_By_کد);
        m_مجموعه_Ref_Lvalue = VAccess::WRITE;
        m_مجموعه_Continuously = خطأ;
        m_مجموعه_Forced_By_کد = درست;
        iterate_And_بعدی_Null(nodep->lhsp());
    }
    فراغ بازدید(AstDeassign* nodep) override {
        VL_RESTORER(m_مجموعه_Ref_Lvalue);
        VL_RESTORER(m_مجموعه_Continuously);
        m_مجموعه_Ref_Lvalue = VAccess::WRITE;
        m_مجموعه_Continuously = خطأ;
        iterate_And_بعدی_Null(nodep->lhsp());
    }
    فراغ بازدید(AstFireEvent* nodep) override {
        VL_RESTORER(m_مجموعه_Ref_Lvalue);
        m_مجموعه_Ref_Lvalue = VAccess::WRITE;
        iterate_And_بعدی_Null(nodep->operandp());
    }
    فراغ بازدید(AstCastDynamic* nodep) override {
        VL_RESTORER(m_مجموعه_Ref_Lvalue);
        m_مجموعه_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_بعدی_Null(nodep->fromp());
        m_مجموعه_Ref_Lvalue = VAccess::WRITE;
        iterate_And_بعدی_Null(nodep->بالاترین());
    }
    فراغ بازدید(AstFError* nodep) override {
        VL_RESTORER(m_مجموعه_Ref_Lvalue);
        iterate_And_بعدی_Null(nodep->filep());
        m_مجموعه_Ref_Lvalue = VAccess::WRITE;
        iterate_And_بعدی_Null(nodep->strp());
    }
    فراغ بازدید(AstFGetS* nodep) override {
        VL_RESTORER(m_مجموعه_Ref_Lvalue);
        iterate_And_بعدی_Null(nodep->filep());
        m_مجموعه_Ref_Lvalue = VAccess::WRITE;
        iterate_And_بعدی_Null(nodep->strgp());
    }
    فراغ بازدید(AstFRead* nodep) override {
        VL_RESTORER(m_مجموعه_Ref_Lvalue);
        iterate_And_بعدی_Null(nodep->filep());
        m_مجموعه_Ref_Lvalue = VAccess::WRITE;
        iterate_And_بعدی_Null(nodep->memp());
    }
    فراغ بازدید(AstFScanF* nodep) override {
        VL_RESTORER(m_مجموعه_Ref_Lvalue);
        iterate_And_بعدی_Null(nodep->filep());
        m_مجموعه_Ref_Lvalue = VAccess::WRITE;
        iterate_And_بعدی_Null(nodep->exprsp());
    }
    فراغ بازدید(AstFUngetC* nodep) override {
        VL_RESTORER(m_مجموعه_Ref_Lvalue);
        iterate_And_بعدی_Null(nodep->filep());
        m_مجموعه_Ref_Lvalue = VAccess::WRITE;
        iterate_And_بعدی_Null(nodep->rhsp());
    }
    فراغ بازدید(AstSScanF* nodep) override {
        VL_RESTORER(m_مجموعه_Ref_Lvalue);
        m_مجموعه_Ref_Lvalue = VAccess::WRITE;
        iterate_And_بعدی_Null(nodep->exprsp());
    }
    فراغ بازدید(AstSysIgnore* nodep) override {
        // Can't know if lvalue or not; presume not
        iterateChildren(nodep);
    }
    فراغ بازدید(AstRand* nodep) override {
        VL_RESTORER(m_مجموعه_Ref_Lvalue);
        إذا (!nodep->urandom()) m_مجموعه_Ref_Lvalue = VAccess::READWRITE;
        iterate_And_بعدی_Null(nodep->seedp());
    }
    فراغ بازدید(AstReadMem* nodep) override {
        VL_RESTORER(m_مجموعه_Ref_Lvalue);
        m_مجموعه_Ref_Lvalue = VAccess::WRITE;
        iterate_And_بعدی_Null(nodep->memp());
        m_مجموعه_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_بعدی_Null(nodep->filenamep());
        iterate_And_بعدی_Null(nodep->lsbp());
        iterate_And_بعدی_Null(nodep->msbp());
    }
    فراغ بازدید(Ast_Test_Plus_آرگومانها* nodep) override {
        VL_RESTORER(m_مجموعه_Ref_Lvalue);
        m_مجموعه_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_بعدی_Null(nodep->searchp());
    }
    فراغ بازدید(Ast_مقدار_Plus_آرگومانها* nodep) override {
        VL_RESTORER(m_مجموعه_Ref_Lvalue);
        m_مجموعه_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_بعدی_Null(nodep->searchp());
        m_مجموعه_Ref_Lvalue = VAccess::WRITE;
        iterate_And_بعدی_Null(nodep->outp());
    }
    فراغ بازدید(AstSFormat* nodep) override {
        VL_RESTORER(m_مجموعه_Ref_Lvalue);
        m_مجموعه_Ref_Lvalue = VAccess::WRITE;
        iterate_And_بعدی_Null(nodep->lhsp());
        m_مجموعه_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_بعدی_Null(nodep->fmtp());
    }
    فراغ بازدید(Ast_گره_Dist_Biop* nodep) override {
        VL_RESTORER(m_مجموعه_Ref_Lvalue);
        m_مجموعه_Ref_Lvalue = VAccess::READWRITE;
        iterate_And_بعدی_Null(nodep->lhsp());
        m_مجموعه_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_بعدی_Null(nodep->rhsp());
    }
    فراغ بازدید(Ast_گره_Dist_Triop* nodep) override {
        VL_RESTORER(m_مجموعه_Ref_Lvalue);
        m_مجموعه_Ref_Lvalue = VAccess::READWRITE;
        iterate_And_بعدی_Null(nodep->lhsp());
        m_مجموعه_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_بعدی_Null(nodep->rhsp());
        iterate_And_بعدی_Null(nodep->thsp());
    }
    // cppcheck-suppress constParameterPointer
    فراغ prepost_بازدید(Ast_گره_Uniop* nodep) {
        VL_RESTORER(m_مجموعه_Ref_Lvalue);
        m_مجموعه_Ref_Lvalue = VAccess::WRITE;
        iterate_And_بعدی_Null(nodep->lhsp());
    }
    فراغ بازدید(AstPreInc* nodep) override { prepost_بازدید(nodep); }
    فراغ بازدید(AstPostInc* nodep) override { prepost_بازدید(nodep); }
    فراغ بازدید(AstPreDec* nodep) override { prepost_بازدید(nodep); }
    فراغ بازدید(AstPostDec* nodep) override { prepost_بازدید(nodep); }

    // Nodes that change LValue state
    فراغ بازدید(AstSel* nodep) override {
        VL_RESTORER(m_مجموعه_Ref_Lvalue);
        iterate_And_بعدی_Null(nodep->fromp());
        // Only set lvalues on the from
        m_مجموعه_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_بعدی_Null(nodep->lsbp());
    }
    فراغ بازدید(Ast_گره_Sel* nodep) override {
        VL_RESTORER(m_مجموعه_Ref_Lvalue);
        // Only set lvalues on the from
        iterate_And_بعدی_Null(nodep->fromp());
        m_مجموعه_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_بعدی_Null(nodep->bitp());
    }
    فراغ بازدید(AstCellArrayRef* nodep) override {
        VL_RESTORER(m_مجموعه_Ref_Lvalue);
        // selp is not an lvalue
        m_مجموعه_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_بعدی_Null(nodep->selp());
    }
    فراغ بازدید(Ast_گره_Pre_Sel* nodep) override {
        إذا (Ast_Sel_بیت* ثابت selbitp = VN_CAST(nodep, Sel_بیت)) selbitp->access(m_مجموعه_Ref_Lvalue);
        VL_RESTORER(m_مجموعه_Ref_Lvalue);
        // Only set lvalues on the from
        iterate_And_بعدی_Null(nodep->fromp());
        m_مجموعه_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_بعدی_Null(nodep->rhsp());
        iterate_And_بعدی_Null(nodep->thsp());
    }
    فراغ بازدید(AstMemberSel* nodep) override {
        إذا (m_مجموعه_Ref_Lvalue != VAccess::NOCHANGE) {
            nodep->access(m_مجموعه_Ref_Lvalue);
            إذا (nodep->varp() && nodep->access().isWriteOrRW()) {
                إذا (ثابت Ast_Clocking_قلم* ثابت itemp
                    = VN_CAST(nodep->varp()->backp(), Clocking_قلم)) {
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
    فراغ بازدید(Ast_گره_FTask_Ref* nodep) override {
        ثابت Ast_گره_FTask* ثابت taskp = nodep->taskp();
        // We'll deal with mismatching pins later
        إذا (!taskp) أرجع;
        ثابت V3TaskConnects tconnects
            = V3Task::taskConnects(nodep, taskp->stmtsp(), اشاره_تهی, خطأ);
        لكل (ثابت خودکار& tconnect : tconnects) {
            ثابت AstVar* ثابت portp = tconnect.اولی;
            ثابت AstArg* ثابت argp = tconnect.دومی;
            إذا (!argp) استمر;
            Ast_گره_عبارت* ثابت pinp = argp->exprp();
            إذا (!pinp) استمر;
            VL_RESTORER(m_مجموعه_Ref_Lvalue);
            m_مجموعه_Ref_Lvalue = portp->direction().pinAccess();
            iterate(pinp);
        }
        إذا (nodep->withp()) iterate(nodep->withp());
    }
    فراغ بازدید(AstConstraint* nodep) override {
        VL_RESTORER(m_مجموعه_If_Rand);
        m_مجموعه_If_Rand = درست;
        iterateChildren(nodep);
    }
    فراغ بازدید(Ast_گره_FTask* nodep) override {
        VL_RESTORER(m_درون_Func);
        m_درون_Func = درست;
        iterateChildren(nodep);
    }

    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    LinkLValueVisitor(Ast_گره* nodep, VAccess شروع)
        : m_مجموعه_Ref_Lvalue{شروع} {
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
فراغ V3LinkLValue::link_LValue_مجموعه(Ast_گره* ثابت nodep, ثابت VAccess access) {
    // Called by later link functions when it is known a node needs
    // to be converted to a lvalue.
    UINFO(9, __FUNCTION__ << ": ");
    { LinkLValueVisitor{nodep, access}; }
}
