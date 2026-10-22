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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Clean.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clean state, as a visitor of each AstNode

ਵਰਗ CleanVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    // Entire netlist:
    //  AstNode::user1()        -> CleanState.  For this node, 0==UNKNOWN
    //  AstNode::user2()        -> bool.  True indicates widthMin has been propagated
    //  AstNodeDType::user3()   -> AstNodeDType*.  Alternative node with C size
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_inuser2;
    ਅਚਲ VNUser3_ਵਿੱਚ_Use m_inuser3;

    // TYPES
    ਗਿਣਤੀ CleanState : uint8_t { CS_UNKNOWN, CS_CLEAN, CS_DIRTY };

    // STATE - for current visit position (use VL_RESTORER)
    ਅਚਲ Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;

    // METHODS

    // Width resetting
    ਪੂਰਨ cpp_ਚੌੜਾਈ(ਅਚਲ Ast_ਗੰਢ* nodep) {
        ਜੇ (nodep->ਚੌੜਾਈ() <= VL_IDATASIZE) {
            ਵਾਪਸ VL_IDATASIZE;
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->ਚੌੜਾਈ() <= VL_QUADSIZE) {
            ਵਾਪਸ VL_QUADSIZE;
        } ਨਹੀਂਤਾਂ {
            ਵਾਪਸ nodep->ਚੌੜਾਈ_ਸ਼ਬਦ() * VL_EDATASIZE;
        }
    }
    ਖਾਲੀ ਸੈੱਟ_Cpp_ਚੌੜਾਈ(Ast_ਗੰਢ* nodep) {
        nodep->user2(ਸੱਚ);  // Don't resize it again
        Ast_ਗੰਢ_DType* ਅਚਲ old_dtypep = nodep->dtypep();
        ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = cpp_ਚੌੜਾਈ(nodep);  // widthMin is unchanged
        ਜੇ (old_dtypep->ਚੌੜਾਈ() != ਚੌੜਾਈ) {
            // Since any given dtype's cppWidth() is the same, we can just
            // remember one conversion for each, and reuse it
            ਜੇ (Ast_ਗੰਢ_DType* ਅਚਲ new_dtypep = VN_CAST(old_dtypep->user3p(), ਗੰਢ_DType)) {
                nodep->dtypep(new_dtypep);
            } ਨਹੀਂਤਾਂ {
                nodep->dtype_Chg_ਚੌੜਾਈ(ਚੌੜਾਈ, nodep->ਚੌੜਾਈ_Min());
                Ast_ਗੰਢ_DType* ਅਚਲ new_dtypep2 = nodep->dtypep();
                UASSERT_OBJ(new_dtypep2 != old_dtypep, nodep,
                            "Dtype didn't change when width changed");
                old_dtypep->user3p(new_dtypep2);  // Remember for next time
            }
        }
    }
    ਖਾਲੀ compute_Cpp_ਚੌੜਾਈ(Ast_ਗੰਢ* nodep) {
        ਜੇ (!nodep->user2() && nodep->hasDType()) {
            ਜੇ (VN_IS(nodep, Var)  //
                || VN_IS(nodep, ਗੰਢ_DType)  // Don't want to change variable widths!
                || VN_IS(nodep, ਗੰਢ_Sel)  // Array selects should reflect variable widths
                || VN_IS(nodep, SFormatArg)  // Retain the logical argument type, not storage width
                || VN_IS(nodep->dtypep()->skipRefp(), AssocArrayDType)  // Or arrays
                || VN_IS(nodep->dtypep()->skipRefp(), WildcardArrayDType)
                || VN_IS(nodep->dtypep()->skipRefp(), DynArrayDType)
                || VN_IS(nodep->dtypep()->skipRefp(), ClassRefDType)
                || VN_IS(nodep->dtypep()->skipRefp(), ਕਤਾਰ_DType)
                || VN_IS(nodep->dtypep()->skipRefp(), ਧਾਰਾ_DType)
                || VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)
                || VN_IS(nodep->dtypep()->skipRefp(), CoverCrossDType)
                || VN_IS(nodep->dtypep()->skipRefp(), CoverpointDType)
                || VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) {
            } ਨਹੀਂਤਾਂ {
                ਅਚਲ Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ dtypep
                    = VN_CAST(nodep->dtypep()->skipRefp(), ਗੰਢ_UOr_Struct_DType);
                ਜੇ (!dtypep || dtypep->packed()) ਸੈੱਟ_Cpp_ਚੌੜਾਈ(nodep);
            }
        }
    }

    // Store the clean state in the userp on each node
    ਖਾਲੀ ਸੈੱਟ_Clean_State(Ast_ਗੰਢ* nodep, CleanState clean) { nodep->user1(clean); }
    CleanState getCleanState(ਅਚਲ Ast_ਗੰਢ* nodep) {
        ਵਾਪਸ static_cast<CleanState>(nodep->user1());
    }
    ਤਾਰਕਿਕ isClean(Ast_ਗੰਢ* nodep) {
        ਅਚਲ CleanState clstate = getCleanState(nodep);
        ਜੇ (clstate == CS_CLEAN) ਵਾਪਸ ਸੱਚ;
        ਜੇ (clstate == CS_DIRTY) ਵਾਪਸ ਝੂਠ;
        nodep->v3fatalSrc("Unknown clean state on node: " + nodep->prettyTypeName());
        ਵਾਪਸ ਝੂਠ;
    }
    ਖਾਲੀ ਸੈੱਟ_Clean(Ast_ਗੰਢ* nodep, ਤਾਰਕਿਕ isClean) {
        compute_Cpp_ਚੌੜਾਈ(nodep);  // Just to be sure it's in widthMin
        ਅਚਲ ਤਾਰਕਿਕ ਸਾਰਾ_Uint
            = (nodep->ਚੌੜਾਈ_Min() == VL_IDATASIZE || nodep->ਚੌੜਾਈ_Min() == VL_QUADSIZE
               || (nodep->ਚੌੜਾਈ_Min() % VL_EDATASIZE) == 0);
        ਸੈੱਟ_Clean_State(nodep, ((isClean || ਸਾਰਾ_Uint) ? CS_CLEAN : CS_DIRTY));
    }

    // Operate on nodes
    ਖਾਲੀ ਪਾਓ_Clean(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {  // We'll insert ABOVE passed node
        UINFO(4, "  NeedClean " << nodep);
        VNRelinker relinkHandle;
        nodep->unlink_Fr_ਪਿੱਠ(&relinkHandle);
        //
        compute_Cpp_ਚੌੜਾਈ(nodep);
        V3_ਸੰਖਿਆ ਨਕਾਬ{nodep, cpp_ਚੌੜਾਈ(nodep)};
        ਨਕਾਬ.ਸੈੱਟ_ਨਕਾਬ(nodep->ਚੌੜਾਈ_Min());
        Ast_ਗੰਢ* ਅਚਲ cleanp
            = ਨਵਾਂ AstAnd{nodep->fileline(), ਨਵਾਂ AstConst{nodep->fileline(), ਨਕਾਬ}, nodep};
        cleanp->dtypeFrom(nodep);  // Otherwise the AND normally picks LHS
        relinkHandle.relink(cleanp);
    }
    ਖਾਲੀ ensureClean(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        compute_Cpp_ਚੌੜਾਈ(nodep);
        ਜੇ (!isClean(nodep)) ਪਾਓ_Clean(nodep);
    }
    ਖਾਲੀ ensure_Clean_And_ਅਗਲਾ(Ast_ਗੰਢ* nodep) {
        // Editing list, careful looping!
        ਲਈ (Ast_ਗੰਢ* argp = nodep; argp;) {
            Ast_ਗੰਢ* ਅਚਲ nextp = argp->nextp();
            ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = VN_CAST(argp, ਗੰਢ_ਵਿਅੰਜਕ)) ensureClean(exprp);
            argp = nextp;
        }
    }

    // Base type handling methods
    ਖਾਲੀ ਸੰਕਾਰਜ_Biop(Ast_ਗੰਢ_Biop* nodep) {
        iterateChildren(nodep);
        compute_Cpp_ਚੌੜਾਈ(nodep);
        ਜੇ (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        ਜੇ (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        // no setClean.. must do it in each user routine.
    }
    ਖਾਲੀ ਸੰਕਾਰਜ_Triop(Ast_ਗੰਢ_Triop* nodep) {
        iterateChildren(nodep);
        compute_Cpp_ਚੌੜਾਈ(nodep);
        ਜੇ (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        ਜੇ (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        ਜੇ (nodep->cleanThs()) ensureClean(nodep->thsp());
        // no setClean.. must do it in each user routine.
    }
    ਖਾਲੀ ਸੰਕਾਰਜ_Quadop(Ast_ਗੰਢ_Quadop* nodep) {
        iterateChildren(nodep);
        compute_Cpp_ਚੌੜਾਈ(nodep);
        ਜੇ (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        ਜੇ (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        ਜੇ (nodep->cleanThs()) ensureClean(nodep->thsp());
        ਜੇ (nodep->cleanFhs()) ensureClean(nodep->fhsp());
        // no setClean.. must do it in each user routine.
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Uniop* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_ਚੌੜਾਈ(nodep);
        ਜੇ (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        ਸੈੱਟ_Clean(nodep, nodep->clean_ਬਾਹਰਲਾ());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Biop* nodep) override {
        ਸੰਕਾਰਜ_Biop(nodep);
        ਸੈੱਟ_Clean(nodep, nodep->clean_ਬਾਹਰਲਾ());
    }
    ਖਾਲੀ ਦੇਖੋ(AstAnd* nodep) override {
        ਸੰਕਾਰਜ_Biop(nodep);
        ਸੈੱਟ_Clean(nodep, isClean(nodep->lhsp()) || isClean(nodep->rhsp()));
    }
    ਖਾਲੀ ਦੇਖੋ(AstXor* nodep) override {
        ਸੰਕਾਰਜ_Biop(nodep);
        ਸੈੱਟ_Clean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    ਖਾਲੀ ਦੇਖੋ(AstOr* nodep) override {
        ਸੰਕਾਰਜ_Biop(nodep);
        ਸੈੱਟ_Clean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Quadop* nodep) override {
        ਸੰਕਾਰਜ_Quadop(nodep);
        ਸੈੱਟ_Clean(nodep, nodep->clean_ਬਾਹਰਲਾ());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਵਿਅੰਜਕ_Stmt* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_ਚੌੜਾਈ(nodep);
        ਸੈੱਟ_Clean(nodep, isClean(nodep->resultp()));
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_ਚੌੜਾਈ(nodep);
        ਸੈੱਟ_Clean(nodep, nodep->clean_ਬਾਹਰਲਾ());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_ਚੌੜਾਈ(nodep);
        ਜੇ (nodep->cleanRhs()) ensureClean(nodep->rhsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstScopeName* nodep) override {  //
        ਸੈੱਟ_Clean(nodep, ਸੱਚ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCNew* nodep) override {
        iterateChildren(nodep);
        ਸੈੱਟ_Clean(nodep, ਸੱਚ);
    }
    // Constructor values are stored, so must be clean
    ਖਾਲੀ ਦੇਖੋ(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        ਜੇ (nodep->defaultp()) ensureClean(nodep->defaultp());
        ਸੈੱਟ_Clean(nodep, ਸੱਚ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        ਜੇ (nodep->defaultp()) ensureClean(nodep->defaultp());
        ਸੈੱਟ_Clean(nodep, ਸੱਚ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        ਜੇ (nodep->defaultp()) ensureClean(nodep->defaultp());
        ਸੈੱਟ_Clean(nodep, ਸੱਚ);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Cons_ਵਸਤੂ* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->valuep());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਮੇਲ_Masked* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        ਸੈੱਟ_Clean(nodep, ਸੱਚ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstSel* nodep) override {
        ਸੰਕਾਰਜ_Biop(nodep);
        ਸੈੱਟ_Clean(nodep, nodep->clean_ਬਾਹਰਲਾ());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_ਚੌੜਾਈ(nodep);
        ਸੈੱਟ_Clean(nodep, ਝੂਠ);
        // We always clean, as we don't trust those pesky users.
        ਜੇ (!VN_IS(nodep->backp(), And)) ਪਾਓ_Clean(nodep);
        ensure_Clean_And_ਅਗਲਾ(nodep->nodesp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstTraceDecl* nodep) override {}  // Nothing to do here
    ਖਾਲੀ ਦੇਖੋ(AstTraceInc* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_ਅਗਲਾ(nodep->valuep());
    }
    ਖਾਲੀ ਦੇਖੋ(AstTypedef* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstParamTypeDType* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }

    // Control flow operators
    ਖਾਲੀ ਦੇਖੋ(AstCond* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
        ਸੈੱਟ_Clean(nodep, isClean(nodep->thenp()) && isClean(nodep->elsep()));
    }
    ਖਾਲੀ ਦੇਖੋ(AstLoop* nodep) override { iterateChildren(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstLoopTest* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_If* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstSFormatArg* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_ਅਗਲਾ(nodep->exprp());
        ਸੈੱਟ_Clean(nodep, ਸੱਚ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_ਅਗਲਾ(nodep->exprsp());
        ਸੈੱਟ_Clean(nodep, ਸੱਚ);  // generates a string, so not relevant
    }
    ਖਾਲੀ ਦੇਖੋ(AstCStmt* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_ਅਗਲਾ(nodep->nodesp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCStmtUser* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_ਅਗਲਾ(nodep->nodesp());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_CCall* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_ਅਗਲਾ(nodep->argsp());
        ਸੈੱਟ_Clean(nodep, ਸੱਚ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_ਅਗਲਾ(nodep->pinsp());
        ਸੈੱਟ_Clean(nodep, nodep->clean_ਬਾਹਰਲਾ());
    }
    ਖਾਲੀ ਦੇਖੋ(AstWith* nodep) override {
        iterateChildren(nodep);
        ਸੈੱਟ_Clean(nodep, ਸੱਚ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCReturn* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        ਸੈੱਟ_Clean(nodep, ਸੱਚ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstIntfRef* nodep) override {
        iterateChildren(nodep);
        ਸੈੱਟ_Clean(nodep, ਸੱਚ);  // generates a string, so not relevant
    }

    //--------------------
    // Default: Just iterate
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_ਚੌੜਾਈ(nodep);
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ CleanVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CleanVisitor() override = ਮੂਲ;
};

//######################################################################
// Clean class functions

ਖਾਲੀ V3Clean::cleanAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CleanVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("clean", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
