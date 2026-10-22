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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Clean.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clean state, as a visitor of each AstNode

رده CleanVisitor final : عمومی VNVisitor {
    // NODE STATE
    // Entire netlist:
    //  AstNode::user1()        -> CleanState.  For this node, 0==UNKNOWN
    //  AstNode::user2()        -> bool.  True indicates widthMin has been propagated
    //  AstNodeDType::user3()   -> AstNodeDType*.  Alternative node with C size
    ثابت VNUser1InUse m_inuser1;
    ثابت VNUser2InUse m_inuser2;
    ثابت VNUser3InUse m_inuser3;

    // TYPES
    شمارش CleanState : uint8_t { CS_UNKNOWN, CS_CLEAN, CS_DIRTY };

    // STATE - for current visit position (use VL_RESTORER)
    ثابت Ast_گره_Module* m_modp = nullptr;

    // METHODS

    // Width resetting
    صحیح cppWidth(ثابت Ast_گره* nodep) {
        اگر (nodep->width() <= VL_IDATASIZE) {
            بازگشت VL_IDATASIZE;
        } وگرنه اگر (nodep->width() <= VL_QUADSIZE) {
            بازگشت VL_QUADSIZE;
        } وگرنه {
            بازگشت nodep->width_واژگان() * VL_EDATASIZE;
        }
    }
    تهی setCppWidth(Ast_گره* nodep) {
        nodep->user2(درست);  // Don't resize it again
        Ast_گره_DType* ثابت old_dtypep = nodep->dtypep();
        ثابت صحیح width = cppWidth(nodep);  // widthMin is unchanged
        اگر (old_dtypep->width() != width) {
            // Since any given dtype's cppWidth() is the same, we can just
            // remember one conversion for each, and reuse it
            اگر (Ast_گره_DType* ثابت new_dtypep = VN_CAST(old_dtypep->user3p(), گره_DType)) {
                nodep->dtypep(new_dtypep);
            } وگرنه {
                nodep->dtypeChgWidth(width, nodep->widthMin());
                Ast_گره_DType* ثابت new_dtypep2 = nodep->dtypep();
                UASSERT_OBJ(new_dtypep2 != old_dtypep, nodep,
                            "Dtype didn't change when width changed");
                old_dtypep->user3p(new_dtypep2);  // Remember for next time
            }
        }
    }
    تهی computeCppWidth(Ast_گره* nodep) {
        اگر (!nodep->user2() && nodep->hasDType()) {
            اگر (VN_IS(nodep, Var)  //
                || VN_IS(nodep, گره_DType)  // Don't want to change variable widths!
                || VN_IS(nodep, گره_Sel)  // Array selects should reflect variable widths
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
            } وگرنه {
                ثابت Ast_گره_UOr_Struct_DType* ثابت dtypep
                    = VN_CAST(nodep->dtypep()->skipRefp(), گره_UOr_Struct_DType);
                اگر (!dtypep || dtypep->packed()) setCppWidth(nodep);
            }
        }
    }

    // Store the clean state in the userp on each node
    تهی setCleanState(Ast_گره* nodep, CleanState clean) { nodep->user1(clean); }
    CleanState getCleanState(ثابت Ast_گره* nodep) {
        بازگشت static_cast<CleanState>(nodep->user1());
    }
    منطقی isClean(Ast_گره* nodep) {
        ثابت CleanState clstate = getCleanState(nodep);
        اگر (clstate == CS_CLEAN) بازگشت درست;
        اگر (clstate == CS_DIRTY) بازگشت نادرست;
        nodep->v3fatalSrc("Unknown clean state on node: " + nodep->prettyTypeName());
        بازگشت نادرست;
    }
    تهی setClean(Ast_گره* nodep, منطقی isClean) {
        computeCppWidth(nodep);  // Just to be sure it's in widthMin
        ثابت منطقی wholeUint
            = (nodep->widthMin() == VL_IDATASIZE || nodep->widthMin() == VL_QUADSIZE
               || (nodep->widthMin() % VL_EDATASIZE) == 0);
        setCleanState(nodep, ((isClean || wholeUint) ? CS_CLEAN : CS_DIRTY));
    }

    // Operate on nodes
    تهی درج_Clean(Ast_گره_عبارت* nodep) {  // We'll insert ABOVE passed node
        UINFO(4, "  NeedClean " << nodep);
        VNRelinker relinkHandle;
        nodep->unlinkFrBack(&relinkHandle);
        //
        computeCppWidth(nodep);
        V3_عدد mask{nodep, cppWidth(nodep)};
        mask.setMask(nodep->widthMin());
        Ast_گره* ثابت cleanp
            = جدید AstAnd{nodep->fileline(), جدید AstConst{nodep->fileline(), mask}, nodep};
        cleanp->dtypeFrom(nodep);  // Otherwise the AND normally picks LHS
        relinkHandle.relink(cleanp);
    }
    تهی ensureClean(Ast_گره_عبارت* nodep) {
        computeCppWidth(nodep);
        اگر (!isClean(nodep)) درج_Clean(nodep);
    }
    تهی ensureCleanAndNext(Ast_گره* nodep) {
        // Editing list, careful looping!
        برای (Ast_گره* argp = nodep; argp;) {
            Ast_گره* ثابت nextp = argp->nextp();
            اگر (Ast_گره_عبارت* ثابت exprp = VN_CAST(argp, گره_عبارت)) ensureClean(exprp);
            argp = nextp;
        }
    }

    // Base type handling methods
    تهی operandBiop(Ast_گره_Biop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        اگر (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        اگر (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        // no setClean.. must do it in each user routine.
    }
    تهی operandTriop(Ast_گره_Triop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        اگر (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        اگر (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        اگر (nodep->cleanThs()) ensureClean(nodep->thsp());
        // no setClean.. must do it in each user routine.
    }
    تهی operandQuadop(Ast_گره_Quadop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        اگر (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        اگر (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        اگر (nodep->cleanThs()) ensureClean(nodep->thsp());
        اگر (nodep->cleanFhs()) ensureClean(nodep->fhsp());
        // no setClean.. must do it in each user routine.
    }

    // VISITORS
    تهی visit(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره_Uniop* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        اگر (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        setClean(nodep, nodep->cleanOut());
    }
    تهی visit(Ast_گره_Biop* nodep) override {
        operandBiop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    تهی visit(AstAnd* nodep) override {
        operandBiop(nodep);
        setClean(nodep, isClean(nodep->lhsp()) || isClean(nodep->rhsp()));
    }
    تهی visit(AstXor* nodep) override {
        operandBiop(nodep);
        setClean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    تهی visit(AstOr* nodep) override {
        operandBiop(nodep);
        setClean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    تهی visit(Ast_گره_Quadop* nodep) override {
        operandQuadop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    تهی visit(Ast_عبارت_Stmt* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, isClean(nodep->resultp()));
    }
    تهی visit(Ast_گره_عبارت* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    تهی visit(Ast_گره_Assign* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        اگر (nodep->cleanRhs()) ensureClean(nodep->rhsp());
    }
    تهی visit(AstScopeName* nodep) override {  //
        setClean(nodep, درست);
    }
    تهی visit(AstCNew* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, درست);
    }
    // Constructor values are stored, so must be clean
    تهی visit(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        اگر (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, درست);
    }
    تهی visit(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        اگر (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, درست);
    }
    تهی visit(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        اگر (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, درست);
    }
    تهی visit(Ast_گره_Cons_Item* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->valuep());
    }
    تهی visit(AstMatchMasked* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        setClean(nodep, درست);
    }
    تهی visit(AstSel* nodep) override {
        operandBiop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    تهی visit(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, نادرست);
        // We always clean, as we don't trust those pesky users.
        اگر (!VN_IS(nodep->backp(), And)) درج_Clean(nodep);
        ensureCleanAndNext(nodep->nodesp());
    }
    تهی visit(AstTraceDecl* nodep) override {}  // Nothing to do here
    تهی visit(AstTraceInc* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->valuep());
    }
    تهی visit(AstTypedef* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }
    تهی visit(AstParamTypeDType* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }

    // Control flow operators
    تهی visit(AstCond* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
        setClean(nodep, isClean(nodep->thenp()) && isClean(nodep->elsep()));
    }
    تهی visit(AstLoop* nodep) override { iterateChildren(nodep); }
    تهی visit(AstLoopTest* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    تهی visit(Ast_گره_If* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    تهی visit(AstSFormatArg* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->exprp());
        setClean(nodep, درست);
    }
    تهی visit(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->exprsp());
        setClean(nodep, درست);  // generates a string, so not relevant
    }
    تهی visit(AstCStmt* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->nodesp());
    }
    تهی visit(AstCStmtUser* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->nodesp());
    }
    تهی visit(Ast_گره_CCall* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->argsp());
        setClean(nodep, درست);
    }
    تهی visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->pinsp());
        setClean(nodep, nodep->cleanOut());
    }
    تهی visit(AstWith* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, درست);
    }
    تهی visit(AstCReturn* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        setClean(nodep, درست);
    }
    تهی visit(AstIntfRef* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, درست);  // generates a string, so not relevant
    }

    //--------------------
    // Default: Just iterate
    تهی visit(Ast_گره* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
    }

عمومی:
    // CONSTRUCTORS
    explicit CleanVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CleanVisitor() override = پیشفرض;
};

//######################################################################
// Clean class functions

تهی V3Clean::cleanAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CleanVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("clean", 0, dump_درخت_Either_Level() >= 3);
}
