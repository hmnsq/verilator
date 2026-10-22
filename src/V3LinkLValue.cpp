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
    منطقي m_سیٹ_Continuously = خطأ;  // Set that var has some continuous assignment
    منطقي m_سیٹ_Forced_By_کوڈ = خطأ;  // Set that var is the target of an AstAssignForce/AstRelease
    منطقي m_سیٹ_If_Rand = خطأ;  // Update VarRefs if var declared as rand
    منطقي m_سیٹ_Strength_Specified = خطأ;  // Set that var has assignment with strength specified.
    منطقي m_وچ_Func = خطأ;  // Set if inside AstNodeFTask
    منطقي m_وچ_Initial_Static = خطأ;  // Set if inside AstInitialStatic
    منطقي m_وچ_Initial_Static_Stmt = خطأ;  // Set if inside AstInitialStaticStmt
    VAccess m_سیٹ_Ref_Lvalue;  // Set VarRefs to lvalues for pin assignments

    // VISITORS
    // Result handing
    فراغ ویکھو(Ast_گرہ_Var_Ref* nodep) override {
        // VarRef: LValue its reference
        إذا (m_سیٹ_If_Rand && !(nodep->varp() && nodep->varp()->isRand())) أرجع;
        إذا (m_سیٹ_Ref_Lvalue != VAccess::NOCHANGE) nodep->access(m_سیٹ_Ref_Lvalue);
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
            إذا (m_سیٹ_Continuously) {
                nodep->varp()->isContinuously(سچ);
                // Strength may only be specified in continuous assignment,
                // so it is needed to check only if m_setContinuously is true
                إذا (m_سیٹ_Strength_Specified) nodep->varp()->hasStrengthAssignment(سچ);
            }
            إذا (ثابت Ast_Clocking_شے* ثابت itemp
                = VN_CAST(nodep->varp()->backp(), Clocking_شے)) {
                UINFO(5, "ClkOut " << nodep);
                إذا (itemp->outputp()) nodep->varp(itemp->outputp()->varp());
            }
            إذا (m_سیٹ_Forced_By_کوڈ) {
                nodep->varp()->سیٹ_Forced_By_کوڈ();
                // If a public signal is being forced in SystemVerilog and VPI
                // is enabled, mark it as forceable to ensure that the VPI
                // functions read the forced value correctly
                إذا (v3Global.opt.vpi()
                    && (nodep->varp()->is_Sig_عوامی() || nodep->varp()->is_Sig_Mod_عوامی())) {
                    nodep->varp()->سیٹ_Forceable();
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
    فراغ ویکھو(AstPin* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        إذا (nodep->modVarp() && nodep->modVarp()->isWritable()) {
            // When the varref's were created, we didn't know the I/O state
            // Now that we do, and it's from a output, we know it's a lvalue
            m_سیٹ_Ref_Lvalue = VAccess::WRITE;
            iterateChildren(nodep);
            m_سیٹ_Ref_Lvalue = VAccess::NOCHANGE;
        } وإلا {
            iterateChildren(nodep);
        }
    }
    فراغ ویکھو(Ast_گرہ_سونپو* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        VL_RESTORER(m_سیٹ_Continuously);
        VL_RESTORER(m_سیٹ_Strength_Specified);
        {
            m_سیٹ_Ref_Lvalue = VAccess::WRITE;
            m_سیٹ_Continuously = VN_IS(nodep, سونپو_W);
            إذا (ثابت Ast_سونپو_W* ثابت assignwp = VN_CAST(nodep, سونپو_W)) {
                إذا (assignwp->strengthSpecp()) m_سیٹ_Strength_Specified = سچ;
            }
            {
                VL_RESTORER(m_سیٹ_Forced_By_کوڈ);
                m_سیٹ_Forced_By_کوڈ = VN_IS(nodep, سونپو_Force);
                iterate_And_اگلا_Null(nodep->lhsp());
            }
            m_سیٹ_Ref_Lvalue = VAccess::NOCHANGE;
            m_سیٹ_Continuously = خطأ;
            m_سیٹ_Strength_Specified = خطأ;
            iterate_And_اگلا_Null(nodep->rhsp());
        }

        إذا ((m_وچ_Initial_Static || m_وچ_Initial_Static_Stmt) && m_وچ_Func) {
            ثابت منطقي rhsHasIO = nodep->rhsp()->exists([](ثابت Ast_گرہ_Var_Ref* ثابت refp) {
                // Exclude module I/O referenced from a function/task.
                أرجع refp->varp() && refp->varp()->isIO()
                       && refp->varp()->lifetime() != VLifetime::NONE;
            });
            إذا (rhsHasIO) {
                nodep->rhsp()->v3warn(E_UNSUPPORTED,
                                      "Static variable initializer\n"
                                          << nodep->rhsp()->warn_ہور()
                                          << "is dependent on function/task I/O variable");
            } وإلا {
                ثابت منطقي rhsHasAutomatic
                    = nodep->rhsp()->exists([](ثابت Ast_گرہ_Var_Ref* ثابت refp) {
                          أرجع refp->varp() && refp->varp()->lifetime().isAutomatic();
                      });
                إذا (rhsHasAutomatic) {
                    nodep->rhsp()->v3error("Static variable initializer\n"
                                           << nodep->rhsp()->warn_ہور()
                                           << "is dependent on automatic variable");
                }
            }
        }
    }
    فراغ ویکھو(AstAlias* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        m_سیٹ_Ref_Lvalue = VAccess::READWRITE;
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstInitialStatic* nodep) override {
        VL_RESTORER(m_وچ_Initial_Static);
        m_وچ_Initial_Static = سچ;
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_وچ_Initial_Static_Stmt);
        m_وچ_Initial_Static_Stmt = سچ;
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstRelease* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        VL_RESTORER(m_سیٹ_Continuously);
        VL_RESTORER(m_سیٹ_Forced_By_کوڈ);
        m_سیٹ_Ref_Lvalue = VAccess::WRITE;
        m_سیٹ_Continuously = خطأ;
        m_سیٹ_Forced_By_کوڈ = سچ;
        iterate_And_اگلا_Null(nodep->lhsp());
    }
    فراغ ویکھو(AstDeassign* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        VL_RESTORER(m_سیٹ_Continuously);
        m_سیٹ_Ref_Lvalue = VAccess::WRITE;
        m_سیٹ_Continuously = خطأ;
        iterate_And_اگلا_Null(nodep->lhsp());
    }
    فراغ ویکھو(AstFireEvent* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        m_سیٹ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_اگلا_Null(nodep->operandp());
    }
    فراغ ویکھو(AstCastDynamic* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        m_سیٹ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_اگلا_Null(nodep->fromp());
        m_سیٹ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_اگلا_Null(nodep->چوٹی());
    }
    فراغ ویکھو(AstFError* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        iterate_And_اگلا_Null(nodep->filep());
        m_سیٹ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_اگلا_Null(nodep->strp());
    }
    فراغ ویکھو(AstFGetS* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        iterate_And_اگلا_Null(nodep->filep());
        m_سیٹ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_اگلا_Null(nodep->strgp());
    }
    فراغ ویکھو(AstFRead* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        iterate_And_اگلا_Null(nodep->filep());
        m_سیٹ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_اگلا_Null(nodep->memp());
    }
    فراغ ویکھو(AstFScanF* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        iterate_And_اگلا_Null(nodep->filep());
        m_سیٹ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_اگلا_Null(nodep->exprsp());
    }
    فراغ ویکھو(AstFUngetC* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        iterate_And_اگلا_Null(nodep->filep());
        m_سیٹ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_اگلا_Null(nodep->rhsp());
    }
    فراغ ویکھو(AstSScanF* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        m_سیٹ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_اگلا_Null(nodep->exprsp());
    }
    فراغ ویکھو(AstSysIgnore* nodep) override {
        // Can't know if lvalue or not; presume not
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstRand* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        إذا (!nodep->urandom()) m_سیٹ_Ref_Lvalue = VAccess::READWRITE;
        iterate_And_اگلا_Null(nodep->seedp());
    }
    فراغ ویکھو(AstReadMem* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        m_سیٹ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_اگلا_Null(nodep->memp());
        m_سیٹ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_اگلا_Null(nodep->filenamep());
        iterate_And_اگلا_Null(nodep->lsbp());
        iterate_And_اگلا_Null(nodep->msbp());
    }
    فراغ ویکھو(Ast_Test_Plus_دلیلاں* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        m_سیٹ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_اگلا_Null(nodep->searchp());
    }
    فراغ ویکھو(Ast_قدر_Plus_دلیلاں* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        m_سیٹ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_اگلا_Null(nodep->searchp());
        m_سیٹ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_اگلا_Null(nodep->outp());
    }
    فراغ ویکھو(AstSFormat* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        m_سیٹ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_اگلا_Null(nodep->lhsp());
        m_سیٹ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_اگلا_Null(nodep->fmtp());
    }
    فراغ ویکھو(Ast_گرہ_Dist_Biop* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        m_سیٹ_Ref_Lvalue = VAccess::READWRITE;
        iterate_And_اگلا_Null(nodep->lhsp());
        m_سیٹ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_اگلا_Null(nodep->rhsp());
    }
    فراغ ویکھو(Ast_گرہ_Dist_Triop* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        m_سیٹ_Ref_Lvalue = VAccess::READWRITE;
        iterate_And_اگلا_Null(nodep->lhsp());
        m_سیٹ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_اگلا_Null(nodep->rhsp());
        iterate_And_اگلا_Null(nodep->thsp());
    }
    // cppcheck-suppress constParameterPointer
    فراغ prepost_ویکھو(Ast_گرہ_Uniop* nodep) {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        m_سیٹ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_اگلا_Null(nodep->lhsp());
    }
    فراغ ویکھو(AstPreInc* nodep) override { prepost_ویکھو(nodep); }
    فراغ ویکھو(AstPostInc* nodep) override { prepost_ویکھو(nodep); }
    فراغ ویکھو(AstPreDec* nodep) override { prepost_ویکھو(nodep); }
    فراغ ویکھو(AstPostDec* nodep) override { prepost_ویکھو(nodep); }

    // Nodes that change LValue state
    فراغ ویکھو(AstSel* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        iterate_And_اگلا_Null(nodep->fromp());
        // Only set lvalues on the from
        m_سیٹ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_اگلا_Null(nodep->lsbp());
    }
    فراغ ویکھو(Ast_گرہ_Sel* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        // Only set lvalues on the from
        iterate_And_اگلا_Null(nodep->fromp());
        m_سیٹ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_اگلا_Null(nodep->bitp());
    }
    فراغ ویکھو(AstCellArrayRef* nodep) override {
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        // selp is not an lvalue
        m_سیٹ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_اگلا_Null(nodep->selp());
    }
    فراغ ویکھو(Ast_گرہ_Pre_Sel* nodep) override {
        إذا (Ast_Sel_ثنائی_ہندسہ* ثابت selbitp = VN_CAST(nodep, Sel_ثنائی_ہندسہ)) selbitp->access(m_سیٹ_Ref_Lvalue);
        VL_RESTORER(m_سیٹ_Ref_Lvalue);
        // Only set lvalues on the from
        iterate_And_اگلا_Null(nodep->fromp());
        m_سیٹ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_اگلا_Null(nodep->rhsp());
        iterate_And_اگلا_Null(nodep->thsp());
    }
    فراغ ویکھو(AstMemberSel* nodep) override {
        إذا (m_سیٹ_Ref_Lvalue != VAccess::NOCHANGE) {
            nodep->access(m_سیٹ_Ref_Lvalue);
            إذا (nodep->varp() && nodep->access().isWriteOrRW()) {
                إذا (ثابت Ast_Clocking_شے* ثابت itemp
                    = VN_CAST(nodep->varp()->backp(), Clocking_شے)) {
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
    فراغ ویکھو(Ast_گرہ_FTask_Ref* nodep) override {
        ثابت Ast_گرہ_FTask* ثابت taskp = nodep->taskp();
        // We'll deal with mismatching pins later
        إذا (!taskp) أرجع;
        ثابت V3TaskConnects tconnects
            = V3Task::taskConnects(nodep, taskp->stmtsp(), خالی_سوچک, خطأ);
        لكل (ثابت خودکار& tconnect : tconnects) {
            ثابت AstVar* ثابت portp = tconnect.پہلا;
            ثابت AstArg* ثابت argp = tconnect.دوجا;
            إذا (!argp) استمر;
            Ast_گرہ_اظہاریہ* ثابت pinp = argp->exprp();
            إذا (!pinp) استمر;
            VL_RESTORER(m_سیٹ_Ref_Lvalue);
            m_سیٹ_Ref_Lvalue = portp->direction().pinAccess();
            iterate(pinp);
        }
        إذا (nodep->withp()) iterate(nodep->withp());
    }
    فراغ ویکھو(AstConstraint* nodep) override {
        VL_RESTORER(m_سیٹ_If_Rand);
        m_سیٹ_If_Rand = سچ;
        iterateChildren(nodep);
    }
    فراغ ویکھو(Ast_گرہ_FTask* nodep) override {
        VL_RESTORER(m_وچ_Func);
        m_وچ_Func = سچ;
        iterateChildren(nodep);
    }

    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    LinkLValueVisitor(Ast_گرہ* nodep, VAccess شروع)
        : m_سیٹ_Ref_Lvalue{شروع} {
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
فراغ V3LinkLValue::link_LValue_سیٹ(Ast_گرہ* ثابت nodep, ثابت VAccess access) {
    // Called by later link functions when it is known a node needs
    // to be converted to a lvalue.
    UINFO(9, __FUNCTION__ << ": ");
    { LinkLValueVisitor{nodep, access}; }
}
