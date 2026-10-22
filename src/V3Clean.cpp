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
    ثابت VNUser1_دننه_Use m_inuser1;
    ثابت VNUser2_دننه_Use m_inuser2;
    ثابت VNUser3_دننه_Use m_inuser3;

    // TYPES
    تعداد CleanState : uint8_t { CS_UNKNOWN, CS_CLEAN, CS_DIRTY };

    // STATE - for current visit position (use VL_RESTORER)
    ثابت Ast_غوټه_Module* m_modp = تش_اشاره;

    // METHODS

    // Width resetting
    صحيح cpp_پلنوالی(ثابت Ast_غوټه* nodep) {
        إذا (nodep->پلنوالی() <= VL_IDATASIZE) {
            أرجع VL_IDATASIZE;
        } وإلا إذا (nodep->پلنوالی() <= VL_QUADSIZE) {
            أرجع VL_QUADSIZE;
        } وإلا {
            أرجع nodep->پلنوالی_ټکي() * VL_EDATASIZE;
        }
    }
    فراغ سېټ_Cpp_پلنوالی(Ast_غوټه* nodep) {
        nodep->user2(ریښتیا);  // Don't resize it again
        Ast_غوټه_DType* ثابت old_dtypep = nodep->dtypep();
        ثابت صحيح پلنوالی = cpp_پلنوالی(nodep);  // widthMin is unchanged
        إذا (old_dtypep->پلنوالی() != پلنوالی) {
            // Since any given dtype's cppWidth() is the same, we can just
            // remember one conversion for each, and reuse it
            إذا (Ast_غوټه_DType* ثابت new_dtypep = VN_CAST(old_dtypep->user3p(), غوټه_DType)) {
                nodep->dtypep(new_dtypep);
            } وإلا {
                nodep->dtype_Chg_پلنوالی(پلنوالی, nodep->پلنوالی_Min());
                Ast_غوټه_DType* ثابت new_dtypep2 = nodep->dtypep();
                UASSERT_OBJ(new_dtypep2 != old_dtypep, nodep,
                            "Dtype didn't change when width changed");
                old_dtypep->user3p(new_dtypep2);  // Remember for next time
            }
        }
    }
    فراغ compute_Cpp_پلنوالی(Ast_غوټه* nodep) {
        إذا (!nodep->user2() && nodep->hasDType()) {
            إذا (VN_IS(nodep, Var)  //
                || VN_IS(nodep, غوټه_DType)  // Don't want to change variable widths!
                || VN_IS(nodep, غوټه_Sel)  // Array selects should reflect variable widths
                || VN_IS(nodep, SFormatArg)  // Retain the logical argument type, not storage width
                || VN_IS(nodep->dtypep()->skipRefp(), AssocArrayDType)  // Or arrays
                || VN_IS(nodep->dtypep()->skipRefp(), WildcardArrayDType)
                || VN_IS(nodep->dtypep()->skipRefp(), DynArrayDType)
                || VN_IS(nodep->dtypep()->skipRefp(), ClassRefDType)
                || VN_IS(nodep->dtypep()->skipRefp(), کتار_DType)
                || VN_IS(nodep->dtypep()->skipRefp(), بهیر_DType)
                || VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)
                || VN_IS(nodep->dtypep()->skipRefp(), CoverCrossDType)
                || VN_IS(nodep->dtypep()->skipRefp(), CoverpointDType)
                || VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) {
            } وإلا {
                ثابت Ast_غوټه_UOr_Struct_DType* ثابت dtypep
                    = VN_CAST(nodep->dtypep()->skipRefp(), غوټه_UOr_Struct_DType);
                إذا (!dtypep || dtypep->packed()) سېټ_Cpp_پلنوالی(nodep);
            }
        }
    }

    // Store the clean state in the userp on each node
    فراغ سېټ_Clean_State(Ast_غوټه* nodep, CleanState clean) { nodep->user1(clean); }
    CleanState getCleanState(ثابت Ast_غوټه* nodep) {
        أرجع static_cast<CleanState>(nodep->user1());
    }
    منطقي isClean(Ast_غوټه* nodep) {
        ثابت CleanState clstate = getCleanState(nodep);
        إذا (clstate == CS_CLEAN) أرجع ریښتیا;
        إذا (clstate == CS_DIRTY) أرجع خطأ;
        nodep->v3fatalSrc("Unknown clean state on node: " + nodep->prettyTypeName());
        أرجع خطأ;
    }
    فراغ سېټ_Clean(Ast_غوټه* nodep, منطقي isClean) {
        compute_Cpp_پلنوالی(nodep);  // Just to be sure it's in widthMin
        ثابت منطقي ټولیز_Uint
            = (nodep->پلنوالی_Min() == VL_IDATASIZE || nodep->پلنوالی_Min() == VL_QUADSIZE
               || (nodep->پلنوالی_Min() % VL_EDATASIZE) == 0);
        سېټ_Clean_State(nodep, ((isClean || ټولیز_Uint) ? CS_CLEAN : CS_DIRTY));
    }

    // Operate on nodes
    فراغ ورننباسه_Clean(Ast_غوټه_بیان* nodep) {  // We'll insert ABOVE passed node
        UINFO(4, "  NeedClean " << nodep);
        VNRelinker relinkHandle;
        nodep->unlink_Fr_شا(&relinkHandle);
        //
        compute_Cpp_پلنوالی(nodep);
        V3_شمېره ماسک{nodep, cpp_پلنوالی(nodep)};
        ماسک.سېټ_ماسک(nodep->پلنوالی_Min());
        Ast_غوټه* ثابت cleanp
            = جديد AstAnd{nodep->fileline(), جديد AstConst{nodep->fileline(), ماسک}, nodep};
        cleanp->dtypeFrom(nodep);  // Otherwise the AND normally picks LHS
        relinkHandle.relink(cleanp);
    }
    فراغ ensureClean(Ast_غوټه_بیان* nodep) {
        compute_Cpp_پلنوالی(nodep);
        إذا (!isClean(nodep)) ورننباسه_Clean(nodep);
    }
    فراغ ensure_Clean_And_بل(Ast_غوټه* nodep) {
        // Editing list, careful looping!
        لكل (Ast_غوټه* argp = nodep; argp;) {
            Ast_غوټه* ثابت nextp = argp->nextp();
            إذا (Ast_غوټه_بیان* ثابت exprp = VN_CAST(argp, غوټه_بیان)) ensureClean(exprp);
            argp = nextp;
        }
    }

    // Base type handling methods
    فراغ عملوند_Biop(Ast_غوټه_Biop* nodep) {
        iterateChildren(nodep);
        compute_Cpp_پلنوالی(nodep);
        إذا (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        إذا (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        // no setClean.. must do it in each user routine.
    }
    فراغ عملوند_Triop(Ast_غوټه_Triop* nodep) {
        iterateChildren(nodep);
        compute_Cpp_پلنوالی(nodep);
        إذا (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        إذا (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        إذا (nodep->cleanThs()) ensureClean(nodep->thsp());
        // no setClean.. must do it in each user routine.
    }
    فراغ عملوند_Quadop(Ast_غوټه_Quadop* nodep) {
        iterateChildren(nodep);
        compute_Cpp_پلنوالی(nodep);
        إذا (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        إذا (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        إذا (nodep->cleanThs()) ensureClean(nodep->thsp());
        إذا (nodep->cleanFhs()) ensureClean(nodep->fhsp());
        // no setClean.. must do it in each user routine.
    }

    // VISITORS
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه_Uniop* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_پلنوالی(nodep);
        إذا (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        سېټ_Clean(nodep, nodep->clean_بهر());
    }
    فراغ لیدنه(Ast_غوټه_Biop* nodep) override {
        عملوند_Biop(nodep);
        سېټ_Clean(nodep, nodep->clean_بهر());
    }
    فراغ لیدنه(AstAnd* nodep) override {
        عملوند_Biop(nodep);
        سېټ_Clean(nodep, isClean(nodep->lhsp()) || isClean(nodep->rhsp()));
    }
    فراغ لیدنه(AstXor* nodep) override {
        عملوند_Biop(nodep);
        سېټ_Clean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    فراغ لیدنه(AstOr* nodep) override {
        عملوند_Biop(nodep);
        سېټ_Clean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    فراغ لیدنه(Ast_غوټه_Quadop* nodep) override {
        عملوند_Quadop(nodep);
        سېټ_Clean(nodep, nodep->clean_بهر());
    }
    فراغ لیدنه(Ast_بیان_Stmt* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_پلنوالی(nodep);
        سېټ_Clean(nodep, isClean(nodep->resultp()));
    }
    فراغ لیدنه(Ast_غوټه_بیان* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_پلنوالی(nodep);
        سېټ_Clean(nodep, nodep->clean_بهر());
    }
    فراغ لیدنه(Ast_غوټه_ټاکل* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_پلنوالی(nodep);
        إذا (nodep->cleanRhs()) ensureClean(nodep->rhsp());
    }
    فراغ لیدنه(AstScopeName* nodep) override {  //
        سېټ_Clean(nodep, ریښتیا);
    }
    فراغ لیدنه(AstCNew* nodep) override {
        iterateChildren(nodep);
        سېټ_Clean(nodep, ریښتیا);
    }
    // Constructor values are stored, so must be clean
    فراغ لیدنه(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        إذا (nodep->defaultp()) ensureClean(nodep->defaultp());
        سېټ_Clean(nodep, ریښتیا);
    }
    فراغ لیدنه(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        إذا (nodep->defaultp()) ensureClean(nodep->defaultp());
        سېټ_Clean(nodep, ریښتیا);
    }
    فراغ لیدنه(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        إذا (nodep->defaultp()) ensureClean(nodep->defaultp());
        سېټ_Clean(nodep, ریښتیا);
    }
    فراغ لیدنه(Ast_غوټه_Cons_توکی* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->valuep());
    }
    فراغ لیدنه(Ast_سمون_Masked* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        سېټ_Clean(nodep, ریښتیا);
    }
    فراغ لیدنه(AstSel* nodep) override {
        عملوند_Biop(nodep);
        سېټ_Clean(nodep, nodep->clean_بهر());
    }
    فراغ لیدنه(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_پلنوالی(nodep);
        سېټ_Clean(nodep, خطأ);
        // We always clean, as we don't trust those pesky users.
        إذا (!VN_IS(nodep->backp(), And)) ورننباسه_Clean(nodep);
        ensure_Clean_And_بل(nodep->nodesp());
    }
    فراغ لیدنه(AstTraceDecl* nodep) override {}  // Nothing to do here
    فراغ لیدنه(AstTraceInc* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_بل(nodep->valuep());
    }
    فراغ لیدنه(AstTypedef* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstParamTypeDType* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }

    // Control flow operators
    فراغ لیدنه(AstCond* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
        سېټ_Clean(nodep, isClean(nodep->thenp()) && isClean(nodep->elsep()));
    }
    فراغ لیدنه(AstLoop* nodep) override { iterateChildren(nodep); }
    فراغ لیدنه(AstLoopTest* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    فراغ لیدنه(Ast_غوټه_If* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    فراغ لیدنه(AstSFormatArg* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_بل(nodep->exprp());
        سېټ_Clean(nodep, ریښتیا);
    }
    فراغ لیدنه(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_بل(nodep->exprsp());
        سېټ_Clean(nodep, ریښتیا);  // generates a string, so not relevant
    }
    فراغ لیدنه(AstCStmt* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_بل(nodep->nodesp());
    }
    فراغ لیدنه(AstCStmtUser* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_بل(nodep->nodesp());
    }
    فراغ لیدنه(Ast_غوټه_CCall* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_بل(nodep->argsp());
        سېټ_Clean(nodep, ریښتیا);
    }
    فراغ لیدنه(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_بل(nodep->pinsp());
        سېټ_Clean(nodep, nodep->clean_بهر());
    }
    فراغ لیدنه(AstWith* nodep) override {
        iterateChildren(nodep);
        سېټ_Clean(nodep, ریښتیا);
    }
    فراغ لیدنه(AstCReturn* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        سېټ_Clean(nodep, ریښتیا);
    }
    فراغ لیدنه(AstIntfRef* nodep) override {
        iterateChildren(nodep);
        سېټ_Clean(nodep, ریښتیا);  // generates a string, so not relevant
    }

    //--------------------
    // Default: Just iterate
    فراغ لیدنه(Ast_غوټه* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_پلنوالی(nodep);
    }

عام:
    // CONSTRUCTORS
    څرګند CleanVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CleanVisitor() override = افتراضي;
};

//######################################################################
// Clean class functions

فراغ V3Clean::cleanAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CleanVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ونه("clean", 0, dump_ونه_Either_Level() >= 3);
}
