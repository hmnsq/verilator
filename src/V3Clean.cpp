// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Add temporaries, such as for clean nodes
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
// V3Clean's Transformations:
//
// Each module:
//      For each expression, if it requires a clean operand,
//      and the operand is dirty, insert a CLEAN node.
//      Resize operands (but not variables, variable selects, or formatting metadata)
//      to C++ 32/64/wide types.
//      Copy all width() values to widthMin() so RANGE, etc can still see orig widths
//
//*************************************************************************

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Clean.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clean state, as a visitor of each AstNode

वग्ग CleanVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // Entire netlist:
    //  AstNode::user1()        -> CleanState.  For this node, 0==UNKNOWN
    //  AstNode::user2()        -> bool.  True indicates widthMin has been propagated
    //  AstNodeDType::user3()   -> AstNodeDType*.  Alternative node with C size
    ध्रुव VNUser1InUse m_inuser1;
    ध्रुव VNUser2InUse m_inuser2;
    ध्रुव VNUser3InUse m_inuser3;

    // TYPES
    गणना CleanState : uint8_t { CS_UNKNOWN, CS_CLEAN, CS_DIRTY };

    // STATE - for current visit position (use VL_RESTORER)
    ध्रुव Ast_ग्रन्थिः_Module* m_modp = nullptr;

    // METHODS

    // Width resetting
    पूर्णाङ्क cppWidth(ध्रुव Ast_ग्रन्थिः* nodep) {
        सचे (nodep->width() <= VL_IDATASIZE) {
            पटिनिवत्त VL_IDATASIZE;
        } अञ्ञथा सचे (nodep->width() <= VL_QUADSIZE) {
            पटिनिवत्त VL_QUADSIZE;
        } अञ्ञथा {
            पटिनिवत्त nodep->width_शब्दाः() * VL_EDATASIZE;
        }
    }
    शून्य setCppWidth(Ast_ग्रन्थिः* nodep) {
        nodep->user2(सच्च);  // Don't resize it again
        Ast_ग्रन्थिः_DType* ध्रुव old_dtypep = nodep->dtypep();
        ध्रुव पूर्णाङ्क width = cppWidth(nodep);  // widthMin is unchanged
        सचे (old_dtypep->width() != width) {
            // Since any given dtype's cppWidth() is the same, we can just
            // remember one conversion for each, and reuse it
            सचे (Ast_ग्रन्थिः_DType* ध्रुव new_dtypep = VN_CAST(old_dtypep->user3p(), ग्रन्थिः_DType)) {
                nodep->dtypep(new_dtypep);
            } अञ्ञथा {
                nodep->dtypeChgWidth(width, nodep->widthMin());
                Ast_ग्रन्थिः_DType* ध्रुव new_dtypep2 = nodep->dtypep();
                UASSERT_OBJ(new_dtypep2 != old_dtypep, nodep,
                            "Dtype didn't change when width changed");
                old_dtypep->user3p(new_dtypep2);  // Remember for next time
            }
        }
    }
    शून्य computeCppWidth(Ast_ग्रन्थिः* nodep) {
        सचे (!nodep->user2() && nodep->hasDType()) {
            सचे (VN_IS(nodep, Var)  //
                || VN_IS(nodep, ग्रन्थिः_DType)  // Don't want to change variable widths!
                || VN_IS(nodep, ग्रन्थिः_Sel)  // Array selects should reflect variable widths
                || VN_IS(nodep, SFormatArg)  // Retain the logical argument type, not storage width
                || VN_IS(nodep->dtypep()->skipRefp(), AssocArrayDType)  // Or arrays
                || VN_IS(nodep->dtypep()->skipRefp(), WildcardArrayDType)
                || VN_IS(nodep->dtypep()->skipRefp(), DynArrayDType)
                || VN_IS(nodep->dtypep()->skipRefp(), ClassRefDType)
                || VN_IS(nodep->dtypep()->skipRefp(), QueueDType)
                || VN_IS(nodep->dtypep()->skipRefp(), StreamDType)
                || VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)
                || VN_IS(nodep->dtypep()->skipRefp(), CoverCrossDType)
                || VN_IS(nodep->dtypep()->skipRefp(), CoverpointDType)
                || VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) {
            } अञ्ञथा {
                ध्रुव Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव dtypep
                    = VN_CAST(nodep->dtypep()->skipRefp(), ग्रन्थिः_UOr_Struct_DType);
                सचे (!dtypep || dtypep->packed()) setCppWidth(nodep);
            }
        }
    }

    // Store the clean state in the userp on each node
    शून्य setCleanState(Ast_ग्रन्थिः* nodep, CleanState clean) { nodep->user1(clean); }
    CleanState getCleanState(ध्रुव Ast_ग्रन्थिः* nodep) {
        पटिनिवत्त static_cast<CleanState>(nodep->user1());
    }
    तार्किक isClean(Ast_ग्रन्थिः* nodep) {
        ध्रुव CleanState clstate = getCleanState(nodep);
        सचे (clstate == CS_CLEAN) पटिनिवत्त सच्च;
        सचे (clstate == CS_DIRTY) पटिनिवत्त मिच्छा;
        nodep->v3fatalSrc("Unknown clean state on node: " + nodep->prettyTypeName());
        पटिनिवत्त मिच्छा;
    }
    शून्य setClean(Ast_ग्रन्थिः* nodep, तार्किक isClean) {
        computeCppWidth(nodep);  // Just to be sure it's in widthMin
        ध्रुव तार्किक wholeUint
            = (nodep->widthMin() == VL_IDATASIZE || nodep->widthMin() == VL_QUADSIZE
               || (nodep->widthMin() % VL_EDATASIZE) == 0);
        setCleanState(nodep, ((isClean || wholeUint) ? CS_CLEAN : CS_DIRTY));
    }

    // Operate on nodes
    शून्य निवेशय_Clean(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {  // We'll insert ABOVE passed node
        UINFO(4, "  NeedClean " << nodep);
        VNRelinker relinkHandle;
        nodep->unlinkFrBack(&relinkHandle);
        //
        computeCppWidth(nodep);
        V3_सङ्ख्या mask{nodep, cppWidth(nodep)};
        mask.setMask(nodep->widthMin());
        Ast_ग्रन्थिः* ध्रुव cleanp
            = नूतन AstAnd{nodep->fileline(), नूतन AstConst{nodep->fileline(), mask}, nodep};
        cleanp->dtypeFrom(nodep);  // Otherwise the AND normally picks LHS
        relinkHandle.relink(cleanp);
    }
    शून्य ensureClean(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        computeCppWidth(nodep);
        सचे (!isClean(nodep)) निवेशय_Clean(nodep);
    }
    शून्य ensureCleanAndNext(Ast_ग्रन्थिः* nodep) {
        // Editing list, careful looping!
        अत्थाय (Ast_ग्रन्थिः* argp = nodep; argp;) {
            Ast_ग्रन्थिः* ध्रुव nextp = argp->nextp();
            सचे (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = VN_CAST(argp, ग्रन्थिः_व्यञ्जकम्)) ensureClean(exprp);
            argp = nextp;
        }
    }

    // Base type handling methods
    शून्य operandBiop(Ast_ग्रन्थिः_Biop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        सचे (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        सचे (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        // no setClean.. must do it in each user routine.
    }
    शून्य operandTriop(Ast_ग्रन्थिः_Triop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        सचे (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        सचे (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        सचे (nodep->cleanThs()) ensureClean(nodep->thsp());
        // no setClean.. must do it in each user routine.
    }
    शून्य operandQuadop(Ast_ग्रन्थिः_Quadop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        सचे (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        सचे (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        सचे (nodep->cleanThs()) ensureClean(nodep->thsp());
        सचे (nodep->cleanFhs()) ensureClean(nodep->fhsp());
        // no setClean.. must do it in each user routine.
    }

    // VISITORS
    शून्य visit(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_ग्रन्थिः_Uniop* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        सचे (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        setClean(nodep, nodep->cleanOut());
    }
    शून्य visit(Ast_ग्रन्थिः_Biop* nodep) override {
        operandBiop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    शून्य visit(AstAnd* nodep) override {
        operandBiop(nodep);
        setClean(nodep, isClean(nodep->lhsp()) || isClean(nodep->rhsp()));
    }
    शून्य visit(AstXor* nodep) override {
        operandBiop(nodep);
        setClean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    शून्य visit(AstOr* nodep) override {
        operandBiop(nodep);
        setClean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    शून्य visit(Ast_ग्रन्थिः_Quadop* nodep) override {
        operandQuadop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    शून्य visit(Ast_व्यञ्जकम्_Stmt* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, isClean(nodep->resultp()));
    }
    शून्य visit(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    शून्य visit(Ast_ग्रन्थिः_Assign* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        सचे (nodep->cleanRhs()) ensureClean(nodep->rhsp());
    }
    शून्य visit(AstScopeName* nodep) override {  //
        setClean(nodep, सच्च);
    }
    शून्य visit(AstCNew* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, सच्च);
    }
    // Constructor values are stored, so must be clean
    शून्य visit(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        सचे (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, सच्च);
    }
    शून्य visit(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        सचे (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, सच्च);
    }
    शून्य visit(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        सचे (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, सच्च);
    }
    शून्य visit(Ast_ग्रन्थिः_Cons_Item* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->valuep());
    }
    शून्य visit(AstMatchMasked* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        setClean(nodep, सच्च);
    }
    शून्य visit(AstSel* nodep) override {
        operandBiop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    शून्य visit(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, मिच्छा);
        // We always clean, as we don't trust those pesky users.
        सचे (!VN_IS(nodep->backp(), And)) निवेशय_Clean(nodep);
        ensureCleanAndNext(nodep->nodesp());
    }
    शून्य visit(AstTraceDecl* nodep) override {}  // Nothing to do here
    शून्य visit(AstTraceInc* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->valuep());
    }
    शून्य visit(AstTypedef* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }
    शून्य visit(AstParamTypeDType* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }

    // Control flow operators
    शून्य visit(AstCond* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
        setClean(nodep, isClean(nodep->thenp()) && isClean(nodep->elsep()));
    }
    शून्य visit(AstLoop* nodep) override { iterateChildren(nodep); }
    शून्य visit(AstLoopTest* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    शून्य visit(Ast_ग्रन्थिः_If* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    शून्य visit(AstSFormatArg* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->exprp());
        setClean(nodep, सच्च);
    }
    शून्य visit(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->exprsp());
        setClean(nodep, सच्च);  // generates a string, so not relevant
    }
    शून्य visit(AstCStmt* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->nodesp());
    }
    शून्य visit(AstCStmtUser* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->nodesp());
    }
    शून्य visit(Ast_ग्रन्थिः_CCall* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->argsp());
        setClean(nodep, सच्च);
    }
    शून्य visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->pinsp());
        setClean(nodep, nodep->cleanOut());
    }
    शून्य visit(AstWith* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, सच्च);
    }
    शून्य visit(AstCReturn* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        setClean(nodep, सच्च);
    }
    शून्य visit(AstIntfRef* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, सच्च);  // generates a string, so not relevant
    }

    //--------------------
    // Default: Just iterate
    शून्य visit(Ast_ग्रन्थिः* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
    }

सार्वजनिक:
    // CONSTRUCTORS
    explicit CleanVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CleanVisitor() override = स्वभाव;
};

//######################################################################
// Clean class functions

शून्य V3Clean::cleanAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CleanVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("clean", 0, dump_वृक्षः_Either_Level() >= 3);
}
