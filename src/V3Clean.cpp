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
    ثابت VNUser1_درون_Use m_inuser1;
    ثابت VNUser2_درون_Use m_inuser2;
    ثابت VNUser3_درون_Use m_inuser3;

    // TYPES
    تعداد CleanState : uint8_t { CS_UNKNOWN, CS_CLEAN, CS_DIRTY };

    // STATE - for current visit position (use VL_RESTORER)
    ثابت Ast_گره_Module* m_modp = اشاره_تهی;

    // METHODS

    // Width resetting
    صحيح cpp_عرض(ثابت Ast_گره* nodep) {
        إذا (nodep->عرض() <= VL_IDATASIZE) {
            أرجع VL_IDATASIZE;
        } وإلا إذا (nodep->عرض() <= VL_QUADSIZE) {
            أرجع VL_QUADSIZE;
        } وإلا {
            أرجع nodep->عرض_واژگان() * VL_EDATASIZE;
        }
    }
    فراغ مجموعه_Cpp_عرض(Ast_گره* nodep) {
        nodep->user2(درست);  // Don't resize it again
        Ast_گره_DType* ثابت old_dtypep = nodep->dtypep();
        ثابت صحيح عرض = cpp_عرض(nodep);  // widthMin is unchanged
        إذا (old_dtypep->عرض() != عرض) {
            // Since any given dtype's cppWidth() is the same, we can just
            // remember one conversion for each, and reuse it
            إذا (Ast_گره_DType* ثابت new_dtypep = VN_CAST(old_dtypep->user3p(), گره_DType)) {
                nodep->dtypep(new_dtypep);
            } وإلا {
                nodep->dtype_Chg_عرض(عرض, nodep->عرض_Min());
                Ast_گره_DType* ثابت new_dtypep2 = nodep->dtypep();
                UASSERT_OBJ(new_dtypep2 != old_dtypep, nodep,
                            "Dtype didn't change when width changed");
                old_dtypep->user3p(new_dtypep2);  // Remember for next time
            }
        }
    }
    فراغ compute_Cpp_عرض(Ast_گره* nodep) {
        إذا (!nodep->user2() && nodep->hasDType()) {
            إذا (VN_IS(nodep, Var)  //
                || VN_IS(nodep, گره_DType)  // Don't want to change variable widths!
                || VN_IS(nodep, گره_Sel)  // Array selects should reflect variable widths
                || VN_IS(nodep, SFormatArg)  // Retain the logical argument type, not storage width
                || VN_IS(nodep->dtypep()->skipRefp(), AssocArrayDType)  // Or arrays
                || VN_IS(nodep->dtypep()->skipRefp(), WildcardArrayDType)
                || VN_IS(nodep->dtypep()->skipRefp(), DynArrayDType)
                || VN_IS(nodep->dtypep()->skipRefp(), ClassRefDType)
                || VN_IS(nodep->dtypep()->skipRefp(), صف_DType)
                || VN_IS(nodep->dtypep()->skipRefp(), جریان_DType)
                || VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)
                || VN_IS(nodep->dtypep()->skipRefp(), CoverCrossDType)
                || VN_IS(nodep->dtypep()->skipRefp(), CoverpointDType)
                || VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) {
            } وإلا {
                ثابت Ast_گره_UOr_Struct_DType* ثابت dtypep
                    = VN_CAST(nodep->dtypep()->skipRefp(), گره_UOr_Struct_DType);
                إذا (!dtypep || dtypep->packed()) مجموعه_Cpp_عرض(nodep);
            }
        }
    }

    // Store the clean state in the userp on each node
    فراغ مجموعه_Clean_State(Ast_گره* nodep, CleanState clean) { nodep->user1(clean); }
    CleanState getCleanState(ثابت Ast_گره* nodep) {
        أرجع static_cast<CleanState>(nodep->user1());
    }
    منطقي isClean(Ast_گره* nodep) {
        ثابت CleanState clstate = getCleanState(nodep);
        إذا (clstate == CS_CLEAN) أرجع درست;
        إذا (clstate == CS_DIRTY) أرجع خطأ;
        nodep->v3fatalSrc("Unknown clean state on node: " + nodep->prettyTypeName());
        أرجع خطأ;
    }
    فراغ مجموعه_Clean(Ast_گره* nodep, منطقي isClean) {
        compute_Cpp_عرض(nodep);  // Just to be sure it's in widthMin
        ثابت منطقي کامل_Uint
            = (nodep->عرض_Min() == VL_IDATASIZE || nodep->عرض_Min() == VL_QUADSIZE
               || (nodep->عرض_Min() % VL_EDATASIZE) == 0);
        مجموعه_Clean_State(nodep, ((isClean || کامل_Uint) ? CS_CLEAN : CS_DIRTY));
    }

    // Operate on nodes
    فراغ درج_Clean(Ast_گره_عبارت* nodep) {  // We'll insert ABOVE passed node
        UINFO(4, "  NeedClean " << nodep);
        VNRelinker relinkHandle;
        nodep->unlink_Fr_پشت(&relinkHandle);
        //
        compute_Cpp_عرض(nodep);
        V3_عدد ماسک{nodep, cpp_عرض(nodep)};
        ماسک.مجموعه_ماسک(nodep->عرض_Min());
        Ast_گره* ثابت cleanp
            = جديد AstAnd{nodep->fileline(), جديد AstConst{nodep->fileline(), ماسک}, nodep};
        cleanp->dtypeFrom(nodep);  // Otherwise the AND normally picks LHS
        relinkHandle.relink(cleanp);
    }
    فراغ ensureClean(Ast_گره_عبارت* nodep) {
        compute_Cpp_عرض(nodep);
        إذا (!isClean(nodep)) درج_Clean(nodep);
    }
    فراغ ensure_Clean_And_بعدی(Ast_گره* nodep) {
        // Editing list, careful looping!
        لكل (Ast_گره* argp = nodep; argp;) {
            Ast_گره* ثابت nextp = argp->nextp();
            إذا (Ast_گره_عبارت* ثابت exprp = VN_CAST(argp, گره_عبارت)) ensureClean(exprp);
            argp = nextp;
        }
    }

    // Base type handling methods
    فراغ عملوند_Biop(Ast_گره_Biop* nodep) {
        iterateChildren(nodep);
        compute_Cpp_عرض(nodep);
        إذا (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        إذا (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        // no setClean.. must do it in each user routine.
    }
    فراغ عملوند_Triop(Ast_گره_Triop* nodep) {
        iterateChildren(nodep);
        compute_Cpp_عرض(nodep);
        إذا (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        إذا (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        إذا (nodep->cleanThs()) ensureClean(nodep->thsp());
        // no setClean.. must do it in each user routine.
    }
    فراغ عملوند_Quadop(Ast_گره_Quadop* nodep) {
        iterateChildren(nodep);
        compute_Cpp_عرض(nodep);
        إذا (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        إذا (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        إذا (nodep->cleanThs()) ensureClean(nodep->thsp());
        إذا (nodep->cleanFhs()) ensureClean(nodep->fhsp());
        // no setClean.. must do it in each user routine.
    }

    // VISITORS
    فراغ بازدید(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    فراغ بازدید(Ast_گره_Uniop* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_عرض(nodep);
        إذا (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        مجموعه_Clean(nodep, nodep->clean_بیرون());
    }
    فراغ بازدید(Ast_گره_Biop* nodep) override {
        عملوند_Biop(nodep);
        مجموعه_Clean(nodep, nodep->clean_بیرون());
    }
    فراغ بازدید(AstAnd* nodep) override {
        عملوند_Biop(nodep);
        مجموعه_Clean(nodep, isClean(nodep->lhsp()) || isClean(nodep->rhsp()));
    }
    فراغ بازدید(AstXor* nodep) override {
        عملوند_Biop(nodep);
        مجموعه_Clean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    فراغ بازدید(AstOr* nodep) override {
        عملوند_Biop(nodep);
        مجموعه_Clean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    فراغ بازدید(Ast_گره_Quadop* nodep) override {
        عملوند_Quadop(nodep);
        مجموعه_Clean(nodep, nodep->clean_بیرون());
    }
    فراغ بازدید(Ast_عبارت_Stmt* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_عرض(nodep);
        مجموعه_Clean(nodep, isClean(nodep->resultp()));
    }
    فراغ بازدید(Ast_گره_عبارت* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_عرض(nodep);
        مجموعه_Clean(nodep, nodep->clean_بیرون());
    }
    فراغ بازدید(Ast_گره_تخصیص_بده* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_عرض(nodep);
        إذا (nodep->cleanRhs()) ensureClean(nodep->rhsp());
    }
    فراغ بازدید(AstScopeName* nodep) override {  //
        مجموعه_Clean(nodep, درست);
    }
    فراغ بازدید(AstCNew* nodep) override {
        iterateChildren(nodep);
        مجموعه_Clean(nodep, درست);
    }
    // Constructor values are stored, so must be clean
    فراغ بازدید(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        إذا (nodep->defaultp()) ensureClean(nodep->defaultp());
        مجموعه_Clean(nodep, درست);
    }
    فراغ بازدید(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        إذا (nodep->defaultp()) ensureClean(nodep->defaultp());
        مجموعه_Clean(nodep, درست);
    }
    فراغ بازدید(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        إذا (nodep->defaultp()) ensureClean(nodep->defaultp());
        مجموعه_Clean(nodep, درست);
    }
    فراغ بازدید(Ast_گره_Cons_قلم* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->valuep());
    }
    فراغ بازدید(Ast_تطابق_Masked* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        مجموعه_Clean(nodep, درست);
    }
    فراغ بازدید(AstSel* nodep) override {
        عملوند_Biop(nodep);
        مجموعه_Clean(nodep, nodep->clean_بیرون());
    }
    فراغ بازدید(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_عرض(nodep);
        مجموعه_Clean(nodep, خطأ);
        // We always clean, as we don't trust those pesky users.
        إذا (!VN_IS(nodep->backp(), And)) درج_Clean(nodep);
        ensure_Clean_And_بعدی(nodep->nodesp());
    }
    فراغ بازدید(AstTraceDecl* nodep) override {}  // Nothing to do here
    فراغ بازدید(AstTraceInc* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_بعدی(nodep->valuep());
    }
    فراغ بازدید(AstTypedef* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }
    فراغ بازدید(AstParamTypeDType* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }

    // Control flow operators
    فراغ بازدید(AstCond* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
        مجموعه_Clean(nodep, isClean(nodep->thenp()) && isClean(nodep->elsep()));
    }
    فراغ بازدید(AstLoop* nodep) override { iterateChildren(nodep); }
    فراغ بازدید(AstLoopTest* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    فراغ بازدید(Ast_گره_If* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    فراغ بازدید(AstSFormatArg* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_بعدی(nodep->exprp());
        مجموعه_Clean(nodep, درست);
    }
    فراغ بازدید(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_بعدی(nodep->exprsp());
        مجموعه_Clean(nodep, درست);  // generates a string, so not relevant
    }
    فراغ بازدید(AstCStmt* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_بعدی(nodep->nodesp());
    }
    فراغ بازدید(AstCStmtUser* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_بعدی(nodep->nodesp());
    }
    فراغ بازدید(Ast_گره_CCall* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_بعدی(nodep->argsp());
        مجموعه_Clean(nodep, درست);
    }
    فراغ بازدید(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_بعدی(nodep->pinsp());
        مجموعه_Clean(nodep, nodep->clean_بیرون());
    }
    فراغ بازدید(AstWith* nodep) override {
        iterateChildren(nodep);
        مجموعه_Clean(nodep, درست);
    }
    فراغ بازدید(AstCReturn* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        مجموعه_Clean(nodep, درست);
    }
    فراغ بازدید(AstIntfRef* nodep) override {
        iterateChildren(nodep);
        مجموعه_Clean(nodep, درست);  // generates a string, so not relevant
    }

    //--------------------
    // Default: Just iterate
    فراغ بازدید(Ast_گره* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_عرض(nodep);
    }

عام:
    // CONSTRUCTORS
    صریح CleanVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CleanVisitor() override = افتراضي;
};

//######################################################################
// Clean class functions

فراغ V3Clean::cleanAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CleanVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("clean", 0, dump_درخت_Either_Level() >= 3);
}
