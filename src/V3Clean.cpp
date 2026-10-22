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
    ثابت VNUser1_اندريون_Use m_inuser1;
    ثابت VNUser2_اندريون_Use m_inuser2;
    ثابت VNUser3_اندريون_Use m_inuser3;

    // TYPES
    تعداد CleanState : uint8_t { CS_UNKNOWN, CS_CLEAN, CS_DIRTY };

    // STATE - for current visit position (use VL_RESTORER)
    ثابت Ast_ڳنڍ_Module* m_modp = خالي_اشارو;

    // METHODS

    // Width resetting
    صحيح cpp_ويڪر(ثابت Ast_ڳنڍ* nodep) {
        إذا (nodep->ويڪر() <= VL_IDATASIZE) {
            أرجع VL_IDATASIZE;
        } وإلا إذا (nodep->ويڪر() <= VL_QUADSIZE) {
            أرجع VL_QUADSIZE;
        } وإلا {
            أرجع nodep->ويڪر_لفظ() * VL_EDATASIZE;
        }
    }
    فراغ سيٽ_Cpp_ويڪر(Ast_ڳنڍ* nodep) {
        nodep->user2(سچ);  // Don't resize it again
        Ast_ڳنڍ_DType* ثابت old_dtypep = nodep->dtypep();
        ثابت صحيح ويڪر = cpp_ويڪر(nodep);  // widthMin is unchanged
        إذا (old_dtypep->ويڪر() != ويڪر) {
            // Since any given dtype's cppWidth() is the same, we can just
            // remember one conversion for each, and reuse it
            إذا (Ast_ڳنڍ_DType* ثابت new_dtypep = VN_CAST(old_dtypep->user3p(), ڳنڍ_DType)) {
                nodep->dtypep(new_dtypep);
            } وإلا {
                nodep->dtype_Chg_ويڪر(ويڪر, nodep->ويڪر_Min());
                Ast_ڳنڍ_DType* ثابت new_dtypep2 = nodep->dtypep();
                UASSERT_OBJ(new_dtypep2 != old_dtypep, nodep,
                            "Dtype didn't change when width changed");
                old_dtypep->user3p(new_dtypep2);  // Remember for next time
            }
        }
    }
    فراغ compute_Cpp_ويڪر(Ast_ڳنڍ* nodep) {
        إذا (!nodep->user2() && nodep->hasDType()) {
            إذا (VN_IS(nodep, Var)  //
                || VN_IS(nodep, ڳنڍ_DType)  // Don't want to change variable widths!
                || VN_IS(nodep, ڳنڍ_Sel)  // Array selects should reflect variable widths
                || VN_IS(nodep, SFormatArg)  // Retain the logical argument type, not storage width
                || VN_IS(nodep->dtypep()->skipRefp(), AssocArrayDType)  // Or arrays
                || VN_IS(nodep->dtypep()->skipRefp(), WildcardArrayDType)
                || VN_IS(nodep->dtypep()->skipRefp(), DynArrayDType)
                || VN_IS(nodep->dtypep()->skipRefp(), ClassRefDType)
                || VN_IS(nodep->dtypep()->skipRefp(), قطار_DType)
                || VN_IS(nodep->dtypep()->skipRefp(), وهڪرو_DType)
                || VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)
                || VN_IS(nodep->dtypep()->skipRefp(), CoverCrossDType)
                || VN_IS(nodep->dtypep()->skipRefp(), CoverpointDType)
                || VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) {
            } وإلا {
                ثابت Ast_ڳنڍ_UOr_Struct_DType* ثابت dtypep
                    = VN_CAST(nodep->dtypep()->skipRefp(), ڳنڍ_UOr_Struct_DType);
                إذا (!dtypep || dtypep->packed()) سيٽ_Cpp_ويڪر(nodep);
            }
        }
    }

    // Store the clean state in the userp on each node
    فراغ سيٽ_Clean_State(Ast_ڳنڍ* nodep, CleanState clean) { nodep->user1(clean); }
    CleanState getCleanState(ثابت Ast_ڳنڍ* nodep) {
        أرجع static_cast<CleanState>(nodep->user1());
    }
    منطقي isClean(Ast_ڳنڍ* nodep) {
        ثابت CleanState clstate = getCleanState(nodep);
        إذا (clstate == CS_CLEAN) أرجع سچ;
        إذا (clstate == CS_DIRTY) أرجع خطأ;
        nodep->v3fatalSrc("Unknown clean state on node: " + nodep->prettyTypeName());
        أرجع خطأ;
    }
    فراغ سيٽ_Clean(Ast_ڳنڍ* nodep, منطقي isClean) {
        compute_Cpp_ويڪر(nodep);  // Just to be sure it's in widthMin
        ثابت منطقي سڄو_Uint
            = (nodep->ويڪر_Min() == VL_IDATASIZE || nodep->ويڪر_Min() == VL_QUADSIZE
               || (nodep->ويڪر_Min() % VL_EDATASIZE) == 0);
        سيٽ_Clean_State(nodep, ((isClean || سڄو_Uint) ? CS_CLEAN : CS_DIRTY));
    }

    // Operate on nodes
    فراغ وجهو_Clean(Ast_ڳنڍ_اظهار* nodep) {  // We'll insert ABOVE passed node
        UINFO(4, "  NeedClean " << nodep);
        VNRelinker relinkHandle;
        nodep->unlink_Fr_پٺ(&relinkHandle);
        //
        compute_Cpp_ويڪر(nodep);
        V3_عدد نقاب{nodep, cpp_ويڪر(nodep)};
        نقاب.سيٽ_نقاب(nodep->ويڪر_Min());
        Ast_ڳنڍ* ثابت cleanp
            = جديد AstAnd{nodep->fileline(), جديد AstConst{nodep->fileline(), نقاب}, nodep};
        cleanp->dtypeFrom(nodep);  // Otherwise the AND normally picks LHS
        relinkHandle.relink(cleanp);
    }
    فراغ ensureClean(Ast_ڳنڍ_اظهار* nodep) {
        compute_Cpp_ويڪر(nodep);
        إذا (!isClean(nodep)) وجهو_Clean(nodep);
    }
    فراغ ensure_Clean_And_ايندڙ(Ast_ڳنڍ* nodep) {
        // Editing list, careful looping!
        لكل (Ast_ڳنڍ* argp = nodep; argp;) {
            Ast_ڳنڍ* ثابت nextp = argp->nextp();
            إذا (Ast_ڳنڍ_اظهار* ثابت exprp = VN_CAST(argp, ڳنڍ_اظهار)) ensureClean(exprp);
            argp = nextp;
        }
    }

    // Base type handling methods
    فراغ عمليو_Biop(Ast_ڳنڍ_Biop* nodep) {
        iterateChildren(nodep);
        compute_Cpp_ويڪر(nodep);
        إذا (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        إذا (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        // no setClean.. must do it in each user routine.
    }
    فراغ عمليو_Triop(Ast_ڳنڍ_Triop* nodep) {
        iterateChildren(nodep);
        compute_Cpp_ويڪر(nodep);
        إذا (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        إذا (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        إذا (nodep->cleanThs()) ensureClean(nodep->thsp());
        // no setClean.. must do it in each user routine.
    }
    فراغ عمليو_Quadop(Ast_ڳنڍ_Quadop* nodep) {
        iterateChildren(nodep);
        compute_Cpp_ويڪر(nodep);
        إذا (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        إذا (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        إذا (nodep->cleanThs()) ensureClean(nodep->thsp());
        إذا (nodep->cleanFhs()) ensureClean(nodep->fhsp());
        // no setClean.. must do it in each user routine.
    }

    // VISITORS
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_Uniop* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_ويڪر(nodep);
        إذا (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        سيٽ_Clean(nodep, nodep->clean_ٻاهريون());
    }
    فراغ گهمو(Ast_ڳنڍ_Biop* nodep) override {
        عمليو_Biop(nodep);
        سيٽ_Clean(nodep, nodep->clean_ٻاهريون());
    }
    فراغ گهمو(AstAnd* nodep) override {
        عمليو_Biop(nodep);
        سيٽ_Clean(nodep, isClean(nodep->lhsp()) || isClean(nodep->rhsp()));
    }
    فراغ گهمو(AstXor* nodep) override {
        عمليو_Biop(nodep);
        سيٽ_Clean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    فراغ گهمو(AstOr* nodep) override {
        عمليو_Biop(nodep);
        سيٽ_Clean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    فراغ گهمو(Ast_ڳنڍ_Quadop* nodep) override {
        عمليو_Quadop(nodep);
        سيٽ_Clean(nodep, nodep->clean_ٻاهريون());
    }
    فراغ گهمو(Ast_اظهار_Stmt* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_ويڪر(nodep);
        سيٽ_Clean(nodep, isClean(nodep->resultp()));
    }
    فراغ گهمو(Ast_ڳنڍ_اظهار* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_ويڪر(nodep);
        سيٽ_Clean(nodep, nodep->clean_ٻاهريون());
    }
    فراغ گهمو(Ast_ڳنڍ_سونپيو* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_ويڪر(nodep);
        إذا (nodep->cleanRhs()) ensureClean(nodep->rhsp());
    }
    فراغ گهمو(AstScopeName* nodep) override {  //
        سيٽ_Clean(nodep, سچ);
    }
    فراغ گهمو(AstCNew* nodep) override {
        iterateChildren(nodep);
        سيٽ_Clean(nodep, سچ);
    }
    // Constructor values are stored, so must be clean
    فراغ گهمو(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        إذا (nodep->defaultp()) ensureClean(nodep->defaultp());
        سيٽ_Clean(nodep, سچ);
    }
    فراغ گهمو(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        إذا (nodep->defaultp()) ensureClean(nodep->defaultp());
        سيٽ_Clean(nodep, سچ);
    }
    فراغ گهمو(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        إذا (nodep->defaultp()) ensureClean(nodep->defaultp());
        سيٽ_Clean(nodep, سچ);
    }
    فراغ گهمو(Ast_ڳنڍ_Cons_شي* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->valuep());
    }
    فراغ گهمو(Ast_ميلاپ_Masked* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        سيٽ_Clean(nodep, سچ);
    }
    فراغ گهمو(AstSel* nodep) override {
        عمليو_Biop(nodep);
        سيٽ_Clean(nodep, nodep->clean_ٻاهريون());
    }
    فراغ گهمو(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_ويڪر(nodep);
        سيٽ_Clean(nodep, خطأ);
        // We always clean, as we don't trust those pesky users.
        إذا (!VN_IS(nodep->backp(), And)) وجهو_Clean(nodep);
        ensure_Clean_And_ايندڙ(nodep->nodesp());
    }
    فراغ گهمو(AstTraceDecl* nodep) override {}  // Nothing to do here
    فراغ گهمو(AstTraceInc* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_ايندڙ(nodep->valuep());
    }
    فراغ گهمو(AstTypedef* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }
    فراغ گهمو(AstParamTypeDType* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }

    // Control flow operators
    فراغ گهمو(AstCond* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
        سيٽ_Clean(nodep, isClean(nodep->thenp()) && isClean(nodep->elsep()));
    }
    فراغ گهمو(AstLoop* nodep) override { iterateChildren(nodep); }
    فراغ گهمو(AstLoopTest* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    فراغ گهمو(Ast_ڳنڍ_If* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    فراغ گهمو(AstSFormatArg* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_ايندڙ(nodep->exprp());
        سيٽ_Clean(nodep, سچ);
    }
    فراغ گهمو(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_ايندڙ(nodep->exprsp());
        سيٽ_Clean(nodep, سچ);  // generates a string, so not relevant
    }
    فراغ گهمو(AstCStmt* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_ايندڙ(nodep->nodesp());
    }
    فراغ گهمو(AstCStmtUser* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_ايندڙ(nodep->nodesp());
    }
    فراغ گهمو(Ast_ڳنڍ_CCall* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_ايندڙ(nodep->argsp());
        سيٽ_Clean(nodep, سچ);
    }
    فراغ گهمو(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_ايندڙ(nodep->pinsp());
        سيٽ_Clean(nodep, nodep->clean_ٻاهريون());
    }
    فراغ گهمو(AstWith* nodep) override {
        iterateChildren(nodep);
        سيٽ_Clean(nodep, سچ);
    }
    فراغ گهمو(AstCReturn* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        سيٽ_Clean(nodep, سچ);
    }
    فراغ گهمو(AstIntfRef* nodep) override {
        iterateChildren(nodep);
        سيٽ_Clean(nodep, سچ);  // generates a string, so not relevant
    }

    //--------------------
    // Default: Just iterate
    فراغ گهمو(Ast_ڳنڍ* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_ويڪر(nodep);
    }

عام:
    // CONSTRUCTORS
    واضح CleanVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CleanVisitor() override = افتراضي;
};

//######################################################################
// Clean class functions

فراغ V3Clean::cleanAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CleanVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_وڻ("clean", 0, dump_وڻ_Either_Level() >= 3);
}
