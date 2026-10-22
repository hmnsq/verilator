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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3Clean.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clean state, as a visitor of each AstNode

শ্ৰেণী CleanVisitor final : সার্ৱজনিক VNVisitor {
    // NODE STATE
    // Entire netlist:
    //  AstNode::user1()        -> CleanState.  For this node, 0==UNKNOWN
    //  AstNode::user2()        -> bool.  True indicates widthMin has been propagated
    //  AstNodeDType::user3()   -> AstNodeDType*.  Alternative node with C size
    ধ্রুৱ VNUser1InUse m_inuser1;
    ধ্রুৱ VNUser2InUse m_inuser2;
    ধ্রুৱ VNUser3InUse m_inuser3;

    // TYPES
    গণনা CleanState : uint8_t { CS_UNKNOWN, CS_CLEAN, CS_DIRTY };

    // STATE - for current visit position (use VL_RESTORER)
    ধ্রুৱ Ast_গ্ৰন্থি_Module* m_modp = nullptr;

    // METHODS

    // Width resetting
    পূর্ণাঙ্ক cppWidth(ধ্রুৱ Ast_গ্ৰন্থি* nodep) {
        যদি (nodep->width() <= VL_IDATASIZE) {
            ঘূৰাই VL_IDATASIZE;
        } অন্যথা যদি (nodep->width() <= VL_QUADSIZE) {
            ঘূৰাই VL_QUADSIZE;
        } অন্যথা {
            ঘূৰাই nodep->width_শব্দগুলি() * VL_EDATASIZE;
        }
    }
    শূন্য setCppWidth(Ast_গ্ৰন্থি* nodep) {
        nodep->user2(সঁচা);  // Don't resize it again
        Ast_গ্ৰন্থি_DType* ধ্রুৱ old_dtypep = nodep->dtypep();
        ধ্রুৱ পূর্ণাঙ্ক width = cppWidth(nodep);  // widthMin is unchanged
        যদি (old_dtypep->width() != width) {
            // Since any given dtype's cppWidth() is the same, we can just
            // remember one conversion for each, and reuse it
            যদি (Ast_গ্ৰন্থি_DType* ধ্রুৱ new_dtypep = VN_CAST(old_dtypep->user3p(), গ্ৰন্থি_DType)) {
                nodep->dtypep(new_dtypep);
            } অন্যথা {
                nodep->dtypeChgWidth(width, nodep->widthMin());
                Ast_গ্ৰন্থি_DType* ধ্রুৱ new_dtypep2 = nodep->dtypep();
                UASSERT_OBJ(new_dtypep2 != old_dtypep, nodep,
                            "Dtype didn't change when width changed");
                old_dtypep->user3p(new_dtypep2);  // Remember for next time
            }
        }
    }
    শূন্য computeCppWidth(Ast_গ্ৰন্থি* nodep) {
        যদি (!nodep->user2() && nodep->hasDType()) {
            যদি (VN_IS(nodep, Var)  //
                || VN_IS(nodep, গ্ৰন্থি_DType)  // Don't want to change variable widths!
                || VN_IS(nodep, গ্ৰন্থি_Sel)  // Array selects should reflect variable widths
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
            } অন্যথা {
                ধ্রুৱ Ast_গ্ৰন্থি_UOr_Struct_DType* ধ্রুৱ dtypep
                    = VN_CAST(nodep->dtypep()->skipRefp(), গ্ৰন্থি_UOr_Struct_DType);
                যদি (!dtypep || dtypep->packed()) setCppWidth(nodep);
            }
        }
    }

    // Store the clean state in the userp on each node
    শূন্য setCleanState(Ast_গ্ৰন্থি* nodep, CleanState clean) { nodep->user1(clean); }
    CleanState getCleanState(ধ্রুৱ Ast_গ্ৰন্থি* nodep) {
        ঘূৰাই static_cast<CleanState>(nodep->user1());
    }
    তার্কিক isClean(Ast_গ্ৰন্থি* nodep) {
        ধ্রুৱ CleanState clstate = getCleanState(nodep);
        যদি (clstate == CS_CLEAN) ঘূৰাই সঁচা;
        যদি (clstate == CS_DIRTY) ঘূৰাই মিছা;
        nodep->v3fatalSrc("Unknown clean state on node: " + nodep->prettyTypeName());
        ঘূৰাই মিছা;
    }
    শূন্য setClean(Ast_গ্ৰন্থি* nodep, তার্কিক isClean) {
        computeCppWidth(nodep);  // Just to be sure it's in widthMin
        ধ্রুৱ তার্কিক wholeUint
            = (nodep->widthMin() == VL_IDATASIZE || nodep->widthMin() == VL_QUADSIZE
               || (nodep->widthMin() % VL_EDATASIZE) == 0);
        setCleanState(nodep, ((isClean || wholeUint) ? CS_CLEAN : CS_DIRTY));
    }

    // Operate on nodes
    শূন্য ঢোকাও_Clean(Ast_গ্ৰন্থি_ৰাশিমালা* nodep) {  // We'll insert ABOVE passed node
        UINFO(4, "  NeedClean " << nodep);
        VNRelinker relinkHandle;
        nodep->unlinkFrBack(&relinkHandle);
        //
        computeCppWidth(nodep);
        V3_সংখ্যা mask{nodep, cppWidth(nodep)};
        mask.setMask(nodep->widthMin());
        Ast_গ্ৰন্থি* ধ্রুৱ cleanp
            = নূতন AstAnd{nodep->fileline(), নূতন AstConst{nodep->fileline(), mask}, nodep};
        cleanp->dtypeFrom(nodep);  // Otherwise the AND normally picks LHS
        relinkHandle.relink(cleanp);
    }
    শূন্য ensureClean(Ast_গ্ৰন্থি_ৰাশিমালা* nodep) {
        computeCppWidth(nodep);
        যদি (!isClean(nodep)) ঢোকাও_Clean(nodep);
    }
    শূন্য ensureCleanAndNext(Ast_গ্ৰন্থি* nodep) {
        // Editing list, careful looping!
        বাবে (Ast_গ্ৰন্থি* argp = nodep; argp;) {
            Ast_গ্ৰন্থি* ধ্রুৱ nextp = argp->nextp();
            যদি (Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ exprp = VN_CAST(argp, গ্ৰন্থি_ৰাশিমালা)) ensureClean(exprp);
            argp = nextp;
        }
    }

    // Base type handling methods
    শূন্য operandBiop(Ast_গ্ৰন্থি_Biop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        যদি (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        যদি (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        // no setClean.. must do it in each user routine.
    }
    শূন্য operandTriop(Ast_গ্ৰন্থি_Triop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        যদি (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        যদি (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        যদি (nodep->cleanThs()) ensureClean(nodep->thsp());
        // no setClean.. must do it in each user routine.
    }
    শূন্য operandQuadop(Ast_গ্ৰন্থি_Quadop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        যদি (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        যদি (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        যদি (nodep->cleanThs()) ensureClean(nodep->thsp());
        যদি (nodep->cleanFhs()) ensureClean(nodep->fhsp());
        // no setClean.. must do it in each user routine.
    }

    // VISITORS
    শূন্য visit(Ast_গ্ৰন্থি_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্ৰন্থি_Uniop* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        যদি (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        setClean(nodep, nodep->cleanOut());
    }
    শূন্য visit(Ast_গ্ৰন্থি_Biop* nodep) override {
        operandBiop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    শূন্য visit(AstAnd* nodep) override {
        operandBiop(nodep);
        setClean(nodep, isClean(nodep->lhsp()) || isClean(nodep->rhsp()));
    }
    শূন্য visit(AstXor* nodep) override {
        operandBiop(nodep);
        setClean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    শূন্য visit(AstOr* nodep) override {
        operandBiop(nodep);
        setClean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    শূন্য visit(Ast_গ্ৰন্থি_Quadop* nodep) override {
        operandQuadop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    শূন্য visit(Ast_ৰাশিমালা_Stmt* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, isClean(nodep->resultp()));
    }
    শূন্য visit(Ast_গ্ৰন্থি_ৰাশিমালা* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    শূন্য visit(Ast_গ্ৰন্থি_Assign* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        যদি (nodep->cleanRhs()) ensureClean(nodep->rhsp());
    }
    শূন্য visit(AstScopeName* nodep) override {  //
        setClean(nodep, সঁচা);
    }
    শূন্য visit(AstCNew* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, সঁচা);
    }
    // Constructor values are stored, so must be clean
    শূন্য visit(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        যদি (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, সঁচা);
    }
    শূন্য visit(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        যদি (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, সঁচা);
    }
    শূন্য visit(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        যদি (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, সঁচা);
    }
    শূন্য visit(Ast_গ্ৰন্থি_Cons_Item* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->valuep());
    }
    শূন্য visit(AstMatchMasked* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        setClean(nodep, সঁচা);
    }
    শূন্য visit(AstSel* nodep) override {
        operandBiop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    শূন্য visit(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, মিছা);
        // We always clean, as we don't trust those pesky users.
        যদি (!VN_IS(nodep->backp(), And)) ঢোকাও_Clean(nodep);
        ensureCleanAndNext(nodep->nodesp());
    }
    শূন্য visit(AstTraceDecl* nodep) override {}  // Nothing to do here
    শূন্য visit(AstTraceInc* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->valuep());
    }
    শূন্য visit(AstTypedef* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }
    শূন্য visit(AstParamTypeDType* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }

    // Control flow operators
    শূন্য visit(AstCond* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
        setClean(nodep, isClean(nodep->thenp()) && isClean(nodep->elsep()));
    }
    শূন্য visit(AstLoop* nodep) override { iterateChildren(nodep); }
    শূন্য visit(AstLoopTest* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    শূন্য visit(Ast_গ্ৰন্থি_If* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    শূন্য visit(AstSFormatArg* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->exprp());
        setClean(nodep, সঁচা);
    }
    শূন্য visit(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->exprsp());
        setClean(nodep, সঁচা);  // generates a string, so not relevant
    }
    শূন্য visit(AstCStmt* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->nodesp());
    }
    শূন্য visit(AstCStmtUser* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->nodesp());
    }
    শূন্য visit(Ast_গ্ৰন্থি_CCall* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->argsp());
        setClean(nodep, সঁচা);
    }
    শূন্য visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->pinsp());
        setClean(nodep, nodep->cleanOut());
    }
    শূন্য visit(AstWith* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, সঁচা);
    }
    শূন্য visit(AstCReturn* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        setClean(nodep, সঁচা);
    }
    শূন্য visit(AstIntfRef* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, সঁচা);  // generates a string, so not relevant
    }

    //--------------------
    // Default: Just iterate
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
    }

সার্ৱজনিক:
    // CONSTRUCTORS
    explicit CleanVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CleanVisitor() override = স্ৱভাৱ;
};

//######################################################################
// Clean class functions

শূন্য V3Clean::cleanAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CleanVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("clean", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
