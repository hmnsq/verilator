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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3LinkLValue.h"

#అన్తర్భావ "V3Task.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

తరగతి LinkLValueVisitor final : బహిరంగ VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    తార్కిక m_సమితి_Continuously = అబద్ధం;  // Set that var has some continuous assignment
    తార్కిక m_సమితి_Forced_By_సంకేతం = అబద్ధం;  // Set that var is the target of an AstAssignForce/AstRelease
    తార్కిక m_సమితి_If_Rand = అబద్ధం;  // Update VarRefs if var declared as rand
    తార్కిక m_సమితి_Strength_Specified = అబద్ధం;  // Set that var has assignment with strength specified.
    తార్కిక m_లోపల_Func = అబద్ధం;  // Set if inside AstNodeFTask
    తార్కిక m_లోపల_Initial_Static = అబద్ధం;  // Set if inside AstInitialStatic
    తార్కిక m_లోపల_Initial_Static_Stmt = అబద్ధం;  // Set if inside AstInitialStaticStmt
    VAccess m_సమితి_Ref_Lvalue;  // Set VarRefs to lvalues for pin assignments

    // VISITORS
    // Result handing
    శూన్య సందర్శించు(Ast_కణుపు_Var_Ref* nodep) override {
        // VarRef: LValue its reference
        ఒకవేళ (m_సమితి_If_Rand && !(nodep->varp() && nodep->varp()->isRand())) తిరిగి;
        ఒకవేళ (m_సమితి_Ref_Lvalue != VAccess::NOCHANGE) nodep->access(m_సమితి_Ref_Lvalue);
        // Linked via an input-only modport by V3LinkDot, before it was known if written
        ఒకవేళ (AstVarXRef* స్థిరరాశి xrefp = VN_CAST(nodep, VarXRef)) {
            ఒకవేళ (xrefp->readOnlyModport()) {
                ఒకవేళ (nodep->access().isWriteOrRW()) {
                    nodep->v3error(
                        "Attempt to drive input-only modport: " << nodep->prettyNameQ());
                }
                xrefp->readOnlyModport(అబద్ధం);
            }
        }
        ఒకవేళ (nodep->varp() && nodep->access().isWriteOrRW()) {
            ఒకవేళ (nodep->varp()->isParam()) {
                // All parameters that did get constified happened before now
                // as V3LinkLValue runs after V3Param
                nodep->v3error("Storing to parameter variable "
                               << nodep->prettyNameQ()
                               << " in a context that is determined only at runtime");
            }
            ఒకవేళ (m_సమితి_Continuously) {
                nodep->varp()->isContinuously(నిజం);
                // Strength may only be specified in continuous assignment,
                // so it is needed to check only if m_setContinuously is true
                ఒకవేళ (m_సమితి_Strength_Specified) nodep->varp()->hasStrengthAssignment(నిజం);
            }
            ఒకవేళ (స్థిరరాశి Ast_Clocking_అంశం* స్థిరరాశి itemp
                = VN_CAST(nodep->varp()->backp(), Clocking_అంశం)) {
                UINFO(5, "ClkOut " << nodep);
                ఒకవేళ (itemp->outputp()) nodep->varp(itemp->outputp()->varp());
            }
            ఒకవేళ (m_సమితి_Forced_By_సంకేతం) {
                nodep->varp()->సమితి_Forced_By_సంకేతం();
                // If a public signal is being forced in SystemVerilog and VPI
                // is enabled, mark it as forceable to ensure that the VPI
                // functions read the forced value correctly
                ఒకవేళ (v3Global.opt.vpi()
                    && (nodep->varp()->is_Sig_ప్రజా() || nodep->varp()->is_Sig_Mod_ప్రజా())) {
                    nodep->varp()->సమితి_Forceable();
                }
            } లేకపోతే ఒకవేళ (!nodep->varp()->isFuncLocal() && nodep->varp()->isReadOnly()) {
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
    శూన్య సందర్శించు(AstPin* nodep) override {
        VL_RESTORER(m_సమితి_Ref_Lvalue);
        ఒకవేళ (nodep->modVarp() && nodep->modVarp()->isWritable()) {
            // When the varref's were created, we didn't know the I/O state
            // Now that we do, and it's from a output, we know it's a lvalue
            m_సమితి_Ref_Lvalue = VAccess::WRITE;
            iterateChildren(nodep);
            m_సమితి_Ref_Lvalue = VAccess::NOCHANGE;
        } లేకపోతే {
            iterateChildren(nodep);
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_కేటాయించు* nodep) override {
        VL_RESTORER(m_సమితి_Ref_Lvalue);
        VL_RESTORER(m_సమితి_Continuously);
        VL_RESTORER(m_సమితి_Strength_Specified);
        {
            m_సమితి_Ref_Lvalue = VAccess::WRITE;
            m_సమితి_Continuously = VN_IS(nodep, కేటాయించు_W);
            ఒకవేళ (స్థిరరాశి Ast_కేటాయించు_W* స్థిరరాశి assignwp = VN_CAST(nodep, కేటాయించు_W)) {
                ఒకవేళ (assignwp->strengthSpecp()) m_సమితి_Strength_Specified = నిజం;
            }
            {
                VL_RESTORER(m_సమితి_Forced_By_సంకేతం);
                m_సమితి_Forced_By_సంకేతం = VN_IS(nodep, కేటాయించు_Force);
                iterate_And_తదుపరి_Null(nodep->lhsp());
            }
            m_సమితి_Ref_Lvalue = VAccess::NOCHANGE;
            m_సమితి_Continuously = అబద్ధం;
            m_సమితి_Strength_Specified = అబద్ధం;
            iterate_And_తదుపరి_Null(nodep->rhsp());
        }

        ఒకవేళ ((m_లోపల_Initial_Static || m_లోపల_Initial_Static_Stmt) && m_లోపల_Func) {
            స్థిరరాశి తార్కిక rhsHasIO = nodep->rhsp()->exists([](స్థిరరాశి Ast_కణుపు_Var_Ref* స్థిరరాశి refp) {
                // Exclude module I/O referenced from a function/task.
                తిరిగి refp->varp() && refp->varp()->isIO()
                       && refp->varp()->lifetime() != VLifetime::NONE;
            });
            ఒకవేళ (rhsHasIO) {
                nodep->rhsp()->v3warn(E_UNSUPPORTED,
                                      "Static variable initializer\n"
                                          << nodep->rhsp()->warn_మరిన్ని()
                                          << "is dependent on function/task I/O variable");
            } లేకపోతే {
                స్థిరరాశి తార్కిక rhsHasAutomatic
                    = nodep->rhsp()->exists([](స్థిరరాశి Ast_కణుపు_Var_Ref* స్థిరరాశి refp) {
                          తిరిగి refp->varp() && refp->varp()->lifetime().isAutomatic();
                      });
                ఒకవేళ (rhsHasAutomatic) {
                    nodep->rhsp()->v3error("Static variable initializer\n"
                                           << nodep->rhsp()->warn_మరిన్ని()
                                           << "is dependent on automatic variable");
                }
            }
        }
    }
    శూన్య సందర్శించు(AstAlias* nodep) override {
        VL_RESTORER(m_సమితి_Ref_Lvalue);
        m_సమితి_Ref_Lvalue = VAccess::READWRITE;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstInitialStatic* nodep) override {
        VL_RESTORER(m_లోపల_Initial_Static);
        m_లోపల_Initial_Static = నిజం;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_లోపల_Initial_Static_Stmt);
        m_లోపల_Initial_Static_Stmt = నిజం;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstRelease* nodep) override {
        VL_RESTORER(m_సమితి_Ref_Lvalue);
        VL_RESTORER(m_సమితి_Continuously);
        VL_RESTORER(m_సమితి_Forced_By_సంకేతం);
        m_సమితి_Ref_Lvalue = VAccess::WRITE;
        m_సమితి_Continuously = అబద్ధం;
        m_సమితి_Forced_By_సంకేతం = నిజం;
        iterate_And_తదుపరి_Null(nodep->lhsp());
    }
    శూన్య సందర్శించు(AstDeassign* nodep) override {
        VL_RESTORER(m_సమితి_Ref_Lvalue);
        VL_RESTORER(m_సమితి_Continuously);
        m_సమితి_Ref_Lvalue = VAccess::WRITE;
        m_సమితి_Continuously = అబద్ధం;
        iterate_And_తదుపరి_Null(nodep->lhsp());
    }
    శూన్య సందర్శించు(AstFireEvent* nodep) override {
        VL_RESTORER(m_సమితి_Ref_Lvalue);
        m_సమితి_Ref_Lvalue = VAccess::WRITE;
        iterate_And_తదుపరి_Null(nodep->operandp());
    }
    శూన్య సందర్శించు(AstCastDynamic* nodep) override {
        VL_RESTORER(m_సమితి_Ref_Lvalue);
        m_సమితి_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_తదుపరి_Null(nodep->fromp());
        m_సమితి_Ref_Lvalue = VAccess::WRITE;
        iterate_And_తదుపరి_Null(nodep->శిఖరం());
    }
    శూన్య సందర్శించు(AstFError* nodep) override {
        VL_RESTORER(m_సమితి_Ref_Lvalue);
        iterate_And_తదుపరి_Null(nodep->filep());
        m_సమితి_Ref_Lvalue = VAccess::WRITE;
        iterate_And_తదుపరి_Null(nodep->strp());
    }
    శూన్య సందర్శించు(AstFGetS* nodep) override {
        VL_RESTORER(m_సమితి_Ref_Lvalue);
        iterate_And_తదుపరి_Null(nodep->filep());
        m_సమితి_Ref_Lvalue = VAccess::WRITE;
        iterate_And_తదుపరి_Null(nodep->strgp());
    }
    శూన్య సందర్శించు(AstFRead* nodep) override {
        VL_RESTORER(m_సమితి_Ref_Lvalue);
        iterate_And_తదుపరి_Null(nodep->filep());
        m_సమితి_Ref_Lvalue = VAccess::WRITE;
        iterate_And_తదుపరి_Null(nodep->memp());
    }
    శూన్య సందర్శించు(AstFScanF* nodep) override {
        VL_RESTORER(m_సమితి_Ref_Lvalue);
        iterate_And_తదుపరి_Null(nodep->filep());
        m_సమితి_Ref_Lvalue = VAccess::WRITE;
        iterate_And_తదుపరి_Null(nodep->exprsp());
    }
    శూన్య సందర్శించు(AstFUngetC* nodep) override {
        VL_RESTORER(m_సమితి_Ref_Lvalue);
        iterate_And_తదుపరి_Null(nodep->filep());
        m_సమితి_Ref_Lvalue = VAccess::WRITE;
        iterate_And_తదుపరి_Null(nodep->rhsp());
    }
    శూన్య సందర్శించు(AstSScanF* nodep) override {
        VL_RESTORER(m_సమితి_Ref_Lvalue);
        m_సమితి_Ref_Lvalue = VAccess::WRITE;
        iterate_And_తదుపరి_Null(nodep->exprsp());
    }
    శూన్య సందర్శించు(AstSysIgnore* nodep) override {
        // Can't know if lvalue or not; presume not
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstRand* nodep) override {
        VL_RESTORER(m_సమితి_Ref_Lvalue);
        ఒకవేళ (!nodep->urandom()) m_సమితి_Ref_Lvalue = VAccess::READWRITE;
        iterate_And_తదుపరి_Null(nodep->seedp());
    }
    శూన్య సందర్శించు(AstReadMem* nodep) override {
        VL_RESTORER(m_సమితి_Ref_Lvalue);
        m_సమితి_Ref_Lvalue = VAccess::WRITE;
        iterate_And_తదుపరి_Null(nodep->memp());
        m_సమితి_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_తదుపరి_Null(nodep->filenamep());
        iterate_And_తదుపరి_Null(nodep->lsbp());
        iterate_And_తదుపరి_Null(nodep->msbp());
    }
    శూన్య సందర్శించు(Ast_Test_Plus_వాదనలు* nodep) override {
        VL_RESTORER(m_సమితి_Ref_Lvalue);
        m_సమితి_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_తదుపరి_Null(nodep->searchp());
    }
    శూన్య సందర్శించు(Ast_విలువ_Plus_వాదనలు* nodep) override {
        VL_RESTORER(m_సమితి_Ref_Lvalue);
        m_సమితి_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_తదుపరి_Null(nodep->searchp());
        m_సమితి_Ref_Lvalue = VAccess::WRITE;
        iterate_And_తదుపరి_Null(nodep->outp());
    }
    శూన్య సందర్శించు(AstSFormat* nodep) override {
        VL_RESTORER(m_సమితి_Ref_Lvalue);
        m_సమితి_Ref_Lvalue = VAccess::WRITE;
        iterate_And_తదుపరి_Null(nodep->lhsp());
        m_సమితి_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_తదుపరి_Null(nodep->fmtp());
    }
    శూన్య సందర్శించు(Ast_కణుపు_Dist_Biop* nodep) override {
        VL_RESTORER(m_సమితి_Ref_Lvalue);
        m_సమితి_Ref_Lvalue = VAccess::READWRITE;
        iterate_And_తదుపరి_Null(nodep->lhsp());
        m_సమితి_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_తదుపరి_Null(nodep->rhsp());
    }
    శూన్య సందర్శించు(Ast_కణుపు_Dist_Triop* nodep) override {
        VL_RESTORER(m_సమితి_Ref_Lvalue);
        m_సమితి_Ref_Lvalue = VAccess::READWRITE;
        iterate_And_తదుపరి_Null(nodep->lhsp());
        m_సమితి_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_తదుపరి_Null(nodep->rhsp());
        iterate_And_తదుపరి_Null(nodep->thsp());
    }
    // cppcheck-suppress constParameterPointer
    శూన్య prepost_సందర్శించు(Ast_కణుపు_Uniop* nodep) {
        VL_RESTORER(m_సమితి_Ref_Lvalue);
        m_సమితి_Ref_Lvalue = VAccess::WRITE;
        iterate_And_తదుపరి_Null(nodep->lhsp());
    }
    శూన్య సందర్శించు(AstPreInc* nodep) override { prepost_సందర్శించు(nodep); }
    శూన్య సందర్శించు(AstPostInc* nodep) override { prepost_సందర్శించు(nodep); }
    శూన్య సందర్శించు(AstPreDec* nodep) override { prepost_సందర్శించు(nodep); }
    శూన్య సందర్శించు(AstPostDec* nodep) override { prepost_సందర్శించు(nodep); }

    // Nodes that change LValue state
    శూన్య సందర్శించు(AstSel* nodep) override {
        VL_RESTORER(m_సమితి_Ref_Lvalue);
        iterate_And_తదుపరి_Null(nodep->fromp());
        // Only set lvalues on the from
        m_సమితి_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_తదుపరి_Null(nodep->lsbp());
    }
    శూన్య సందర్శించు(Ast_కణుపు_Sel* nodep) override {
        VL_RESTORER(m_సమితి_Ref_Lvalue);
        // Only set lvalues on the from
        iterate_And_తదుపరి_Null(nodep->fromp());
        m_సమితి_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_తదుపరి_Null(nodep->bitp());
    }
    శూన్య సందర్శించు(AstCellArrayRef* nodep) override {
        VL_RESTORER(m_సమితి_Ref_Lvalue);
        // selp is not an lvalue
        m_సమితి_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_తదుపరి_Null(nodep->selp());
    }
    శూన్య సందర్శించు(Ast_కణుపు_Pre_Sel* nodep) override {
        ఒకవేళ (Ast_Sel_బిట్టు* స్థిరరాశి selbitp = VN_CAST(nodep, Sel_బిట్టు)) selbitp->access(m_సమితి_Ref_Lvalue);
        VL_RESTORER(m_సమితి_Ref_Lvalue);
        // Only set lvalues on the from
        iterate_And_తదుపరి_Null(nodep->fromp());
        m_సమితి_Ref_Lvalue = VAccess::NOCHANGE;
        iterate_And_తదుపరి_Null(nodep->rhsp());
        iterate_And_తదుపరి_Null(nodep->thsp());
    }
    శూన్య సందర్శించు(AstMemberSel* nodep) override {
        ఒకవేళ (m_సమితి_Ref_Lvalue != VAccess::NOCHANGE) {
            nodep->access(m_సమితి_Ref_Lvalue);
            ఒకవేళ (nodep->varp() && nodep->access().isWriteOrRW()) {
                ఒకవేళ (స్థిరరాశి Ast_Clocking_అంశం* స్థిరరాశి itemp
                    = VN_CAST(nodep->varp()->backp(), Clocking_అంశం)) {
                    UINFO(5, "ClkOut " << nodep);
                    ఒకవేళ (itemp->outputp()) nodep->varp(itemp->outputp()->varp());
                }
            }
        } లేకపోతే {
            // It is the only place where the access is set to member select nodes.
            // If it doesn't have to be set to WRITE, it means that it is READ.
            nodep->access(VAccess::READ);
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask_Ref* nodep) override {
        స్థిరరాశి Ast_కణుపు_FTask* స్థిరరాశి taskp = nodep->taskp();
        // We'll deal with mismatching pins later
        ఒకవేళ (!taskp) తిరిగి;
        స్థిరరాశి V3TaskConnects tconnects
            = V3Task::taskConnects(nodep, taskp->stmtsp(), శూన్యసూచిక, అబద్ధం);
        కొరకు (స్థిరరాశి స్వయంచాలక& tconnect : tconnects) {
            స్థిరరాశి AstVar* స్థిరరాశి portp = tconnect.మొదటి;
            స్థిరరాశి AstArg* స్థిరరాశి argp = tconnect.రెండవ;
            ఒకవేళ (!argp) కొనసాగు;
            Ast_కణుపు_సమాసం* స్థిరరాశి pinp = argp->exprp();
            ఒకవేళ (!pinp) కొనసాగు;
            VL_RESTORER(m_సమితి_Ref_Lvalue);
            m_సమితి_Ref_Lvalue = portp->direction().pinAccess();
            iterate(pinp);
        }
        ఒకవేళ (nodep->withp()) iterate(nodep->withp());
    }
    శూన్య సందర్శించు(AstConstraint* nodep) override {
        VL_RESTORER(m_సమితి_If_Rand);
        m_సమితి_If_Rand = నిజం;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask* nodep) override {
        VL_RESTORER(m_లోపల_Func);
        m_లోపల_Func = నిజం;
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    LinkLValueVisitor(Ast_కణుపు* nodep, VAccess ప్రారంభం)
        : m_సమితి_Ref_Lvalue{ప్రారంభం} {
        iterate(nodep);
    }
    ~LinkLValueVisitor() override = అప్రమేయ;
};

//######################################################################
// Link class functions

శూన్య V3LinkLValue::linkLValue(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    { LinkLValueVisitor{nodep, VAccess::NOCHANGE}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("linklvalue", 0, dump_వృక్షం_Either_Level() >= 6);
}
శూన్య V3LinkLValue::link_LValue_సమితి(Ast_కణుపు* స్థిరరాశి nodep, స్థిరరాశి VAccess access) {
    // Called by later link functions when it is known a node needs
    // to be converted to a lvalue.
    UINFO(9, __FUNCTION__ << ": ");
    { LinkLValueVisitor{nodep, access}; }
}
