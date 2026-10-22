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

جماعت LinkLValueVisitor final : عوامی VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    منطقی m_سیٹ_Continuously = جھوٹ;  // Set that var has some continuous assignment
    منطقی m_سیٹ_Forced_By_رمز = جھوٹ;  // Set that var is the target of an AstAssignForce/AstRelease
    منطقی m_سیٹ_If_Rand = جھوٹ;  // Update VarRefs if var declared as rand
    منطقی m_سیٹ_Strength_Specified = جھوٹ;  // Set that var has assignment with strength specified.
    منطقی m_اندر_Func = جھوٹ;  // Set if inside AstNodeFTask
    منطقی m_اندر_Initial_Static = جھوٹ;  // Set if inside AstInitialStatic
    منطقی m_اندر_Initial_Static_Stmt = جھوٹ;  // Set if inside AstInitialStaticStmt
    VAccess m_سیٹ_Ref_Lvalue;  // Set VarRefs to lvalues for pin assignments

    // VISITORS
    // Result handing
    خالی دورہ(Ast_گرہ_Var_Ref* nodep) override {
        // VarRef: LValue its reference
        اگر (m_سیٹ_If_Rand && !(nodep->varp() && nodep->varp()->isRand())) واپس;
        اگر (m_سیٹ_Ref_Lvalue != VAccess::NOCHANGE) nodep->access(m_سیٹ_Ref_Lvalue);
        // Linked via an input-only modport by V3LinkDot, before it was known if written
        اگر (AstVarXRef* ثابت xrefp = VN_CAST(nodep, VarXRef)) {
            اگر (xrefp->readOnlyModport()) {
                اگر (nodep->access().isWriteOrRW()) {
                    nodep->v3error(
                        "Attempt to drive input-only modport: " << nodep->prettyNameQ());
                }
                xrefp->readOnlyModport(جھوٹ);
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
            اگر (m_سیٹ_Continuously) {
                nodep->varp()->isContinuously(سچ);
                // Strength may only be specified in continuous assignment,
                // so it is needed to check only if m_setContinuously is true
                اگر (m_سیٹ_Strength_Specified) nodep->varp()->hasStrengthAssignment(سچ);
            }
            اگر (ثابت Ast_Clocking_شے* ثابت itemp
                = VN_CAST(nodep->varp()->backp(), Clocking_شے)) {
                UINFO(5, "ClkOut " << nodep);
                اگر (itemp->outputp()) nodep->varp(itemp->outputp()->varp());
            }
            اگر (m_سیٹ_Forced_By_رمز) {
                nodep->varp()->سیٹ_Forced_By_رمز();
                // If a public signal is being forced in SystemVerilog and VPI
                // is enabled, mark it as forceable to ensure that the VPI
                // functions read the forced value correctly
                اگر (v3Global.opt.vpi()
                    && (nodep->varp()->is_Sig_عوامی() || nodep->varp()->is_Sig_Mod_عوامی())) {
                    nodep->varp()->سیٹ_Forceable();
                }
            } ورنہ اگر (!nodep->varp()->isFuncLocal() && nodep->varp()->isReadOnly()) {
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
    خالی دورہ(AstPin* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        اگر (nodep->modVarp() && nodep->modVarp()->isWritable()) {
            // When the varref's were created, we didn't know the I/O state
            // Now that we do, and it's from a output, we know it's a lvalue
            m_سیٹ_Ref_Lvalue = VAccess::WRITE;
            iterateChildren(nodep);
            m_سیٹ_Ref_Lvalue = VAccess::NOCHANGE;
        } ورنہ {
            iterateChildren(nodep);
        }
    }
    خالی دورہ(Ast_گرہ_تفویض* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        VL_RESTORER(m_سیٹ_Continuously);
        VL_RESTORER(m_سیٹ_Strength_Specified);
        {
            m_سیٹ_Ref_Lvalue = VAccess::WRITE;
            m_سیٹ_Continuously = VN_IS(nodep, تفویض_W);
            اگر (ثابت Ast_تفویض_W* ثابت assignwp = VN_CAST(nodep, تفویض_W)) {
                اگر (assignwp->strengthSpecp()) m_سیٹ_Strength_Specified = سچ;
            }
            {
                VL_RESTORER(m_سیٹ_Forced_By_رمز);
                m_سیٹ_Forced_By_رمز = VN_IS(nodep, تفویض_Force);
                iterate_And_اگلا_Null(nodep->lhsp());
            }
            m_سیٹ_Ref_Lvalue = VAccess::NOCHANGE;
            m_سیٹ_Continuously = جھوٹ;
            m_سیٹ_Strength_Specified = جھوٹ;
            iterate_And_اگلا_Null(nodep->rhsp());
        }

        اگر ((m_اندر_Initial_Static || m_اندر_Initial_Static_Stmt) && m_اندر_Func) {
            ثابت منطقی rhsHasIO = nodep->rhsp()->exists([](ثابت Ast_گرہ_Var_Ref* ثابت refp) {
                // Exclude module I/O referenced from a function/task.
                واپس refp->varp() && refp->varp()->isIO()
                       && refp->varp()->lifetime() != VLifetime::NONE;
            });
            اگر (rhsHasIO) {
                nodep->rhsp()->v3warn(E_UNSUPPORTED,
                                      "Static variable initializer\n"
                                          << nodep->rhsp()->warn_مزید()
                                          << "is dependent on function/task I/O variable");
            } ورنہ {
                ثابت منطقی rhsHasAutomatic
                    = nodep->rhsp()->exists([](ثابت Ast_گرہ_Var_Ref* ثابت refp) {
                          واپس refp->varp() && refp->varp()->lifetime().isAutomatic();
                      });
                اگر (rhsHasAutomatic) {
                    nodep->rhsp()->v3error("Static variable initializer\n"
                                           << nodep->rhsp()->warn_مزید()
                                           << "is dependent on automatic variable");
                }
            }
        }
    }
    خالی دورہ(AstAlias* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        m_سیٹ_Ref_Lvalue = VAccess::READWRITE;
        iterateChildren(nodep);
    }
    خالی دورہ(AstInitialStatic* nodep) override {
        VL_RESTORER(m_اندر_Initial_Static);
        m_اندر_Initial_Static = سچ;
        iterateChildren(nodep);
    }
    خالی دورہ(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_اندر_Initial_Static_Stmt);
        m_اندر_Initial_Static_Stmt = سچ;
        iterateChildren(nodep);
    }
    خالی دورہ(AstRelease* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        VL_RESTORER(m_سیٹ_Continuously);
        VL_RESTORER(m_سیٹ_Forced_By_رمز);
        m_سیٹ_Ref_Lvalue = VAccess::WRITE;
        m_سیٹ_Continuously = جھوٹ;
        m_سیٹ_Forced_By_رمز = سچ;
        iterate_And_اگلا_Null(nodep->lhsp());
    }
    خالی دورہ(AstDeassign* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        VL_RESTORER(m_سیٹ_Continuously);
        m_سیٹ_Ref_Lvalue = VAccess::WRITE;
        m_سیٹ_Continuously = جھوٹ;
        iterate_And_اگلا_Null(nodep->lhsp());
    }
    خالی دورہ(AstFireEvent* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        m_سیٹ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_اگلا_Null(nodep->operandp());
    }
    خالی دورہ(AstCastDynamic* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        m_سیٹ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_اگلا_Null(nodep->fromp());
        m_سیٹ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_اگلا_Null(nodep->چوٹی());
    }
    خالی دورہ(AstFError* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        iterate_And_اگلا_Null(nodep->filep());
        m_سیٹ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_اگلا_Null(nodep->strp());
    }
    خالی دورہ(AstFGetS* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        iterate_And_اگلا_Null(nodep->filep());
        m_سیٹ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_اگلا_Null(nodep->strgp());
    }
    خالی دورہ(AstFRead* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        iterate_And_اگلا_Null(nodep->filep());
        m_سیٹ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_اگلا_Null(nodep->memp());
    }
    خالی دورہ(AstFScanF* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        iterate_And_اگلا_Null(nodep->filep());
        m_سیٹ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_اگلا_Null(nodep->exprsp());
    }
    خالی دورہ(AstFUngetC* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        iterate_And_اگلا_Null(nodep->filep());
        m_سیٹ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_اگلا_Null(nodep->rhsp());
    }
    خالی دورہ(AstSScanF* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        m_سیٹ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_اگلا_Null(nodep->exprsp());
    }
    خالی دورہ(AstSysIgnore* nodep) override {
        // Can't know if lvalue or not; presume not
        iterateChildren(nodep);
    }
    خالی دورہ(AstRand* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        اگر (!nodep->urandom()) m_سیٹ_Ref_Lvalue = VAccess::READWRITE;
        iterate_And_اگلا_Null(nodep->seedp());
    }
    خالی دورہ(AstReadMem* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        m_سیٹ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_اگلا_Null(nodep->memp());
        m_سیٹ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_اگلا_Null(nodep->filenamep());
        iterate_And_اگلا_Null(nodep->lsbp());
        iterate_And_اگلا_Null(nodep->msbp());
    }
    خالی دورہ(Ast_Test_Plus_دلائل* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        m_سیٹ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_اگلا_Null(nodep->searchp());
    }
    خالی دورہ(Ast_قدر_Plus_دلائل* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        m_سیٹ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_اگلا_Null(nodep->searchp());
        m_سیٹ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_اگلا_Null(nodep->outp());
    }
    خالی دورہ(AstSFormat* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        m_سیٹ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_اگلا_Null(nodep->lhsp());
        m_سیٹ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_اگلا_Null(nodep->fmtp());
    }
    خالی دورہ(Ast_گرہ_Dist_Biop* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        m_سیٹ_Ref_Lvalue = VAccess::READWRITE;
        iterate_And_اگلا_Null(nodep->lhsp());
        m_سیٹ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_اگلا_Null(nodep->rhsp());
    }
    خالی دورہ(Ast_گرہ_Dist_Triop* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        m_سیٹ_Ref_Lvalue = VAccess::READWRITE;
        iterate_And_اگلا_Null(nodep->lhsp());
        m_سیٹ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_اگلا_Null(nodep->rhsp());
        iterate_And_اگلا_Null(nodep->thsp());
    }
    // cppcheck-suppress constParameterPointer
    خالی prepost_دورہ(Ast_گرہ_Uniop* nodep) {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        m_سیٹ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_اگلا_Null(nodep->lhsp());
    }
    خالی دورہ(AstPreInc* nodep) override { prepost_دورہ(nodep); }
    خالی دورہ(AstPostInc* nodep) override { prepost_دورہ(nodep); }
    خالی دورہ(AstPreDec* nodep) override { prepost_دورہ(nodep); }
    خالی دورہ(AstPostDec* nodep) override { prepost_دورہ(nodep); }

    // Nodes that change LValue state
    خالی دورہ(AstSel* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        iterate_And_اگلا_Null(nodep->fromp());
        // Only set lvalues on the from
        m_سیٹ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_اگلا_Null(nodep->lsbp());
    }
    خالی دورہ(Ast_گرہ_Sel* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        // Only set lvalues on the from
        iterate_And_اگلا_Null(nodep->fromp());
        m_سیٹ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_اگلا_Null(nodep->bitp());
    }
    خالی دورہ(AstCellArrayRef* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        // selp is not an lvalue
        m_سیٹ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_اگلا_Null(nodep->selp());
    }
    خالی دورہ(Ast_گرہ_Pre_Sel* nodep) override {
        اگر (Ast_Sel_ثنائی_ہندسہ* ثابت selbitp = VN_CAST(nodep, Sel_ثنائی_ہندسہ)) selbitp->access(m_سیٹ_Ref_Lvalue);
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        // Only set lvalues on the from
        iterate_And_اگلا_Null(nodep->fromp());
        m_سیٹ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_اگلا_Null(nodep->rhsp());
        iterate_And_اگلا_Null(nodep->thsp());
    }
    خالی دورہ(AstMemberSel* nodep) override {
        اگر (m_سیٹ_Ref_Lvalue != VAccess::NOCHANGE) {
            nodep->access(m_سیٹ_Ref_Lvalue);
            اگر (nodep->varp() && nodep->access().isWriteOrRW()) {
                اگر (ثابت Ast_Clocking_شے* ثابت itemp
                    = VN_CAST(nodep->varp()->backp(), Clocking_شے)) {
                    UINFO(5, "ClkOut " << nodep);
                    اگر (itemp->outputp()) nodep->varp(itemp->outputp()->varp());
                }
            }
        } ورنہ {
            // It is the only place where the access is set to member select nodes.
            // If it doesn't have to be set to WRITE, it means that it is READ.
            nodep->access(VAccess::READ);
        }
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ_FTask_Ref* nodep) override {
        ثابت Ast_گرہ_FTask* ثابت taskp = nodep->taskp();
        // We'll deal with mismatching pins later
        اگر (!taskp) واپس;
        ثابت V3TaskConnects tconnects
            = V3Task::taskConnects(nodep, taskp->stmtsp(), خالی_اشاریہ, جھوٹ);
        برائے (ثابت خودکار& tconnect : tconnects) {
            ثابت AstVar* ثابت portp = tconnect.پہلا;
            ثابت AstArg* ثابت argp = tconnect.دوسرا;
            اگر (!argp) جاری;
            Ast_گرہ_اظہاریہ* ثابت pinp = argp->exprp();
            اگر (!pinp) جاری;
            VL_RESTORER(m_سیٹ_Ref_Lvalue);
            m_سیٹ_Ref_Lvalue = portp->direction().pinAccess();
            iterate(pinp);
        }
        اگر (nodep->withp()) iterate(nodep->withp());
    }
    خالی دورہ(AstConstraint* nodep) override {
        VL_RESTORER(m_سیٹ_If_Rand);
        m_سیٹ_If_Rand = سچ;
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ_FTask* nodep) override {
        VL_RESTORER(m_اندر_Func);
        m_اندر_Func = سچ;
        iterateChildren(nodep);
    }

    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    LinkLValueVisitor(Ast_گرہ* nodep, VAccess آغاز)
        : m_سیٹ_Ref_Lvalue{آغاز} {
        iterate(nodep);
    }
    ~LinkLValueVisitor() override = طےشدہ;
};

//######################################################################
// Link class functions

خالی V3LinkLValue::linkLValue(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    { LinkLValueVisitor{nodep, VAccess::NOCHANGE}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("linklvalue", 0, dump_درخت_Either_Level() >= 6);
}
خالی V3LinkLValue::link_LValue_سیٹ(Ast_گرہ* ثابت nodep, ثابت VAccess access) {
    // Called by later link functions when it is known a node needs
    // to be converted to a lvalue.
    UINFO(9, __FUNCTION__ << ": ");
    { LinkLValueVisitor{nodep, access}; }
}
