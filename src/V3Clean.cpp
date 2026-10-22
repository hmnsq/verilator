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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Clean.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clean state, as a visitor of each AstNode

ವರ್ಗ CleanVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // Entire netlist:
    //  AstNode::user1()        -> CleanState.  For this node, 0==UNKNOWN
    //  AstNode::user2()        -> bool.  True indicates widthMin has been propagated
    //  AstNodeDType::user3()   -> AstNodeDType*.  Alternative node with C size
    ಧ್ರುವ VNUser1InUse m_inuser1;
    ಧ್ರುವ VNUser2InUse m_inuser2;
    ಧ್ರುವ VNUser3InUse m_inuser3;

    // TYPES
    ಗಣನಾ CleanState : uint8_t { CS_UNKNOWN, CS_CLEAN, CS_DIRTY };

    // STATE - for current visit position (use VL_RESTORER)
    ಧ್ರುವ Ast_ಗಂಟು_Module* m_modp = nullptr;

    // METHODS

    // Width resetting
    ಪೂರ್ಣಾಙ್ಕ cppWidth(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        ಯದಿ (nodep->width() <= VL_IDATASIZE) {
            ಪ್ರತ್ಯಾಗಮ VL_IDATASIZE;
        } ಅನ್ಯಥಾ ಯದಿ (nodep->width() <= VL_QUADSIZE) {
            ಪ್ರತ್ಯಾಗಮ VL_QUADSIZE;
        } ಅನ್ಯಥಾ {
            ಪ್ರತ್ಯಾಗಮ nodep->width_ಪದಗಳು() * VL_EDATASIZE;
        }
    }
    ಶೂನ್ಯ setCppWidth(Ast_ಗಂಟು* nodep) {
        nodep->user2(ಸತ್ಯ);  // Don't resize it again
        Ast_ಗಂಟು_DType* ಧ್ರುವ old_dtypep = nodep->dtypep();
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = cppWidth(nodep);  // widthMin is unchanged
        ಯದಿ (old_dtypep->width() != width) {
            // Since any given dtype's cppWidth() is the same, we can just
            // remember one conversion for each, and reuse it
            ಯದಿ (Ast_ಗಂಟು_DType* ಧ್ರುವ new_dtypep = VN_CAST(old_dtypep->user3p(), ಗಂಟು_DType)) {
                nodep->dtypep(new_dtypep);
            } ಅನ್ಯಥಾ {
                nodep->dtypeChgWidth(width, nodep->widthMin());
                Ast_ಗಂಟು_DType* ಧ್ರುವ new_dtypep2 = nodep->dtypep();
                UASSERT_OBJ(new_dtypep2 != old_dtypep, nodep,
                            "Dtype didn't change when width changed");
                old_dtypep->user3p(new_dtypep2);  // Remember for next time
            }
        }
    }
    ಶೂನ್ಯ computeCppWidth(Ast_ಗಂಟು* nodep) {
        ಯದಿ (!nodep->user2() && nodep->hasDType()) {
            ಯದಿ (VN_IS(nodep, Var)  //
                || VN_IS(nodep, ಗಂಟು_DType)  // Don't want to change variable widths!
                || VN_IS(nodep, ಗಂಟು_Sel)  // Array selects should reflect variable widths
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
            } ಅನ್ಯಥಾ {
                ಧ್ರುವ Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ dtypep
                    = VN_CAST(nodep->dtypep()->skipRefp(), ಗಂಟು_UOr_Struct_DType);
                ಯದಿ (!dtypep || dtypep->packed()) setCppWidth(nodep);
            }
        }
    }

    // Store the clean state in the userp on each node
    ಶೂನ್ಯ setCleanState(Ast_ಗಂಟು* nodep, CleanState clean) { nodep->user1(clean); }
    CleanState getCleanState(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        ಪ್ರತ್ಯಾಗಮ static_cast<CleanState>(nodep->user1());
    }
    ತಾರ್ಕಿಕ isClean(Ast_ಗಂಟು* nodep) {
        ಧ್ರುವ CleanState clstate = getCleanState(nodep);
        ಯದಿ (clstate == CS_CLEAN) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಯದಿ (clstate == CS_DIRTY) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        nodep->v3fatalSrc("Unknown clean state on node: " + nodep->prettyTypeName());
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    ಶೂನ್ಯ setClean(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ isClean) {
        computeCppWidth(nodep);  // Just to be sure it's in widthMin
        ಧ್ರುವ ತಾರ್ಕಿಕ wholeUint
            = (nodep->widthMin() == VL_IDATASIZE || nodep->widthMin() == VL_QUADSIZE
               || (nodep->widthMin() % VL_EDATASIZE) == 0);
        setCleanState(nodep, ((isClean || wholeUint) ? CS_CLEAN : CS_DIRTY));
    }

    // Operate on nodes
    ಶೂನ್ಯ ಸೇರಿಸು_Clean(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {  // We'll insert ABOVE passed node
        UINFO(4, "  NeedClean " << nodep);
        VNRelinker relinkHandle;
        nodep->unlinkFrBack(&relinkHandle);
        //
        computeCppWidth(nodep);
        V3_ಸಂಖ್ಯೆ mask{nodep, cppWidth(nodep)};
        mask.setMask(nodep->widthMin());
        Ast_ಗಂಟು* ಧ್ರುವ cleanp
            = ನೂತನ AstAnd{nodep->fileline(), ನೂತನ AstConst{nodep->fileline(), mask}, nodep};
        cleanp->dtypeFrom(nodep);  // Otherwise the AND normally picks LHS
        relinkHandle.relink(cleanp);
    }
    ಶೂನ್ಯ ensureClean(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        computeCppWidth(nodep);
        ಯದಿ (!isClean(nodep)) ಸೇರಿಸು_Clean(nodep);
    }
    ಶೂನ್ಯ ensureCleanAndNext(Ast_ಗಂಟು* nodep) {
        // Editing list, careful looping!
        ಪ್ರತಿ (Ast_ಗಂಟು* argp = nodep; argp;) {
            Ast_ಗಂಟು* ಧ್ರುವ nextp = argp->nextp();
            ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = VN_CAST(argp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) ensureClean(exprp);
            argp = nextp;
        }
    }

    // Base type handling methods
    ಶೂನ್ಯ operandBiop(Ast_ಗಂಟು_Biop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        ಯದಿ (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        ಯದಿ (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        // no setClean.. must do it in each user routine.
    }
    ಶೂನ್ಯ operandTriop(Ast_ಗಂಟು_Triop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        ಯದಿ (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        ಯದಿ (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        ಯದಿ (nodep->cleanThs()) ensureClean(nodep->thsp());
        // no setClean.. must do it in each user routine.
    }
    ಶೂನ್ಯ operandQuadop(Ast_ಗಂಟು_Quadop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        ಯದಿ (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        ಯದಿ (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        ಯದಿ (nodep->cleanThs()) ensureClean(nodep->thsp());
        ಯದಿ (nodep->cleanFhs()) ensureClean(nodep->fhsp());
        // no setClean.. must do it in each user routine.
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Uniop* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        ಯದಿ (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        setClean(nodep, nodep->cleanOut());
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Biop* nodep) override {
        operandBiop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    ಶೂನ್ಯ visit(AstAnd* nodep) override {
        operandBiop(nodep);
        setClean(nodep, isClean(nodep->lhsp()) || isClean(nodep->rhsp()));
    }
    ಶೂನ್ಯ visit(AstXor* nodep) override {
        operandBiop(nodep);
        setClean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    ಶೂನ್ಯ visit(AstOr* nodep) override {
        operandBiop(nodep);
        setClean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Quadop* nodep) override {
        operandQuadop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    ಶೂನ್ಯ visit(Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, isClean(nodep->resultp()));
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Assign* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        ಯದಿ (nodep->cleanRhs()) ensureClean(nodep->rhsp());
    }
    ಶೂನ್ಯ visit(AstScopeName* nodep) override {  //
        setClean(nodep, ಸತ್ಯ);
    }
    ಶೂನ್ಯ visit(AstCNew* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, ಸತ್ಯ);
    }
    // Constructor values are stored, so must be clean
    ಶೂನ್ಯ visit(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, ಸತ್ಯ);
    }
    ಶೂನ್ಯ visit(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, ಸತ್ಯ);
    }
    ಶೂನ್ಯ visit(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, ಸತ್ಯ);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Cons_Item* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->valuep());
    }
    ಶೂನ್ಯ visit(AstMatchMasked* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        setClean(nodep, ಸತ್ಯ);
    }
    ಶೂನ್ಯ visit(AstSel* nodep) override {
        operandBiop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    ಶೂನ್ಯ visit(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, ಅಸತ್ಯ);
        // We always clean, as we don't trust those pesky users.
        ಯದಿ (!VN_IS(nodep->backp(), And)) ಸೇರಿಸು_Clean(nodep);
        ensureCleanAndNext(nodep->nodesp());
    }
    ಶೂನ್ಯ visit(AstTraceDecl* nodep) override {}  // Nothing to do here
    ಶೂನ್ಯ visit(AstTraceInc* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->valuep());
    }
    ಶೂನ್ಯ visit(AstTypedef* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstParamTypeDType* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }

    // Control flow operators
    ಶೂನ್ಯ visit(AstCond* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
        setClean(nodep, isClean(nodep->thenp()) && isClean(nodep->elsep()));
    }
    ಶೂನ್ಯ visit(AstLoop* nodep) override { iterateChildren(nodep); }
    ಶೂನ್ಯ visit(AstLoopTest* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_If* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    ಶೂನ್ಯ visit(AstSFormatArg* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->exprp());
        setClean(nodep, ಸತ್ಯ);
    }
    ಶೂನ್ಯ visit(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->exprsp());
        setClean(nodep, ಸತ್ಯ);  // generates a string, so not relevant
    }
    ಶೂನ್ಯ visit(AstCStmt* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->nodesp());
    }
    ಶೂನ್ಯ visit(AstCStmtUser* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->nodesp());
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_CCall* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->argsp());
        setClean(nodep, ಸತ್ಯ);
    }
    ಶೂನ್ಯ visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->pinsp());
        setClean(nodep, nodep->cleanOut());
    }
    ಶೂನ್ಯ visit(AstWith* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, ಸತ್ಯ);
    }
    ಶೂನ್ಯ visit(AstCReturn* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        setClean(nodep, ಸತ್ಯ);
    }
    ಶೂನ್ಯ visit(AstIntfRef* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, ಸತ್ಯ);  // generates a string, so not relevant
    }

    //--------------------
    // Default: Just iterate
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit CleanVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CleanVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// Clean class functions

ಶೂನ್ಯ V3Clean::cleanAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CleanVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("clean", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
