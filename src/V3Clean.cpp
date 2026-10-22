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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Clean.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clean state, as a visitor of each AstNode

صنف CleanVisitor final : عام VNVisitor {
    // NODE STATE
    // Entire netlist:
    //  AstNode::user1()        -> CleanState.  For this node, 0==UNKNOWN
    //  AstNode::user2()        -> bool.  True indicates widthMin has been propagated
    //  AstNodeDType::user3()   -> AstNodeDType*.  Alternative node with C size
    ثابت VNUser1InUse m_inuser1;
    ثابت VNUser2InUse m_inuser2;
    ثابت VNUser3InUse m_inuser3;

    // TYPES
    تعداد CleanState : uint8_t { CS_UNKNOWN, CS_CLEAN, CS_DIRTY };

    // STATE - for current visit position (use VL_RESTORER)
    ثابت Ast_گرێ_Module* m_modp = nullptr;

    // METHODS

    // Width resetting
    صحيح cppWidth(ثابت Ast_گرێ* nodep) {
        إذا (nodep->width() <= VL_IDATASIZE) {
            أرجع VL_IDATASIZE;
        } وإلا إذا (nodep->width() <= VL_QUADSIZE) {
            أرجع VL_QUADSIZE;
        } وإلا {
            أرجع nodep->width_وشەکان() * VL_EDATASIZE;
        }
    }
    فراغ setCppWidth(Ast_گرێ* nodep) {
        nodep->user2(true);  // Don't resize it again
        Ast_گرێ_DType* ثابت old_dtypep = nodep->dtypep();
        ثابت صحيح width = cppWidth(nodep);  // widthMin is unchanged
        إذا (old_dtypep->width() != width) {
            // Since any given dtype's cppWidth() is the same, we can just
            // remember one conversion for each, and reuse it
            إذا (Ast_گرێ_DType* ثابت new_dtypep = VN_CAST(old_dtypep->user3p(), گرێ_DType)) {
                nodep->dtypep(new_dtypep);
            } وإلا {
                nodep->dtypeChgWidth(width, nodep->widthMin());
                Ast_گرێ_DType* ثابت new_dtypep2 = nodep->dtypep();
                UASSERT_OBJ(new_dtypep2 != old_dtypep, nodep,
                            "Dtype didn't change when width changed");
                old_dtypep->user3p(new_dtypep2);  // Remember for next time
            }
        }
    }
    فراغ computeCppWidth(Ast_گرێ* nodep) {
        إذا (!nodep->user2() && nodep->hasDType()) {
            إذا (VN_IS(nodep, Var)  //
                || VN_IS(nodep, گرێ_DType)  // Don't want to change variable widths!
                || VN_IS(nodep, گرێ_Sel)  // Array selects should reflect variable widths
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
            } وإلا {
                ثابت Ast_گرێ_UOr_Struct_DType* ثابت dtypep
                    = VN_CAST(nodep->dtypep()->skipRefp(), گرێ_UOr_Struct_DType);
                إذا (!dtypep || dtypep->packed()) setCppWidth(nodep);
            }
        }
    }

    // Store the clean state in the userp on each node
    فراغ setCleanState(Ast_گرێ* nodep, CleanState clean) { nodep->user1(clean); }
    CleanState getCleanState(ثابت Ast_گرێ* nodep) {
        أرجع static_cast<CleanState>(nodep->user1());
    }
    منطقي isClean(Ast_گرێ* nodep) {
        ثابت CleanState clstate = getCleanState(nodep);
        إذا (clstate == CS_CLEAN) أرجع true;
        إذا (clstate == CS_DIRTY) أرجع خطأ;
        nodep->v3fatalSrc("Unknown clean state on node: " + nodep->prettyTypeName());
        أرجع خطأ;
    }
    فراغ setClean(Ast_گرێ* nodep, منطقي isClean) {
        computeCppWidth(nodep);  // Just to be sure it's in widthMin
        ثابت منطقي wholeUint
            = (nodep->widthMin() == VL_IDATASIZE || nodep->widthMin() == VL_QUADSIZE
               || (nodep->widthMin() % VL_EDATASIZE) == 0);
        setCleanState(nodep, ((isClean || wholeUint) ? CS_CLEAN : CS_DIRTY));
    }

    // Operate on nodes
    فراغ زیادکردن_Clean(Ast_گرێ_دەربڕین* nodep) {  // We'll insert ABOVE passed node
        UINFO(4, "  NeedClean " << nodep);
        VNRelinker relinkHandle;
        nodep->unlinkFrBack(&relinkHandle);
        //
        computeCppWidth(nodep);
        V3_ژمار mask{nodep, cppWidth(nodep)};
        mask.setMask(nodep->widthMin());
        Ast_گرێ* ثابت cleanp
            = جديد AstAnd{nodep->fileline(), جديد AstConst{nodep->fileline(), mask}, nodep};
        cleanp->dtypeFrom(nodep);  // Otherwise the AND normally picks LHS
        relinkHandle.relink(cleanp);
    }
    فراغ ensureClean(Ast_گرێ_دەربڕین* nodep) {
        computeCppWidth(nodep);
        إذا (!isClean(nodep)) زیادکردن_Clean(nodep);
    }
    فراغ ensureCleanAndNext(Ast_گرێ* nodep) {
        // Editing list, careful looping!
        لكل (Ast_گرێ* argp = nodep; argp;) {
            Ast_گرێ* ثابت nextp = argp->nextp();
            إذا (Ast_گرێ_دەربڕین* ثابت exprp = VN_CAST(argp, گرێ_دەربڕین)) ensureClean(exprp);
            argp = nextp;
        }
    }

    // Base type handling methods
    فراغ operandBiop(Ast_گرێ_Biop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        إذا (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        إذا (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        // no setClean.. must do it in each user routine.
    }
    فراغ operandTriop(Ast_گرێ_Triop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        إذا (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        إذا (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        إذا (nodep->cleanThs()) ensureClean(nodep->thsp());
        // no setClean.. must do it in each user routine.
    }
    فراغ operandQuadop(Ast_گرێ_Quadop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        إذا (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        إذا (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        إذا (nodep->cleanThs()) ensureClean(nodep->thsp());
        إذا (nodep->cleanFhs()) ensureClean(nodep->fhsp());
        // no setClean.. must do it in each user routine.
    }

    // VISITORS
    فراغ visit(Ast_گرێ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    فراغ visit(Ast_گرێ_Uniop* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        إذا (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        setClean(nodep, nodep->cleanOut());
    }
    فراغ visit(Ast_گرێ_Biop* nodep) override {
        operandBiop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    فراغ visit(AstAnd* nodep) override {
        operandBiop(nodep);
        setClean(nodep, isClean(nodep->lhsp()) || isClean(nodep->rhsp()));
    }
    فراغ visit(AstXor* nodep) override {
        operandBiop(nodep);
        setClean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    فراغ visit(AstOr* nodep) override {
        operandBiop(nodep);
        setClean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    فراغ visit(Ast_گرێ_Quadop* nodep) override {
        operandQuadop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    فراغ visit(Ast_دەربڕین_Stmt* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, isClean(nodep->resultp()));
    }
    فراغ visit(Ast_گرێ_دەربڕین* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    فراغ visit(Ast_گرێ_Assign* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        إذا (nodep->cleanRhs()) ensureClean(nodep->rhsp());
    }
    فراغ visit(AstScopeName* nodep) override {  //
        setClean(nodep, true);
    }
    فراغ visit(AstCNew* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, true);
    }
    // Constructor values are stored, so must be clean
    فراغ visit(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        إذا (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, true);
    }
    فراغ visit(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        إذا (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, true);
    }
    فراغ visit(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        إذا (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, true);
    }
    فراغ visit(Ast_گرێ_Cons_Item* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->valuep());
    }
    فراغ visit(AstMatchMasked* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        setClean(nodep, true);
    }
    فراغ visit(AstSel* nodep) override {
        operandBiop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    فراغ visit(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, خطأ);
        // We always clean, as we don't trust those pesky users.
        إذا (!VN_IS(nodep->backp(), And)) زیادکردن_Clean(nodep);
        ensureCleanAndNext(nodep->nodesp());
    }
    فراغ visit(AstTraceDecl* nodep) override {}  // Nothing to do here
    فراغ visit(AstTraceInc* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->valuep());
    }
    فراغ visit(AstTypedef* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }
    فراغ visit(AstParamTypeDType* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }

    // Control flow operators
    فراغ visit(AstCond* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
        setClean(nodep, isClean(nodep->thenp()) && isClean(nodep->elsep()));
    }
    فراغ visit(AstLoop* nodep) override { iterateChildren(nodep); }
    فراغ visit(AstLoopTest* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    فراغ visit(Ast_گرێ_If* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    فراغ visit(AstSFormatArg* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->exprp());
        setClean(nodep, true);
    }
    فراغ visit(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->exprsp());
        setClean(nodep, true);  // generates a string, so not relevant
    }
    فراغ visit(AstCStmt* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->nodesp());
    }
    فراغ visit(AstCStmtUser* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->nodesp());
    }
    فراغ visit(Ast_گرێ_CCall* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->argsp());
        setClean(nodep, true);
    }
    فراغ visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->pinsp());
        setClean(nodep, nodep->cleanOut());
    }
    فراغ visit(AstWith* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, true);
    }
    فراغ visit(AstCReturn* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        setClean(nodep, true);
    }
    فراغ visit(AstIntfRef* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, true);  // generates a string, so not relevant
    }

    //--------------------
    // Default: Just iterate
    فراغ visit(Ast_گرێ* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
    }

عام:
    // CONSTRUCTORS
    explicit CleanVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CleanVisitor() override = افتراضي;
};

//######################################################################
// Clean class functions

فراغ V3Clean::cleanAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CleanVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درەخت("clean", 0, dump_درەخت_Either_Level() >= 3);
}
