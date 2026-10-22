// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Add temporaries, such as for expand nodes
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2004-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
// V3Expand's Transformations:
//
// Each module:
//      Expand verilated.h macros into internal micro optimizations (RTL)
//      this will enable later optimizations.
//      Wide operands become assignments to each word of the vector, (WORDSELs)
//          Note in this case that the widthMin is not correct for the MSW of
//          the vector.  This must be accounted for if doing later constant
//          propagation across signals.
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Expand.h"

#ضمن "V3Const.h"
#ضمن "V3Stats.h"

#ضمن <algorithm>
#ضمن <array>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// The distinct expansion patterns, for statistics. One entry for each
// unique transformation ExpandVisitor applies. Keep sorted.

// clang-format off
#تعریف_کړه FOR_EACH_EXPAND_نمونه(macro) \
    macro(ټاکل_AND) \
    macro(ټاکل_ARRAYSEL) \
    macro(ټاکل_CONCAT) \
    macro(ټاکل_COND) \
    macro(ټاکل_CONST) \
    macro(ټاکل_EXTEND) \
    macro(ټاکل_NOT) \
    macro(ټاکل_OR) \
    macro(ټاکل_REPLICATE) \
    macro(ټاکل_SEL_ALIGNED) \
    macro(ټاکل_SEL_UNALIGNED) \
    macro(ټاکل_SHIFTL) \
    macro(ټاکل_SHIFTR) \
    macro(ټاکل_منزل_SEL_CONST_NARROW) \
    macro(ټاکل_منزل_SEL_CONST_WIDE) \
    macro(ټاکل_منزل_SEL_VAR_NARROW) \
    macro(ټاکل_منزل_SEL_VAR_WIDE_ONE_بټ) \
    macro(ټاکل_VARREF) \
    macro(ټاکل_XOR) \
    macro(CONCAT) \
    macro(REDAND_منزل_EQ) \
    macro(REDOR_منزل_NEQ) \
    macro(REPLICATE) \
    macro(REPLICATE_ONE_بټ) \
    macro(SEL_FROM_NARROW) \
    macro(SEL_FROM_WIDE) \
    macro(WORDIZE_EQ) \
    macro(WORDIZE_NEQ) \
    macro(WORDIZE_REDAND) \
    macro(WORDIZE_REDOR) \
    macro(WORDIZE_REDXOR)
// clang-format on

صنف VExpand_نمونه final {
عام:
    تعداد en : uint8_t {
#تعریف_کړه EXPAND_نمونه_ID(id) id,
        FOR_EACH_EXPAND_نمونه(EXPAND_نمونه_ID)
#تعریف_لغوه EXPAND_نمونه_ID
            ENUM_پای_ټکی
    };
    تعداد en m_e;
    ثابت حرف* ascii() ثابت {
        ساكن ثابت حرف* ثابت نومونه[] = {
#تعریف_کړه EXPAND_نمونه_NAME(id) #id,
            FOR_EACH_EXPAND_نمونه(EXPAND_نمونه_NAME)
#تعریف_لغوه EXPAND_نمونه_NAME
                "_ENUM_END"  //
        };
        أرجع نومونه[m_e];
    }

    // cppcheck-suppress noExplicitConstructor
    VExpand_نمونه(en _e)
        : m_e{_e} {}
    چلوونکی en() ثابت { أرجع m_e; }
};

//######################################################################
// Find nodes with side effects, to mark as non-expandable

صنف ExpandOkVisitor final : عام VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> bool.  Is pure (along with all children)
    ثابت VNUser2_دننه_Use m_inuser2;

    // STATE - for current visit position (use VL_RESTORER)
    // Tracks similar to AstNode::isTreePureRecurse(), but avoid O(n^2)
    // False = pure, as nodes that ExpandVisitor inserts preserve pureness
    منطقي m_isImpure = ریښتیا;  // Currently pure

    فراغ لیدنه(Ast_غوټه* nodep) override {
        منطقي selfImpure = !nodep->is_خالص();
        {
            VL_RESTORER(m_isImpure);
            m_isImpure = خطأ;
            iterateChildrenConst(nodep);
            selfImpure |= m_isImpure;
            nodep->user2(selfImpure);
        }
        m_isImpure |= selfImpure;
    }

عام:
    // CONSTRUCTORS
    څرګند ExpandOkVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~ExpandOkVisitor() = افتراضي;
};

//######################################################################
// Expand state, as a visitor of each AstNode

