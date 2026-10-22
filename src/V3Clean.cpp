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

جماعت CleanVisitor final : عوامی VNVisitor {
    // NODE STATE
    // Entire netlist:
    //  AstNode::user1()        -> CleanState.  For this node, 0==UNKNOWN
    //  AstNode::user2()        -> bool.  True indicates widthMin has been propagated
    //  AstNodeDType::user3()   -> AstNodeDType*.  Alternative node with C size
    ثابت VNUser1_اندر_Use m_inuser1;
    ثابت VNUser2_اندر_Use m_inuser2;
    ثابت VNUser3_اندر_Use m_inuser3;

    // TYPES
    شمار CleanState : uint8_t { CS_UNKNOWN, CS_CLEAN, CS_DIRTY };

    // STATE - for current visit position (use VL_RESTORER)
    ثابت Ast_گرہ_Module* m_modp = خالی_اشاریہ;

    // METHODS

    // Width resetting
    صحیح cpp_چوڑائی(ثابت Ast_گرہ* nodep) {
        اگر (nodep->چوڑائی() <= VL_IDATASIZE) {
            واپس VL_IDATASIZE;
        } ورنہ اگر (nodep->چوڑائی() <= VL_QUADSIZE) {
            واپس VL_QUADSIZE;
        } ورنہ {
            واپس nodep->چوڑائی_الفاظ() * VL_EDATASIZE;
        }
    }
    خالی سیٹ_Cpp_چوڑائی(Ast_گرہ* nodep) {
        nodep->user2(سچ);  // Don't resize it again
        Ast_گرہ_DType* ثابت old_dtypep = nodep->dtypep();
        ثابت صحیح چوڑائی = cpp_چوڑائی(nodep);  // widthMin is unchanged
        اگر (old_dtypep->چوڑائی() != چوڑائی) {
            // Since any given dtype's cppWidth() is the same, we can just
            // remember one conversion for each, and reuse it
            اگر (Ast_گرہ_DType* ثابت new_dtypep = VN_CAST(old_dtypep->user3p(), گرہ_DType)) {
                nodep->dtypep(new_dtypep);
            } ورنہ {
                nodep->dtype_Chg_چوڑائی(چوڑائی, nodep->چوڑائی_Min());
                Ast_گرہ_DType* ثابت new_dtypep2 = nodep->dtypep();
                UASSERT_OBJ(new_dtypep2 != old_dtypep, nodep,
                            "Dtype didn't change when width changed");
                old_dtypep->user3p(new_dtypep2);  // Remember for next time
            }
        }
    }
    خالی compute_Cpp_چوڑائی(Ast_گرہ* nodep) {
        اگر (!nodep->user2() && nodep->hasDType()) {
            اگر (VN_IS(nodep, Var)  //
                || VN_IS(nodep, گرہ_DType)  // Don't want to change variable widths!
                || VN_IS(nodep, گرہ_Sel)  // Array selects should reflect variable widths
                || VN_IS(nodep, SFormatArg)  // Retain the logical argument type, not storage width
                || VN_IS(nodep->dtypep()->skipRefp(), AssocArrayDType)  // Or arrays
                || VN_IS(nodep->dtypep()->skipRefp(), WildcardArrayDType)
                || VN_IS(nodep->dtypep()->skipRefp(), DynArrayDType)
                || VN_IS(nodep->dtypep()->skipRefp(), ClassRefDType)
                || VN_IS(nodep->dtypep()->skipRefp(), قطار_DType)
                || VN_IS(nodep->dtypep()->skipRefp(), دھارا_DType)
                || VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)
                || VN_IS(nodep->dtypep()->skipRefp(), CoverCrossDType)
                || VN_IS(nodep->dtypep()->skipRefp(), CoverpointDType)
                || VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) {
            } ورنہ {
                ثابت Ast_گرہ_UOr_Struct_DType* ثابت dtypep
                    = VN_CAST(nodep->dtypep()->skipRefp(), گرہ_UOr_Struct_DType);
                اگر (!dtypep || dtypep->packed()) سیٹ_Cpp_چوڑائی(nodep);
            }
        }
    }

    // Store the clean state in the userp on each node
    خالی سیٹ_Clean_State(Ast_گرہ* nodep, CleanState clean) { nodep->user1(clean); }
    CleanState getCleanState(ثابت Ast_گرہ* nodep) {
        واپس static_cast<CleanState>(nodep->user1());
    }
    منطقی isClean(Ast_گرہ* nodep) {
        ثابت CleanState clstate = getCleanState(nodep);
        اگر (clstate == CS_CLEAN) واپس سچ;
        اگر (clstate == CS_DIRTY) واپس جھوٹ;
        nodep->v3fatalSrc("Unknown clean state on node: " + nodep->prettyTypeName());
        واپس جھوٹ;
    }
    خالی سیٹ_Clean(Ast_گرہ* nodep, منطقی isClean) {
        compute_Cpp_چوڑائی(nodep);  // Just to be sure it's in widthMin
        ثابت منطقی پورا_Uint
            = (nodep->چوڑائی_Min() == VL_IDATASIZE || nodep->چوڑائی_Min() == VL_QUADSIZE
               || (nodep->چوڑائی_Min() % VL_EDATASIZE) == 0);
        سیٹ_Clean_State(nodep, ((isClean || پورا_Uint) ? CS_CLEAN : CS_DIRTY));
    }

    // Operate on nodes
    خالی داخل_کرو_Clean(Ast_گرہ_اظہاریہ* nodep) {  // We'll insert ABOVE passed node
        UINFO(4, "  NeedClean " << nodep);
        VNRelinker relinkHandle;
        nodep->unlink_Fr_عقب(&relinkHandle);
        //
        compute_Cpp_چوڑائی(nodep);
        V3_نمبر نقاب{nodep, cpp_چوڑائی(nodep)};
        نقاب.سیٹ_نقاب(nodep->چوڑائی_Min());
        Ast_گرہ* ثابت cleanp
            = نیا AstAnd{nodep->fileline(), نیا AstConst{nodep->fileline(), نقاب}, nodep};
        cleanp->dtypeFrom(nodep);  // Otherwise the AND normally picks LHS
        relinkHandle.relink(cleanp);
    }
    خالی ensureClean(Ast_گرہ_اظہاریہ* nodep) {
        compute_Cpp_چوڑائی(nodep);
        اگر (!isClean(nodep)) داخل_کرو_Clean(nodep);
    }
    خالی ensure_Clean_And_اگلا(Ast_گرہ* nodep) {
        // Editing list, careful looping!
        برائے (Ast_گرہ* argp = nodep; argp;) {
            Ast_گرہ* ثابت nextp = argp->nextp();
            اگر (Ast_گرہ_اظہاریہ* ثابت exprp = VN_CAST(argp, گرہ_اظہاریہ)) ensureClean(exprp);
            argp = nextp;
        }
    }

    // Base type handling methods
    خالی عملیہ_Biop(Ast_گرہ_Biop* nodep) {
        iterateChildren(nodep);
        compute_Cpp_چوڑائی(nodep);
        اگر (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        اگر (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        // no setClean.. must do it in each user routine.
    }
    خالی عملیہ_Triop(Ast_گرہ_Triop* nodep) {
        iterateChildren(nodep);
        compute_Cpp_چوڑائی(nodep);
        اگر (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        اگر (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        اگر (nodep->cleanThs()) ensureClean(nodep->thsp());
        // no setClean.. must do it in each user routine.
    }
    خالی عملیہ_Quadop(Ast_گرہ_Quadop* nodep) {
        iterateChildren(nodep);
        compute_Cpp_چوڑائی(nodep);
        اگر (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        اگر (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        اگر (nodep->cleanThs()) ensureClean(nodep->thsp());
        اگر (nodep->cleanFhs()) ensureClean(nodep->fhsp());
        // no setClean.. must do it in each user routine.
    }

    // VISITORS
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ_Uniop* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_چوڑائی(nodep);
        اگر (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        سیٹ_Clean(nodep, nodep->clean_برآمد());
    }
    خالی دورہ(Ast_گرہ_Biop* nodep) override {
        عملیہ_Biop(nodep);
        سیٹ_Clean(nodep, nodep->clean_برآمد());
    }
    خالی دورہ(AstAnd* nodep) override {
        عملیہ_Biop(nodep);
        سیٹ_Clean(nodep, isClean(nodep->lhsp()) || isClean(nodep->rhsp()));
    }
    خالی دورہ(AstXor* nodep) override {
        عملیہ_Biop(nodep);
        سیٹ_Clean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    خالی دورہ(AstOr* nodep) override {
        عملیہ_Biop(nodep);
        سیٹ_Clean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    خالی دورہ(Ast_گرہ_Quadop* nodep) override {
        عملیہ_Quadop(nodep);
        سیٹ_Clean(nodep, nodep->clean_برآمد());
    }
    خالی دورہ(Ast_اظہاریہ_Stmt* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_چوڑائی(nodep);
        سیٹ_Clean(nodep, isClean(nodep->resultp()));
    }
    خالی دورہ(Ast_گرہ_اظہاریہ* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_چوڑائی(nodep);
        سیٹ_Clean(nodep, nodep->clean_برآمد());
    }
    خالی دورہ(Ast_گرہ_تفویض* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_چوڑائی(nodep);
        اگر (nodep->cleanRhs()) ensureClean(nodep->rhsp());
    }
    خالی دورہ(AstScopeName* nodep) override {  //
        سیٹ_Clean(nodep, سچ);
    }
    خالی دورہ(AstCNew* nodep) override {
        iterateChildren(nodep);
        سیٹ_Clean(nodep, سچ);
    }
    // Constructor values are stored, so must be clean
    خالی دورہ(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        اگر (nodep->defaultp()) ensureClean(nodep->defaultp());
        سیٹ_Clean(nodep, سچ);
    }
    خالی دورہ(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        اگر (nodep->defaultp()) ensureClean(nodep->defaultp());
        سیٹ_Clean(nodep, سچ);
    }
    خالی دورہ(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        اگر (nodep->defaultp()) ensureClean(nodep->defaultp());
        سیٹ_Clean(nodep, سچ);
    }
    خالی دورہ(Ast_گرہ_Cons_شے* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->valuep());
    }
    خالی دورہ(Ast_مطابقت_Masked* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        سیٹ_Clean(nodep, سچ);
    }
    خالی دورہ(AstSel* nodep) override {
        عملیہ_Biop(nodep);
        سیٹ_Clean(nodep, nodep->clean_برآمد());
    }
    خالی دورہ(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_چوڑائی(nodep);
        سیٹ_Clean(nodep, جھوٹ);
        // We always clean, as we don't trust those pesky users.
        اگر (!VN_IS(nodep->backp(), And)) داخل_کرو_Clean(nodep);
        ensure_Clean_And_اگلا(nodep->nodesp());
    }
    خالی دورہ(AstTraceDecl* nodep) override {}  // Nothing to do here
    خالی دورہ(AstTraceInc* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_اگلا(nodep->valuep());
    }
    خالی دورہ(AstTypedef* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }
    خالی دورہ(AstParamTypeDType* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }

    // Control flow operators
    خالی دورہ(AstCond* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
        سیٹ_Clean(nodep, isClean(nodep->thenp()) && isClean(nodep->elsep()));
    }
    خالی دورہ(AstLoop* nodep) override { iterateChildren(nodep); }
    خالی دورہ(AstLoopTest* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    خالی دورہ(Ast_گرہ_If* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    خالی دورہ(AstSFormatArg* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_اگلا(nodep->exprp());
        سیٹ_Clean(nodep, سچ);
    }
    خالی دورہ(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_اگلا(nodep->exprsp());
        سیٹ_Clean(nodep, سچ);  // generates a string, so not relevant
    }
    خالی دورہ(AstCStmt* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_اگلا(nodep->nodesp());
    }
    خالی دورہ(AstCStmtUser* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_اگلا(nodep->nodesp());
    }
    خالی دورہ(Ast_گرہ_CCall* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_اگلا(nodep->argsp());
        سیٹ_Clean(nodep, سچ);
    }
    خالی دورہ(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_اگلا(nodep->pinsp());
        سیٹ_Clean(nodep, nodep->clean_برآمد());
    }
    خالی دورہ(AstWith* nodep) override {
        iterateChildren(nodep);
        سیٹ_Clean(nodep, سچ);
    }
    خالی دورہ(AstCReturn* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        سیٹ_Clean(nodep, سچ);
    }
    خالی دورہ(AstIntfRef* nodep) override {
        iterateChildren(nodep);
        سیٹ_Clean(nodep, سچ);  // generates a string, so not relevant
    }

    //--------------------
    // Default: Just iterate
    خالی دورہ(Ast_گرہ* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_چوڑائی(nodep);
    }

عوامی:
    // CONSTRUCTORS
    صریح CleanVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CleanVisitor() override = طےشدہ;
};

//######################################################################
// Clean class functions

خالی V3Clean::cleanAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CleanVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("clean", 0, dump_درخت_Either_Level() >= 3);
}
