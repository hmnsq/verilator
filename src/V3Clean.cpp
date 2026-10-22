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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Clean.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clean state, as a visitor of each AstNode

ଶ୍ରେଣୀ CleanVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    // Entire netlist:
    //  AstNode::user1()        -> CleanState.  For this node, 0==UNKNOWN
    //  AstNode::user2()        -> bool.  True indicates widthMin has been propagated
    //  AstNodeDType::user3()   -> AstNodeDType*.  Alternative node with C size
    ଅଚଳ VNUser1InUse m_inuser1;
    ଅଚଳ VNUser2InUse m_inuser2;
    ଅଚଳ VNUser3InUse m_inuser3;

    // TYPES
    ଗଣନା CleanState : uint8_t { CS_UNKNOWN, CS_CLEAN, CS_DIRTY };

    // STATE - for current visit position (use VL_RESTORER)
    ଅଚଳ Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;

    // METHODS

    // Width resetting
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା cppWidth(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (nodep->width() <= VL_IDATASIZE) {
            ଫେରନ୍ତୁ VL_IDATASIZE;
        } ଅନ୍ୟଥା ଯଦି (nodep->width() <= VL_QUADSIZE) {
            ଫେରନ୍ତୁ VL_QUADSIZE;
        } ଅନ୍ୟଥା {
            ଫେରନ୍ତୁ nodep->width_ଶବ୍ଦଗୁଡ଼ିକ() * VL_EDATASIZE;
        }
    }
    ଶୂନ୍ୟ setCppWidth(Ast_ଗଣ୍ଠି* nodep) {
        nodep->user2(ସତ୍ୟ);  // Don't resize it again
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ old_dtypep = nodep->dtypep();
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = cppWidth(nodep);  // widthMin is unchanged
        ଯଦି (old_dtypep->width() != width) {
            // Since any given dtype's cppWidth() is the same, we can just
            // remember one conversion for each, and reuse it
            ଯଦି (Ast_ଗଣ୍ଠି_DType* ଅଚଳ new_dtypep = VN_CAST(old_dtypep->user3p(), ଗଣ୍ଠି_DType)) {
                nodep->dtypep(new_dtypep);
            } ଅନ୍ୟଥା {
                nodep->dtypeChgWidth(width, nodep->widthMin());
                Ast_ଗଣ୍ଠି_DType* ଅଚଳ new_dtypep2 = nodep->dtypep();
                UASSERT_OBJ(new_dtypep2 != old_dtypep, nodep,
                            "Dtype didn't change when width changed");
                old_dtypep->user3p(new_dtypep2);  // Remember for next time
            }
        }
    }
    ଶୂନ୍ୟ computeCppWidth(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (!nodep->user2() && nodep->hasDType()) {
            ଯଦି (VN_IS(nodep, Var)  //
                || VN_IS(nodep, ଗଣ୍ଠି_DType)  // Don't want to change variable widths!
                || VN_IS(nodep, ଗଣ୍ଠି_Sel)  // Array selects should reflect variable widths
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
            } ଅନ୍ୟଥା {
                ଅଚଳ Ast_ଗଣ୍ଠି_UOr_Struct_DType* ଅଚଳ dtypep
                    = VN_CAST(nodep->dtypep()->skipRefp(), ଗଣ୍ଠି_UOr_Struct_DType);
                ଯଦି (!dtypep || dtypep->packed()) setCppWidth(nodep);
            }
        }
    }

    // Store the clean state in the userp on each node
    ଶୂନ୍ୟ setCleanState(Ast_ଗଣ୍ଠି* nodep, CleanState clean) { nodep->user1(clean); }
    CleanState getCleanState(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        ଫେରନ୍ତୁ static_cast<CleanState>(nodep->user1());
    }
    ତାର୍କିକ isClean(Ast_ଗଣ୍ଠି* nodep) {
        ଅଚଳ CleanState clstate = getCleanState(nodep);
        ଯଦି (clstate == CS_CLEAN) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଯଦି (clstate == CS_DIRTY) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        nodep->v3fatalSrc("Unknown clean state on node: " + nodep->prettyTypeName());
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    ଶୂନ୍ୟ setClean(Ast_ଗଣ୍ଠି* nodep, ତାର୍କିକ isClean) {
        computeCppWidth(nodep);  // Just to be sure it's in widthMin
        ଅଚଳ ତାର୍କିକ wholeUint
            = (nodep->widthMin() == VL_IDATASIZE || nodep->widthMin() == VL_QUADSIZE
               || (nodep->widthMin() % VL_EDATASIZE) == 0);
        setCleanState(nodep, ((isClean || wholeUint) ? CS_CLEAN : CS_DIRTY));
    }

    // Operate on nodes
    ଶୂନ୍ୟ ଭର୍ତ୍ତି_Clean(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {  // We'll insert ABOVE passed node
        UINFO(4, "  NeedClean " << nodep);
        VNRelinker relinkHandle;
        nodep->unlinkFrBack(&relinkHandle);
        //
        computeCppWidth(nodep);
        V3_ସଂଖ୍ୟା mask{nodep, cppWidth(nodep)};
        mask.setMask(nodep->widthMin());
        Ast_ଗଣ୍ଠି* ଅଚଳ cleanp
            = ନୂତନ AstAnd{nodep->fileline(), ନୂତନ AstConst{nodep->fileline(), mask}, nodep};
        cleanp->dtypeFrom(nodep);  // Otherwise the AND normally picks LHS
        relinkHandle.relink(cleanp);
    }
    ଶୂନ୍ୟ ensureClean(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        computeCppWidth(nodep);
        ଯଦି (!isClean(nodep)) ଭର୍ତ୍ତି_Clean(nodep);
    }
    ଶୂନ୍ୟ ensureCleanAndNext(Ast_ଗଣ୍ଠି* nodep) {
        // Editing list, careful looping!
        ପାଇଁ (Ast_ଗଣ୍ଠି* argp = nodep; argp;) {
            Ast_ଗଣ୍ଠି* ଅଚଳ nextp = argp->nextp();
            ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = VN_CAST(argp, ଗଣ୍ଠି_ପଦସମଷ୍ଟି)) ensureClean(exprp);
            argp = nextp;
        }
    }

    // Base type handling methods
    ଶୂନ୍ୟ operandBiop(Ast_ଗଣ୍ଠି_Biop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        ଯଦି (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        ଯଦି (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        // no setClean.. must do it in each user routine.
    }
    ଶୂନ୍ୟ operandTriop(Ast_ଗଣ୍ଠି_Triop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        ଯଦି (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        ଯଦି (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        ଯଦି (nodep->cleanThs()) ensureClean(nodep->thsp());
        // no setClean.. must do it in each user routine.
    }
    ଶୂନ୍ୟ operandQuadop(Ast_ଗଣ୍ଠି_Quadop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        ଯଦି (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        ଯଦି (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        ଯଦି (nodep->cleanThs()) ensureClean(nodep->thsp());
        ଯଦି (nodep->cleanFhs()) ensureClean(nodep->fhsp());
        // no setClean.. must do it in each user routine.
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Uniop* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        ଯଦି (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        setClean(nodep, nodep->cleanOut());
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Biop* nodep) override {
        operandBiop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    ଶୂନ୍ୟ visit(AstAnd* nodep) override {
        operandBiop(nodep);
        setClean(nodep, isClean(nodep->lhsp()) || isClean(nodep->rhsp()));
    }
    ଶୂନ୍ୟ visit(AstXor* nodep) override {
        operandBiop(nodep);
        setClean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    ଶୂନ୍ୟ visit(AstOr* nodep) override {
        operandBiop(nodep);
        setClean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Quadop* nodep) override {
        operandQuadop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    ଶୂନ୍ୟ visit(Ast_ପଦସମଷ୍ଟି_Stmt* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, isClean(nodep->resultp()));
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Assign* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        ଯଦି (nodep->cleanRhs()) ensureClean(nodep->rhsp());
    }
    ଶୂନ୍ୟ visit(AstScopeName* nodep) override {  //
        setClean(nodep, ସତ୍ୟ);
    }
    ଶୂନ୍ୟ visit(AstCNew* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, ସତ୍ୟ);
    }
    // Constructor values are stored, so must be clean
    ଶୂନ୍ୟ visit(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        ଯଦି (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, ସତ୍ୟ);
    }
    ଶୂନ୍ୟ visit(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        ଯଦି (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, ସତ୍ୟ);
    }
    ଶୂନ୍ୟ visit(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        ଯଦି (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, ସତ୍ୟ);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Cons_Item* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->valuep());
    }
    ଶୂନ୍ୟ visit(AstMatchMasked* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        setClean(nodep, ସତ୍ୟ);
    }
    ଶୂନ୍ୟ visit(AstSel* nodep) override {
        operandBiop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    ଶୂନ୍ୟ visit(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, ମିଥ୍ୟା);
        // We always clean, as we don't trust those pesky users.
        ଯଦି (!VN_IS(nodep->backp(), And)) ଭର୍ତ୍ତି_Clean(nodep);
        ensureCleanAndNext(nodep->nodesp());
    }
    ଶୂନ୍ୟ visit(AstTraceDecl* nodep) override {}  // Nothing to do here
    ଶୂନ୍ୟ visit(AstTraceInc* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->valuep());
    }
    ଶୂନ୍ୟ visit(AstTypedef* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstParamTypeDType* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }

    // Control flow operators
    ଶୂନ୍ୟ visit(AstCond* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
        setClean(nodep, isClean(nodep->thenp()) && isClean(nodep->elsep()));
    }
    ଶୂନ୍ୟ visit(AstLoop* nodep) override { iterateChildren(nodep); }
    ଶୂନ୍ୟ visit(AstLoopTest* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_If* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    ଶୂନ୍ୟ visit(AstSFormatArg* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->exprp());
        setClean(nodep, ସତ୍ୟ);
    }
    ଶୂନ୍ୟ visit(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->exprsp());
        setClean(nodep, ସତ୍ୟ);  // generates a string, so not relevant
    }
    ଶୂନ୍ୟ visit(AstCStmt* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->nodesp());
    }
    ଶୂନ୍ୟ visit(AstCStmtUser* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->nodesp());
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_CCall* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->argsp());
        setClean(nodep, ସତ୍ୟ);
    }
    ଶୂନ୍ୟ visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->pinsp());
        setClean(nodep, nodep->cleanOut());
    }
    ଶୂନ୍ୟ visit(AstWith* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, ସତ୍ୟ);
    }
    ଶୂନ୍ୟ visit(AstCReturn* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        setClean(nodep, ସତ୍ୟ);
    }
    ଶୂନ୍ୟ visit(AstIntfRef* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, ସତ୍ୟ);  // generates a string, so not relevant
    }

    //--------------------
    // Default: Just iterate
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit CleanVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CleanVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Clean class functions

ଶୂନ୍ୟ V3Clean::cleanAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CleanVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("clean", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
