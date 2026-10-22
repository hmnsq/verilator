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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3LinkLValue.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Task.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

ਵਰਗ LinkLValueVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    ਤਾਰਕਿਕ m_ਸੈੱਟ_Continuously = ਝੂਠ;  // Set that var has some continuous assignment
    ਤਾਰਕਿਕ m_ਸੈੱਟ_Forced_By_ਕੋਡ = ਝੂਠ;  // Set that var is the target of an AstAssignForce/AstRelease
    ਤਾਰਕਿਕ m_ਸੈੱਟ_If_Rand = ਝੂਠ;  // Update VarRefs if var declared as rand
    ਤਾਰਕਿਕ m_ਸੈੱਟ_Strength_Specified = ਝੂਠ;  // Set that var has assignment with strength specified.
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Func = ਝੂਠ;  // Set if inside AstNodeFTask
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Initial_Static = ਝੂਠ;  // Set if inside AstInitialStatic
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Initial_Static_Stmt = ਝੂਠ;  // Set if inside AstInitialStaticStmt
    VAccess m_ਸੈੱਟ_Ref_Lvalue;  // Set VarRefs to lvalues for pin assignments

    // VISITORS
    // Result handing
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Var_Ref* nodep) override {
        // VarRef: LValue its reference
        ਜੇ (m_ਸੈੱਟ_If_Rand && !(nodep->varp() && nodep->varp()->isRand())) ਵਾਪਸ;
        ਜੇ (m_ਸੈੱਟ_Ref_Lvalue != VAccess::NOCHANGE) nodep->access(m_ਸੈੱਟ_Ref_Lvalue);
        // Linked via an input-only modport by V3LinkDot, before it was known if written
        ਜੇ (AstVarXRef* ਅਚਲ xrefp = VN_CAST(nodep, VarXRef)) {
            ਜੇ (xrefp->readOnlyModport()) {
                ਜੇ (nodep->access().isWriteOrRW()) {
                    nodep->v3error(
                        "Attempt to drive input-only modport: " << nodep->prettyNameQ());
                }
                xrefp->readOnlyModport(ਝੂਠ);
            }
        }
        ਜੇ (nodep->varp() && nodep->access().isWriteOrRW()) {
            ਜੇ (nodep->varp()->isParam()) {
                // All parameters that did get constified happened before now
                // as V3LinkLValue runs after V3Param
                nodep->v3error("Storing to parameter variable "
                               << nodep->prettyNameQ()
                               << " in a context that is determined only at runtime");
            }
            ਜੇ (m_ਸੈੱਟ_Continuously) {
                nodep->varp()->isContinuously(ਸੱਚ);
                // Strength may only be specified in continuous assignment,
                // so it is needed to check only if m_setContinuously is true
                ਜੇ (m_ਸੈੱਟ_Strength_Specified) nodep->varp()->hasStrengthAssignment(ਸੱਚ);
            }
            ਜੇ (ਅਚਲ Ast_Clocking_ਵਸਤੂ* ਅਚਲ itemp
                = VN_CAST(nodep->varp()->backp(), Clocking_ਵਸਤੂ)) {
                UINFO(5, "ClkOut " << nodep);
                ਜੇ (itemp->outputp()) nodep->varp(itemp->outputp()->varp());
            }
            ਜੇ (m_ਸੈੱਟ_Forced_By_ਕੋਡ) {
                nodep->varp()->ਸੈੱਟ_Forced_By_ਕੋਡ();
                // If a public signal is being forced in SystemVerilog and VPI
                // is enabled, mark it as forceable to ensure that the VPI
                // functions read the forced value correctly
                ਜੇ (v3Global.opt.vpi()
                    && (nodep->varp()->is_Sig_ਜਨਤਕ() || nodep->varp()->is_Sig_Mod_ਜਨਤਕ())) {
                    nodep->varp()->ਸੈੱਟ_Forceable();
                }
            } ਨਹੀਂਤਾਂ ਜੇ (!nodep->varp()->isFuncLocal() && nodep->varp()->isReadOnly()) {
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
    ਖਾਲੀ ਦੇਖੋ(AstPin* nodep) override {
        VL_RESTORER(m_ਸੈੱਟ_Ref_Lvalue);
        ਜੇ (nodep->modVarp() && nodep->modVarp()->isWritable()) {
            // When the varref's were created, we didn't know the I/O state
            // Now that we do, and it's from a output, we know it's a lvalue
            m_ਸੈੱਟ_Ref_Lvalue = VAccess::WRITE;
            iterateChildren(nodep);
            m_ਸੈੱਟ_Ref_Lvalue = VAccess::NOCHANGE;
        } ਨਹੀਂਤਾਂ {
            iterateChildren(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) override {
        VL_RESTORER(m_ਸੈੱਟ_Ref_Lvalue);
        VL_RESTORER(m_ਸੈੱਟ_Continuously);
        VL_RESTORER(m_ਸੈੱਟ_Strength_Specified);
        {
            m_ਸੈੱਟ_Ref_Lvalue = VAccess::WRITE;
            m_ਸੈੱਟ_Continuously = VN_IS(nodep, ਸੌਂਪੋ_W);
            ਜੇ (ਅਚਲ Ast_ਸੌਂਪੋ_W* ਅਚਲ assignwp = VN_CAST(nodep, ਸੌਂਪੋ_W)) {
                ਜੇ (assignwp->strengthSpecp()) m_ਸੈੱਟ_Strength_Specified = ਸੱਚ;
            }
            {
                VL_RESTORER(m_ਸੈੱਟ_Forced_By_ਕੋਡ);
                m_ਸੈੱਟ_Forced_By_ਕੋਡ = VN_IS(nodep, ਸੌਂਪੋ_Force);
                iterate_And_ਅਗਲਾ_Null(nodep->lhsp());
            }
            m_ਸੈੱਟ_Ref_Lvalue = VAccess::NOCHANGE;
            m_ਸੈੱਟ_Continuously = ਝੂਠ;
            m_ਸੈੱਟ_Strength_Specified = ਝੂਠ;
            iterate_And_ਅਗਲਾ_Null(nodep->rhsp());
        }

        ਜੇ ((m_ਵਿੱਚ_Initial_Static || m_ਵਿੱਚ_Initial_Static_Stmt) && m_ਵਿੱਚ_Func) {
            ਅਚਲ ਤਾਰਕਿਕ rhsHasIO = nodep->rhsp()->exists([](ਅਚਲ Ast_ਗੰਢ_Var_Ref* ਅਚਲ refp) {
                // Exclude module I/O referenced from a function/task.
                ਵਾਪਸ refp->varp() && refp->varp()->isIO()
                       && refp->varp()->lifetime() != VLifetime::NONE;
            });
            ਜੇ (rhsHasIO) {
                nodep->rhsp()->v3warn(E_UNSUPPORTED,
                                      "Static variable initializer\n"
                                          << nodep->rhsp()->warn_ਹੋਰ()
                                          << "is dependent on function/task I/O variable");
            } ਨਹੀਂਤਾਂ {
                ਅਚਲ ਤਾਰਕਿਕ rhsHasAutomatic
                    = nodep->rhsp()->exists([](ਅਚਲ Ast_ਗੰਢ_Var_Ref* ਅਚਲ refp) {
                          ਵਾਪਸ refp->varp() && refp->varp()->lifetime().isAutomatic();
                      });
                ਜੇ (rhsHasAutomatic) {
                    nodep->rhsp()->v3error("Static variable initializer\n"
                                           << nodep->rhsp()->warn_ਹੋਰ()
                                           << "is dependent on automatic variable");
                }
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstAlias* nodep) override {
        VL_RESTORER(m_ਸੈੱਟ_Ref_Lvalue);
        m_ਸੈੱਟ_Ref_Lvalue = VAccess::READWRITE;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstInitialStatic* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_Initial_Static);
        m_ਵਿੱਚ_Initial_Static = ਸੱਚ;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_Initial_Static_Stmt);
        m_ਵਿੱਚ_Initial_Static_Stmt = ਸੱਚ;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstRelease* nodep) override {
        VL_RESTORER(m_ਸੈੱਟ_Ref_Lvalue);
        VL_RESTORER(m_ਸੈੱਟ_Continuously);
        VL_RESTORER(m_ਸੈੱਟ_Forced_By_ਕੋਡ);
        m_ਸੈੱਟ_Ref_Lvalue = VAccess::WRITE;
        m_ਸੈੱਟ_Continuously = ਝੂਠ;
        m_ਸੈੱਟ_Forced_By_ਕੋਡ = ਸੱਚ;
        iterate_And_ਅਗਲਾ_Null(nodep->lhsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstDeassign* nodep) override {
        VL_RESTORER(m_ਸੈੱਟ_Ref_Lvalue);
        VL_RESTORER(m_ਸੈੱਟ_Continuously);
        m_ਸੈੱਟ_Ref_Lvalue = VAccess::WRITE;
        m_ਸੈੱਟ_Continuously = ਝੂਠ;
        iterate_And_ਅਗਲਾ_Null(nodep->lhsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstFireEvent* nodep) override {
        VL_RESTORER(m_ਸੈੱਟ_Ref_Lvalue);
        m_ਸੈੱਟ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ਅਗਲਾ_Null(nodep->operandp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCastDynamic* nodep) override {
        VL_RESTORER(m_ਸੈੱਟ_Ref_Lvalue);
        m_ਸੈੱਟ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_ਅਗਲਾ_Null(nodep->fromp());
        m_ਸੈੱਟ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ਅਗਲਾ_Null(nodep->ਸਿਖਰ());
    }
    ਖਾਲੀ ਦੇਖੋ(AstFError* nodep) override {
        VL_RESTORER(m_ਸੈੱਟ_Ref_Lvalue);
        iterate_And_ਅਗਲਾ_Null(nodep->filep());
        m_ਸੈੱਟ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ਅਗਲਾ_Null(nodep->strp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstFGetS* nodep) override {
        VL_RESTORER(m_ਸੈੱਟ_Ref_Lvalue);
        iterate_And_ਅਗਲਾ_Null(nodep->filep());
        m_ਸੈੱਟ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ਅਗਲਾ_Null(nodep->strgp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstFRead* nodep) override {
        VL_RESTORER(m_ਸੈੱਟ_Ref_Lvalue);
        iterate_And_ਅਗਲਾ_Null(nodep->filep());
        m_ਸੈੱਟ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ਅਗਲਾ_Null(nodep->memp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstFScanF* nodep) override {
        VL_RESTORER(m_ਸੈੱਟ_Ref_Lvalue);
        iterate_And_ਅਗਲਾ_Null(nodep->filep());
        m_ਸੈੱਟ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ਅਗਲਾ_Null(nodep->exprsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstFUngetC* nodep) override {
        VL_RESTORER(m_ਸੈੱਟ_Ref_Lvalue);
        iterate_And_ਅਗਲਾ_Null(nodep->filep());
        m_ਸੈੱਟ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ਅਗਲਾ_Null(nodep->rhsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstSScanF* nodep) override {
        VL_RESTORER(m_ਸੈੱਟ_Ref_Lvalue);
        m_ਸੈੱਟ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ਅਗਲਾ_Null(nodep->exprsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstSysIgnore* nodep) override {
        // Can't know if lvalue or not; presume not
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstRand* nodep) override {
        VL_RESTORER(m_ਸੈੱਟ_Ref_Lvalue);
        ਜੇ (!nodep->urandom()) m_ਸੈੱਟ_Ref_Lvalue = VAccess::READWRITE;
        iterate_And_ਅਗਲਾ_Null(nodep->seedp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstReadMem* nodep) override {
        VL_RESTORER(m_ਸੈੱਟ_Ref_Lvalue);
        m_ਸੈੱਟ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ਅਗਲਾ_Null(nodep->memp());
        m_ਸੈੱਟ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_ਅਗਲਾ_Null(nodep->filenamep());
        iterate_And_ਅਗਲਾ_Null(nodep->lsbp());
        iterate_And_ਅਗਲਾ_Null(nodep->msbp());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Test_Plus_ਦਲੀਲਾਂ* nodep) override {
        VL_RESTORER(m_ਸੈੱਟ_Ref_Lvalue);
        m_ਸੈੱਟ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_ਅਗਲਾ_Null(nodep->searchp());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਮੁੱਲ_Plus_ਦਲੀਲਾਂ* nodep) override {
        VL_RESTORER(m_ਸੈੱਟ_Ref_Lvalue);
        m_ਸੈੱਟ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_ਅਗਲਾ_Null(nodep->searchp());
        m_ਸੈੱਟ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ਅਗਲਾ_Null(nodep->outp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstSFormat* nodep) override {
        VL_RESTORER(m_ਸੈੱਟ_Ref_Lvalue);
        m_ਸੈੱਟ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ਅਗਲਾ_Null(nodep->lhsp());
        m_ਸੈੱਟ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_ਅਗਲਾ_Null(nodep->fmtp());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Dist_Biop* nodep) override {
        VL_RESTORER(m_ਸੈੱਟ_Ref_Lvalue);
        m_ਸੈੱਟ_Ref_Lvalue = VAccess::READWRITE;
        iterate_And_ਅਗਲਾ_Null(nodep->lhsp());
        m_ਸੈੱਟ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_ਅਗਲਾ_Null(nodep->rhsp());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Dist_Triop* nodep) override {
        VL_RESTORER(m_ਸੈੱਟ_Ref_Lvalue);
        m_ਸੈੱਟ_Ref_Lvalue = VAccess::READWRITE;
        iterate_And_ਅਗਲਾ_Null(nodep->lhsp());
        m_ਸੈੱਟ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_ਅਗਲਾ_Null(nodep->rhsp());
        iterate_And_ਅਗਲਾ_Null(nodep->thsp());
    }
    // cppcheck-suppress constParameterPointer
    ਖਾਲੀ prepost_ਦੇਖੋ(Ast_ਗੰਢ_Uniop* nodep) {
        VL_RESTORER(m_ਸੈੱਟ_Ref_Lvalue);
        m_ਸੈੱਟ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ਅਗਲਾ_Null(nodep->lhsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstPreInc* nodep) override { prepost_ਦੇਖੋ(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstPostInc* nodep) override { prepost_ਦੇਖੋ(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstPreDec* nodep) override { prepost_ਦੇਖੋ(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstPostDec* nodep) override { prepost_ਦੇਖੋ(nodep); }

    // Nodes that change LValue state
    ਖਾਲੀ ਦੇਖੋ(AstSel* nodep) override {
        VL_RESTORER(m_ਸੈੱਟ_Ref_Lvalue);
        iterate_And_ਅਗਲਾ_Null(nodep->fromp());
        // Only set lvalues on the from
        m_ਸੈੱਟ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_ਅਗਲਾ_Null(nodep->lsbp());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Sel* nodep) override {
        VL_RESTORER(m_ਸੈੱਟ_Ref_Lvalue);
        // Only set lvalues on the from
        iterate_And_ਅਗਲਾ_Null(nodep->fromp());
        m_ਸੈੱਟ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_ਅਗਲਾ_Null(nodep->bitp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCellArrayRef* nodep) override {
        VL_RESTORER(m_ਸੈੱਟ_Ref_Lvalue);
        // selp is not an lvalue
        m_ਸੈੱਟ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_ਅਗਲਾ_Null(nodep->selp());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Pre_Sel* nodep) override {
        ਜੇ (Ast_Sel_ਦੋਆਧਾਰੀ_ਅੰਕ* ਅਚਲ selbitp = VN_CAST(nodep, Sel_ਦੋਆਧਾਰੀ_ਅੰਕ)) selbitp->access(m_ਸੈੱਟ_Ref_Lvalue);
        VL_RESTORER(m_ਸੈੱਟ_Ref_Lvalue);
        // Only set lvalues on the from
        iterate_And_ਅਗਲਾ_Null(nodep->fromp());
        m_ਸੈੱਟ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_ਅਗਲਾ_Null(nodep->rhsp());
        iterate_And_ਅਗਲਾ_Null(nodep->thsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstMemberSel* nodep) override {
        ਜੇ (m_ਸੈੱਟ_Ref_Lvalue != VAccess::NOCHANGE) {
            nodep->access(m_ਸੈੱਟ_Ref_Lvalue);
            ਜੇ (nodep->varp() && nodep->access().isWriteOrRW()) {
                ਜੇ (ਅਚਲ Ast_Clocking_ਵਸਤੂ* ਅਚਲ itemp
                    = VN_CAST(nodep->varp()->backp(), Clocking_ਵਸਤੂ)) {
                    UINFO(5, "ClkOut " << nodep);
                    ਜੇ (itemp->outputp()) nodep->varp(itemp->outputp()->varp());
                }
            }
        } ਨਹੀਂਤਾਂ {
            // It is the only place where the access is set to member select nodes.
            // If it doesn't have to be set to WRITE, it means that it is READ.
            nodep->access(VAccess::READ);
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask_Ref* nodep) override {
        ਅਚਲ Ast_ਗੰਢ_FTask* ਅਚਲ taskp = nodep->taskp();
        // We'll deal with mismatching pins later
        ਜੇ (!taskp) ਵਾਪਸ;
        ਅਚਲ V3TaskConnects tconnects
            = V3Task::taskConnects(nodep, taskp->stmtsp(), ਖਾਲੀ_ਸੂਚਕ, ਝੂਠ);
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& tconnect : tconnects) {
            ਅਚਲ AstVar* ਅਚਲ portp = tconnect.ਪਹਿਲਾ;
            ਅਚਲ AstArg* ਅਚਲ argp = tconnect.ਦੂਜਾ;
            ਜੇ (!argp) ਜਾਰੀ;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ pinp = argp->exprp();
            ਜੇ (!pinp) ਜਾਰੀ;
            VL_RESTORER(m_ਸੈੱਟ_Ref_Lvalue);
            m_ਸੈੱਟ_Ref_Lvalue = portp->direction().pinAccess();
            iterate(pinp);
        }
        ਜੇ (nodep->withp()) iterate(nodep->withp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstConstraint* nodep) override {
        VL_RESTORER(m_ਸੈੱਟ_If_Rand);
        m_ਸੈੱਟ_If_Rand = ਸੱਚ;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_Func);
        m_ਵਿੱਚ_Func = ਸੱਚ;
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    LinkLValueVisitor(Ast_ਗੰਢ* nodep, VAccess ਸ਼ੁਰੂ)
        : m_ਸੈੱਟ_Ref_Lvalue{ਸ਼ੁਰੂ} {
        iterate(nodep);
    }
    ~LinkLValueVisitor() override = ਮੂਲ;
};

//######################################################################
// Link class functions

ਖਾਲੀ V3LinkLValue::linkLValue(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    { LinkLValueVisitor{nodep, VAccess::NOCHANGE}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("linklvalue", 0, dump_ਰੁੱਖ_Either_Level() >= 6);
}
ਖਾਲੀ V3LinkLValue::link_LValue_ਸੈੱਟ(Ast_ਗੰਢ* ਅਚਲ nodep, ਅਚਲ VAccess access) {
    // Called by later link functions when it is known a node needs
    // to be converted to a lvalue.
    UINFO(9, __FUNCTION__ << ": ");
    { LinkLValueVisitor{nodep, access}; }
}
