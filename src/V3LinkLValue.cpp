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
    منطقي m_سيٽ_Continuously = خطأ;  // Set that var has some continuous assignment
    منطقي m_سيٽ_Forced_By_ڪوڊ = خطأ;  // Set that var is the target of an AstAssignForce/AstRelease
    منطقي m_سيٽ_If_Rand = خطأ;  // Update VarRefs if var declared as rand
    منطقي m_سيٽ_Strength_Specified = خطأ;  // Set that var has assignment with strength specified.
    منطقي m_اندريون_Func = خطأ;  // Set if inside AstNodeFTask
    منطقي m_اندريون_Initial_Static = خطأ;  // Set if inside AstInitialStatic
    منطقي m_اندريون_Initial_Static_Stmt = خطأ;  // Set if inside AstInitialStaticStmt
    VAccess m_سيٽ_Ref_Lvalue;  // Set VarRefs to lvalues for pin assignments

    // VISITORS
    // Result handing
    فراغ گهمو(Ast_ڳنڍ_Var_Ref* nodep) override {
        // VarRef: LValue its reference
        إذا (m_سيٽ_If_Rand && !(nodep->varp() && nodep->varp()->isRand())) أرجع;
        إذا (m_سيٽ_Ref_Lvalue != VAccess::NOCHANGE) nodep->access(m_سيٽ_Ref_Lvalue);
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
            إذا (m_سيٽ_Continuously) {
                nodep->varp()->isContinuously(سچ);
                // Strength may only be specified in continuous assignment,
                // so it is needed to check only if m_setContinuously is true
                إذا (m_سيٽ_Strength_Specified) nodep->varp()->hasStrengthAssignment(سچ);
            }
            إذا (ثابت Ast_Clocking_شي* ثابت itemp
                = VN_CAST(nodep->varp()->backp(), Clocking_شي)) {
                UINFO(5, "ClkOut " << nodep);
                إذا (itemp->outputp()) nodep->varp(itemp->outputp()->varp());
            }
            إذا (m_سيٽ_Forced_By_ڪوڊ) {
                nodep->varp()->سيٽ_Forced_By_ڪوڊ();
                // If a public signal is being forced in SystemVerilog and VPI
                // is enabled, mark it as forceable to ensure that the VPI
                // functions read the forced value correctly
                إذا (v3Global.opt.vpi()
                    && (nodep->varp()->is_Sig_عوامي() || nodep->varp()->is_Sig_Mod_عوامي())) {
                    nodep->varp()->سيٽ_Forceable();
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
    فراغ گهمو(AstPin* nodep) override {
        VL_RESTORER(m_سيٽ_Ref_Lvalue);
        إذا (nodep->modVarp() && nodep->modVarp()->isWritable()) {
            // When the varref's were created, we didn't know the I/O state
            // Now that we do, and it's from a output, we know it's a lvalue
            m_سيٽ_Ref_Lvalue = VAccess::WRITE;
            iterateChildren(nodep);
            m_سيٽ_Ref_Lvalue = VAccess::NOCHANGE;
        } وإلا {
            iterateChildren(nodep);
        }
    }
    فراغ گهمو(Ast_ڳنڍ_سونپيو* nodep) override {
        VL_RESTORER(m_سيٽ_Ref_Lvalue);
        VL_RESTORER(m_سيٽ_Continuously);
        VL_RESTORER(m_سيٽ_Strength_Specified);
        {
            m_سيٽ_Ref_Lvalue = VAccess::WRITE;
            m_سيٽ_Continuously = VN_IS(nodep, سونپيو_W);
            إذا (ثابت Ast_سونپيو_W* ثابت assignwp = VN_CAST(nodep, سونپيو_W)) {
                إذا (assignwp->strengthSpecp()) m_سيٽ_Strength_Specified = سچ;
            }
            {
                VL_RESTORER(m_سيٽ_Forced_By_ڪوڊ);
                m_سيٽ_Forced_By_ڪوڊ = VN_IS(nodep, سونپيو_Force);
                iterate_And_ايندڙ_Null(nodep->lhsp());
            }
            m_سيٽ_Ref_Lvalue = VAccess::NOCHANGE;
            m_سيٽ_Continuously = خطأ;
            m_سيٽ_Strength_Specified = خطأ;
            iterate_And_ايندڙ_Null(nodep->rhsp());
        }

        إذا ((m_اندريون_Initial_Static || m_اندريون_Initial_Static_Stmt) && m_اندريون_Func) {
            ثابت منطقي rhsHasIO = nodep->rhsp()->exists([](ثابت Ast_ڳنڍ_Var_Ref* ثابت refp) {
                // Exclude module I/O referenced from a function/task.
                أرجع refp->varp() && refp->varp()->isIO()
                       && refp->varp()->lifetime() != VLifetime::NONE;
            });
            إذا (rhsHasIO) {
                nodep->rhsp()->v3warn(E_UNSUPPORTED,
                                      "Static variable initializer\n"
                                          << nodep->rhsp()->warn_وڌيڪ()
                                          << "is dependent on function/task I/O variable");
            } وإلا {
                ثابت منطقي rhsHasAutomatic
                    = nodep->rhsp()->exists([](ثابت Ast_ڳنڍ_Var_Ref* ثابت refp) {
                          أرجع refp->varp() && refp->varp()->lifetime().isAutomatic();
                      });
                إذا (rhsHasAutomatic) {
                    nodep->rhsp()->v3error("Static variable initializer\n"
                                           << nodep->rhsp()->warn_وڌيڪ()
                                           << "is dependent on automatic variable");
                }
            }
        }
    }
    فراغ گهمو(AstAlias* nodep) override {
        VL_RESTORER(m_سيٽ_Ref_Lvalue);
        m_سيٽ_Ref_Lvalue = VAccess::READWRITE;
        iterateChildren(nodep);
    }
    فراغ گهمو(AstInitialStatic* nodep) override {
        VL_RESTORER(m_اندريون_Initial_Static);
        m_اندريون_Initial_Static = سچ;
        iterateChildren(nodep);
    }
    فراغ گهمو(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_اندريون_Initial_Static_Stmt);
        m_اندريون_Initial_Static_Stmt = سچ;
        iterateChildren(nodep);
    }
    فراغ گهمو(AstRelease* nodep) override {
        VL_RESTORER(m_سيٽ_Ref_Lvalue);
        VL_RESTORER(m_سيٽ_Continuously);
        VL_RESTORER(m_سيٽ_Forced_By_ڪوڊ);
        m_سيٽ_Ref_Lvalue = VAccess::WRITE;
        m_سيٽ_Continuously = خطأ;
        m_سيٽ_Forced_By_ڪوڊ = سچ;
        iterate_And_ايندڙ_Null(nodep->lhsp());
    }
    فراغ گهمو(AstDeassign* nodep) override {
        VL_RESTORER(m_سيٽ_Ref_Lvalue);
        VL_RESTORER(m_سيٽ_Continuously);
        m_سيٽ_Ref_Lvalue = VAccess::WRITE;
        m_سيٽ_Continuously = خطأ;
        iterate_And_ايندڙ_Null(nodep->lhsp());
    }
    فراغ گهمو(AstFireEvent* nodep) override {
        VL_RESTORER(m_سيٽ_Ref_Lvalue);
        m_سيٽ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ايندڙ_Null(nodep->operandp());
    }
    فراغ گهمو(AstCastDynamic* nodep) override {
        VL_RESTORER(m_سيٽ_Ref_Lvalue);
        m_سيٽ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_ايندڙ_Null(nodep->fromp());
        m_سيٽ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ايندڙ_Null(nodep->چوٽي());
    }
    فراغ گهمو(AstFError* nodep) override {
        VL_RESTORER(m_سيٽ_Ref_Lvalue);
        iterate_And_ايندڙ_Null(nodep->filep());
        m_سيٽ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ايندڙ_Null(nodep->strp());
    }
    فراغ گهمو(AstFGetS* nodep) override {
        VL_RESTORER(m_سيٽ_Ref_Lvalue);
        iterate_And_ايندڙ_Null(nodep->filep());
        m_سيٽ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ايندڙ_Null(nodep->strgp());
    }
    فراغ گهمو(AstFRead* nodep) override {
        VL_RESTORER(m_سيٽ_Ref_Lvalue);
        iterate_And_ايندڙ_Null(nodep->filep());
        m_سيٽ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ايندڙ_Null(nodep->memp());
    }
    فراغ گهمو(AstFScanF* nodep) override {
        VL_RESTORER(m_سيٽ_Ref_Lvalue);
        iterate_And_ايندڙ_Null(nodep->filep());
        m_سيٽ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ايندڙ_Null(nodep->exprsp());
    }
    فراغ گهمو(AstFUngetC* nodep) override {
        VL_RESTORER(m_سيٽ_Ref_Lvalue);
        iterate_And_ايندڙ_Null(nodep->filep());
        m_سيٽ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ايندڙ_Null(nodep->rhsp());
    }
    فراغ گهمو(AstSScanF* nodep) override {
        VL_RESTORER(m_سيٽ_Ref_Lvalue);
        m_سيٽ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ايندڙ_Null(nodep->exprsp());
    }
    فراغ گهمو(AstSysIgnore* nodep) override {
        // Can't know if lvalue or not; presume not
        iterateChildren(nodep);
    }
    فراغ گهمو(AstRand* nodep) override {
        VL_RESTORER(m_سيٽ_Ref_Lvalue);
        إذا (!nodep->urandom()) m_سيٽ_Ref_Lvalue = VAccess::READWRITE;
        iterate_And_ايندڙ_Null(nodep->seedp());
    }
    فراغ گهمو(AstReadMem* nodep) override {
        VL_RESTORER(m_سيٽ_Ref_Lvalue);
        m_سيٽ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ايندڙ_Null(nodep->memp());
        m_سيٽ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_ايندڙ_Null(nodep->filenamep());
        iterate_And_ايندڙ_Null(nodep->lsbp());
        iterate_And_ايندڙ_Null(nodep->msbp());
    }
    فراغ گهمو(Ast_Test_Plus_دليلون* nodep) override {
        VL_RESTORER(m_سيٽ_Ref_Lvalue);
        m_سيٽ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_ايندڙ_Null(nodep->searchp());
    }
    فراغ گهمو(Ast_قدر_Plus_دليلون* nodep) override {
        VL_RESTORER(m_سيٽ_Ref_Lvalue);
        m_سيٽ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_ايندڙ_Null(nodep->searchp());
        m_سيٽ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ايندڙ_Null(nodep->outp());
    }
    فراغ گهمو(AstSFormat* nodep) override {
        VL_RESTORER(m_سيٽ_Ref_Lvalue);
        m_سيٽ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ايندڙ_Null(nodep->lhsp());
        m_سيٽ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_ايندڙ_Null(nodep->fmtp());
    }
    فراغ گهمو(Ast_ڳنڍ_Dist_Biop* nodep) override {
        VL_RESTORER(m_سيٽ_Ref_Lvalue);
        m_سيٽ_Ref_Lvalue = VAccess::READWRITE;
        iterate_And_ايندڙ_Null(nodep->lhsp());
        m_سيٽ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_ايندڙ_Null(nodep->rhsp());
    }
    فراغ گهمو(Ast_ڳنڍ_Dist_Triop* nodep) override {
        VL_RESTORER(m_سيٽ_Ref_Lvalue);
        m_سيٽ_Ref_Lvalue = VAccess::READWRITE;
        iterate_And_ايندڙ_Null(nodep->lhsp());
        m_سيٽ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_ايندڙ_Null(nodep->rhsp());
        iterate_And_ايندڙ_Null(nodep->thsp());
    }
    // cppcheck-suppress constParameterPointer
    فراغ prepost_گهمو(Ast_ڳنڍ_Uniop* nodep) {
        VL_RESTORER(m_سيٽ_Ref_Lvalue);
        m_سيٽ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ايندڙ_Null(nodep->lhsp());
    }
    فراغ گهمو(AstPreInc* nodep) override { prepost_گهمو(nodep); }
    فراغ گهمو(AstPostInc* nodep) override { prepost_گهمو(nodep); }
    فراغ گهمو(AstPreDec* nodep) override { prepost_گهمو(nodep); }
    فراغ گهمو(AstPostDec* nodep) override { prepost_گهمو(nodep); }

    // Nodes that change LValue state
    فراغ گهمو(AstSel* nodep) override {
        VL_RESTORER(m_سيٽ_Ref_Lvalue);
        iterate_And_ايندڙ_Null(nodep->fromp());
        // Only set lvalues on the from
        m_سيٽ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_ايندڙ_Null(nodep->lsbp());
    }
    فراغ گهمو(Ast_ڳنڍ_Sel* nodep) override {
        VL_RESTORER(m_سيٽ_Ref_Lvalue);
        // Only set lvalues on the from
        iterate_And_ايندڙ_Null(nodep->fromp());
        m_سيٽ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_ايندڙ_Null(nodep->bitp());
    }
    فراغ گهمو(AstCellArrayRef* nodep) override {
        VL_RESTORER(m_سيٽ_Ref_Lvalue);
        // selp is not an lvalue
        m_سيٽ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_ايندڙ_Null(nodep->selp());
    }
    فراغ گهمو(Ast_ڳنڍ_Pre_Sel* nodep) override {
        إذا (Ast_Sel_ٻٽو_انگ* ثابت selbitp = VN_CAST(nodep, Sel_ٻٽو_انگ)) selbitp->access(m_سيٽ_Ref_Lvalue);
        VL_RESTORER(m_سيٽ_Ref_Lvalue);
        // Only set lvalues on the from
        iterate_And_ايندڙ_Null(nodep->fromp());
        m_سيٽ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_ايندڙ_Null(nodep->rhsp());
        iterate_And_ايندڙ_Null(nodep->thsp());
    }
    فراغ گهمو(AstMemberSel* nodep) override {
        إذا (m_سيٽ_Ref_Lvalue != VAccess::NOCHANGE) {
            nodep->access(m_سيٽ_Ref_Lvalue);
            إذا (nodep->varp() && nodep->access().isWriteOrRW()) {
                إذا (ثابت Ast_Clocking_شي* ثابت itemp
                    = VN_CAST(nodep->varp()->backp(), Clocking_شي)) {
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
    فراغ گهمو(Ast_ڳنڍ_FTask_Ref* nodep) override {
        ثابت Ast_ڳنڍ_FTask* ثابت taskp = nodep->taskp();
        // We'll deal with mismatching pins later
        إذا (!taskp) أرجع;
        ثابت V3TaskConnects tconnects
            = V3Task::taskConnects(nodep, taskp->stmtsp(), خالي_اشارو, خطأ);
        لكل (ثابت خودڪار& tconnect : tconnects) {
            ثابت AstVar* ثابت portp = tconnect.پهريون;
            ثابت AstArg* ثابت argp = tconnect.ٻيو;
            إذا (!argp) استمر;
            Ast_ڳنڍ_اظهار* ثابت pinp = argp->exprp();
            إذا (!pinp) استمر;
            VL_RESTORER(m_سيٽ_Ref_Lvalue);
            m_سيٽ_Ref_Lvalue = portp->direction().pinAccess();
            iterate(pinp);
        }
        إذا (nodep->withp()) iterate(nodep->withp());
    }
    فراغ گهمو(AstConstraint* nodep) override {
        VL_RESTORER(m_سيٽ_If_Rand);
        m_سيٽ_If_Rand = سچ;
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_FTask* nodep) override {
        VL_RESTORER(m_اندريون_Func);
        m_اندريون_Func = سچ;
        iterateChildren(nodep);
    }

    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    LinkLValueVisitor(Ast_ڳنڍ* nodep, VAccess شروعات)
        : m_سيٽ_Ref_Lvalue{شروعات} {
        iterate(nodep);
    }
    ~LinkLValueVisitor() override = افتراضي;
};

//######################################################################
// Link class functions

فراغ V3LinkLValue::linkLValue(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    { LinkLValueVisitor{nodep, VAccess::NOCHANGE}; }  // Destruct before checking
    V3Global::dump_Check_Global_وڻ("linklvalue", 0, dump_وڻ_Either_Level() >= 6);
}
فراغ V3LinkLValue::link_LValue_سيٽ(Ast_ڳنڍ* ثابت nodep, ثابت VAccess access) {
    // Called by later link functions when it is known a node needs
    // to be converted to a lvalue.
    UINFO(9, __FUNCTION__ << ": ");
    { LinkLValueVisitor{nodep, access}; }
}