صنف ExpandVisitor final : عام VNVisitor {
    // NODE STATE
    //  AstNode::user1()        -> bool.  Processed
    //  AstNode::user2()        -> See ExpandOkVisitor
    ثابت VNUser1_دننه_Use m_inuser1;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_غوټه* m_stmtp = تش_اشاره;  // Current statement

    // STATE - across all visitors
    AstCFunc* m_funcp = تش_اشاره;  // Current function
    VDouble0 m_statImpure;  // Statistic tracking
    VDouble0 m_statWides;  // Statistic tracking
    VDouble0 m_stat_Wide_ټکي;  // Statistic tracking
    VDouble0 m_statWideLimited;  // Statistic tracking
    VDouble0 m_statTemps;  // Statistic tracking
    // Number of applications of each expansion pattern for statistics
    std::array<VDouble0, VExpand_نمونه::ENUM_پای_ټکی> m_statPatterns;

    // STATE - for current function
    اندازه_t m_nTmps = 0;  // Sequence numbers for temopraries

    // METHODS
    // Note the application of the given expansion pattern to 'nodep'
    فراغ did_نمونه(VExpand_نمونه نمونه_2, Ast_غوټه* nodep) {
        UINFO(8, "    " << نمونه_2.ascii() << " " << nodep);
        ++m_statPatterns[نمونه_2];
    }

    // Use state that ExpandOkVisitor calculated
    ساكن منطقي isImpure(Ast_غوټه* nodep) {
        ثابت منطقي impure = nodep->user2();
        إذا (impure) UINFO(9, "      impure " << nodep);
        أرجع impure;
    }

    منطقي doExpandWide(Ast_غوټه* nodep) {
        إذا (isImpure(nodep)) {
            ++m_statImpure;
            أرجع خطأ;
        }
        إذا (nodep->پلنوالی_ټکي() <= v3Global.opt.expand_حد()) {
            ++m_statWides;
            m_stat_Wide_ټکي += nodep->پلنوالی_ټکي();
            أرجع ریښتیا;
        } وإلا {
            ++m_statWideLimited;
            أرجع خطأ;
        }
    }

    ساكن صحيح long_Or_Quad_پلنوالی(Ast_غوټه* nodep) {
        أرجع (nodep->پلنوالی() + (VL_EDATASIZE - 1)) & ~(VL_EDATASIZE - 1);
    }
    ساكن V3_شمېره not_Wide_ماسک(Ast_غوټه* nodep) {
        أرجع V3_شمېره{nodep, VL_EDATASIZE, ~VL_ماسک_E(nodep->پلنوالی_Min())};
    }
    ساكن V3_شمېره کلمه_ماسک(Ast_غوټه* nodep) {
        إذا (nodep->isWide()) {
            أرجع V3_شمېره{nodep, VL_EDATASIZE, VL_ماسک_E(nodep->پلنوالی_Min())};
        } وإلا {
            V3_شمېره ماسک{nodep, long_Or_Quad_پلنوالی(nodep)};
            ماسک.سېټ_ماسک(nodep->پلنوالی_Min());
            أرجع ماسک;
        }
    }

    ساكن فراغ ورننباسه_Before(Ast_غوټه* placep, Ast_غوټه* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        placep->جمع_کړه_دلته_This_As_بل(newp);
    }
    ساكن فراغ replaceWithDelete(Ast_غوټه* nodep, Ast_غوټه* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        إذا (newp->پلنوالی() != nodep->پلنوالی()) {
            UASSERT_OBJ(newp->پلنوالی_Min() == nodep->پلنوالی_Min(), nodep,
                        "Replacement width mismatch");
            newp->dtype_Chg_پلنوالی(nodep->پلنوالی(), nodep->پلنوالی_Min());
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
    }
    ساكن Ast_غوټه* new_کلمه_ټاکل(Ast_غوټه_ټاکل* placep, صحيح کلمه, Ast_غوټه_بیان* lhsp,
                                  Ast_غوټه_بیان* rhsp) {
        File_کرښه* ثابت fl = placep->fileline();
        أرجع جديد Ast_ټاکل{fl,
                             جديد Ast_کلمه_Sel{fl, lhsp->clone_ونه_خالص(ریښتیا),
                                            جديد AstConst{fl, static_cast<uint32_t>(کلمه)}},
                             rhsp};
    }
    ساكن فراغ جمع_کړه_کلمه_ټاکل(Ast_غوټه_ټاکل* placep, صحيح کلمه, Ast_غوټه_بیان* lhsp,
                              Ast_غوټه_بیان* rhsp) {
        ورننباسه_Before(placep, new_کلمه_ټاکل(placep, کلمه, lhsp, rhsp));
    }
    ساكن فراغ جمع_کړه_کلمه_ټاکل(Ast_غوټه_ټاکل* placep, صحيح کلمه, Ast_غوټه_بیان* rhsp) {
        جمع_کړه_کلمه_ټاکل(placep, کلمه, placep->lhsp(), rhsp);
    }

    AstVar* جمع_کړه_Local_Tmp(Ast_غوټه* placep, ثابت حرف* namep, Ast_غوټه_بیان* valuep) {
        ثابت std::string name = "__V"s + namep + "_"s + std::منزل_string(m_nTmps);
        File_کرښه* ثابت flp = placep->fileline();
        AstVar* ثابت tmpp = جديد AstVar{flp, VVarType::STMTTEMP, name, valuep->dtypep()};
        tmpp->funcLocal(ریښتیا);
        tmpp->is_دننی(ریښتیا);
        tmpp->noReset(ریښتیا);
        tmpp->substConstOnly(ریښتیا);
        m_funcp->جمع_کړه_Varsp(tmpp);
        ورننباسه_Before(placep, جديد Ast_ټاکل{flp, جديد AstVarRef{flp, tmpp, VAccess::WRITE}, valuep});
        ++m_statTemps;
        أرجع tmpp;
    }

    ساكن فراغ fixCloneLvalue(Ast_غوټه* nodep) {
        // In AstSel transforms, we call clone() on VarRefs that were lvalues,
        // but are now being used on the RHS of the assignment
        إذا (VN_IS(nodep, VarRef)) VN_AS(nodep, VarRef)->access(VAccess::READ);
        // Iterate
        إذا (Ast_غوټه* ثابت refp = nodep->op1p()) fixCloneLvalue(refp);
        إذا (Ast_غوټه* ثابت refp = nodep->op2p()) fixCloneLvalue(refp);
        إذا (Ast_غوټه* ثابت refp = nodep->op3p()) fixCloneLvalue(refp);
        إذا (Ast_غوټه* ثابت refp = nodep->op4p()) fixCloneLvalue(refp);
    }

    ساكن Ast_غوټه_بیان* new_Ast_کلمه_Sel_Clone(Ast_غوټه_بیان* nodep, صحيح کلمه) {
        // Get the specified word number from a wide array
        // Or, if it's a long/quad, do appropriate conversion to wide
        // Concat may pass negative word numbers, that means it wants a zero
        File_کرښه* ثابت fl = nodep->fileline();
        إذا (nodep->isWide() && کلمه >= 0 && کلمه < nodep->پلنوالی_ټکي()) {
            أرجع جديد Ast_کلمه_Sel{fl, nodep->clone_ونه_خالص(ریښتیا),
                                  جديد AstConst{fl, static_cast<uint32_t>(کلمه)}};
        } وإلا إذا (nodep->isQuad() && کلمه == 0) {
            Ast_غوټه_بیان* ثابت quadfromp = nodep->clone_ونه_خالص(ریښتیا);
            quadfromp->dtype_سېټ_بټ_Unsized(VL_QUADSIZE, quadfromp->پلنوالی_Min(), VSigning::UNSIGNED);
            أرجع جديد AstCCast{fl, quadfromp, VL_EDATASIZE};
        } وإلا إذا (nodep->isQuad() && کلمه == 1) {
            Ast_غوټه_بیان* ثابت quadfromp = nodep->clone_ونه_خالص(ریښتیا);
            quadfromp->dtype_سېټ_بټ_Unsized(VL_QUADSIZE, quadfromp->پلنوالی_Min(), VSigning::UNSIGNED);
            أرجع جديد AstCCast{
                fl, جديد Ast_ښويدنه_R{fl, quadfromp, جديد AstConst{fl, VL_EDATASIZE}, VL_EDATASIZE},
                VL_EDATASIZE};
        } وإلا إذا (!nodep->isWide() && !nodep->isQuad() && کلمه == 0) {
            أرجع nodep->clone_ونه_خالص(ریښتیا);
        } وإلا {  // Out of bounds
            أرجع جديد AstConst{fl, 0};
        }
    }

    ساكن Ast_غوټه_بیان* new_کلمه_Grab_ښويدنه(File_کرښه* fl, صحيح کلمه, Ast_غوټه_بیان* lhsp, صحيح ښويدنه_2) {
        // Extract the expression to grab the value for the specified word, if it's the shift
        // of shift bits from lhsp
        // Negative word numbers requested for lhs when it's "before" what we want.
        // We get a 0 then.
        ثابت صحيح othword = کلمه - ښويدنه_2 / VL_EDATASIZE;
        Ast_غوټه_بیان* ثابت lop = new_Ast_کلمه_Sel_Clone(lhsp, othword);
        ثابت uint32_t lo_ښويدنه = VL_BITBIT_E(ښويدنه_2);
        إذا (!lo_ښويدنه) أرجع lop;
        Ast_غوټه_بیان* ثابت hip = new_Ast_کلمه_Sel_Clone(lhsp, othword - 1);
        ثابت uint32_t hi_ښويدنه = VL_EDATASIZE - lo_ښويدنه;  // Complement offset
        Ast_غوټه_بیان* ثابت newp
            = جديد AstOr{fl,  //
                        جديد Ast_ښويدنه_R{fl, hip, جديد AstConst{fl, hi_ښويدنه}, VL_EDATASIZE},
                        جديد Ast_ښويدنه_L{fl, lop, جديد AstConst{fl, lo_ښويدنه}, VL_EDATASIZE}};
        أرجع V3Const::constifyEditCpp(newp);
    }

    // Return expression indexing the word that contains 'lsbp' + the given word offset
    ساكن Ast_غوټه_بیان* new_کلمه_شاخص(Ast_غوټه_بیان* lsbp, uint32_t کلمه_دننه_واټن = 0) {
        // This is indexing a WordSel, so a 32 bit constants are fine
        File_کرښه* ثابت flp = lsbp->fileline();
        إذا (ثابت AstConst* constp = VN_CAST(lsbp, Const)) {
            أرجع جديد AstConst{flp, کلمه_دننه_واټن + VL_BITWORD_E(constp->منزل_UInt())};
        }

        إذا (lsbp->backp()) lsbp = lsbp->clone_ونه_خالص(خطأ);
        Ast_غوټه_بیان* ثابت wwl2p = جديد AstConst{flp, VL_EDATASIZE_LOG2};  // Word width log 2
        Ast_غوټه_بیان* indexp = جديد Ast_ښويدنه_R{flp, lsbp, wwl2p, VL_EDATASIZE};
        إذا (کلمه_دننه_واټن) indexp = جديد Ast_جمع_کړه{flp, جديد AstConst{flp, کلمه_دننه_واټن}, indexp};
        أرجع indexp;
    }

    // Return word of fromp that contains word indexp + the given word offset.
    ساكن Ast_غوټه_بیان* new_کلمه_Sel_کلمه(File_کرښه* flp, Ast_غوټه_بیان* fromp, Ast_غوټه_بیان* indexp,
                                       uint32_t کلمه_دننه_واټن = 0) {
        UASSERT_OBJ(fromp->isWide(), fromp, "Only need AstWordSel on wide from's");

        إذا (ثابت AstConst* ثابت constp = VN_CAST(indexp, Const)) {
            indexp = تش_اشاره;
            کلمه_دننه_واټن += constp->منزل_UInt();
        }

        // e.g. "logic [95:0] var[0]; logic [0] sel; out = var[sel];"
        // Squash before C++ to avoid getting a C++ compiler warning
        // (even though code would be unreachable as presumably a
        // AstCond is protecting above this node.
        إذا (کلمه_دننه_واټن >= static_cast<uint32_t>(fromp->پلنوالی_ټکي())) {
            أرجع جديد AstConst{flp, AstConst::SizedEData{}, 0};
        }

        إذا (fromp->backp()) fromp = fromp->clone_ونه_خالص(خطأ);

        // If indexp was constant, just use it
        إذا (!indexp) أرجع جديد Ast_کلمه_Sel{flp, fromp, جديد AstConst{flp, کلمه_دننه_واټن}};

        // Otherwise compute at runtime
        إذا (indexp->backp()) indexp = indexp->clone_ونه_خالص(خطأ);
        إذا (کلمه_دننه_واټن) indexp = جديد Ast_جمع_کړه{flp, جديد AstConst{flp, کلمه_دننه_واټن}, indexp};
        أرجع جديد Ast_کلمه_Sel{flp, fromp, indexp};
    }

    // Return word of fromp that contains bit lsbp + the given word offset.
    ساكن Ast_غوټه_بیان* new_کلمه_Sel_بټ(File_کرښه* flp, Ast_غوټه_بیان* fromp, Ast_غوټه_بیان* lsbp,
                                      uint32_t کلمه_دننه_واټن = 0) {
        Ast_غوټه_بیان* ثابت indexp = new_کلمه_شاخص(lsbp, کلمه_دننه_واټن);
        Ast_غوټه_بیان* ثابت کلمه_Selp = new_کلمه_Sel_کلمه(flp, fromp, indexp);
        إذا (!indexp->backp()) VL_DO_DANGLING(indexp->delete_ونه(), indexp);
        أرجع کلمه_Selp;
    }

    ساكن Ast_غوټه_بیان* new_Sel_بټ_بټ(Ast_غوټه_بیان* lsbp) {
        // Return equation to get the VL_BITBIT of a constant or non-constant
        File_کرښه* ثابت fl = lsbp->fileline();
        إذا (VN_IS(lsbp, Const)) {
            أرجع جديد AstConst{fl, VL_BITBIT_E(VN_AS(lsbp, Const)->منزل_UInt())};
        } وإلا {
            أرجع جديد AstAnd{fl, جديد AstConst{fl, VL_EDATASIZE - 1}, lsbp->clone_ونه_خالص(ریښتیا)};
        }
    }

    //====================

    منطقي expandWide(Ast_غوټه_ټاکل* nodep, AstConst* rhsp) {
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        did_نمونه(VExpand_نمونه::ټاکل_CONST, nodep);
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),WORDSEL(CONST,#))}}
        إذا (rhsp->عدد().isFourState()) {
            rhsp->v3warn(E_UNSUPPORTED,  // LCOV_EXCL_LINE  // impossible?
                         "Unsupported: 4-state numbers in this context");
        }
        File_کرښه* ثابت fl = nodep->fileline();
        لكل (صحيح w = 0; w < nodep->پلنوالی_ټکي(); ++w) {
            جمع_کړه_کلمه_ټاکل(nodep, w,
                          جديد AstConst{fl, AstConst::SizedEData{}, rhsp->عدد().edata_کلمه(w)});
        }
        أرجع ریښتیا;
    }
    //-------- Uniops
    منطقي expandWide(Ast_غوټه_ټاکل* nodep, AstVarRef* rhsp) {
        // Special case: do not expand assignment of constant pool variables.
        // V3Subst undestands these directly.
        إذا (rhsp->varp()->const_Pool_ثبت()) أرجع خطأ;
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        did_نمونه(VExpand_نمونه::ټاکل_VARREF, nodep);
        لكل (صحيح w = 0; w < nodep->پلنوالی_ټکي(); ++w) {
            جمع_کړه_کلمه_ټاکل(nodep, w, new_Ast_کلمه_Sel_Clone(rhsp, w));
        }
        أرجع ریښتیا;
    }
    منطقي expandWide(Ast_غوټه_ټاکل* nodep, AstArraySel* rhsp) {
        UASSERT_OBJ(!VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType), nodep,
                    "ArraySel with unpacked arrays should have been removed in V3Slice");
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        did_نمونه(VExpand_نمونه::ټاکل_ARRAYSEL, nodep);
        لكل (صحيح w = 0; w < nodep->پلنوالی_ټکي(); ++w) {
            جمع_کړه_کلمه_ټاکل(nodep, w, new_Ast_کلمه_Sel_Clone(rhsp, w));
        }
        أرجع ریښتیا;
    }
    منطقي expandWide(Ast_غوټه_ټاکل* nodep, AstNot* rhsp) {
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),NOT(WORDSEL(lhs,#))) }}
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        did_نمونه(VExpand_نمونه::ټاکل_NOT, nodep);
        File_کرښه* ثابت fl = rhsp->fileline();
        لكل (صحيح w = 0; w < nodep->پلنوالی_ټکي(); ++w) {
            جمع_کړه_کلمه_ټاکل(nodep, w, جديد AstNot{fl, new_Ast_کلمه_Sel_Clone(rhsp->lhsp(), w)});
        }
        أرجع ریښتیا;
    }
    //-------- Biops
    منطقي expandWide(Ast_غوټه_ټاکل* nodep, AstAnd* rhsp) {
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        did_نمونه(VExpand_نمونه::ټاکل_AND, nodep);
        File_کرښه* ثابت fl = nodep->fileline();
        لكل (صحيح w = 0; w < nodep->پلنوالی_ټکي(); ++w) {
            جمع_کړه_کلمه_ټاکل(nodep, w,
                          جديد AstAnd{fl, new_Ast_کلمه_Sel_Clone(rhsp->lhsp(), w),
                                     new_Ast_کلمه_Sel_Clone(rhsp->rhsp(), w)});
        }
        أرجع ریښتیا;
    }
    منطقي expandWide(Ast_غوټه_ټاکل* nodep, AstOr* rhsp) {
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        did_نمونه(VExpand_نمونه::ټاکل_OR, nodep);
        File_کرښه* ثابت fl = nodep->fileline();
        لكل (صحيح w = 0; w < nodep->پلنوالی_ټکي(); ++w) {
            جمع_کړه_کلمه_ټاکل(nodep, w,
                          جديد AstOr{fl, new_Ast_کلمه_Sel_Clone(rhsp->lhsp(), w),
                                    new_Ast_کلمه_Sel_Clone(rhsp->rhsp(), w)});
        }
        أرجع ریښتیا;
    }
    منطقي expandWide(Ast_غوټه_ټاکل* nodep, AstXor* rhsp) {
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        did_نمونه(VExpand_نمونه::ټاکل_XOR, nodep);
        File_کرښه* ثابت fl = nodep->fileline();
        لكل (صحيح w = 0; w < nodep->پلنوالی_ټکي(); ++w) {
            جمع_کړه_کلمه_ټاکل(nodep, w,
                          جديد AstXor{fl, new_Ast_کلمه_Sel_Clone(rhsp->lhsp(), w),
                                     new_Ast_کلمه_Sel_Clone(rhsp->rhsp(), w)});
        }
        أرجع ریښتیا;
    }
    منطقي expand_Wide_ښويدنه(Ast_غوټه_ټاکل* nodep, Ast_غوټه_Biop* rhsp, منطقي is_کیڼ_ښويدنه) {
        إذا (!doExpandWide(nodep)) أرجع خطأ;

        // Simplify the shift amount, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->rhsp());

        // If it's a constant shift by whole words, expand it so V3Subst can substitute it
        إذا (ثابت AstConst* ثابت rhsConstp = VN_CAST(rhsp->rhsp(), Const)) {
            ثابت uint32_t ښويدنه_بټونه = rhsConstp->منزل_UInt();
            إذا (VL_BITBIT_E(ښويدنه_بټونه) == 0) {
                ثابت صحيح پلنوالی_ټکي = nodep->پلنوالی_ټکي();
                ثابت صحيح ښويدنه_ټکي = std::min<صحيح>(VL_BITWORD_E(ښويدنه_بټونه), پلنوالی_ټکي);
                File_کرښه* ثابت flp = rhsp->fileline();
                إذا (is_کیڼ_ښويدنه) {
                    did_نمونه(VExpand_نمونه::ټاکل_SHIFTL, nodep);
                    // Low words of the result are zero
                    لكل (صحيح w = 0; w < ښويدنه_ټکي; ++w) {
                        جمع_کړه_کلمه_ټاکل(nodep, w, جديد AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                    // High words of the result are copied from higher words of the source
                    لكل (صحيح w = ښويدنه_ټکي; w < پلنوالی_ټکي; ++w) {
                        جمع_کړه_کلمه_ټاکل(nodep, w, new_Ast_کلمه_Sel_Clone(rhsp->lhsp(), w - ښويدنه_ټکي));
                    }
                } وإلا {
                    did_نمونه(VExpand_نمونه::ټاکل_SHIFTR, nodep);
                    // Low words of the result are copied from higher words of the source
                    لكل (صحيح w = 0; w < پلنوالی_ټکي - ښويدنه_ټکي; ++w) {
                        جمع_کړه_کلمه_ټاکل(nodep, w, new_Ast_کلمه_Sel_Clone(rhsp->lhsp(), w + ښويدنه_ټکي));
                    }
                    // High words of the result are zero
                    لكل (صحيح w = پلنوالی_ټکي - ښويدنه_ټکي; w < پلنوالی_ټکي; ++w) {
                        جمع_کړه_کلمه_ټاکل(nodep, w, جديد AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                }
                أرجع ریښتیا;
            }
        }

        أرجع خطأ;
    }

    //-------- Triops
    منطقي expandWide(Ast_غوټه_ټاکل* nodep, AstCond* rhsp) {
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        did_نمونه(VExpand_نمونه::ټاکل_COND, nodep);
        File_کرښه* ثابت fl = nodep->fileline();
        لكل (صحيح w = 0; w < nodep->پلنوالی_ټکي(); ++w) {
            جمع_کړه_کلمه_ټاکل(nodep, w,
                          جديد AstCond{fl, rhsp->condp()->clone_ونه_خالص(ریښتیا),
                                      new_Ast_کلمه_Sel_Clone(rhsp->thenp(), w),
                                      new_Ast_کلمه_Sel_Clone(rhsp->elsep(), w)});
        }
        أرجع ریښتیا;
    }

    // VISITORS
    فراغ لیدنه(AstNetlist* nodep) override { iterate_And_بل_Null(nodep->modulesp()); }

    فراغ لیدنه(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_nTmps);
        m_funcp = nodep;
        m_nTmps = 0;
        ثابت VDouble0 statWidesBefore = m_statWides;
        iterateChildren(nodep);

        // Constant fold here if anything was expanded, as Ast size can likely be reduced
        إذا (v3Global.opt.fConstEager() && m_statWides != statWidesBefore) {
            Ast_غوټه* ثابت editedp = V3Const::constifyEditCpp(nodep);
            UASSERT_OBJ(editedp == nodep, editedp, "Should not have replaced CFunc");
        }
    }

    فراغ لیدنه(AstExtend* nodep) override {
        إذا (nodep->user1_سېټ_Once()) أرجع;  // Process once
        iterateChildren(nodep);
        إذا (nodep->isWide()) {
            // See under ASSIGN(EXTEND)
        } وإلا {
            إذا (isImpure(nodep)) أرجع;
            Ast_غوټه_بیان* ثابت lhsp = nodep->lhsp()->unlink_Fr_شا();
            Ast_غوټه_بیان* newp = lhsp;
            إذا (nodep->isQuad()) {
                إذا (lhsp->isQuad()) {
                    lhsp->dtypeFrom(nodep);  // Just mark it, else nop
                } وإلا إذا (lhsp->isWide()) {
                    nodep->v3fatalSrc("extending larger thing into smaller?");
                } وإلا {
                    newp = جديد AstCCast{nodep->fileline(), lhsp, nodep};
                }
            } وإلا {  // Long
                UASSERT_OBJ(!(lhsp->isQuad() || lhsp->isWide()), nodep,
                            "extending larger thing into smaller?");
                lhsp->dtypeFrom(nodep);  // Just mark it, else nop
            }
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    منطقي expandWide(Ast_غوټه_ټاکل* nodep, AstExtend* rhsp) {
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        did_نمونه(VExpand_نمونه::ټاکل_EXTEND, nodep);
        Ast_غوټه_بیان* ثابت rlhsp = rhsp->lhsp();
        لكل (صحيح w = 0; w < rlhsp->پلنوالی_ټکي(); ++w) {
            جمع_کړه_کلمه_ټاکل(nodep, w, new_Ast_کلمه_Sel_Clone(rlhsp, w));
        }
        لكل (صحيح w = rlhsp->پلنوالی_ټکي(); w < nodep->پلنوالی_ټکي(); ++w) {
            جمع_کړه_کلمه_ټاکل(nodep, w, جديد AstConst{nodep->fileline(), AstConst::SizedEData{}, 0});
        }
        أرجع ریښتیا;
    }

    فراغ لیدنه(AstSel* nodep) override {
        إذا (nodep->user1_سېټ_Once()) أرجع;  // Process once
        iterateChildren(nodep);

        ثابت uint32_t پلنوالی = static_cast<uint32_t>(nodep->پلنوالی_Const());
        UASSERT_OBJ(nodep->پلنوالی_Min() == static_cast<صحيح>(پلنوالی), nodep, "Width mismatch");

        // Skip if Sel is an LHS assignment select
        إذا (Ast_غوټه_ټاکل* ثابت assignp = VN_CAST(nodep->backp(), غوټه_ټاکل)) {
            إذا (nodep == assignp->lhsp()) أرجع;
        }

        // Skip if wide: See under ASSIGN(WIDE)
        إذا (nodep->isWide()) أرجع;

        // Skip Sel from stream or queue
        إذا (VN_IS(nodep->fromp()->dtypep(), بهیر_DType)) أرجع;
        إذا (VN_IS(nodep->fromp()->dtypep(), کتار_DType)) أرجع;

        // Result must be non-wide after skipping all the above
        UASSERT_OBJ(پلنوالی <= 64, nodep, "Inconsistent result width");

        إذا (nodep->fromp()->isWide()) {  // Long/Quad from Wide
            إذا (isImpure(nodep)) أرجع;
            did_نمونه(VExpand_نمونه::SEL_FROM_WIDE, nodep);
            File_کرښه* ثابت nfl = nodep->fileline();
            File_کرښه* ثابت lfl = nodep->lsbp()->fileline();
            File_کرښه* ثابت ffl = nodep->fromp()->fileline();
            Ast_غوټه_بیان* ثابت fromp = nodep->fromp();

            // To extract an up-to-64-bit value from a wide source, up to 3 words might be needed
            // from the source operand. Each word might need to be shifted by a different amount.
            // All word indices and shift amounts are derived from the LSB expression, so it might
            // be duplicated several times. The problem arises when the LSB expression itself is
            // a similar select, which results in an exponential size expansion.
            //
            // To avoid this, if the required expressions derived from the LSB expression are
            // large, and the LSB expression is required multiple times for the expansion, then
            // the LSB expression is evaluated into a temporary variable. This bounds the size
            // of the expansion on each Sel instance, hence the total expansion of nested Sels
            // is bounded linearly with the number of nested Sels (instead of exponentially).

            // Return simplified 'exprp' if it simplifies to a small tree, otherwise nullptr
            ثابت خپلکاری tryFold = [&](Ast_غوټه_بیان* exprp) -> Ast_غوټه_بیان* {
                // Size limit for inlining expressions (any arbitrary value results in
                // a bounded expansion. Picked a value to allow common simple forms.)
                ساكن constexpr صحيح EXPAND_SEL_LSB_حد = 8;
                // Simplify
                exprp = V3Const::constifyEditCpp(exprp);
                // Accept if not larger than the limit
                إذا (!exprp->isLargerThan(EXPAND_SEL_LSB_حد)) أرجع exprp;
                // Delete if rejected
                VL_DO_DANGLING(exprp->delete_ونه(), exprp);
                أرجع تش_اشاره;
            };

            // Return the index of the word holding the bit 'offset' bits above the LSB
            ثابت خپلکاری کلمه_Idx = [&](uint32_t دننه_واټن) -> Ast_غوټه_بیان* {
                Ast_غوټه_بیان* ثابت msbp = جديد Ast_جمع_کړه{lfl, جديد AstConst{lfl, دننه_واټن},
                                                     nodep->lsbp()->clone_ونه_خالص(خطأ)};
                Ast_غوټه_بیان* ثابت idxp = new_کلمه_شاخص(msbp);
                إذا (!msbp->backp()) VL_DO_DANGLING(msbp->delete_ونه(), msbp);
                أرجع idxp;
            };

            // Bit index of the selected LSB within its word
            Ast_غوټه_بیان* lBitp = tryFold(new_Sel_بټ_بټ(nodep->lsbp()));
            ثابت منطقي aligned = lBitp && lBitp->isZero();

            // Which of the other 2 words the expansion might need
            ثابت منطقي mNeeded = پلنوالی > 1 && !aligned;
            ثابت منطقي hNeeded = پلنوالی > VL_EDATASIZE;
            UASSERT_OBJ(!hNeeded || nodep->isQuad(), nodep, "Width mismatch");

            // Word indices
            ثابت uint32_t m_Msb_دننه_واټن = std::min<uint32_t>(پلنوالی, VL_EDATASIZE) - 1;
            ثابت uint32_t h_Msb_دننه_واټن = پلنوالی - 1;
            Ast_غوټه_بیان* lIdxp = tryFold(new_کلمه_شاخص(nodep->lsbp()));
            Ast_غوټه_بیان* mIdxp = mNeeded ? tryFold(کلمه_Idx(m_Msb_دننه_واټن)) : تش_اشاره;
            Ast_غوټه_بیان* hIdxp = hNeeded ? tryFold(کلمه_Idx(h_Msb_دننه_واټن)) : تش_اشاره;

            // If the LSB expression is needed multiple times, evaluate it into a temporary
            ثابت صحيح nCopies = !lBitp + !lIdxp + (mNeeded && !mIdxp) + (hNeeded && !hIdxp);
            إذا (nCopies > 1 && m_funcp && m_stmtp && !isImpure(m_stmtp)) {
                ++m_nTmps;  // Use fresh set of temporaries
                Ast_غوټه_بیان* ثابت lsbp = nodep->lsbp()->unlink_Fr_شا();
                AstVar* ثابت tmpp = جمع_کړه_Local_Tmp(m_stmtp, "ExpandSel_Lsb", lsbp);
                nodep->lsbp(جديد AstVarRef{lfl, tmpp, VAccess::READ});
            }

            // Rebuild remaining expressions derived from LSB if needed, now using the temporary
            إذا (!lBitp) lBitp = new_Sel_بټ_بټ(nodep->lsbp());
            إذا (!lIdxp) lIdxp = new_کلمه_شاخص(nodep->lsbp());
            إذا (!mIdxp) mIdxp = mNeeded ? کلمه_Idx(m_Msb_دننه_واټن) : تش_اشاره;
            إذا (!hIdxp) hIdxp = hNeeded ? کلمه_Idx(h_Msb_دننه_واټن) : تش_اشاره;

            // Return word 'idxp' of 'fromp', without consuming 'idxp'
            ثابت خپلکاری کلمه_Sel_2 = [&](Ast_غوټه_بیان* idxp) -> Ast_غوټه_بیان* {
                Ast_غوټه_بیان* ثابت clonep = idxp->clone_ونه_خالص(خطأ);
                Ast_غوټه_بیان* ثابت wordp = new_کلمه_Sel_کلمه(ffl, fromp, clonep);
                إذا (!clonep->backp()) VL_DO_DANGLING(clonep->delete_ونه(), clonep);
                أرجع wordp;
            };

            // Construct term containing the bits of the low word - always needed
            Ast_غوټه_بیان* ثابت lTermp = [&]() -> Ast_غوټه_بیان* {
                Ast_غوټه_بیان* lWordp = کلمه_Sel_2(lIdxp);
                إذا (nodep->isQuad()) lWordp = جديد AstCCast{nfl, lWordp, nodep};
                أرجع جديد Ast_ښويدنه_R{nfl, lWordp, lBitp->clone_ونه_خالص(خطأ), nodep->پلنوالی()};
            }();

            // Construct term containing the bits of the middle word - if needed
            Ast_غوټه_بیان* ثابت mTermp = [&]() -> Ast_غوټه_بیان* {
                إذا (!mNeeded) أرجع تش_اشاره;

                Ast_غوټه_بیان* mWordp = کلمه_Sel_2(mIdxp);
                إذا (nodep->isQuad()) mWordp = جديد AstCCast{nfl, mWordp, nodep};
                Ast_غوټه_بیان* ثابت mShiftp = جديد AstSub{lfl, جديد AstConst{lfl, VL_EDATASIZE},
                                                        lBitp->clone_ونه_خالص(خطأ)};
                // If we're selecting bit zero, then all 32 bits in the mid word
                // get shifted << by 32 bits, so ignore them.
                ثابت V3_شمېره zero{nodep, long_Or_Quad_پلنوالی(nodep)};
                أرجع جديد AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    جديد AstEq{nfl, جديد AstConst{nfl, 0}, lBitp->clone_ونه_خالص(خطأ)},
                    // 0 :
                    جديد AstConst{nfl, zero},
                    //  midword >> (VL_EDATASIZE - (lbs % VL_EDATASIZE))
                    جديد Ast_ښويدنه_L{nfl, mWordp, mShiftp, nodep->پلنوالی()}};
            }();

            // Construct term containing the bits of the high word - if needed
            Ast_غوټه_بیان* ثابت hTermp = [&]() -> Ast_غوټه_بیان* {
                إذا (!hNeeded) أرجع تش_اشاره;

                Ast_غوټه_بیان* hWordp = کلمه_Sel_2(hIdxp);
                hWordp = جديد AstCCast{nfl, hWordp, nodep};
                Ast_غوټه_بیان* ثابت hShiftp = جديد AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    جديد AstEq{nfl, جديد AstConst{nfl, 0}, lBitp->clone_ونه_خالص(خطأ)},
                    // VL_EDATASIZE :
                    جديد AstConst{lfl, VL_EDATASIZE},
                    // 64 - (lbs % VL_EDATASIZE)
                    جديد AstSub{lfl, جديد AstConst{lfl, 64}, lBitp->clone_ونه_خالص(خطأ)}};
                أرجع جديد Ast_ښويدنه_L{nfl, hWordp, hShiftp, nodep->پلنوالی()};
            }();

            // Delete parts not captured during construction
            VL_DO_DANGLING(lBitp->delete_ونه(), lBitp);
            VL_DO_DANGLING(lIdxp->delete_ونه(), lIdxp);
            إذا (mIdxp) VL_DO_DANGLING(mIdxp->delete_ونه(), mIdxp);
            إذا (hIdxp) VL_DO_DANGLING(hIdxp->delete_ونه(), hIdxp);

            // Or reduce the terms
            Ast_غوټه_بیان* newp = lTermp;
            إذا (mTermp) newp = جديد AstOr{nfl, mTermp, newp};
            إذا (hTermp) newp = جديد AstOr{nfl, hTermp, newp};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } وإلا {  // Long/Quad from Long/Quad
            // No isImpure() check - can handle side effects in below
            did_نمونه(VExpand_نمونه::SEL_FROM_NARROW, nodep);
            File_کرښه* ثابت fl = nodep->fileline();
            Ast_غوټه_بیان* fromp = nodep->fromp()->unlink_Fr_شا();
            Ast_غوټه_بیان* ثابت lsbp = nodep->lsbp()->unlink_Fr_شا();
            إذا (nodep->isQuad() && !fromp->isQuad()) fromp = جديد AstCCast{fl, fromp, nodep};
            // {large}>>32 requires 64-bit shift operation; then cast
            Ast_غوټه_بیان* newp = جديد Ast_ښويدنه_R{fl, fromp, lsbp, fromp->پلنوالی()};
            newp->dtypeFrom(fromp);
            إذا (!nodep->isQuad() && fromp->isQuad()) newp = جديد AstCCast{fl, newp, nodep};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }

    منطقي expandWide(Ast_غوټه_ټاکل* nodep, AstSel* rhsp) {
        UASSERT_OBJ(nodep->پلنوالی_Min() == rhsp->پلنوالی_Const(), nodep, "Width mismatch");
        إذا (!doExpandWide(nodep)) أرجع خطأ;

        // Simplify the index, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->lsbp());

        // If it's a constant select and aligned, we can just copy the words
        إذا (ثابت AstConst* ثابت lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            ثابت uint32_t lsb = lsbConstp->منزل_UInt();
            إذا (VL_BITBIT_E(lsb) == 0) {
                did_نمونه(VExpand_نمونه::ټاکل_SEL_ALIGNED, nodep);
                ثابت uint32_t کلمه = VL_BITWORD_E(lsb);
                لكل (صحيح w = 0; w < nodep->پلنوالی_ټکي(); ++w) {
                    جمع_کړه_کلمه_ټاکل(nodep, w, new_Ast_کلمه_Sel_Clone(rhsp->fromp(), w + کلمه));
                }
                أرجع ریښتیا;
            }
        }

        did_نمونه(VExpand_نمونه::ټاکل_SEL_UNALIGNED, nodep);
        File_کرښه* ثابت flp = rhsp->fileline();

        // Use fresh set of temporaries
        ++m_nTmps;

        // Compute word index of LSB, store to temporary if not constant
        Ast_غوټه_بیان* ثابت کلمه_Lsbp = rhsp->lsbp()->clone_ونه_خالص(خطأ);
        Ast_غوټه_بیان* کلمه_Idxp = new_کلمه_شاخص(کلمه_Lsbp);
        إذا (!کلمه_Lsbp->backp()) VL_DO_DANGLING(کلمه_Lsbp->delete_ونه(), کلمه_Lsbp);
        کلمه_Idxp = V3Const::constifyEditCpp(کلمه_Idxp);
        إذا (!VN_IS(کلمه_Idxp, Const)) {
            AstVar* ثابت tmpp = جمع_کړه_Local_Tmp(nodep, "ExpandSel_WordIdx", کلمه_Idxp);
            کلمه_Idxp = جديد AstVarRef{flp, tmpp, VAccess::READ};
        }

        // Compute shift amounts and mask, store to temporaries if not constants
        Ast_غوټه_بیان* loShftp = تش_اشاره;
        Ast_غوټه_بیان* hiShftp = تش_اشاره;
        Ast_غوټه_بیان* hiMaskp = تش_اشاره;
        إذا (ثابت AstConst* ثابت lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            ثابت uint32_t بټ_دننه_واټن = VL_BITBIT_E(lsbConstp->منزل_UInt());
            // Must be unaligned, otherwise we would have handled it above
            UASSERT_OBJ(بټ_دننه_واټن, nodep, "Missed aligned wide select");
            loShftp = جديد AstConst{flp, بټ_دننه_واټن};
            hiShftp = جديد AstConst{flp, VL_EDATASIZE - بټ_دننه_واټن};
            hiMaskp = تش_اشاره;
        } وإلا {
            // Compute Low word shift amount: bottom bits of lsb index
            Ast_غوټه_بیان* ثابت lsbp = rhsp->lsbp()->clone_ونه_خالص(خطأ);
            loShftp = جديد AstAnd{flp, جديد AstConst{flp, VL_SIZEBITS_E}, lsbp};
            AstVar* ثابت loTmpp = جمع_کړه_Local_Tmp(nodep, "ExpandSel_LoShift", loShftp);
            loShftp = جديد AstVarRef{flp, loTmpp, VAccess::READ};

            // Compute if aligned: Low word shift amount is 0
            Ast_غوټه_بیان* ثابت zerop = جديد AstConst{flp, 0};
            Ast_غوټه_بیان* alignedp = جديد AstEq{flp, zerop, loShftp->clone_ونه_خالص(خطأ)};
            AstVar* ثابت alignedTmpp = جمع_کړه_Local_Tmp(nodep, "ExpandSel_Aligned", alignedp);
            alignedp = جديد AstVarRef{flp, alignedTmpp, VAccess::READ};

            // Computed High word shift amount: 0 if aligned else VL_EDATASIZE - Low word shift
            Ast_غوټه_بیان* ثابت edsp = جديد AstConst{flp, VL_EDATASIZE};
            Ast_غوټه_بیان* ثابت subp = جديد AstSub{flp, edsp, loShftp->clone_ونه_خالص(خطأ)};
            hiShftp = جديد AstCond{flp, alignedp, جديد AstConst{flp, 0}, subp};
            AstVar* ثابت hiTmpp = جمع_کړه_Local_Tmp(nodep, "ExpandSel_HiShift", hiShftp);
            hiShftp = جديد AstVarRef{flp, hiTmpp, VAccess::READ};

            // Compute the High word mask: 0 if aligned, ones otherwise
            hiMaskp = جديد AstCond{flp, alignedp->clone_ونه_خالص(خطأ),
                                  جديد AstConst{flp, AstConst::SizedEData{}, 0},
                                  جديد AstConst{flp, AstConst::SizedEData{}, -1ULL}};
            AstVar* ثابت ماسک_Tmpp = جمع_کړه_Local_Tmp(nodep, "ExpandSel_HiMask", hiMaskp);
            hiMaskp = جديد AstVarRef{flp, ماسک_Tmpp, VAccess::READ};
        }

        // Create each word of the selected result
        Ast_غوټه_بیان* ثابت fromp = rhsp->fromp();
        ثابت صحيح sel_ټکي = nodep->پلنوالی_ټکي();
        لكل (صحيح w = 0; w < sel_ټکي; ++w) {
            // Grab bits from word 'VL_BITWORD_E(lsb) + w'
            Ast_غوټه_بیان* ثابت loWordp = new_کلمه_Sel_کلمه(fromp->fileline(), fromp, کلمه_Idxp, w);
            Ast_غوټه_بیان* ثابت loShftClonep = loShftp->clone_ونه_خالص(خطأ);
            Ast_غوټه_بیان* ثابت lop = جديد Ast_ښويدنه_R{flp, loWordp, loShftClonep, VL_EDATASIZE};
            // Grab bits from word 'VL_BITWORD_E(lsb) + w + 1'
            Ast_غوټه_بیان* hiWordp = new_کلمه_Sel_کلمه(fromp->fileline(), fromp, کلمه_Idxp, w + 1);
            // For the last word of the result, avoid an OOB access when the Sel is not OOB
            إذا (w == sel_ټکي - 1) {
                ثابت uint32_t max = fromp->پلنوالی_ټکي() - w - 1;
                Ast_غوټه_بیان* ثابت maxp = جديد AstConst{flp, max};
                Ast_غوټه_بیان* ثابت condp = جديد AstGte{flp, کلمه_Idxp->clone_ونه_خالص(خطأ), maxp};
                Ast_غوټه_بیان* ثابت zerop = جديد AstConst{flp, AstConst::SizedEData{}, 0};
                hiWordp = جديد AstCond{flp, condp, zerop, hiWordp};
            }
            Ast_غوټه_بیان* ثابت hiShftClonep = hiShftp->clone_ونه_خالص(خطأ);
            Ast_غوټه_بیان* ثابت hiPartp = جديد Ast_ښويدنه_L{flp, hiWordp, hiShftClonep, VL_EDATASIZE};
            Ast_غوټه_بیان* ثابت hip
                = hiMaskp ? جديد AstAnd{flp, hiPartp, hiMaskp->clone_ونه_خالص(خطأ)} : hiPartp;
            // Combine them
            جمع_کړه_کلمه_ټاکل(nodep, w, جديد AstOr{nodep->fileline(), hip, lop});
        }

        // Delete parts not captured during construction
        إذا (!کلمه_Idxp->backp()) VL_DO_DANGLING(کلمه_Idxp->delete_ونه(), کلمه_Idxp);
        إذا (!loShftp->backp()) VL_DO_DANGLING(loShftp->delete_ونه(), loShftp);
        إذا (!hiShftp->backp()) VL_DO_DANGLING(hiShftp->delete_ونه(), hiShftp);
        إذا (hiMaskp && !hiMaskp->backp()) VL_DO_DANGLING(hiMaskp->delete_ونه(), hiMaskp);

        أرجع ریښتیا;
    }

    منطقي expandLhs(Ast_غوټه_ټاکل* nodep, AstSel* lhsp) {
        // Possibilities
        //      destp: wide or narrow
        //      rhsp:  wide (destp must be wide), narrow, or 1 bit wide
        //      rhsp:  may be allones and can remove AND NOT gate
        //      lsbp:  constant or variable
        // Yuk.
        إذا (isImpure(nodep)) أرجع خطأ;
        File_کرښه* ثابت nfl = nodep->fileline();
        File_کرښه* ثابت lfl = lhsp->fileline();
        ثابت منطقي destwide = lhsp->fromp()->isWide();
        ثابت منطقي ones = nodep->rhsp()->isAllOnesV();
        إذا (VN_IS(lhsp->lsbp(), Const)) {
            // The code should work without this constant test, but it won't
            // constify as nicely as we'd like.
            Ast_غوټه_بیان* rhsp = nodep->rhsp()->unlink_Fr_شا();
            Ast_غوټه_بیان* ثابت destp = lhsp->fromp()->unlink_Fr_شا();
            ثابت صحيح lsb = lhsp->lsbConst();
            ثابت صحيح msb = lhsp->msbConst();
            V3_شمېره maskset{nodep, destp->پلنوالی_Min()};
            maskset.سېټ_ماسک(msb + 1 - lsb, lsb);
            V3_شمېره maskold{nodep, destp->پلنوالی_Min()};
            maskold.opNot(maskset);
            إذا (destwide) {
                did_نمونه(VExpand_نمونه::ټاکل_منزل_SEL_CONST_WIDE, nodep);
                لكل (صحيح w = 0; w < destp->پلنوالی_ټکي(); ++w) {
                    إذا (w >= VL_BITWORD_E(lsb) && w <= VL_BITWORD_E(msb)) {
                        // else we would just be setting it to the same exact value
                        Ast_غوټه_بیان* oldvalp = new_Ast_کلمه_Sel_Clone(destp, w);
                        fixCloneLvalue(oldvalp);
                        إذا (!ones) {
                            oldvalp = جديد AstAnd{
                                lfl,
                                جديد AstConst{lfl, AstConst::SizedEData{}, maskold.edata_کلمه(w)},
                                oldvalp};
                        }

                        // Appropriate word of new value to insert:
                        Ast_غوټه_بیان* newp = new_کلمه_Grab_ښويدنه(lfl, w, rhsp, lsb);

                        // Apply cleaning at the top word of the destination
                        // (no cleaning to do if dst's width is a whole number
                        // of words).
                        إذا (w == destp->پلنوالی_ټکي() - 1 && VL_BITBIT_E(destp->پلنوالی_Min()) != 0) {
                            V3_شمېره cleanmask{nodep, VL_EDATASIZE};
                            cleanmask.سېټ_ماسک(VL_BITBIT_E(destp->پلنوالی_Min()));
                            newp = جديد AstAnd{lfl, newp, جديد AstConst{lfl, cleanmask}};
                        }
                        Ast_غوټه_بیان* ثابت orp
                            = V3Const::constifyEditCpp(جديد AstOr{lfl, oldvalp, newp});
                        جمع_کړه_کلمه_ټاکل(nodep, w, destp, orp);
                    }
                }
                VL_DO_DANGLING(rhsp->delete_ونه(), rhsp);
                VL_DO_DANGLING(destp->delete_ونه(), destp);
            } وإلا {
                did_نمونه(VExpand_نمونه::ټاکل_منزل_SEL_CONST_NARROW, nodep);
                إذا (destp->isQuad() && !rhsp->isQuad()) rhsp = جديد AstCCast{nfl, rhsp, nodep};
                Ast_غوټه_بیان* oldvalp = destp->clone_ونه_خالص(ریښتیا);
                fixCloneLvalue(oldvalp);
                إذا (!ones) oldvalp = جديد AstAnd{lfl, جديد AstConst{lfl, maskold}, oldvalp};

                // The bit-select can refer to bits outside the width of nodep
                // which we aren't allowed to assign to.  This is a mask of the
                // valid range of nodep which we apply to the new shifted RHS.
                V3_شمېره cleanmask{nodep, destp->پلنوالی_Min()};
                cleanmask.سېټ_ماسک(destp->پلنوالی_Min());
                Ast_غوټه_بیان* ثابت shifted = جديد Ast_ښويدنه_L{
                    lfl, rhsp, جديد AstConst{lfl, static_cast<uint32_t>(lsb)}, destp->پلنوالی()};
                Ast_غوټه_بیان* ثابت cleaned
                    = جديد AstAnd{lfl, shifted, جديد AstConst{lfl, cleanmask}};
                Ast_غوټه_بیان* ثابت orp
                    = V3Const::constifyEditCpp(جديد AstOr{lfl, oldvalp, cleaned});
                ورننباسه_Before(nodep, جديد Ast_ټاکل{nfl, destp, orp});
            }
            أرجع ریښتیا;
        } وإلا {  // non-const select offset
            إذا (destwide && lhsp->پلنوالی_Const() == 1) {
                did_نمونه(VExpand_نمونه::ټاکل_منزل_SEL_VAR_WIDE_ONE_بټ, nodep);
                Ast_غوټه_بیان* ثابت rhsp = nodep->rhsp()->unlink_Fr_شا();
                Ast_غوټه_بیان* ثابت destp = lhsp->fromp()->unlink_Fr_شا();
                Ast_غوټه_بیان* oldvalp
                    = new_کلمه_Sel_بټ(lfl, destp->clone_ونه_خالص(خطأ), lhsp->lsbp());
                fixCloneLvalue(oldvalp);
                إذا (!ones) {
                    oldvalp = جديد AstAnd{
                        lfl,
                        جديد AstNot{
                            lfl, جديد Ast_ښويدنه_L{lfl, جديد AstConst{nfl, 1},
                                               // newSelBitBit may exceed the MSB of this variable.
                                               // That's ok as we'd just AND with a larger value,
                                               // but oldval would clip the upper bits to sanity
                                               new_Sel_بټ_بټ(lhsp->lsbp()), VL_EDATASIZE}},
                        oldvalp};
                }
                // Restrict the shift amount to 0-31, see bug804.
                Ast_غوټه_بیان* ثابت shiftp = جديد AstAnd{nfl, lhsp->lsbp()->clone_ونه_خالص(ریښتیا),
                                                       جديد AstConst{nfl, VL_EDATASIZE - 1}};
                Ast_غوټه* ثابت newp = جديد Ast_ټاکل{
                    nfl, new_کلمه_Sel_بټ(nfl, destp, lhsp->lsbp()),
                    جديد AstOr{lfl, oldvalp, جديد Ast_ښويدنه_L{lfl, rhsp, shiftp, VL_EDATASIZE}}};
                ورننباسه_Before(nodep, newp);
                أرجع ریښتیا;
            } وإلا إذا (destwide) {
                UINFO(8, "    ASSIGNSEL(varlsb,wide) -- NoOp -- " << nodep);
                //   For wide destp, we can either form a equation for every destination word,
                // with the appropriate long equation of if it's being written or not.
                //   Or, we can use a LHS variable arraysel with
                //   non-constant index to set the vector.
                // Doing the variable arraysel is better for globals and large arrays,
                // doing every word is better for temporaries and if we're setting most words
                // since it may result in better substitution optimizations later.
                //   This results in so much code, we're better off leaving a function call.
                // Reconsider if we get subexpression elimination.
                أرجع خطأ;
            } وإلا {
                did_نمونه(VExpand_نمونه::ټاکل_منزل_SEL_VAR_NARROW, nodep);
                // nodep->dumpTree("-  old: ");
                Ast_غوټه_بیان* rhsp = nodep->rhsp()->unlink_Fr_شا();
                Ast_غوټه_بیان* ثابت destp = lhsp->fromp()->unlink_Fr_شا();
                Ast_غوټه_بیان* oldvalp = destp->clone_ونه_خالص(ریښتیا);
                fixCloneLvalue(oldvalp);

                V3_شمېره maskwidth{nodep, destp->پلنوالی_Min()};
                maskwidth.سېټ_ماسک(lhsp->پلنوالی_Const());

                إذا (destp->isQuad() && !rhsp->isQuad()) rhsp = جديد AstCCast{nfl, rhsp, nodep};
                إذا (!ones) {
                    oldvalp = جديد AstAnd{
                        lfl,
                        جديد AstNot{lfl, جديد Ast_ښويدنه_L{lfl, جديد AstConst{nfl, maskwidth},
                                                      lhsp->lsbp()->clone_ونه_خالص(ریښتیا),
                                                      destp->پلنوالی()}},
                        oldvalp};
                }
                Ast_غوټه_بیان* newp
                    = جديد Ast_ښويدنه_L{lfl, rhsp, lhsp->lsbp()->clone_ونه_خالص(ریښتیا), destp->پلنوالی()};
                // Apply cleaning to the new value being inserted.  Mask is
                // slightly wider than necessary to avoid an AND with all ones
                // being optimized out.  No need to clean if destp is
                // quad-sized as there are no extra bits to contaminate
                إذا (destp->پلنوالی_Min() != 64) {
                    V3_شمېره cleanmask{nodep, destp->پلنوالی_Min() + 1};
                    cleanmask.سېټ_ماسک(destp->پلنوالی_Min());
                    newp = جديد AstAnd{lfl, newp, جديد AstConst{lfl, cleanmask}};
                }

                ورننباسه_Before(nodep, جديد Ast_ټاکل{nfl, destp, جديد AstOr{lfl, oldvalp, newp}});
                أرجع ریښتیا;
            }
        }
    }

    فراغ لیدنه(AstConcat* nodep) override {
        إذا (nodep->user1_سېټ_Once()) أرجع;  // Process once
        iterateChildren(nodep);
        إذا (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } وإلا {
            // No isImpure() check - can handle side effects in below
            did_نمونه(VExpand_نمونه::CONCAT, nodep);
            File_کرښه* ثابت fl = nodep->fileline();
            Ast_غوټه_بیان* lhsp = nodep->lhsp()->unlink_Fr_شا();
            Ast_غوټه_بیان* rhsp = nodep->rhsp()->unlink_Fr_شا();
            ثابت uint32_t rhsshift = rhsp->پلنوالی_Min();
            إذا (nodep->isQuad() && !lhsp->isQuad()) lhsp = جديد AstCCast{fl, lhsp, nodep};
            إذا (nodep->isQuad() && !rhsp->isQuad()) rhsp = جديد AstCCast{fl, rhsp, nodep};
            Ast_غوټه_بیان* ثابت newp = جديد AstOr{
                fl, جديد Ast_ښويدنه_L{fl, lhsp, جديد AstConst{fl, rhsshift}, nodep->پلنوالی()}, rhsp};
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    منطقي expandWide(Ast_غوټه_ټاکل* nodep, AstConcat* rhsp) {
        إذا (!doExpandWide(rhsp)) أرجع خطأ;
        did_نمونه(VExpand_نمونه::ټاکل_CONCAT, nodep);
        File_کرښه* ثابت fl = rhsp->fileline();
        // Lhs or Rhs may be word, long, or quad.
        // newAstWordSelClone nicely abstracts the difference.
        ثابت صحيح rhsshift = rhsp->rhsp()->پلنوالی_Min();
        // Sometimes doing the words backwards is preferable.
        // When we have x={x,foo} backwards is better, when x={foo,x} forward is better
        // However V3Subst tends to rip this up, so not worth optimizing now.
        لكل (صحيح w = 0; w < rhsp->پلنوالی_ټکي(); ++w) {
            جمع_کړه_کلمه_ټاکل(nodep, w,
                          جديد AstOr{fl, new_کلمه_Grab_ښويدنه(fl, w, rhsp->lhsp(), rhsshift),
                                    new_Ast_کلمه_Sel_Clone(rhsp->rhsp(), w)});
        }
        أرجع ریښتیا;
    }

    فراغ لیدنه(AstReplicate* nodep) override {
        إذا (nodep->user1_سېټ_Once()) أرجع;  // Process once
        iterateChildren(nodep);
        إذا (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } وإلا {
            إذا (isImpure(nodep)) أرجع;
            File_کرښه* ثابت fl = nodep->fileline();
            Ast_غوټه_بیان* lhsp = nodep->srcp()->unlink_Fr_شا();
            Ast_غوټه_بیان* newp;
            ثابت صحيح lhswidth = lhsp->پلنوالی_Min();
            إذا (lhswidth == 1) {
                did_نمونه(VExpand_نمونه::REPLICATE_ONE_بټ, nodep);
                newp = جديد AstNegate{fl, lhsp};
            } وإلا {
                did_نمونه(VExpand_نمونه::REPLICATE, nodep);
                ثابت AstConst* ثابت constp = VN_AS(nodep->countp(), Const);
                UASSERT_OBJ(constp, nodep,
                            "Replication value isn't a constant.  Checked earlier!");
                ثابت uint32_t times = constp->منزل_UInt();
                إذا (nodep->isQuad() && !lhsp->isQuad()) lhsp = جديد AstCCast{fl, lhsp, nodep};
                newp = lhsp->clone_ونه_خالص(ریښتیا);
                لكل (بینښه repnum = 1; repnum < times; repnum++) {
                    ثابت صحيح rhsshift = repnum * lhswidth;
                    newp = جديد AstOr{
                        fl,
                        جديد Ast_ښويدنه_L{fl, lhsp->clone_ونه_خالص(ریښتیا),
                                      جديد AstConst{fl, static_cast<uint32_t>(rhsshift)},
                                      nodep->پلنوالی()},
                        newp};
                    newp->dtypeFrom(nodep);  // Unsigned
                }
                VL_DO_DANGLING(lhsp->delete_ونه(), lhsp);  // Never used
            }
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    منطقي expandWide(Ast_غوټه_ټاکل* nodep, AstReplicate* rhsp) {
        إذا (!doExpandWide(rhsp)) أرجع خطأ;
        did_نمونه(VExpand_نمونه::ټاکل_REPLICATE, nodep);
        File_کرښه* ثابت fl = nodep->fileline();
        Ast_غوټه_بیان* ثابت lhsp = rhsp->srcp();
        ثابت صحيح lhswidth = lhsp->پلنوالی_Min();
        ثابت AstConst* ثابت constp = VN_AS(rhsp->countp(), Const);
        UASSERT_OBJ(constp, rhsp, "Replication value isn't a constant.  Checked earlier!");
        ثابت uint32_t times = constp->منزل_UInt();
        لكل (صحيح w = 0; w < rhsp->پلنوالی_ټکي(); ++w) {
            Ast_غوټه_بیان* newp;
            إذا (lhswidth == 1) {
                newp = جديد AstNegate{fl, lhsp->clone_ونه_خالص(ریښتیا)};
                // Replicate always unsigned
                newp->dtype_Chg_پلنوالی_Signed(VL_EDATASIZE, VL_EDATASIZE, VSigning::UNSIGNED);
            } وإلا {
                newp = new_Ast_کلمه_Sel_Clone(lhsp, w);
                File_کرښه* ثابت rfl = rhsp->fileline();
                لكل (بینښه repnum = 1; repnum < times; repnum++) {
                    newp = جديد AstOr{fl, new_کلمه_Grab_ښويدنه(rfl, w, lhsp, lhswidth * repnum), newp};
                }
            }
            جمع_کړه_کلمه_ټاکل(nodep, w, newp);
        }
        أرجع ریښتیا;
    }

    فراغ لیدنه_Eq_Neq(Ast_غوټه_Biop* nodep) {
        إذا (nodep->user1_سېټ_Once()) أرجع;  // Process once
        iterateChildren(nodep);
        إذا (nodep->lhsp()->isWide()
            && !(VN_IS(nodep->lhsp()->dtypep()->skipRefp(), بهیر_DType)
                 || VN_IS(nodep->rhsp()->dtypep()->skipRefp(), بهیر_DType))) {
            إذا (isImpure(nodep)) أرجع;
            إذا (!doExpandWide(nodep->lhsp())) أرجع;
            إذا (!doExpandWide(nodep->rhsp())) أرجع;
            did_نمونه(VN_IS(nodep, Neq) ? VExpand_نمونه::WORDIZE_NEQ
                                         : VExpand_نمونه::WORDIZE_EQ,
                       nodep);
            // -> (0=={or{for each_word{WORDSEL(lhs,#)^WORDSEL(rhs,#)}}}
            File_کرښه* ثابت fl = nodep->fileline();
            Ast_غوټه_بیان* newp = تش_اشاره;
            لكل (صحيح w = 0; w < nodep->lhsp()->پلنوالی_ټکي(); ++w) {
                Ast_غوټه_بیان* ثابت eqp = جديد AstXor{fl, new_Ast_کلمه_Sel_Clone(nodep->lhsp(), w),
                                                    new_Ast_کلمه_Sel_Clone(nodep->rhsp(), w)};
                newp = newp ? جديد AstOr{fl, newp, eqp} : eqp;
            }
            إذا (VN_IS(nodep, Neq)) {
                newp = جديد AstNeq{fl, جديد AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            } وإلا {
                newp = جديد AstEq{fl, جديد AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            }
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    فراغ لیدنه(AstEq* nodep) override { لیدنه_Eq_Neq(nodep); }
    فراغ لیدنه(AstNeq* nodep) override { لیدنه_Eq_Neq(nodep); }

    فراغ لیدنه(AstRedOr* nodep) override {
        إذا (nodep->user1_سېټ_Once()) أرجع;  // Process once
        iterateChildren(nodep);
        File_کرښه* ثابت fl = nodep->fileline();
        إذا (nodep->lhsp()->isWide()) {
            إذا (isImpure(nodep)) أرجع;
            إذا (!doExpandWide(nodep->lhsp())) أرجع;
            did_نمونه(VExpand_نمونه::WORDIZE_REDOR, nodep);
            // -> (0!={or{for each_word{WORDSEL(lhs,#)}}}
            Ast_غوټه_بیان* newp = تش_اشاره;
            لكل (صحيح w = 0; w < nodep->lhsp()->پلنوالی_ټکي(); ++w) {
                Ast_غوټه_بیان* ثابت eqp = new_Ast_کلمه_Sel_Clone(nodep->lhsp(), w);
                newp = newp ? جديد AstOr{fl, newp, eqp} : eqp;
            }
            newp = جديد AstNeq{fl, جديد AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } وإلا {
            // No isImpure() check - can handle side effects in below
            did_نمونه(VExpand_نمونه::REDOR_منزل_NEQ, nodep);
            Ast_غوټه_بیان* ثابت lhsp = nodep->lhsp()->unlink_Fr_شا();
            Ast_غوټه_بیان* ثابت newp = جديد AstNeq{
                fl, جديد AstConst{fl, AstConst::Widthed_ارزښت{}, long_Or_Quad_پلنوالی(nodep), 0}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    فراغ لیدنه(AstRedAnd* nodep) override {
        إذا (nodep->user1_سېټ_Once()) أرجع;  // Process once
        iterateChildren(nodep);
        File_کرښه* ثابت fl = nodep->fileline();
        إذا (nodep->lhsp()->isWide()) {
            إذا (isImpure(nodep)) أرجع;
            إذا (!doExpandWide(nodep->lhsp())) أرجع;
            did_نمونه(VExpand_نمونه::WORDIZE_REDAND, nodep);
            // -> (0!={and{for each_word{WORDSEL(lhs,#)}}}
            Ast_غوټه_بیان* newp = تش_اشاره;
            لكل (صحيح w = 0; w < nodep->lhsp()->پلنوالی_ټکي(); ++w) {
                Ast_غوټه_بیان* eqp = new_Ast_کلمه_Sel_Clone(nodep->lhsp(), w);
                إذا (w == nodep->lhsp()->پلنوالی_ټکي() - 1) {
                    // Rather than doing a (slowish) ==##, we OR in the
                    // bits that aren't part of the mask
                    eqp = جديد AstOr{fl, جديد AstConst{fl, not_Wide_ماسک(nodep->lhsp())},
                                    // Bug in cppcheck
                                    // cppcheck-suppress memleak
                                    eqp};
                }
                newp = newp ? جديد AstAnd{fl, newp, eqp} : eqp;
            }
            newp = جديد AstEq{fl, جديد AstConst{fl, AstConst::SizedEData{}, VL_ماسک_E(VL_EDATASIZE)},
                             newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } وإلا {
            // No isImpure() check - can handle side effects in below
            did_نمونه(VExpand_نمونه::REDAND_منزل_EQ, nodep);
            Ast_غوټه_بیان* ثابت lhsp = nodep->lhsp()->unlink_Fr_شا();
            Ast_غوټه_بیان* ثابت newp = جديد AstEq{fl, جديد AstConst{fl, کلمه_ماسک(lhsp)}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    فراغ لیدنه(AstRedXor* nodep) override {
        إذا (nodep->user1_سېټ_Once()) أرجع;  // Process once
        iterateChildren(nodep);
        إذا (nodep->lhsp()->isWide()) {
            إذا (isImpure(nodep)) أرجع;
            إذا (!doExpandWide(nodep->lhsp())) أرجع;
            did_نمونه(VExpand_نمونه::WORDIZE_REDXOR, nodep);
            // -> (0!={redxor{for each_word{XOR(WORDSEL(lhs,#))}}}
            File_کرښه* ثابت fl = nodep->fileline();
            Ast_غوټه_بیان* newp = تش_اشاره;
            لكل (صحيح w = 0; w < nodep->lhsp()->پلنوالی_ټکي(); ++w) {
                Ast_غوټه_بیان* ثابت eqp = new_Ast_کلمه_Sel_Clone(nodep->lhsp(), w);
                newp = newp ? جديد AstXor{fl, newp, eqp} : eqp;
            }
            newp = جديد AstRedXor{fl, newp};
            UINFO(8, "    Wordize REDXORnew " << newp);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
        // We don't reduce non-wide XORs, as its more efficient to use a temp register,
        // which the inlined function does nicely.
    }

    فراغ لیدنه(Ast_غوټه_Stmt* nodep) override {
        إذا (nodep->user1_سېټ_Once()) أرجع;  // Process once
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه_ټاکل* nodep) override {
        إذا (nodep->user1_سېټ_Once()) أرجع;  // Process once
        إذا (VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            أرجع;  // Skip for UnpackArrayDType
        }

        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
        منطقي did = خطأ;
        إذا (nodep->isWide()  //
            && ((VN_IS(nodep->lhsp(), VarRef) || VN_IS(nodep->lhsp(), ArraySel)))
            && !AstVar::scVarRecurse(nodep->lhsp())  // Need special function for SC
            && !AstVar::scVarRecurse(nodep->rhsp())) {
            إذا (AstConst* ثابت rhsp = VN_CAST(nodep->rhsp(), Const)) {
                did = expandWide(nodep, rhsp);
            } وإلا إذا (AstVarRef* ثابت rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                did = expandWide(nodep, rhsp);
            } وإلا إذا (AstSel* ثابت rhsp = VN_CAST(nodep->rhsp(), Sel)) {
                did = expandWide(nodep, rhsp);
            } وإلا إذا (AstArraySel* ثابت rhsp = VN_CAST(nodep->rhsp(), ArraySel)) {
                did = expandWide(nodep, rhsp);
            } وإلا إذا (AstConcat* ثابت rhsp = VN_CAST(nodep->rhsp(), Concat)) {
                did = expandWide(nodep, rhsp);
            } وإلا إذا (AstExtend* ثابت rhsp = VN_CAST(nodep->rhsp(), Extend)) {
                did = expandWide(nodep, rhsp);
            } وإلا إذا (AstReplicate* ثابت rhsp = VN_CAST(nodep->rhsp(), Replicate)) {
                did = expandWide(nodep, rhsp);
            } وإلا إذا (AstAnd* ثابت rhsp = VN_CAST(nodep->rhsp(), And)) {
                did = expandWide(nodep, rhsp);
            } وإلا إذا (AstOr* ثابت rhsp = VN_CAST(nodep->rhsp(), Or)) {
                did = expandWide(nodep, rhsp);
            } وإلا إذا (AstNot* ثابت rhsp = VN_CAST(nodep->rhsp(), Not)) {
                did = expandWide(nodep, rhsp);
            } وإلا إذا (AstXor* ثابت rhsp = VN_CAST(nodep->rhsp(), Xor)) {
                did = expandWide(nodep, rhsp);
            } وإلا إذا (Ast_ښويدنه_L* ثابت rhsp = VN_CAST(nodep->rhsp(), ښويدنه_L)) {
                did = expand_Wide_ښويدنه(nodep, rhsp, /* isLeftShift: */ ریښتیا);
            } وإلا إذا (Ast_ښويدنه_R* ثابت rhsp = VN_CAST(nodep->rhsp(), ښويدنه_R)) {
                did = expand_Wide_ښويدنه(nodep, rhsp, /* isLeftShift: */ خطأ);
            } وإلا إذا (AstCond* ثابت rhsp = VN_CAST(nodep->rhsp(), Cond)) {
                did = expandWide(nodep, rhsp);
            }
        } وإلا إذا (AstSel* ثابت lhsp = VN_CAST(nodep->lhsp(), Sel)) {
            did = expandLhs(nodep, lhsp);
        }
        // Cleanup common code
        إذا (did) VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
    }

    //--------------------
    فراغ لیدنه(AstVar*) override {}  // Don't hit varrefs under vars
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    څرګند ExpandVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ExpandVisitor() override {
        V3Stats::جمع_کړه_Stat("Optimizations, Expand, expanded wides", m_statWides);
        V3Stats::جمع_کړه_Stat("Optimizations, Expand, expanded wide words", m_stat_Wide_ټکي);
        V3Stats::جمع_کړه_Stat("Optimizations, Expand, not expanded impure", m_statImpure);
        V3Stats::جمع_کړه_Stat("Optimizations, Expand, not expanded limit", m_statWideLimited);
        V3Stats::جمع_کړه_Stat("Optimizations, Expand, temporaries created", m_statTemps);
        ثابت خپلکاری emitStat = [هذا](VExpand_نمونه نمونه_2) {
            std::string str{نمونه_2.ascii()};
            std::transform(str.begin(), str.پای_ټکی(), str.begin(), [](بینښه حرف c) {  //
                أرجع c == '_' ? ' ' : std::tolower(c);
            });
            V3Stats::جمع_کړه_Stat("Optimizations, Expand, pattern " + str, m_statPatterns[نمونه_2]);
        };
#تعریف_کړه EXPAND_نمونه_EMIT_STAT(id) emitStat(VExpand_نمونه::id);
        FOR_EACH_EXPAND_نمونه(EXPAND_نمونه_EMIT_STAT)
#تعریف_لغوه EXPAND_نمونه_EMIT_STAT
    }
};

//######################################################################
// Expand class functions

فراغ V3Expand::expandAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ثابت ExpandOkVisitor okVisitor{nodep};
        ExpandVisitor{nodep};
    }  // Destruct before checking
    V3Global::dump_Check_Global_ونه("expand", 0, dump_ونه_Either_Level() >= 3);
}
