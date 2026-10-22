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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3LinkLValue.h"

#ಅನ್ತರ್ಭಾವ "V3Task.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

ವರ್ಗ LinkLValueVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    ತಾರ್ಕಿಕ m_ಗಣ_Continuously = ಸುಳ್ಳು;  // Set that var has some continuous assignment
    ತಾರ್ಕಿಕ m_ಗಣ_Forced_By_ಸಂಕೇತ = ಸುಳ್ಳು;  // Set that var is the target of an AstAssignForce/AstRelease
    ತಾರ್ಕಿಕ m_ಗಣ_If_Rand = ಸುಳ್ಳು;  // Update VarRefs if var declared as rand
    ತಾರ್ಕಿಕ m_ಗಣ_Strength_Specified = ಸುಳ್ಳು;  // Set that var has assignment with strength specified.
    ತಾರ್ಕಿಕ m_ಒಳ_Func = ಸುಳ್ಳು;  // Set if inside AstNodeFTask
    ತಾರ್ಕಿಕ m_ಒಳ_Initial_Static = ಸುಳ್ಳು;  // Set if inside AstInitialStatic
    ತಾರ್ಕಿಕ m_ಒಳ_Initial_Static_Stmt = ಸುಳ್ಳು;  // Set if inside AstInitialStaticStmt
    VAccess m_ಗಣ_Ref_Lvalue;  // Set VarRefs to lvalues for pin assignments

    // VISITORS
    // Result handing
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Var_Ref* nodep) override {
        // VarRef: LValue its reference
        ಆದರೆ (m_ಗಣ_If_Rand && !(nodep->varp() && nodep->varp()->isRand())) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (m_ಗಣ_Ref_Lvalue != VAccess::NOCHANGE) nodep->access(m_ಗಣ_Ref_Lvalue);
        // Linked via an input-only modport by V3LinkDot, before it was known if written
        ಆದರೆ (AstVarXRef* ಸ್ಥಿರಾಂಕ xrefp = VN_CAST(nodep, VarXRef)) {
            ಆದರೆ (xrefp->readOnlyModport()) {
                ಆದರೆ (nodep->access().isWriteOrRW()) {
                    nodep->v3error(
                        "Attempt to drive input-only modport: " << nodep->prettyNameQ());
                }
                xrefp->readOnlyModport(ಸುಳ್ಳು);
            }
        }
        ಆದರೆ (nodep->varp() && nodep->access().isWriteOrRW()) {
            ಆದರೆ (nodep->varp()->isParam()) {
                // All parameters that did get constified happened before now
                // as V3LinkLValue runs after V3Param
                nodep->v3error("Storing to parameter variable "
                               << nodep->prettyNameQ()
                               << " in a context that is determined only at runtime");
            }
            ಆದರೆ (m_ಗಣ_Continuously) {
                nodep->varp()->isContinuously(ನಿಜ);
                // Strength may only be specified in continuous assignment,
                // so it is needed to check only if m_setContinuously is true
                ಆದರೆ (m_ಗಣ_Strength_Specified) nodep->varp()->hasStrengthAssignment(ನಿಜ);
            }
            ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_Clocking_ಅಂಶ* ಸ್ಥಿರಾಂಕ itemp
                = VN_CAST(nodep->varp()->backp(), Clocking_ಅಂಶ)) {
                UINFO(5, "ClkOut " << nodep);
                ಆದರೆ (itemp->outputp()) nodep->varp(itemp->outputp()->varp());
            }
            ಆದರೆ (m_ಗಣ_Forced_By_ಸಂಕೇತ) {
                nodep->varp()->ಗಣ_Forced_By_ಸಂಕೇತ();
                // If a public signal is being forced in SystemVerilog and VPI
                // is enabled, mark it as forceable to ensure that the VPI
                // functions read the forced value correctly
                ಆದರೆ (v3Global.opt.vpi()
                    && (nodep->varp()->is_Sig_ಬಹಿರಂಗ() || nodep->varp()->is_Sig_Mod_ಬಹಿರಂಗ())) {
                    nodep->varp()->ಗಣ_Forceable();
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!nodep->varp()->isFuncLocal() && nodep->varp()->isReadOnly()) {
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
    ಶೂನ್ಯ ಭೇಟಿ(AstPin* nodep) override {
        VL_RESTORER(m_ಗಣ_Ref_Lvalue);
        ಆದರೆ (nodep->modVarp() && nodep->modVarp()->isWritable()) {
            // When the varref's were created, we didn't know the I/O state
            // Now that we do, and it's from a output, we know it's a lvalue
            m_ಗಣ_Ref_Lvalue = VAccess::WRITE;
            iterateChildren(nodep);
            m_ಗಣ_Ref_Lvalue = VAccess::NOCHANGE;
        } ಇಲ್ಲದಿದ್ದರೆ {
            iterateChildren(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) override {
        VL_RESTORER(m_ಗಣ_Ref_Lvalue);
        VL_RESTORER(m_ಗಣ_Continuously);
        VL_RESTORER(m_ಗಣ_Strength_Specified);
        {
            m_ಗಣ_Ref_Lvalue = VAccess::WRITE;
            m_ಗಣ_Continuously = VN_IS(nodep, ನಿಯೋಜಿಸು_W);
            ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ assignwp = VN_CAST(nodep, ನಿಯೋಜಿಸು_W)) {
                ಆದರೆ (assignwp->strengthSpecp()) m_ಗಣ_Strength_Specified = ನಿಜ;
            }
            {
                VL_RESTORER(m_ಗಣ_Forced_By_ಸಂಕೇತ);
                m_ಗಣ_Forced_By_ಸಂಕೇತ = VN_IS(nodep, ನಿಯೋಜಿಸು_Force);
                iterate_And_ಮುಂದಿನ_Null(nodep->lhsp());
            }
            m_ಗಣ_Ref_Lvalue = VAccess::NOCHANGE;
            m_ಗಣ_Continuously = ಸುಳ್ಳು;
            m_ಗಣ_Strength_Specified = ಸುಳ್ಳು;
            iterate_And_ಮುಂದಿನ_Null(nodep->rhsp());
        }

        ಆದರೆ ((m_ಒಳ_Initial_Static || m_ಒಳ_Initial_Static_Stmt) && m_ಒಳ_Func) {
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ rhsHasIO = nodep->rhsp()->exists([](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ refp) {
                // Exclude module I/O referenced from a function/task.
                ಹಿಂತಿರುಗಿ refp->varp() && refp->varp()->isIO()
                       && refp->varp()->lifetime() != VLifetime::NONE;
            });
            ಆದರೆ (rhsHasIO) {
                nodep->rhsp()->v3warn(E_UNSUPPORTED,
                                      "Static variable initializer\n"
                                          << nodep->rhsp()->warn_ಇನ್ನಷ್ಟು()
                                          << "is dependent on function/task I/O variable");
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ rhsHasAutomatic
                    = nodep->rhsp()->exists([](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ refp) {
                          ಹಿಂತಿರುಗಿ refp->varp() && refp->varp()->lifetime().isAutomatic();
                      });
                ಆದರೆ (rhsHasAutomatic) {
                    nodep->rhsp()->v3error("Static variable initializer\n"
                                           << nodep->rhsp()->warn_ಇನ್ನಷ್ಟು()
                                           << "is dependent on automatic variable");
                }
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAlias* nodep) override {
        VL_RESTORER(m_ಗಣ_Ref_Lvalue);
        m_ಗಣ_Ref_Lvalue = VAccess::READWRITE;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstInitialStatic* nodep) override {
        VL_RESTORER(m_ಒಳ_Initial_Static);
        m_ಒಳ_Initial_Static = ನಿಜ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_ಒಳ_Initial_Static_Stmt);
        m_ಒಳ_Initial_Static_Stmt = ನಿಜ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRelease* nodep) override {
        VL_RESTORER(m_ಗಣ_Ref_Lvalue);
        VL_RESTORER(m_ಗಣ_Continuously);
        VL_RESTORER(m_ಗಣ_Forced_By_ಸಂಕೇತ);
        m_ಗಣ_Ref_Lvalue = VAccess::WRITE;
        m_ಗಣ_Continuously = ಸುಳ್ಳು;
        m_ಗಣ_Forced_By_ಸಂಕೇತ = ನಿಜ;
        iterate_And_ಮುಂದಿನ_Null(nodep->lhsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDeassign* nodep) override {
        VL_RESTORER(m_ಗಣ_Ref_Lvalue);
        VL_RESTORER(m_ಗಣ_Continuously);
        m_ಗಣ_Ref_Lvalue = VAccess::WRITE;
        m_ಗಣ_Continuously = ಸುಳ್ಳು;
        iterate_And_ಮುಂದಿನ_Null(nodep->lhsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFireEvent* nodep) override {
        VL_RESTORER(m_ಗಣ_Ref_Lvalue);
        m_ಗಣ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ಮುಂದಿನ_Null(nodep->operandp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCastDynamic* nodep) override {
        VL_RESTORER(m_ಗಣ_Ref_Lvalue);
        m_ಗಣ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_ಮುಂದಿನ_Null(nodep->fromp());
        m_ಗಣ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ಮುಂದಿನ_Null(nodep->ತುದಿ());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFError* nodep) override {
        VL_RESTORER(m_ಗಣ_Ref_Lvalue);
        iterate_And_ಮುಂದಿನ_Null(nodep->filep());
        m_ಗಣ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ಮುಂದಿನ_Null(nodep->strp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFGetS* nodep) override {
        VL_RESTORER(m_ಗಣ_Ref_Lvalue);
        iterate_And_ಮುಂದಿನ_Null(nodep->filep());
        m_ಗಣ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ಮುಂದಿನ_Null(nodep->strgp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFRead* nodep) override {
        VL_RESTORER(m_ಗಣ_Ref_Lvalue);
        iterate_And_ಮುಂದಿನ_Null(nodep->filep());
        m_ಗಣ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ಮುಂದಿನ_Null(nodep->memp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFScanF* nodep) override {
        VL_RESTORER(m_ಗಣ_Ref_Lvalue);
        iterate_And_ಮುಂದಿನ_Null(nodep->filep());
        m_ಗಣ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ಮುಂದಿನ_Null(nodep->exprsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFUngetC* nodep) override {
        VL_RESTORER(m_ಗಣ_Ref_Lvalue);
        iterate_And_ಮುಂದಿನ_Null(nodep->filep());
        m_ಗಣ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ಮುಂದಿನ_Null(nodep->rhsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSScanF* nodep) override {
        VL_RESTORER(m_ಗಣ_Ref_Lvalue);
        m_ಗಣ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ಮುಂದಿನ_Null(nodep->exprsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSysIgnore* nodep) override {
        // Can't know if lvalue or not; presume not
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRand* nodep) override {
        VL_RESTORER(m_ಗಣ_Ref_Lvalue);
        ಆದರೆ (!nodep->urandom()) m_ಗಣ_Ref_Lvalue = VAccess::READWRITE;
        iterate_And_ಮುಂದಿನ_Null(nodep->seedp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstReadMem* nodep) override {
        VL_RESTORER(m_ಗಣ_Ref_Lvalue);
        m_ಗಣ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ಮುಂದಿನ_Null(nodep->memp());
        m_ಗಣ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_ಮುಂದಿನ_Null(nodep->filenamep());
        iterate_And_ಮುಂದಿನ_Null(nodep->lsbp());
        iterate_And_ಮುಂದಿನ_Null(nodep->msbp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Test_Plus_ವಾದಗಳು* nodep) override {
        VL_RESTORER(m_ಗಣ_Ref_Lvalue);
        m_ಗಣ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_ಮುಂದಿನ_Null(nodep->searchp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಮೌಲ್ಯ_Plus_ವಾದಗಳು* nodep) override {
        VL_RESTORER(m_ಗಣ_Ref_Lvalue);
        m_ಗಣ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_ಮುಂದಿನ_Null(nodep->searchp());
        m_ಗಣ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ಮುಂದಿನ_Null(nodep->outp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSFormat* nodep) override {
        VL_RESTORER(m_ಗಣ_Ref_Lvalue);
        m_ಗಣ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ಮುಂದಿನ_Null(nodep->lhsp());
        m_ಗಣ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_ಮುಂದಿನ_Null(nodep->fmtp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Dist_Biop* nodep) override {
        VL_RESTORER(m_ಗಣ_Ref_Lvalue);
        m_ಗಣ_Ref_Lvalue = VAccess::READWRITE;
        iterate_And_ಮುಂದಿನ_Null(nodep->lhsp());
        m_ಗಣ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_ಮುಂದಿನ_Null(nodep->rhsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Dist_Triop* nodep) override {
        VL_RESTORER(m_ಗಣ_Ref_Lvalue);
        m_ಗಣ_Ref_Lvalue = VAccess::READWRITE;
        iterate_And_ಮುಂದಿನ_Null(nodep->lhsp());
        m_ಗಣ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_ಮುಂದಿನ_Null(nodep->rhsp());
        iterate_And_ಮುಂದಿನ_Null(nodep->thsp());
    }
    // cppcheck-suppress constParameterPointer
    ಶೂನ್ಯ prepost_ಭೇಟಿ(Ast_ಗಂಟು_Uniop* nodep) {
        VL_RESTORER(m_ಗಣ_Ref_Lvalue);
        m_ಗಣ_Ref_Lvalue = VAccess::WRITE;
        iterate_And_ಮುಂದಿನ_Null(nodep->lhsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPreInc* nodep) override { prepost_ಭೇಟಿ(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstPostInc* nodep) override { prepost_ಭೇಟಿ(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstPreDec* nodep) override { prepost_ಭೇಟಿ(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstPostDec* nodep) override { prepost_ಭೇಟಿ(nodep); }

    // Nodes that change LValue state
    ಶೂನ್ಯ ಭೇಟಿ(AstSel* nodep) override {
        VL_RESTORER(m_ಗಣ_Ref_Lvalue);
        iterate_And_ಮುಂದಿನ_Null(nodep->fromp());
        // Only set lvalues on the from
        m_ಗಣ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_ಮುಂದಿನ_Null(nodep->lsbp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Sel* nodep) override {
        VL_RESTORER(m_ಗಣ_Ref_Lvalue);
        // Only set lvalues on the from
        iterate_And_ಮುಂದಿನ_Null(nodep->fromp());
        m_ಗಣ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_ಮುಂದಿನ_Null(nodep->bitp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCellArrayRef* nodep) override {
        VL_RESTORER(m_ಗಣ_Ref_Lvalue);
        // selp is not an lvalue
        m_ಗಣ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_ಮುಂದಿನ_Null(nodep->selp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Pre_Sel* nodep) override {
        ಆದರೆ (Ast_Sel_ಬಿಟ್* ಸ್ಥಿರಾಂಕ selbitp = VN_CAST(nodep, Sel_ಬಿಟ್)) selbitp->access(m_ಗಣ_Ref_Lvalue);
        VL_RESTORER(m_ಗಣ_Ref_Lvalue);
        // Only set lvalues on the from
        iterate_And_ಮುಂದಿನ_Null(nodep->fromp());
        m_ಗಣ_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_ಮುಂದಿನ_Null(nodep->rhsp());
        iterate_And_ಮುಂದಿನ_Null(nodep->thsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstMemberSel* nodep) override {
        ಆದರೆ (m_ಗಣ_Ref_Lvalue != VAccess::NOCHANGE) {
            nodep->access(m_ಗಣ_Ref_Lvalue);
            ಆದರೆ (nodep->varp() && nodep->access().isWriteOrRW()) {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_Clocking_ಅಂಶ* ಸ್ಥಿರಾಂಕ itemp
                    = VN_CAST(nodep->varp()->backp(), Clocking_ಅಂಶ)) {
                    UINFO(5, "ClkOut " << nodep);
                    ಆದರೆ (itemp->outputp()) nodep->varp(itemp->outputp()->varp());
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            // It is the only place where the access is set to member select nodes.
            // If it doesn't have to be set to WRITE, it means that it is READ.
            nodep->access(VAccess::READ);
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ taskp = nodep->taskp();
        // We'll deal with mismatching pins later
        ಆದರೆ (!taskp) ಹಿಂತಿರುಗಿ;
        ಸ್ಥಿರಾಂಕ V3TaskConnects tconnects
            = V3Task::taskConnects(nodep, taskp->stmtsp(), ಶೂನ್ಯಸೂಚಕ, ಸುಳ್ಳು);
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& tconnect : tconnects) {
            ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ portp = tconnect.ಮೊದಲ;
            ಸ್ಥಿರಾಂಕ AstArg* ಸ್ಥಿರಾಂಕ argp = tconnect.ಎರಡನೇ;
            ಆದರೆ (!argp) ಮುಂದುವರಿ;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ pinp = argp->exprp();
            ಆದರೆ (!pinp) ಮುಂದುವರಿ;
            VL_RESTORER(m_ಗಣ_Ref_Lvalue);
            m_ಗಣ_Ref_Lvalue = portp->direction().pinAccess();
            iterate(pinp);
        }
        ಆದರೆ (nodep->withp()) iterate(nodep->withp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConstraint* nodep) override {
        VL_RESTORER(m_ಗಣ_If_Rand);
        m_ಗಣ_If_Rand = ನಿಜ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask* nodep) override {
        VL_RESTORER(m_ಒಳ_Func);
        m_ಒಳ_Func = ನಿಜ;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    LinkLValueVisitor(Ast_ಗಂಟು* nodep, VAccess ಆರಂಭ)
        : m_ಗಣ_Ref_Lvalue{ಆರಂಭ} {
        iterate(nodep);
    }
    ~LinkLValueVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Link class functions

ಶೂನ್ಯ V3LinkLValue::linkLValue(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    { LinkLValueVisitor{nodep, VAccess::NOCHANGE}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("linklvalue", 0, dump_ವೃಕ್ಷ_Either_Level() >= 6);
}
ಶೂನ್ಯ V3LinkLValue::link_LValue_ಗಣ(Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep, ಸ್ಥಿರಾಂಕ VAccess access) {
    // Called by later link functions when it is known a node needs
    // to be converted to a lvalue.
    UINFO(9, __FUNCTION__ << ": ");
    { LinkLValueVisitor{nodep, access}; }
}
