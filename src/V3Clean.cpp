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
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_inuser2;
    ಸ್ಥಿರಾಂಕ VNUser3_ಒಳ_Use m_inuser3;

    // TYPES
    ಎಣಿಕೆ CleanState : uint8_t { CS_UNKNOWN, CS_CLEAN, CS_DIRTY };

    // STATE - for current visit position (use VL_RESTORER)
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;

    // METHODS

    // Width resetting
    ಪೂರ್ಣಾಂಕ cpp_ಅಗಲ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        ಆದರೆ (nodep->ಅಗಲ() <= VL_IDATASIZE) {
            ಹಿಂತಿರುಗಿ VL_IDATASIZE;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->ಅಗಲ() <= VL_QUADSIZE) {
            ಹಿಂತಿರುಗಿ VL_QUADSIZE;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹಿಂತಿರುಗಿ nodep->ಅಗಲ_ಪದಗಳು() * VL_EDATASIZE;
        }
    }
    ಶೂನ್ಯ ಗಣ_Cpp_ಅಗಲ(Ast_ಗಂಟು* nodep) {
        nodep->user2(ನಿಜ);  // Don't resize it again
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ old_dtypep = nodep->dtypep();
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = cpp_ಅಗಲ(nodep);  // widthMin is unchanged
        ಆದರೆ (old_dtypep->ಅಗಲ() != ಅಗಲ) {
            // Since any given dtype's cppWidth() is the same, we can just
            // remember one conversion for each, and reuse it
            ಆದರೆ (Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ new_dtypep = VN_CAST(old_dtypep->user3p(), ಗಂಟು_DType)) {
                nodep->dtypep(new_dtypep);
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->dtype_Chg_ಅಗಲ(ಅಗಲ, nodep->ಅಗಲ_Min());
                Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ new_dtypep2 = nodep->dtypep();
                UASSERT_OBJ(new_dtypep2 != old_dtypep, nodep,
                            "Dtype didn't change when width changed");
                old_dtypep->user3p(new_dtypep2);  // Remember for next time
            }
        }
    }
    ಶೂನ್ಯ compute_Cpp_ಅಗಲ(Ast_ಗಂಟು* nodep) {
        ಆದರೆ (!nodep->user2() && nodep->hasDType()) {
            ಆದರೆ (VN_IS(nodep, Var)  //
                || VN_IS(nodep, ಗಂಟು_DType)  // Don't want to change variable widths!
                || VN_IS(nodep, ಗಂಟು_Sel)  // Array selects should reflect variable widths
                || VN_IS(nodep, SFormatArg)  // Retain the logical argument type, not storage width
                || VN_IS(nodep->dtypep()->skipRefp(), AssocArrayDType)  // Or arrays
                || VN_IS(nodep->dtypep()->skipRefp(), WildcardArrayDType)
                || VN_IS(nodep->dtypep()->skipRefp(), DynArrayDType)
                || VN_IS(nodep->dtypep()->skipRefp(), ClassRefDType)
                || VN_IS(nodep->dtypep()->skipRefp(), ಸರತಿ_DType)
                || VN_IS(nodep->dtypep()->skipRefp(), ಹರಿವು_DType)
                || VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)
                || VN_IS(nodep->dtypep()->skipRefp(), CoverCrossDType)
                || VN_IS(nodep->dtypep()->skipRefp(), CoverpointDType)
                || VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) {
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ dtypep
                    = VN_CAST(nodep->dtypep()->skipRefp(), ಗಂಟು_UOr_Struct_DType);
                ಆದರೆ (!dtypep || dtypep->packed()) ಗಣ_Cpp_ಅಗಲ(nodep);
            }
        }
    }

    // Store the clean state in the userp on each node
    ಶೂನ್ಯ ಗಣ_Clean_State(Ast_ಗಂಟು* nodep, CleanState clean) { nodep->user1(clean); }
    CleanState getCleanState(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        ಹಿಂತಿರುಗಿ static_cast<CleanState>(nodep->user1());
    }
    ತಾರ್ಕಿಕ isClean(Ast_ಗಂಟು* nodep) {
        ಸ್ಥಿರಾಂಕ CleanState clstate = getCleanState(nodep);
        ಆದರೆ (clstate == CS_CLEAN) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಆದರೆ (clstate == CS_DIRTY) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        nodep->v3fatalSrc("Unknown clean state on node: " + nodep->prettyTypeName());
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    ಶೂನ್ಯ ಗಣ_Clean(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ isClean) {
        compute_Cpp_ಅಗಲ(nodep);  // Just to be sure it's in widthMin
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಸಂಪೂರ್ಣ_Uint
            = (nodep->ಅಗಲ_Min() == VL_IDATASIZE || nodep->ಅಗಲ_Min() == VL_QUADSIZE
               || (nodep->ಅಗಲ_Min() % VL_EDATASIZE) == 0);
        ಗಣ_Clean_State(nodep, ((isClean || ಸಂಪೂರ್ಣ_Uint) ? CS_CLEAN : CS_DIRTY));
    }

    // Operate on nodes
    ಶೂನ್ಯ ಸೇರಿಸು_Clean(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {  // We'll insert ABOVE passed node
        UINFO(4, "  NeedClean " << nodep);
        VNRelinker relinkHandle;
        nodep->unlink_Fr_ಹಿಂಭಾಗ(&relinkHandle);
        //
        compute_Cpp_ಅಗಲ(nodep);
        V3_ಸಂಖ್ಯೆ ಮುಖವಾಡ{nodep, cpp_ಅಗಲ(nodep)};
        ಮುಖವಾಡ.ಗಣ_ಮುಖವಾಡ(nodep->ಅಗಲ_Min());
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ cleanp
            = ಹೊಸ AstAnd{nodep->fileline(), ಹೊಸ AstConst{nodep->fileline(), ಮುಖವಾಡ}, nodep};
        cleanp->dtypeFrom(nodep);  // Otherwise the AND normally picks LHS
        relinkHandle.relink(cleanp);
    }
    ಶೂನ್ಯ ensureClean(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        compute_Cpp_ಅಗಲ(nodep);
        ಆದರೆ (!isClean(nodep)) ಸೇರಿಸು_Clean(nodep);
    }
    ಶೂನ್ಯ ensure_Clean_And_ಮುಂದಿನ(Ast_ಗಂಟು* nodep) {
        // Editing list, careful looping!
        ಗಾಗಿ (Ast_ಗಂಟು* argp = nodep; argp;) {
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nextp = argp->nextp();
            ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = VN_CAST(argp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) ensureClean(exprp);
            argp = nextp;
        }
    }

    // Base type handling methods
    ಶೂನ್ಯ ಕಾರ್ಯಾಂಶ_Biop(Ast_ಗಂಟು_Biop* nodep) {
        iterateChildren(nodep);
        compute_Cpp_ಅಗಲ(nodep);
        ಆದರೆ (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        ಆದರೆ (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        // no setClean.. must do it in each user routine.
    }
    ಶೂನ್ಯ ಕಾರ್ಯಾಂಶ_Triop(Ast_ಗಂಟು_Triop* nodep) {
        iterateChildren(nodep);
        compute_Cpp_ಅಗಲ(nodep);
        ಆದರೆ (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        ಆದರೆ (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        ಆದರೆ (nodep->cleanThs()) ensureClean(nodep->thsp());
        // no setClean.. must do it in each user routine.
    }
    ಶೂನ್ಯ ಕಾರ್ಯಾಂಶ_Quadop(Ast_ಗಂಟು_Quadop* nodep) {
        iterateChildren(nodep);
        compute_Cpp_ಅಗಲ(nodep);
        ಆದರೆ (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        ಆದರೆ (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        ಆದರೆ (nodep->cleanThs()) ensureClean(nodep->thsp());
        ಆದರೆ (nodep->cleanFhs()) ensureClean(nodep->fhsp());
        // no setClean.. must do it in each user routine.
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Uniop* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_ಅಗಲ(nodep);
        ಆದರೆ (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        ಗಣ_Clean(nodep, nodep->clean_ಹೊರಹೋಗು());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Biop* nodep) override {
        ಕಾರ್ಯಾಂಶ_Biop(nodep);
        ಗಣ_Clean(nodep, nodep->clean_ಹೊರಹೋಗು());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAnd* nodep) override {
        ಕಾರ್ಯಾಂಶ_Biop(nodep);
        ಗಣ_Clean(nodep, isClean(nodep->lhsp()) || isClean(nodep->rhsp()));
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstXor* nodep) override {
        ಕಾರ್ಯಾಂಶ_Biop(nodep);
        ಗಣ_Clean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstOr* nodep) override {
        ಕಾರ್ಯಾಂಶ_Biop(nodep);
        ಗಣ_Clean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Quadop* nodep) override {
        ಕಾರ್ಯಾಂಶ_Quadop(nodep);
        ಗಣ_Clean(nodep, nodep->clean_ಹೊರಹೋಗು());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_ಅಗಲ(nodep);
        ಗಣ_Clean(nodep, isClean(nodep->resultp()));
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_ಅಗಲ(nodep);
        ಗಣ_Clean(nodep, nodep->clean_ಹೊರಹೋಗು());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_ಅಗಲ(nodep);
        ಆದರೆ (nodep->cleanRhs()) ensureClean(nodep->rhsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstScopeName* nodep) override {  //
        ಗಣ_Clean(nodep, ನಿಜ);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCNew* nodep) override {
        iterateChildren(nodep);
        ಗಣ_Clean(nodep, ನಿಜ);
    }
    // Constructor values are stored, so must be clean
    ಶೂನ್ಯ ಭೇಟಿ(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (nodep->defaultp()) ensureClean(nodep->defaultp());
        ಗಣ_Clean(nodep, ನಿಜ);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (nodep->defaultp()) ensureClean(nodep->defaultp());
        ಗಣ_Clean(nodep, ನಿಜ);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (nodep->defaultp()) ensureClean(nodep->defaultp());
        ಗಣ_Clean(nodep, ನಿಜ);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Cons_ಅಂಶ* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->valuep());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಹೊಂದಿಕೆ_Masked* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        ಗಣ_Clean(nodep, ನಿಜ);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSel* nodep) override {
        ಕಾರ್ಯಾಂಶ_Biop(nodep);
        ಗಣ_Clean(nodep, nodep->clean_ಹೊರಹೋಗು());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_ಅಗಲ(nodep);
        ಗಣ_Clean(nodep, ಸುಳ್ಳು);
        // We always clean, as we don't trust those pesky users.
        ಆದರೆ (!VN_IS(nodep->backp(), And)) ಸೇರಿಸು_Clean(nodep);
        ensure_Clean_And_ಮುಂದಿನ(nodep->nodesp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstTraceDecl* nodep) override {}  // Nothing to do here
    ಶೂನ್ಯ ಭೇಟಿ(AstTraceInc* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_ಮುಂದಿನ(nodep->valuep());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstTypedef* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstParamTypeDType* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }

    // Control flow operators
    ಶೂನ್ಯ ಭೇಟಿ(AstCond* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
        ಗಣ_Clean(nodep, isClean(nodep->thenp()) && isClean(nodep->elsep()));
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstLoop* nodep) override { iterateChildren(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstLoopTest* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_If* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSFormatArg* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_ಮುಂದಿನ(nodep->exprp());
        ಗಣ_Clean(nodep, ನಿಜ);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_ಮುಂದಿನ(nodep->exprsp());
        ಗಣ_Clean(nodep, ನಿಜ);  // generates a string, so not relevant
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCStmt* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_ಮುಂದಿನ(nodep->nodesp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCStmtUser* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_ಮುಂದಿನ(nodep->nodesp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_CCall* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_ಮುಂದಿನ(nodep->argsp());
        ಗಣ_Clean(nodep, ನಿಜ);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_ಮುಂದಿನ(nodep->pinsp());
        ಗಣ_Clean(nodep, nodep->clean_ಹೊರಹೋಗು());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstWith* nodep) override {
        iterateChildren(nodep);
        ಗಣ_Clean(nodep, ನಿಜ);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCReturn* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        ಗಣ_Clean(nodep, ನಿಜ);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstIntfRef* nodep) override {
        iterateChildren(nodep);
        ಗಣ_Clean(nodep, ನಿಜ);  // generates a string, so not relevant
    }

    //--------------------
    // Default: Just iterate
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_ಅಗಲ(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ CleanVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CleanVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Clean class functions

ಶೂನ್ಯ V3Clean::cleanAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CleanVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("clean", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
