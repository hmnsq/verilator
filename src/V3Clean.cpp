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

वर्ग CleanVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // Entire netlist:
    //  AstNode::user1()        -> CleanState.  For this node, 0==UNKNOWN
    //  AstNode::user2()        -> bool.  True indicates widthMin has been propagated
    //  AstNodeDType::user3()   -> AstNodeDType*.  Alternative node with C size
    अचर VNUser1InUse m_inuser1;
    अचर VNUser2InUse m_inuser2;
    अचर VNUser3InUse m_inuser3;

    // TYPES
    गणना CleanState : uint8_t { CS_UNKNOWN, CS_CLEAN, CS_DIRTY };

    // STATE - for current visit position (use VL_RESTORER)
    अचर Ast_गाँठो_Module* m_modp = nullptr;

    // METHODS

    // Width resetting
    पूर्णाङ्क cppWidth(अचर Ast_गाँठो* nodep) {
        यदि (nodep->width() <= VL_IDATASIZE) {
            फर्काऊ VL_IDATASIZE;
        } अन्यथा यदि (nodep->width() <= VL_QUADSIZE) {
            फर्काऊ VL_QUADSIZE;
        } अन्यथा {
            फर्काऊ nodep->width_शब्दहरू() * VL_EDATASIZE;
        }
    }
    शून्य setCppWidth(Ast_गाँठो* nodep) {
        nodep->user2(सत्य);  // Don't resize it again
        Ast_गाँठो_DType* अचर old_dtypep = nodep->dtypep();
        अचर पूर्णाङ्क width = cppWidth(nodep);  // widthMin is unchanged
        यदि (old_dtypep->width() != width) {
            // Since any given dtype's cppWidth() is the same, we can just
            // remember one conversion for each, and reuse it
            यदि (Ast_गाँठो_DType* अचर new_dtypep = VN_CAST(old_dtypep->user3p(), गाँठो_DType)) {
                nodep->dtypep(new_dtypep);
            } अन्यथा {
                nodep->dtypeChgWidth(width, nodep->widthMin());
                Ast_गाँठो_DType* अचर new_dtypep2 = nodep->dtypep();
                UASSERT_OBJ(new_dtypep2 != old_dtypep, nodep,
                            "Dtype didn't change when width changed");
                old_dtypep->user3p(new_dtypep2);  // Remember for next time
            }
        }
    }
    शून्य computeCppWidth(Ast_गाँठो* nodep) {
        यदि (!nodep->user2() && nodep->hasDType()) {
            यदि (VN_IS(nodep, Var)  //
                || VN_IS(nodep, गाँठो_DType)  // Don't want to change variable widths!
                || VN_IS(nodep, गाँठो_Sel)  // Array selects should reflect variable widths
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
            } अन्यथा {
                अचर Ast_गाँठो_UOr_Struct_DType* अचर dtypep
                    = VN_CAST(nodep->dtypep()->skipRefp(), गाँठो_UOr_Struct_DType);
                यदि (!dtypep || dtypep->packed()) setCppWidth(nodep);
            }
        }
    }

    // Store the clean state in the userp on each node
    शून्य setCleanState(Ast_गाँठो* nodep, CleanState clean) { nodep->user1(clean); }
    CleanState getCleanState(अचर Ast_गाँठो* nodep) {
        फर्काऊ static_cast<CleanState>(nodep->user1());
    }
    तार्किक isClean(Ast_गाँठो* nodep) {
        अचर CleanState clstate = getCleanState(nodep);
        यदि (clstate == CS_CLEAN) फर्काऊ सत्य;
        यदि (clstate == CS_DIRTY) फर्काऊ असत्य;
        nodep->v3fatalSrc("Unknown clean state on node: " + nodep->prettyTypeName());
        फर्काऊ असत्य;
    }
    शून्य setClean(Ast_गाँठो* nodep, तार्किक isClean) {
        computeCppWidth(nodep);  // Just to be sure it's in widthMin
        अचर तार्किक wholeUint
            = (nodep->widthMin() == VL_IDATASIZE || nodep->widthMin() == VL_QUADSIZE
               || (nodep->widthMin() % VL_EDATASIZE) == 0);
        setCleanState(nodep, ((isClean || wholeUint) ? CS_CLEAN : CS_DIRTY));
    }

    // Operate on nodes
    शून्य घुसाउ_Clean(Ast_गाँठो_अभिव्यक्ति* nodep) {  // We'll insert ABOVE passed node
        UINFO(4, "  NeedClean " << nodep);
        VNRelinker relinkHandle;
        nodep->unlinkFrBack(&relinkHandle);
        //
        computeCppWidth(nodep);
        V3_सङ्ख्या mask{nodep, cppWidth(nodep)};
        mask.setMask(nodep->widthMin());
        Ast_गाँठो* अचर cleanp
            = नूतन AstAnd{nodep->fileline(), नूतन AstConst{nodep->fileline(), mask}, nodep};
        cleanp->dtypeFrom(nodep);  // Otherwise the AND normally picks LHS
        relinkHandle.relink(cleanp);
    }
    शून्य ensureClean(Ast_गाँठो_अभिव्यक्ति* nodep) {
        computeCppWidth(nodep);
        यदि (!isClean(nodep)) घुसाउ_Clean(nodep);
    }
    शून्य ensureCleanAndNext(Ast_गाँठो* nodep) {
        // Editing list, careful looping!
        लागि (Ast_गाँठो* argp = nodep; argp;) {
            Ast_गाँठो* अचर nextp = argp->nextp();
            यदि (Ast_गाँठो_अभिव्यक्ति* अचर exprp = VN_CAST(argp, गाँठो_अभिव्यक्ति)) ensureClean(exprp);
            argp = nextp;
        }
    }

    // Base type handling methods
    शून्य operandBiop(Ast_गाँठो_Biop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        यदि (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        यदि (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        // no setClean.. must do it in each user routine.
    }
    शून्य operandTriop(Ast_गाँठो_Triop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        यदि (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        यदि (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        यदि (nodep->cleanThs()) ensureClean(nodep->thsp());
        // no setClean.. must do it in each user routine.
    }
    शून्य operandQuadop(Ast_गाँठो_Quadop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        यदि (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        यदि (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        यदि (nodep->cleanThs()) ensureClean(nodep->thsp());
        यदि (nodep->cleanFhs()) ensureClean(nodep->fhsp());
        // no setClean.. must do it in each user routine.
    }

    // VISITORS
    शून्य visit(Ast_गाँठो_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाँठो_Uniop* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        यदि (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        setClean(nodep, nodep->cleanOut());
    }
    शून्य visit(Ast_गाँठो_Biop* nodep) override {
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
    शून्य visit(Ast_गाँठो_Quadop* nodep) override {
        operandQuadop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    शून्य visit(Ast_अभिव्यक्ति_Stmt* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, isClean(nodep->resultp()));
    }
    शून्य visit(Ast_गाँठो_अभिव्यक्ति* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    शून्य visit(Ast_गाँठो_Assign* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        यदि (nodep->cleanRhs()) ensureClean(nodep->rhsp());
    }
    शून्य visit(AstScopeName* nodep) override {  //
        setClean(nodep, सत्य);
    }
    शून्य visit(AstCNew* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, सत्य);
    }
    // Constructor values are stored, so must be clean
    शून्य visit(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        यदि (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, सत्य);
    }
    शून्य visit(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        यदि (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, सत्य);
    }
    शून्य visit(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        यदि (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, सत्य);
    }
    शून्य visit(Ast_गाँठो_Cons_Item* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->valuep());
    }
    शून्य visit(AstMatchMasked* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        setClean(nodep, सत्य);
    }
    शून्य visit(AstSel* nodep) override {
        operandBiop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    शून्य visit(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, असत्य);
        // We always clean, as we don't trust those pesky users.
        यदि (!VN_IS(nodep->backp(), And)) घुसाउ_Clean(nodep);
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
    शून्य visit(Ast_गाँठो_If* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    शून्य visit(AstSFormatArg* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->exprp());
        setClean(nodep, सत्य);
    }
    शून्य visit(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->exprsp());
        setClean(nodep, सत्य);  // generates a string, so not relevant
    }
    शून्य visit(AstCStmt* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->nodesp());
    }
    शून्य visit(AstCStmtUser* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->nodesp());
    }
    शून्य visit(Ast_गाँठो_CCall* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->argsp());
        setClean(nodep, सत्य);
    }
    शून्य visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->pinsp());
        setClean(nodep, nodep->cleanOut());
    }
    शून्य visit(AstWith* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, सत्य);
    }
    शून्य visit(AstCReturn* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        setClean(nodep, सत्य);
    }
    शून्य visit(AstIntfRef* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, सत्य);  // generates a string, so not relevant
    }

    //--------------------
    // Default: Just iterate
    शून्य visit(Ast_गाँठो* nodep) override {
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
    V3Global::dump_Check_Global_रूख("clean", 0, dump_रूख_Either_Level() >= 3);
}
