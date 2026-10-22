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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Clean.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clean state, as a visitor of each AstNode

வகுப்பு CleanVisitor final : பொது VNVisitor {
    // NODE STATE
    // Entire netlist:
    //  AstNode::user1()        -> CleanState.  For this node, 0==UNKNOWN
    //  AstNode::user2()        -> bool.  True indicates widthMin has been propagated
    //  AstNodeDType::user3()   -> AstNodeDType*.  Alternative node with C size
    மாறிலி VNUser1InUse m_inuser1;
    மாறிலி VNUser2InUse m_inuser2;
    மாறிலி VNUser3InUse m_inuser3;

    // TYPES
    எண்ணிக்கை CleanState : uint8_t { CS_UNKNOWN, CS_CLEAN, CS_DIRTY };

    // STATE - for current visit position (use VL_RESTORER)
    மாறிலி Ast_முனை_Module* m_modp = nullptr;

    // METHODS

    // Width resetting
    முழு cppWidth(மாறிலி Ast_முனை* nodep) {
        எனில் (nodep->width() <= VL_IDATASIZE) {
            திரும்பு VL_IDATASIZE;
        } இல்லையெனில் எனில் (nodep->width() <= VL_QUADSIZE) {
            திரும்பு VL_QUADSIZE;
        } இல்லையெனில் {
            திரும்பு nodep->width_சொற்கள்() * VL_EDATASIZE;
        }
    }
    வெறுமை setCppWidth(Ast_முனை* nodep) {
        nodep->user2(உண்மை);  // Don't resize it again
        Ast_முனை_DType* மாறிலி old_dtypep = nodep->dtypep();
        மாறிலி முழு width = cppWidth(nodep);  // widthMin is unchanged
        எனில் (old_dtypep->width() != width) {
            // Since any given dtype's cppWidth() is the same, we can just
            // remember one conversion for each, and reuse it
            எனில் (Ast_முனை_DType* மாறிலி new_dtypep = VN_CAST(old_dtypep->user3p(), முனை_DType)) {
                nodep->dtypep(new_dtypep);
            } இல்லையெனில் {
                nodep->dtypeChgWidth(width, nodep->widthMin());
                Ast_முனை_DType* மாறிலி new_dtypep2 = nodep->dtypep();
                UASSERT_OBJ(new_dtypep2 != old_dtypep, nodep,
                            "Dtype didn't change when width changed");
                old_dtypep->user3p(new_dtypep2);  // Remember for next time
            }
        }
    }
    வெறுமை computeCppWidth(Ast_முனை* nodep) {
        எனில் (!nodep->user2() && nodep->hasDType()) {
            எனில் (VN_IS(nodep, Var)  //
                || VN_IS(nodep, முனை_DType)  // Don't want to change variable widths!
                || VN_IS(nodep, முனை_Sel)  // Array selects should reflect variable widths
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
            } இல்லையெனில் {
                மாறிலி Ast_முனை_UOr_Struct_DType* மாறிலி dtypep
                    = VN_CAST(nodep->dtypep()->skipRefp(), முனை_UOr_Struct_DType);
                எனில் (!dtypep || dtypep->packed()) setCppWidth(nodep);
            }
        }
    }

    // Store the clean state in the userp on each node
    வெறுமை setCleanState(Ast_முனை* nodep, CleanState clean) { nodep->user1(clean); }
    CleanState getCleanState(மாறிலி Ast_முனை* nodep) {
        திரும்பு static_cast<CleanState>(nodep->user1());
    }
    தருக்க isClean(Ast_முனை* nodep) {
        மாறிலி CleanState clstate = getCleanState(nodep);
        எனில் (clstate == CS_CLEAN) திரும்பு உண்மை;
        எனில் (clstate == CS_DIRTY) திரும்பு பொய்;
        nodep->v3fatalSrc("Unknown clean state on node: " + nodep->prettyTypeName());
        திரும்பு பொய்;
    }
    வெறுமை setClean(Ast_முனை* nodep, தருக்க isClean) {
        computeCppWidth(nodep);  // Just to be sure it's in widthMin
        மாறிலி தருக்க wholeUint
            = (nodep->widthMin() == VL_IDATASIZE || nodep->widthMin() == VL_QUADSIZE
               || (nodep->widthMin() % VL_EDATASIZE) == 0);
        setCleanState(nodep, ((isClean || wholeUint) ? CS_CLEAN : CS_DIRTY));
    }

    // Operate on nodes
    வெறுமை செருகு_Clean(Ast_முனை_கோவை* nodep) {  // We'll insert ABOVE passed node
        UINFO(4, "  NeedClean " << nodep);
        VNRelinker relinkHandle;
        nodep->unlinkFrBack(&relinkHandle);
        //
        computeCppWidth(nodep);
        V3_எண் mask{nodep, cppWidth(nodep)};
        mask.setMask(nodep->widthMin());
        Ast_முனை* மாறிலி cleanp
            = புதிய AstAnd{nodep->fileline(), புதிய AstConst{nodep->fileline(), mask}, nodep};
        cleanp->dtypeFrom(nodep);  // Otherwise the AND normally picks LHS
        relinkHandle.relink(cleanp);
    }
    வெறுமை ensureClean(Ast_முனை_கோவை* nodep) {
        computeCppWidth(nodep);
        எனில் (!isClean(nodep)) செருகு_Clean(nodep);
    }
    வெறுமை ensureCleanAndNext(Ast_முனை* nodep) {
        // Editing list, careful looping!
        க்கு (Ast_முனை* argp = nodep; argp;) {
            Ast_முனை* மாறிலி nextp = argp->nextp();
            எனில் (Ast_முனை_கோவை* மாறிலி exprp = VN_CAST(argp, முனை_கோவை)) ensureClean(exprp);
            argp = nextp;
        }
    }

    // Base type handling methods
    வெறுமை operandBiop(Ast_முனை_Biop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        எனில் (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        எனில் (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        // no setClean.. must do it in each user routine.
    }
    வெறுமை operandTriop(Ast_முனை_Triop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        எனில் (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        எனில் (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        எனில் (nodep->cleanThs()) ensureClean(nodep->thsp());
        // no setClean.. must do it in each user routine.
    }
    வெறுமை operandQuadop(Ast_முனை_Quadop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        எனில் (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        எனில் (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        எனில் (nodep->cleanThs()) ensureClean(nodep->thsp());
        எனில் (nodep->cleanFhs()) ensureClean(nodep->fhsp());
        // no setClean.. must do it in each user routine.
    }

    // VISITORS
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை_Uniop* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        எனில் (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        setClean(nodep, nodep->cleanOut());
    }
    வெறுமை visit(Ast_முனை_Biop* nodep) override {
        operandBiop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    வெறுமை visit(AstAnd* nodep) override {
        operandBiop(nodep);
        setClean(nodep, isClean(nodep->lhsp()) || isClean(nodep->rhsp()));
    }
    வெறுமை visit(AstXor* nodep) override {
        operandBiop(nodep);
        setClean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    வெறுமை visit(AstOr* nodep) override {
        operandBiop(nodep);
        setClean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    வெறுமை visit(Ast_முனை_Quadop* nodep) override {
        operandQuadop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    வெறுமை visit(Ast_கோவை_Stmt* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, isClean(nodep->resultp()));
    }
    வெறுமை visit(Ast_முனை_கோவை* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    வெறுமை visit(Ast_முனை_Assign* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        எனில் (nodep->cleanRhs()) ensureClean(nodep->rhsp());
    }
    வெறுமை visit(AstScopeName* nodep) override {  //
        setClean(nodep, உண்மை);
    }
    வெறுமை visit(AstCNew* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, உண்மை);
    }
    // Constructor values are stored, so must be clean
    வெறுமை visit(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        எனில் (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, உண்மை);
    }
    வெறுமை visit(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        எனில் (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, உண்மை);
    }
    வெறுமை visit(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        எனில் (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, உண்மை);
    }
    வெறுமை visit(Ast_முனை_Cons_Item* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->valuep());
    }
    வெறுமை visit(AstMatchMasked* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        setClean(nodep, உண்மை);
    }
    வெறுமை visit(AstSel* nodep) override {
        operandBiop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    வெறுமை visit(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, பொய்);
        // We always clean, as we don't trust those pesky users.
        எனில் (!VN_IS(nodep->backp(), And)) செருகு_Clean(nodep);
        ensureCleanAndNext(nodep->nodesp());
    }
    வெறுமை visit(AstTraceDecl* nodep) override {}  // Nothing to do here
    வெறுமை visit(AstTraceInc* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->valuep());
    }
    வெறுமை visit(AstTypedef* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }
    வெறுமை visit(AstParamTypeDType* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }

    // Control flow operators
    வெறுமை visit(AstCond* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
        setClean(nodep, isClean(nodep->thenp()) && isClean(nodep->elsep()));
    }
    வெறுமை visit(AstLoop* nodep) override { iterateChildren(nodep); }
    வெறுமை visit(AstLoopTest* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    வெறுமை visit(Ast_முனை_If* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    வெறுமை visit(AstSFormatArg* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->exprp());
        setClean(nodep, உண்மை);
    }
    வெறுமை visit(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->exprsp());
        setClean(nodep, உண்மை);  // generates a string, so not relevant
    }
    வெறுமை visit(AstCStmt* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->nodesp());
    }
    வெறுமை visit(AstCStmtUser* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->nodesp());
    }
    வெறுமை visit(Ast_முனை_CCall* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->argsp());
        setClean(nodep, உண்மை);
    }
    வெறுமை visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->pinsp());
        setClean(nodep, nodep->cleanOut());
    }
    வெறுமை visit(AstWith* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, உண்மை);
    }
    வெறுமை visit(AstCReturn* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        setClean(nodep, உண்மை);
    }
    வெறுமை visit(AstIntfRef* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, உண்மை);  // generates a string, so not relevant
    }

    //--------------------
    // Default: Just iterate
    வெறுமை visit(Ast_முனை* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
    }

பொது:
    // CONSTRUCTORS
    explicit CleanVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CleanVisitor() override = இயல்பு;
};

//######################################################################
// Clean class functions

வெறுமை V3Clean::cleanAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CleanVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("clean", 0, dump_மரம்_Either_Level() >= 3);
}
