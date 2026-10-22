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
#تعریف_کن FOR_EACH_EXPAND_الگو(macro) \
    macro(تخصیص_بده_AND) \
    macro(تخصیص_بده_ARRAYSEL) \
    macro(تخصیص_بده_CONCAT) \
    macro(تخصیص_بده_COND) \
    macro(تخصیص_بده_CONST) \
    macro(تخصیص_بده_EXTEND) \
    macro(تخصیص_بده_NOT) \
    macro(تخصیص_بده_OR) \
    macro(تخصیص_بده_REPLICATE) \
    macro(تخصیص_بده_SEL_ALIGNED) \
    macro(تخصیص_بده_SEL_UNALIGNED) \
    macro(تخصیص_بده_SHIFTL) \
    macro(تخصیص_بده_SHIFTR) \
    macro(تخصیص_بده_مقصد_SEL_CONST_NARROW) \
    macro(تخصیص_بده_مقصد_SEL_CONST_WIDE) \
    macro(تخصیص_بده_مقصد_SEL_VAR_NARROW) \
    macro(تخصیص_بده_مقصد_SEL_VAR_WIDE_ONE_بیت) \
    macro(تخصیص_بده_VARREF) \
    macro(تخصیص_بده_XOR) \
    macro(CONCAT) \
    macro(REDAND_مقصد_EQ) \
    macro(REDOR_مقصد_NEQ) \
    macro(REPLICATE) \
    macro(REPLICATE_ONE_بیت) \
    macro(SEL_FROM_NARROW) \
    macro(SEL_FROM_WIDE) \
    macro(WORDIZE_EQ) \
    macro(WORDIZE_NEQ) \
    macro(WORDIZE_REDAND) \
    macro(WORDIZE_REDOR) \
    macro(WORDIZE_REDXOR)
// clang-format on

صنف VExpand_الگو final {
عام:
    تعداد en : uint8_t {
#تعریف_کن EXPAND_الگو_ID(id) id,
        FOR_EACH_EXPAND_الگو(EXPAND_الگو_ID)
#لغو_تعریف EXPAND_الگو_ID
            ENUM_انتها
    };
    تعداد en m_e;
    ثابت حرف* ascii() ثابت {
        ساكن ثابت حرف* ثابت نامها[] = {
#تعریف_کن EXPAND_الگو_NAME(id) #id,
            FOR_EACH_EXPAND_الگو(EXPAND_الگو_NAME)
#لغو_تعریف EXPAND_الگو_NAME
                "_ENUM_END"  //
        };
        أرجع نامها[m_e];
    }

    // cppcheck-suppress noExplicitConstructor
    VExpand_الگو(en _e)
        : m_e{_e} {}
    عملگر en() ثابت { أرجع m_e; }
};

//######################################################################
// Find nodes with side effects, to mark as non-expandable

صنف ExpandOkVisitor final : عام VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> bool.  Is pure (along with all children)
    ثابت VNUser2_درون_Use m_inuser2;

    // STATE - for current visit position (use VL_RESTORER)
    // Tracks similar to AstNode::isTreePureRecurse(), but avoid O(n^2)
    // False = pure, as nodes that ExpandVisitor inserts preserve pureness
    منطقي m_isImpure = درست;  // Currently pure

    فراغ بازدید(Ast_گره* nodep) override {
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
    صریح ExpandOkVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~ExpandOkVisitor() = افتراضي;
};

//######################################################################
// Expand state, as a visitor of each AstNode

صنف ExpandVisitor final : عام VNVisitor {
    // NODE STATE
    //  AstNode::user1()        -> bool.  Processed
    //  AstNode::user2()        -> See ExpandOkVisitor
    ثابت VNUser1_درون_Use m_inuser1;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_گره* m_stmtp = اشاره_تهی;  // Current statement

    // STATE - across all visitors
    AstCFunc* m_funcp = اشاره_تهی;  // Current function
    VDouble0 m_statImpure;  // Statistic tracking
    VDouble0 m_statWides;  // Statistic tracking
    VDouble0 m_stat_Wide_واژگان;  // Statistic tracking
    VDouble0 m_statWideLimited;  // Statistic tracking
    VDouble0 m_statTemps;  // Statistic tracking
    // Number of applications of each expansion pattern for statistics
    std::array<VDouble0, VExpand_الگو::ENUM_انتها> m_statPatterns;

    // STATE - for current function
    اندازه_t m_nTmps = 0;  // Sequence numbers for temopraries

    // METHODS
    // Note the application of the given expansion pattern to 'nodep'
    فراغ did_الگو(VExpand_الگو الگو_2, Ast_گره* nodep) {
        UINFO(8, "    " << الگو_2.ascii() << " " << nodep);
        ++m_statPatterns[الگو_2];
    }

    // Use state that ExpandOkVisitor calculated
    ساكن منطقي isImpure(Ast_گره* nodep) {
        ثابت منطقي impure = nodep->user2();
        إذا (impure) UINFO(9, "      impure " << nodep);
        أرجع impure;
    }

    منطقي doExpandWide(Ast_گره* nodep) {
        إذا (isImpure(nodep)) {
            ++m_statImpure;
            أرجع خطأ;
        }
        إذا (nodep->عرض_واژگان() <= v3Global.opt.expand_حد()) {
            ++m_statWides;
            m_stat_Wide_واژگان += nodep->عرض_واژگان();
            أرجع درست;
        } وإلا {
            ++m_statWideLimited;
            أرجع خطأ;
        }
    }

    ساكن صحيح long_Or_Quad_عرض(Ast_گره* nodep) {
        أرجع (nodep->عرض() + (VL_EDATASIZE - 1)) & ~(VL_EDATASIZE - 1);
    }
    ساكن V3_عدد not_Wide_ماسک(Ast_گره* nodep) {
        أرجع V3_عدد{nodep, VL_EDATASIZE, ~VL_ماسک_E(nodep->عرض_Min())};
    }
    ساكن V3_عدد کلمه_ماسک(Ast_گره* nodep) {
        إذا (nodep->isWide()) {
            أرجع V3_عدد{nodep, VL_EDATASIZE, VL_ماسک_E(nodep->عرض_Min())};
        } وإلا {
            V3_عدد ماسک{nodep, long_Or_Quad_عرض(nodep)};
            ماسک.مجموعه_ماسک(nodep->عرض_Min());
            أرجع ماسک;
        }
    }

    ساكن فراغ درج_Before(Ast_گره* placep, Ast_گره* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        placep->جمع_کن_اینجا_This_As_بعدی(newp);
    }
    ساكن فراغ replaceWithDelete(Ast_گره* nodep, Ast_گره* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        إذا (newp->عرض() != nodep->عرض()) {
            UASSERT_OBJ(newp->عرض_Min() == nodep->عرض_Min(), nodep,
                        "Replacement width mismatch");
            newp->dtype_Chg_عرض(nodep->عرض(), nodep->عرض_Min());
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    ساكن Ast_گره* new_کلمه_تخصیص_بده(Ast_گره_تخصیص_بده* placep, صحيح کلمه, Ast_گره_عبارت* lhsp,
                                  Ast_گره_عبارت* rhsp) {
        File_سطر* ثابت fl = placep->fileline();
        أرجع جديد Ast_تخصیص_بده{fl,
                             جديد Ast_کلمه_Sel{fl, lhsp->clone_درخت_خالص(درست),
                                            جديد AstConst{fl, static_cast<uint32_t>(کلمه)}},
                             rhsp};
    }
    ساكن فراغ جمع_کن_کلمه_تخصیص_بده(Ast_گره_تخصیص_بده* placep, صحيح کلمه, Ast_گره_عبارت* lhsp,
                              Ast_گره_عبارت* rhsp) {
        درج_Before(placep, new_کلمه_تخصیص_بده(placep, کلمه, lhsp, rhsp));
    }
    ساكن فراغ جمع_کن_کلمه_تخصیص_بده(Ast_گره_تخصیص_بده* placep, صحيح کلمه, Ast_گره_عبارت* rhsp) {
        جمع_کن_کلمه_تخصیص_بده(placep, کلمه, placep->lhsp(), rhsp);
    }

    AstVar* جمع_کن_Local_Tmp(Ast_گره* placep, ثابت حرف* namep, Ast_گره_عبارت* valuep) {
        ثابت std::string name = "__V"s + namep + "_"s + std::مقصد_string(m_nTmps);
        File_سطر* ثابت flp = placep->fileline();
        AstVar* ثابت tmpp = جديد AstVar{flp, VVarType::STMTTEMP, name, valuep->dtypep()};
        tmpp->funcLocal(درست);
        tmpp->is_داخلی(درست);
        tmpp->noReset(درست);
        tmpp->substConstOnly(درست);
        m_funcp->جمع_کن_Varsp(tmpp);
        درج_Before(placep, جديد Ast_تخصیص_بده{flp, جديد AstVarRef{flp, tmpp, VAccess::WRITE}, valuep});
        ++m_statTemps;
        أرجع tmpp;
    }

    ساكن فراغ fixCloneLvalue(Ast_گره* nodep) {
        // In AstSel transforms, we call clone() on VarRefs that were lvalues,
        // but are now being used on the RHS of the assignment
        إذا (VN_IS(nodep, VarRef)) VN_AS(nodep, VarRef)->access(VAccess::READ);
        // Iterate
        إذا (Ast_گره* ثابت refp = nodep->op1p()) fixCloneLvalue(refp);
        إذا (Ast_گره* ثابت refp = nodep->op2p()) fixCloneLvalue(refp);
        إذا (Ast_گره* ثابت refp = nodep->op3p()) fixCloneLvalue(refp);
        إذا (Ast_گره* ثابت refp = nodep->op4p()) fixCloneLvalue(refp);
    }

    ساكن Ast_گره_عبارت* new_Ast_کلمه_Sel_Clone(Ast_گره_عبارت* nodep, صحيح کلمه) {
        // Get the specified word number from a wide array
        // Or, if it's a long/quad, do appropriate conversion to wide
        // Concat may pass negative word numbers, that means it wants a zero
        File_سطر* ثابت fl = nodep->fileline();
        إذا (nodep->isWide() && کلمه >= 0 && کلمه < nodep->عرض_واژگان()) {
            أرجع جديد Ast_کلمه_Sel{fl, nodep->clone_درخت_خالص(درست),
                                  جديد AstConst{fl, static_cast<uint32_t>(کلمه)}};
        } وإلا إذا (nodep->isQuad() && کلمه == 0) {
            Ast_گره_عبارت* ثابت quadfromp = nodep->clone_درخت_خالص(درست);
            quadfromp->dtype_مجموعه_بیت_Unsized(VL_QUADSIZE, quadfromp->عرض_Min(), VSigning::UNSIGNED);
            أرجع جديد AstCCast{fl, quadfromp, VL_EDATASIZE};
        } وإلا إذا (nodep->isQuad() && کلمه == 1) {
            Ast_گره_عبارت* ثابت quadfromp = nodep->clone_درخت_خالص(درست);
            quadfromp->dtype_مجموعه_بیت_Unsized(VL_QUADSIZE, quadfromp->عرض_Min(), VSigning::UNSIGNED);
            أرجع جديد AstCCast{
                fl, جديد Ast_لغزش_R{fl, quadfromp, جديد AstConst{fl, VL_EDATASIZE}, VL_EDATASIZE},
                VL_EDATASIZE};
        } وإلا إذا (!nodep->isWide() && !nodep->isQuad() && کلمه == 0) {
            أرجع nodep->clone_درخت_خالص(درست);
        } وإلا {  // Out of bounds
            أرجع جديد AstConst{fl, 0};
        }
    }

    ساكن Ast_گره_عبارت* new_کلمه_Grab_لغزش(File_سطر* fl, صحيح کلمه, Ast_گره_عبارت* lhsp, صحيح لغزش_2) {
        // Extract the expression to grab the value for the specified word, if it's the shift
        // of shift bits from lhsp
        // Negative word numbers requested for lhs when it's "before" what we want.
        // We get a 0 then.
        ثابت صحيح othword = کلمه - لغزش_2 / VL_EDATASIZE;
        Ast_گره_عبارت* ثابت lop = new_Ast_کلمه_Sel_Clone(lhsp, othword);
        ثابت uint32_t lo_لغزش = VL_BITBIT_E(لغزش_2);
        إذا (!lo_لغزش) أرجع lop;
        Ast_گره_عبارت* ثابت hip = new_Ast_کلمه_Sel_Clone(lhsp, othword - 1);
        ثابت uint32_t hi_لغزش = VL_EDATASIZE - lo_لغزش;  // Complement offset
        Ast_گره_عبارت* ثابت newp
            = جديد AstOr{fl,  //
                        جديد Ast_لغزش_R{fl, hip, جديد AstConst{fl, hi_لغزش}, VL_EDATASIZE},
                        جديد Ast_لغزش_L{fl, lop, جديد AstConst{fl, lo_لغزش}, VL_EDATASIZE}};
        أرجع V3Const::constifyEditCpp(newp);
    }

    // Return expression indexing the word that contains 'lsbp' + the given word offset
    ساكن Ast_گره_عبارت* new_کلمه_شاخص(Ast_گره_عبارت* lsbp, uint32_t کلمه_جابه_جایی = 0) {
        // This is indexing a WordSel, so a 32 bit constants are fine
        File_سطر* ثابت flp = lsbp->fileline();
        إذا (ثابت AstConst* constp = VN_CAST(lsbp, Const)) {
            أرجع جديد AstConst{flp, کلمه_جابه_جایی + VL_BITWORD_E(constp->مقصد_UInt())};
        }

        إذا (lsbp->backp()) lsbp = lsbp->clone_درخت_خالص(خطأ);
        Ast_گره_عبارت* ثابت wwl2p = جديد AstConst{flp, VL_EDATASIZE_LOG2};  // Word width log 2
        Ast_گره_عبارت* indexp = جديد Ast_لغزش_R{flp, lsbp, wwl2p, VL_EDATASIZE};
        إذا (کلمه_جابه_جایی) indexp = جديد Ast_جمع_کن{flp, جديد AstConst{flp, کلمه_جابه_جایی}, indexp};
        أرجع indexp;
    }

    // Return word of fromp that contains word indexp + the given word offset.
    ساكن Ast_گره_عبارت* new_کلمه_Sel_کلمه(File_سطر* flp, Ast_گره_عبارت* fromp, Ast_گره_عبارت* indexp,
                                       uint32_t کلمه_جابه_جایی = 0) {
        UASSERT_OBJ(fromp->isWide(), fromp, "Only need AstWordSel on wide from's");

        إذا (ثابت AstConst* ثابت constp = VN_CAST(indexp, Const)) {
            indexp = اشاره_تهی;
            کلمه_جابه_جایی += constp->مقصد_UInt();
        }

        // e.g. "logic [95:0] var[0]; logic [0] sel; out = var[sel];"
        // Squash before C++ to avoid getting a C++ compiler warning
        // (even though code would be unreachable as presumably a
        // AstCond is protecting above this node.
        إذا (کلمه_جابه_جایی >= static_cast<uint32_t>(fromp->عرض_واژگان())) {
            أرجع جديد AstConst{flp, AstConst::SizedEData{}, 0};
        }

        إذا (fromp->backp()) fromp = fromp->clone_درخت_خالص(خطأ);

        // If indexp was constant, just use it
        إذا (!indexp) أرجع جديد Ast_کلمه_Sel{flp, fromp, جديد AstConst{flp, کلمه_جابه_جایی}};

        // Otherwise compute at runtime
        إذا (indexp->backp()) indexp = indexp->clone_درخت_خالص(خطأ);
        إذا (کلمه_جابه_جایی) indexp = جديد Ast_جمع_کن{flp, جديد AstConst{flp, کلمه_جابه_جایی}, indexp};
        أرجع جديد Ast_کلمه_Sel{flp, fromp, indexp};
    }

    // Return word of fromp that contains bit lsbp + the given word offset.
    ساكن Ast_گره_عبارت* new_کلمه_Sel_بیت(File_سطر* flp, Ast_گره_عبارت* fromp, Ast_گره_عبارت* lsbp,
                                      uint32_t کلمه_جابه_جایی = 0) {
        Ast_گره_عبارت* ثابت indexp = new_کلمه_شاخص(lsbp, کلمه_جابه_جایی);
        Ast_گره_عبارت* ثابت کلمه_Selp = new_کلمه_Sel_کلمه(flp, fromp, indexp);
        إذا (!indexp->backp()) VL_DO_DANGLING(indexp->delete_درخت(), indexp);
        أرجع کلمه_Selp;
    }

    ساكن Ast_گره_عبارت* new_Sel_بیت_بیت(Ast_گره_عبارت* lsbp) {
        // Return equation to get the VL_BITBIT of a constant or non-constant
        File_سطر* ثابت fl = lsbp->fileline();
        إذا (VN_IS(lsbp, Const)) {
            أرجع جديد AstConst{fl, VL_BITBIT_E(VN_AS(lsbp, Const)->مقصد_UInt())};
        } وإلا {
            أرجع جديد AstAnd{fl, جديد AstConst{fl, VL_EDATASIZE - 1}, lsbp->clone_درخت_خالص(درست)};
        }
    }

    //====================

    منطقي expandWide(Ast_گره_تخصیص_بده* nodep, AstConst* rhsp) {
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        did_الگو(VExpand_الگو::تخصیص_بده_CONST, nodep);
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),WORDSEL(CONST,#))}}
        إذا (rhsp->رقم().isFourState()) {
            rhsp->v3warn(E_UNSUPPORTED,  // LCOV_EXCL_LINE  // impossible?
                         "Unsupported: 4-state numbers in this context");
        }
        File_سطر* ثابت fl = nodep->fileline();
        لكل (صحيح w = 0; w < nodep->عرض_واژگان(); ++w) {
            جمع_کن_کلمه_تخصیص_بده(nodep, w,
                          جديد AstConst{fl, AstConst::SizedEData{}, rhsp->رقم().edata_کلمه(w)});
        }
        أرجع درست;
    }
    //-------- Uniops
    منطقي expandWide(Ast_گره_تخصیص_بده* nodep, AstVarRef* rhsp) {
        // Special case: do not expand assignment of constant pool variables.
        // V3Subst undestands these directly.
        إذا (rhsp->varp()->const_Pool_مدخل()) أرجع خطأ;
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        did_الگو(VExpand_الگو::تخصیص_بده_VARREF, nodep);
        لكل (صحيح w = 0; w < nodep->عرض_واژگان(); ++w) {
            جمع_کن_کلمه_تخصیص_بده(nodep, w, new_Ast_کلمه_Sel_Clone(rhsp, w));
        }
        أرجع درست;
    }
    منطقي expandWide(Ast_گره_تخصیص_بده* nodep, AstArraySel* rhsp) {
        UASSERT_OBJ(!VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType), nodep,
                    "ArraySel with unpacked arrays should have been removed in V3Slice");
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        did_الگو(VExpand_الگو::تخصیص_بده_ARRAYSEL, nodep);
        لكل (صحيح w = 0; w < nodep->عرض_واژگان(); ++w) {
            جمع_کن_کلمه_تخصیص_بده(nodep, w, new_Ast_کلمه_Sel_Clone(rhsp, w));
        }
        أرجع درست;
    }
    منطقي expandWide(Ast_گره_تخصیص_بده* nodep, AstNot* rhsp) {
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),NOT(WORDSEL(lhs,#))) }}
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        did_الگو(VExpand_الگو::تخصیص_بده_NOT, nodep);
        File_سطر* ثابت fl = rhsp->fileline();
        لكل (صحيح w = 0; w < nodep->عرض_واژگان(); ++w) {
            جمع_کن_کلمه_تخصیص_بده(nodep, w, جديد AstNot{fl, new_Ast_کلمه_Sel_Clone(rhsp->lhsp(), w)});
        }
        أرجع درست;
    }
    //-------- Biops
    منطقي expandWide(Ast_گره_تخصیص_بده* nodep, AstAnd* rhsp) {
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        did_الگو(VExpand_الگو::تخصیص_بده_AND, nodep);
        File_سطر* ثابت fl = nodep->fileline();
        لكل (صحيح w = 0; w < nodep->عرض_واژگان(); ++w) {
            جمع_کن_کلمه_تخصیص_بده(nodep, w,
                          جديد AstAnd{fl, new_Ast_کلمه_Sel_Clone(rhsp->lhsp(), w),
                                     new_Ast_کلمه_Sel_Clone(rhsp->rhsp(), w)});
        }
        أرجع درست;
    }
    منطقي expandWide(Ast_گره_تخصیص_بده* nodep, AstOr* rhsp) {
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        did_الگو(VExpand_الگو::تخصیص_بده_OR, nodep);
        File_سطر* ثابت fl = nodep->fileline();
        لكل (صحيح w = 0; w < nodep->عرض_واژگان(); ++w) {
            جمع_کن_کلمه_تخصیص_بده(nodep, w,
                          جديد AstOr{fl, new_Ast_کلمه_Sel_Clone(rhsp->lhsp(), w),
                                    new_Ast_کلمه_Sel_Clone(rhsp->rhsp(), w)});
        }
        أرجع درست;
    }
    منطقي expandWide(Ast_گره_تخصیص_بده* nodep, AstXor* rhsp) {
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        did_الگو(VExpand_الگو::تخصیص_بده_XOR, nodep);
        File_سطر* ثابت fl = nodep->fileline();
        لكل (صحيح w = 0; w < nodep->عرض_واژگان(); ++w) {
            جمع_کن_کلمه_تخصیص_بده(nodep, w,
                          جديد AstXor{fl, new_Ast_کلمه_Sel_Clone(rhsp->lhsp(), w),
                                     new_Ast_کلمه_Sel_Clone(rhsp->rhsp(), w)});
        }
        أرجع درست;
    }
    منطقي expand_Wide_لغزش(Ast_گره_تخصیص_بده* nodep, Ast_گره_Biop* rhsp, منطقي is_چپ_لغزش) {
        إذا (!doExpandWide(nodep)) أرجع خطأ;

        // Simplify the shift amount, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->rhsp());

        // If it's a constant shift by whole words, expand it so V3Subst can substitute it
        إذا (ثابت AstConst* ثابت rhsConstp = VN_CAST(rhsp->rhsp(), Const)) {
            ثابت uint32_t لغزش_بیتها = rhsConstp->مقصد_UInt();
            إذا (VL_BITBIT_E(لغزش_بیتها) == 0) {
                ثابت صحيح عرض_واژگان = nodep->عرض_واژگان();
                ثابت صحيح لغزش_واژگان = std::min<صحيح>(VL_BITWORD_E(لغزش_بیتها), عرض_واژگان);
                File_سطر* ثابت flp = rhsp->fileline();
                إذا (is_چپ_لغزش) {
                    did_الگو(VExpand_الگو::تخصیص_بده_SHIFTL, nodep);
                    // Low words of the result are zero
                    لكل (صحيح w = 0; w < لغزش_واژگان; ++w) {
                        جمع_کن_کلمه_تخصیص_بده(nodep, w, جديد AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                    // High words of the result are copied from higher words of the source
                    لكل (صحيح w = لغزش_واژگان; w < عرض_واژگان; ++w) {
                        جمع_کن_کلمه_تخصیص_بده(nodep, w, new_Ast_کلمه_Sel_Clone(rhsp->lhsp(), w - لغزش_واژگان));
                    }
                } وإلا {
                    did_الگو(VExpand_الگو::تخصیص_بده_SHIFTR, nodep);
                    // Low words of the result are copied from higher words of the source
                    لكل (صحيح w = 0; w < عرض_واژگان - لغزش_واژگان; ++w) {
                        جمع_کن_کلمه_تخصیص_بده(nodep, w, new_Ast_کلمه_Sel_Clone(rhsp->lhsp(), w + لغزش_واژگان));
                    }
                    // High words of the result are zero
                    لكل (صحيح w = عرض_واژگان - لغزش_واژگان; w < عرض_واژگان; ++w) {
                        جمع_کن_کلمه_تخصیص_بده(nodep, w, جديد AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                }
                أرجع درست;
            }
        }

        أرجع خطأ;
    }

    //-------- Triops
    منطقي expandWide(Ast_گره_تخصیص_بده* nodep, AstCond* rhsp) {
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        did_الگو(VExpand_الگو::تخصیص_بده_COND, nodep);
        File_سطر* ثابت fl = nodep->fileline();
        لكل (صحيح w = 0; w < nodep->عرض_واژگان(); ++w) {
            جمع_کن_کلمه_تخصیص_بده(nodep, w,
                          جديد AstCond{fl, rhsp->condp()->clone_درخت_خالص(درست),
                                      new_Ast_کلمه_Sel_Clone(rhsp->thenp(), w),
                                      new_Ast_کلمه_Sel_Clone(rhsp->elsep(), w)});
        }
        أرجع درست;
    }

    // VISITORS
    فراغ بازدید(AstNetlist* nodep) override { iterate_And_بعدی_Null(nodep->modulesp()); }

    فراغ بازدید(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_nTmps);
        m_funcp = nodep;
        m_nTmps = 0;
        ثابت VDouble0 statWidesBefore = m_statWides;
        iterateChildren(nodep);

        // Constant fold here if anything was expanded, as Ast size can likely be reduced
        إذا (v3Global.opt.fConstEager() && m_statWides != statWidesBefore) {
            Ast_گره* ثابت editedp = V3Const::constifyEditCpp(nodep);
            UASSERT_OBJ(editedp == nodep, editedp, "Should not have replaced CFunc");
        }
    }

    فراغ بازدید(AstExtend* nodep) override {
        إذا (nodep->user1_مجموعه_Once()) أرجع;  // Process once
        iterateChildren(nodep);
        إذا (nodep->isWide()) {
            // See under ASSIGN(EXTEND)
        } وإلا {
            إذا (isImpure(nodep)) أرجع;
            Ast_گره_عبارت* ثابت lhsp = nodep->lhsp()->unlink_Fr_پشت();
            Ast_گره_عبارت* newp = lhsp;
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
    منطقي expandWide(Ast_گره_تخصیص_بده* nodep, AstExtend* rhsp) {
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        did_الگو(VExpand_الگو::تخصیص_بده_EXTEND, nodep);
        Ast_گره_عبارت* ثابت rlhsp = rhsp->lhsp();
        لكل (صحيح w = 0; w < rlhsp->عرض_واژگان(); ++w) {
            جمع_کن_کلمه_تخصیص_بده(nodep, w, new_Ast_کلمه_Sel_Clone(rlhsp, w));
        }
        لكل (صحيح w = rlhsp->عرض_واژگان(); w < nodep->عرض_واژگان(); ++w) {
            جمع_کن_کلمه_تخصیص_بده(nodep, w, جديد AstConst{nodep->fileline(), AstConst::SizedEData{}, 0});
        }
        أرجع درست;
    }

    فراغ بازدید(AstSel* nodep) override {
        إذا (nodep->user1_مجموعه_Once()) أرجع;  // Process once
        iterateChildren(nodep);

        ثابت uint32_t عرض = static_cast<uint32_t>(nodep->عرض_Const());
        UASSERT_OBJ(nodep->عرض_Min() == static_cast<صحيح>(عرض), nodep, "Width mismatch");

        // Skip if Sel is an LHS assignment select
        إذا (Ast_گره_تخصیص_بده* ثابت assignp = VN_CAST(nodep->backp(), گره_تخصیص_بده)) {
            إذا (nodep == assignp->lhsp()) أرجع;
        }

        // Skip if wide: See under ASSIGN(WIDE)
        إذا (nodep->isWide()) أرجع;

        // Skip Sel from stream or queue
        إذا (VN_IS(nodep->fromp()->dtypep(), جریان_DType)) أرجع;
        إذا (VN_IS(nodep->fromp()->dtypep(), صف_DType)) أرجع;

        // Result must be non-wide after skipping all the above
        UASSERT_OBJ(عرض <= 64, nodep, "Inconsistent result width");

        إذا (nodep->fromp()->isWide()) {  // Long/Quad from Wide
            إذا (isImpure(nodep)) أرجع;
            did_الگو(VExpand_الگو::SEL_FROM_WIDE, nodep);
            File_سطر* ثابت nfl = nodep->fileline();
            File_سطر* ثابت lfl = nodep->lsbp()->fileline();
            File_سطر* ثابت ffl = nodep->fromp()->fileline();
            Ast_گره_عبارت* ثابت fromp = nodep->fromp();

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
            ثابت خودکار tryFold = [&](Ast_گره_عبارت* exprp) -> Ast_گره_عبارت* {
                // Size limit for inlining expressions (any arbitrary value results in
                // a bounded expansion. Picked a value to allow common simple forms.)
                ساكن constexpr صحيح EXPAND_SEL_LSB_حد = 8;
                // Simplify
                exprp = V3Const::constifyEditCpp(exprp);
                // Accept if not larger than the limit
                إذا (!exprp->isLargerThan(EXPAND_SEL_LSB_حد)) أرجع exprp;
                // Delete if rejected
                VL_DO_DANGLING(exprp->delete_درخت(), exprp);
                أرجع اشاره_تهی;
            };

            // Return the index of the word holding the bit 'offset' bits above the LSB
            ثابت خودکار کلمه_Idx = [&](uint32_t جابه_جایی) -> Ast_گره_عبارت* {
                Ast_گره_عبارت* ثابت msbp = جديد Ast_جمع_کن{lfl, جديد AstConst{lfl, جابه_جایی},
                                                     nodep->lsbp()->clone_درخت_خالص(خطأ)};
                Ast_گره_عبارت* ثابت idxp = new_کلمه_شاخص(msbp);
                إذا (!msbp->backp()) VL_DO_DANGLING(msbp->delete_درخت(), msbp);
                أرجع idxp;
            };

            // Bit index of the selected LSB within its word
            Ast_گره_عبارت* lBitp = tryFold(new_Sel_بیت_بیت(nodep->lsbp()));
            ثابت منطقي aligned = lBitp && lBitp->isZero();

            // Which of the other 2 words the expansion might need
            ثابت منطقي mNeeded = عرض > 1 && !aligned;
            ثابت منطقي hNeeded = عرض > VL_EDATASIZE;
            UASSERT_OBJ(!hNeeded || nodep->isQuad(), nodep, "Width mismatch");

            // Word indices
            ثابت uint32_t m_Msb_جابه_جایی = std::min<uint32_t>(عرض, VL_EDATASIZE) - 1;
            ثابت uint32_t h_Msb_جابه_جایی = عرض - 1;
            Ast_گره_عبارت* lIdxp = tryFold(new_کلمه_شاخص(nodep->lsbp()));
            Ast_گره_عبارت* mIdxp = mNeeded ? tryFold(کلمه_Idx(m_Msb_جابه_جایی)) : اشاره_تهی;
            Ast_گره_عبارت* hIdxp = hNeeded ? tryFold(کلمه_Idx(h_Msb_جابه_جایی)) : اشاره_تهی;

            // If the LSB expression is needed multiple times, evaluate it into a temporary
            ثابت صحيح nCopies = !lBitp + !lIdxp + (mNeeded && !mIdxp) + (hNeeded && !hIdxp);
            إذا (nCopies > 1 && m_funcp && m_stmtp && !isImpure(m_stmtp)) {
                ++m_nTmps;  // Use fresh set of temporaries
                Ast_گره_عبارت* ثابت lsbp = nodep->lsbp()->unlink_Fr_پشت();
                AstVar* ثابت tmpp = جمع_کن_Local_Tmp(m_stmtp, "ExpandSel_Lsb", lsbp);
                nodep->lsbp(جديد AstVarRef{lfl, tmpp, VAccess::READ});
            }

            // Rebuild remaining expressions derived from LSB if needed, now using the temporary
            إذا (!lBitp) lBitp = new_Sel_بیت_بیت(nodep->lsbp());
            إذا (!lIdxp) lIdxp = new_کلمه_شاخص(nodep->lsbp());
            إذا (!mIdxp) mIdxp = mNeeded ? کلمه_Idx(m_Msb_جابه_جایی) : اشاره_تهی;
            إذا (!hIdxp) hIdxp = hNeeded ? کلمه_Idx(h_Msb_جابه_جایی) : اشاره_تهی;

            // Return word 'idxp' of 'fromp', without consuming 'idxp'
            ثابت خودکار کلمه_Sel_2 = [&](Ast_گره_عبارت* idxp) -> Ast_گره_عبارت* {
                Ast_گره_عبارت* ثابت clonep = idxp->clone_درخت_خالص(خطأ);
                Ast_گره_عبارت* ثابت wordp = new_کلمه_Sel_کلمه(ffl, fromp, clonep);
                إذا (!clonep->backp()) VL_DO_DANGLING(clonep->delete_درخت(), clonep);
                أرجع wordp;
            };

            // Construct term containing the bits of the low word - always needed
            Ast_گره_عبارت* ثابت lTermp = [&]() -> Ast_گره_عبارت* {
                Ast_گره_عبارت* lWordp = کلمه_Sel_2(lIdxp);
                إذا (nodep->isQuad()) lWordp = جديد AstCCast{nfl, lWordp, nodep};
                أرجع جديد Ast_لغزش_R{nfl, lWordp, lBitp->clone_درخت_خالص(خطأ), nodep->عرض()};
            }();

            // Construct term containing the bits of the middle word - if needed
            Ast_گره_عبارت* ثابت mTermp = [&]() -> Ast_گره_عبارت* {
                إذا (!mNeeded) أرجع اشاره_تهی;

                Ast_گره_عبارت* mWordp = کلمه_Sel_2(mIdxp);
                إذا (nodep->isQuad()) mWordp = جديد AstCCast{nfl, mWordp, nodep};
                Ast_گره_عبارت* ثابت mShiftp = جديد AstSub{lfl, جديد AstConst{lfl, VL_EDATASIZE},
                                                        lBitp->clone_درخت_خالص(خطأ)};
                // If we're selecting bit zero, then all 32 bits in the mid word
                // get shifted << by 32 bits, so ignore them.
                ثابت V3_عدد zero{nodep, long_Or_Quad_عرض(nodep)};
                أرجع جديد AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    جديد AstEq{nfl, جديد AstConst{nfl, 0}, lBitp->clone_درخت_خالص(خطأ)},
                    // 0 :
                    جديد AstConst{nfl, zero},
                    //  midword >> (VL_EDATASIZE - (lbs % VL_EDATASIZE))
                    جديد Ast_لغزش_L{nfl, mWordp, mShiftp, nodep->عرض()}};
            }();

            // Construct term containing the bits of the high word - if needed
            Ast_گره_عبارت* ثابت hTermp = [&]() -> Ast_گره_عبارت* {
                إذا (!hNeeded) أرجع اشاره_تهی;

                Ast_گره_عبارت* hWordp = کلمه_Sel_2(hIdxp);
                hWordp = جديد AstCCast{nfl, hWordp, nodep};
                Ast_گره_عبارت* ثابت hShiftp = جديد AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    جديد AstEq{nfl, جديد AstConst{nfl, 0}, lBitp->clone_درخت_خالص(خطأ)},
                    // VL_EDATASIZE :
                    جديد AstConst{lfl, VL_EDATASIZE},
                    // 64 - (lbs % VL_EDATASIZE)
                    جديد AstSub{lfl, جديد AstConst{lfl, 64}, lBitp->clone_درخت_خالص(خطأ)}};
                أرجع جديد Ast_لغزش_L{nfl, hWordp, hShiftp, nodep->عرض()};
            }();

            // Delete parts not captured during construction
            VL_DO_DANGLING(lBitp->delete_درخت(), lBitp);
            VL_DO_DANGLING(lIdxp->delete_درخت(), lIdxp);
            إذا (mIdxp) VL_DO_DANGLING(mIdxp->delete_درخت(), mIdxp);
            إذا (hIdxp) VL_DO_DANGLING(hIdxp->delete_درخت(), hIdxp);

            // Or reduce the terms
            Ast_گره_عبارت* newp = lTermp;
            إذا (mTermp) newp = جديد AstOr{nfl, mTermp, newp};
            إذا (hTermp) newp = جديد AstOr{nfl, hTermp, newp};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } وإلا {  // Long/Quad from Long/Quad
            // No isImpure() check - can handle side effects in below
            did_الگو(VExpand_الگو::SEL_FROM_NARROW, nodep);
            File_سطر* ثابت fl = nodep->fileline();
            Ast_گره_عبارت* fromp = nodep->fromp()->unlink_Fr_پشت();
            Ast_گره_عبارت* ثابت lsbp = nodep->lsbp()->unlink_Fr_پشت();
            إذا (nodep->isQuad() && !fromp->isQuad()) fromp = جديد AstCCast{fl, fromp, nodep};
            // {large}>>32 requires 64-bit shift operation; then cast
            Ast_گره_عبارت* newp = جديد Ast_لغزش_R{fl, fromp, lsbp, fromp->عرض()};
            newp->dtypeFrom(fromp);
            إذا (!nodep->isQuad() && fromp->isQuad()) newp = جديد AstCCast{fl, newp, nodep};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }

    منطقي expandWide(Ast_گره_تخصیص_بده* nodep, AstSel* rhsp) {
        UASSERT_OBJ(nodep->عرض_Min() == rhsp->عرض_Const(), nodep, "Width mismatch");
        إذا (!doExpandWide(nodep)) أرجع خطأ;

        // Simplify the index, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->lsbp());

        // If it's a constant select and aligned, we can just copy the words
        إذا (ثابت AstConst* ثابت lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            ثابت uint32_t lsb = lsbConstp->مقصد_UInt();
            إذا (VL_BITBIT_E(lsb) == 0) {
                did_الگو(VExpand_الگو::تخصیص_بده_SEL_ALIGNED, nodep);
                ثابت uint32_t کلمه = VL_BITWORD_E(lsb);
                لكل (صحيح w = 0; w < nodep->عرض_واژگان(); ++w) {
                    جمع_کن_کلمه_تخصیص_بده(nodep, w, new_Ast_کلمه_Sel_Clone(rhsp->fromp(), w + کلمه));
                }
                أرجع درست;
            }
        }

        did_الگو(VExpand_الگو::تخصیص_بده_SEL_UNALIGNED, nodep);
        File_سطر* ثابت flp = rhsp->fileline();

        // Use fresh set of temporaries
        ++m_nTmps;

        // Compute word index of LSB, store to temporary if not constant
        Ast_گره_عبارت* ثابت کلمه_Lsbp = rhsp->lsbp()->clone_درخت_خالص(خطأ);
        Ast_گره_عبارت* کلمه_Idxp = new_کلمه_شاخص(کلمه_Lsbp);
        إذا (!کلمه_Lsbp->backp()) VL_DO_DANGLING(کلمه_Lsbp->delete_درخت(), کلمه_Lsbp);
        کلمه_Idxp = V3Const::constifyEditCpp(کلمه_Idxp);
        إذا (!VN_IS(کلمه_Idxp, Const)) {
            AstVar* ثابت tmpp = جمع_کن_Local_Tmp(nodep, "ExpandSel_WordIdx", کلمه_Idxp);
            کلمه_Idxp = جديد AstVarRef{flp, tmpp, VAccess::READ};
        }

        // Compute shift amounts and mask, store to temporaries if not constants
        Ast_گره_عبارت* loShftp = اشاره_تهی;
        Ast_گره_عبارت* hiShftp = اشاره_تهی;
        Ast_گره_عبارت* hiMaskp = اشاره_تهی;
        إذا (ثابت AstConst* ثابت lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            ثابت uint32_t بیت_جابه_جایی = VL_BITBIT_E(lsbConstp->مقصد_UInt());
            // Must be unaligned, otherwise we would have handled it above
            UASSERT_OBJ(بیت_جابه_جایی, nodep, "Missed aligned wide select");
            loShftp = جديد AstConst{flp, بیت_جابه_جایی};
            hiShftp = جديد AstConst{flp, VL_EDATASIZE - بیت_جابه_جایی};
            hiMaskp = اشاره_تهی;
        } وإلا {
            // Compute Low word shift amount: bottom bits of lsb index
            Ast_گره_عبارت* ثابت lsbp = rhsp->lsbp()->clone_درخت_خالص(خطأ);
            loShftp = جديد AstAnd{flp, جديد AstConst{flp, VL_SIZEBITS_E}, lsbp};
            AstVar* ثابت loTmpp = جمع_کن_Local_Tmp(nodep, "ExpandSel_LoShift", loShftp);
            loShftp = جديد AstVarRef{flp, loTmpp, VAccess::READ};

            // Compute if aligned: Low word shift amount is 0
            Ast_گره_عبارت* ثابت zerop = جديد AstConst{flp, 0};
            Ast_گره_عبارت* alignedp = جديد AstEq{flp, zerop, loShftp->clone_درخت_خالص(خطأ)};
            AstVar* ثابت alignedTmpp = جمع_کن_Local_Tmp(nodep, "ExpandSel_Aligned", alignedp);
            alignedp = جديد AstVarRef{flp, alignedTmpp, VAccess::READ};

            // Computed High word shift amount: 0 if aligned else VL_EDATASIZE - Low word shift
            Ast_گره_عبارت* ثابت edsp = جديد AstConst{flp, VL_EDATASIZE};
            Ast_گره_عبارت* ثابت subp = جديد AstSub{flp, edsp, loShftp->clone_درخت_خالص(خطأ)};
            hiShftp = جديد AstCond{flp, alignedp, جديد AstConst{flp, 0}, subp};
            AstVar* ثابت hiTmpp = جمع_کن_Local_Tmp(nodep, "ExpandSel_HiShift", hiShftp);
            hiShftp = جديد AstVarRef{flp, hiTmpp, VAccess::READ};

            // Compute the High word mask: 0 if aligned, ones otherwise
            hiMaskp = جديد AstCond{flp, alignedp->clone_درخت_خالص(خطأ),
                                  جديد AstConst{flp, AstConst::SizedEData{}, 0},
                                  جديد AstConst{flp, AstConst::SizedEData{}, -1ULL}};
            AstVar* ثابت ماسک_Tmpp = جمع_کن_Local_Tmp(nodep, "ExpandSel_HiMask", hiMaskp);
            hiMaskp = جديد AstVarRef{flp, ماسک_Tmpp, VAccess::READ};
        }

        // Create each word of the selected result
        Ast_گره_عبارت* ثابت fromp = rhsp->fromp();
        ثابت صحيح sel_واژگان = nodep->عرض_واژگان();
        لكل (صحيح w = 0; w < sel_واژگان; ++w) {
            // Grab bits from word 'VL_BITWORD_E(lsb) + w'
            Ast_گره_عبارت* ثابت loWordp = new_کلمه_Sel_کلمه(fromp->fileline(), fromp, کلمه_Idxp, w);
            Ast_گره_عبارت* ثابت loShftClonep = loShftp->clone_درخت_خالص(خطأ);
            Ast_گره_عبارت* ثابت lop = جديد Ast_لغزش_R{flp, loWordp, loShftClonep, VL_EDATASIZE};
            // Grab bits from word 'VL_BITWORD_E(lsb) + w + 1'
            Ast_گره_عبارت* hiWordp = new_کلمه_Sel_کلمه(fromp->fileline(), fromp, کلمه_Idxp, w + 1);
            // For the last word of the result, avoid an OOB access when the Sel is not OOB
            إذا (w == sel_واژگان - 1) {
                ثابت uint32_t max = fromp->عرض_واژگان() - w - 1;
                Ast_گره_عبارت* ثابت maxp = جديد AstConst{flp, max};
                Ast_گره_عبارت* ثابت condp = جديد AstGte{flp, کلمه_Idxp->clone_درخت_خالص(خطأ), maxp};
                Ast_گره_عبارت* ثابت zerop = جديد AstConst{flp, AstConst::SizedEData{}, 0};
                hiWordp = جديد AstCond{flp, condp, zerop, hiWordp};
            }
            Ast_گره_عبارت* ثابت hiShftClonep = hiShftp->clone_درخت_خالص(خطأ);
            Ast_گره_عبارت* ثابت hiPartp = جديد Ast_لغزش_L{flp, hiWordp, hiShftClonep, VL_EDATASIZE};
            Ast_گره_عبارت* ثابت hip
                = hiMaskp ? جديد AstAnd{flp, hiPartp, hiMaskp->clone_درخت_خالص(خطأ)} : hiPartp;
            // Combine them
            جمع_کن_کلمه_تخصیص_بده(nodep, w, جديد AstOr{nodep->fileline(), hip, lop});
        }

        // Delete parts not captured during construction
        إذا (!کلمه_Idxp->backp()) VL_DO_DANGLING(کلمه_Idxp->delete_درخت(), کلمه_Idxp);
        إذا (!loShftp->backp()) VL_DO_DANGLING(loShftp->delete_درخت(), loShftp);
        إذا (!hiShftp->backp()) VL_DO_DANGLING(hiShftp->delete_درخت(), hiShftp);
        إذا (hiMaskp && !hiMaskp->backp()) VL_DO_DANGLING(hiMaskp->delete_درخت(), hiMaskp);

        أرجع درست;
    }

    منطقي expandLhs(Ast_گره_تخصیص_بده* nodep, AstSel* lhsp) {
        // Possibilities
        //      destp: wide or narrow
        //      rhsp:  wide (destp must be wide), narrow, or 1 bit wide
        //      rhsp:  may be allones and can remove AND NOT gate
        //      lsbp:  constant or variable
        // Yuk.
        إذا (isImpure(nodep)) أرجع خطأ;
        File_سطر* ثابت nfl = nodep->fileline();
        File_سطر* ثابت lfl = lhsp->fileline();
        ثابت منطقي destwide = lhsp->fromp()->isWide();
        ثابت منطقي ones = nodep->rhsp()->isAllOnesV();
        إذا (VN_IS(lhsp->lsbp(), Const)) {
            // The code should work without this constant test, but it won't
            // constify as nicely as we'd like.
            Ast_گره_عبارت* rhsp = nodep->rhsp()->unlink_Fr_پشت();
            Ast_گره_عبارت* ثابت destp = lhsp->fromp()->unlink_Fr_پشت();
            ثابت صحيح lsb = lhsp->lsbConst();
            ثابت صحيح msb = lhsp->msbConst();
            V3_عدد maskset{nodep, destp->عرض_Min()};
            maskset.مجموعه_ماسک(msb + 1 - lsb, lsb);
            V3_عدد maskold{nodep, destp->عرض_Min()};
            maskold.opNot(maskset);
            إذا (destwide) {
                did_الگو(VExpand_الگو::تخصیص_بده_مقصد_SEL_CONST_WIDE, nodep);
                لكل (صحيح w = 0; w < destp->عرض_واژگان(); ++w) {
                    إذا (w >= VL_BITWORD_E(lsb) && w <= VL_BITWORD_E(msb)) {
                        // else we would just be setting it to the same exact value
                        Ast_گره_عبارت* oldvalp = new_Ast_کلمه_Sel_Clone(destp, w);
                        fixCloneLvalue(oldvalp);
                        إذا (!ones) {
                            oldvalp = جديد AstAnd{
                                lfl,
                                جديد AstConst{lfl, AstConst::SizedEData{}, maskold.edata_کلمه(w)},
                                oldvalp};
                        }

                        // Appropriate word of new value to insert:
                        Ast_گره_عبارت* newp = new_کلمه_Grab_لغزش(lfl, w, rhsp, lsb);

                        // Apply cleaning at the top word of the destination
                        // (no cleaning to do if dst's width is a whole number
                        // of words).
                        إذا (w == destp->عرض_واژگان() - 1 && VL_BITBIT_E(destp->عرض_Min()) != 0) {
                            V3_عدد cleanmask{nodep, VL_EDATASIZE};
                            cleanmask.مجموعه_ماسک(VL_BITBIT_E(destp->عرض_Min()));
                            newp = جديد AstAnd{lfl, newp, جديد AstConst{lfl, cleanmask}};
                        }
                        Ast_گره_عبارت* ثابت orp
                            = V3Const::constifyEditCpp(جديد AstOr{lfl, oldvalp, newp});
                        جمع_کن_کلمه_تخصیص_بده(nodep, w, destp, orp);
                    }
                }
                VL_DO_DANGLING(rhsp->delete_درخت(), rhsp);
                VL_DO_DANGLING(destp->delete_درخت(), destp);
            } وإلا {
                did_الگو(VExpand_الگو::تخصیص_بده_مقصد_SEL_CONST_NARROW, nodep);
                إذا (destp->isQuad() && !rhsp->isQuad()) rhsp = جديد AstCCast{nfl, rhsp, nodep};
                Ast_گره_عبارت* oldvalp = destp->clone_درخت_خالص(درست);
                fixCloneLvalue(oldvalp);
                إذا (!ones) oldvalp = جديد AstAnd{lfl, جديد AstConst{lfl, maskold}, oldvalp};

                // The bit-select can refer to bits outside the width of nodep
                // which we aren't allowed to assign to.  This is a mask of the
                // valid range of nodep which we apply to the new shifted RHS.
                V3_عدد cleanmask{nodep, destp->عرض_Min()};
                cleanmask.مجموعه_ماسک(destp->عرض_Min());
                Ast_گره_عبارت* ثابت shifted = جديد Ast_لغزش_L{
                    lfl, rhsp, جديد AstConst{lfl, static_cast<uint32_t>(lsb)}, destp->عرض()};
                Ast_گره_عبارت* ثابت cleaned
                    = جديد AstAnd{lfl, shifted, جديد AstConst{lfl, cleanmask}};
                Ast_گره_عبارت* ثابت orp
                    = V3Const::constifyEditCpp(جديد AstOr{lfl, oldvalp, cleaned});
                درج_Before(nodep, جديد Ast_تخصیص_بده{nfl, destp, orp});
            }
            أرجع درست;
        } وإلا {  // non-const select offset
            إذا (destwide && lhsp->عرض_Const() == 1) {
                did_الگو(VExpand_الگو::تخصیص_بده_مقصد_SEL_VAR_WIDE_ONE_بیت, nodep);
                Ast_گره_عبارت* ثابت rhsp = nodep->rhsp()->unlink_Fr_پشت();
                Ast_گره_عبارت* ثابت destp = lhsp->fromp()->unlink_Fr_پشت();
                Ast_گره_عبارت* oldvalp
                    = new_کلمه_Sel_بیت(lfl, destp->clone_درخت_خالص(خطأ), lhsp->lsbp());
                fixCloneLvalue(oldvalp);
                إذا (!ones) {
                    oldvalp = جديد AstAnd{
                        lfl,
                        جديد AstNot{
                            lfl, جديد Ast_لغزش_L{lfl, جديد AstConst{nfl, 1},
                                               // newSelBitBit may exceed the MSB of this variable.
                                               // That's ok as we'd just AND with a larger value,
                                               // but oldval would clip the upper bits to sanity
                                               new_Sel_بیت_بیت(lhsp->lsbp()), VL_EDATASIZE}},
                        oldvalp};
                }
                // Restrict the shift amount to 0-31, see bug804.
                Ast_گره_عبارت* ثابت shiftp = جديد AstAnd{nfl, lhsp->lsbp()->clone_درخت_خالص(درست),
                                                       جديد AstConst{nfl, VL_EDATASIZE - 1}};
                Ast_گره* ثابت newp = جديد Ast_تخصیص_بده{
                    nfl, new_کلمه_Sel_بیت(nfl, destp, lhsp->lsbp()),
                    جديد AstOr{lfl, oldvalp, جديد Ast_لغزش_L{lfl, rhsp, shiftp, VL_EDATASIZE}}};
                درج_Before(nodep, newp);
                أرجع درست;
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
                did_الگو(VExpand_الگو::تخصیص_بده_مقصد_SEL_VAR_NARROW, nodep);
                // nodep->dumpTree("-  old: ");
                Ast_گره_عبارت* rhsp = nodep->rhsp()->unlink_Fr_پشت();
                Ast_گره_عبارت* ثابت destp = lhsp->fromp()->unlink_Fr_پشت();
                Ast_گره_عبارت* oldvalp = destp->clone_درخت_خالص(درست);
                fixCloneLvalue(oldvalp);

                V3_عدد maskwidth{nodep, destp->عرض_Min()};
                maskwidth.مجموعه_ماسک(lhsp->عرض_Const());

                إذا (destp->isQuad() && !rhsp->isQuad()) rhsp = جديد AstCCast{nfl, rhsp, nodep};
                إذا (!ones) {
                    oldvalp = جديد AstAnd{
                        lfl,
                        جديد AstNot{lfl, جديد Ast_لغزش_L{lfl, جديد AstConst{nfl, maskwidth},
                                                      lhsp->lsbp()->clone_درخت_خالص(درست),
                                                      destp->عرض()}},
                        oldvalp};
                }
                Ast_گره_عبارت* newp
                    = جديد Ast_لغزش_L{lfl, rhsp, lhsp->lsbp()->clone_درخت_خالص(درست), destp->عرض()};
                // Apply cleaning to the new value being inserted.  Mask is
                // slightly wider than necessary to avoid an AND with all ones
                // being optimized out.  No need to clean if destp is
                // quad-sized as there are no extra bits to contaminate
                إذا (destp->عرض_Min() != 64) {
                    V3_عدد cleanmask{nodep, destp->عرض_Min() + 1};
                    cleanmask.مجموعه_ماسک(destp->عرض_Min());
                    newp = جديد AstAnd{lfl, newp, جديد AstConst{lfl, cleanmask}};
                }

                درج_Before(nodep, جديد Ast_تخصیص_بده{nfl, destp, جديد AstOr{lfl, oldvalp, newp}});
                أرجع درست;
            }
        }
    }

    فراغ بازدید(AstConcat* nodep) override {
        إذا (nodep->user1_مجموعه_Once()) أرجع;  // Process once
        iterateChildren(nodep);
        إذا (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } وإلا {
            // No isImpure() check - can handle side effects in below
            did_الگو(VExpand_الگو::CONCAT, nodep);
            File_سطر* ثابت fl = nodep->fileline();
            Ast_گره_عبارت* lhsp = nodep->lhsp()->unlink_Fr_پشت();
            Ast_گره_عبارت* rhsp = nodep->rhsp()->unlink_Fr_پشت();
            ثابت uint32_t rhsshift = rhsp->عرض_Min();
            إذا (nodep->isQuad() && !lhsp->isQuad()) lhsp = جديد AstCCast{fl, lhsp, nodep};
            إذا (nodep->isQuad() && !rhsp->isQuad()) rhsp = جديد AstCCast{fl, rhsp, nodep};
            Ast_گره_عبارت* ثابت newp = جديد AstOr{
                fl, جديد Ast_لغزش_L{fl, lhsp, جديد AstConst{fl, rhsshift}, nodep->عرض()}, rhsp};
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    منطقي expandWide(Ast_گره_تخصیص_بده* nodep, AstConcat* rhsp) {
        إذا (!doExpandWide(rhsp)) أرجع خطأ;
        did_الگو(VExpand_الگو::تخصیص_بده_CONCAT, nodep);
        File_سطر* ثابت fl = rhsp->fileline();
        // Lhs or Rhs may be word, long, or quad.
        // newAstWordSelClone nicely abstracts the difference.
        ثابت صحيح rhsshift = rhsp->rhsp()->عرض_Min();
        // Sometimes doing the words backwards is preferable.
        // When we have x={x,foo} backwards is better, when x={foo,x} forward is better
        // However V3Subst tends to rip this up, so not worth optimizing now.
        لكل (صحيح w = 0; w < rhsp->عرض_واژگان(); ++w) {
            جمع_کن_کلمه_تخصیص_بده(nodep, w,
                          جديد AstOr{fl, new_کلمه_Grab_لغزش(fl, w, rhsp->lhsp(), rhsshift),
                                    new_Ast_کلمه_Sel_Clone(rhsp->rhsp(), w)});
        }
        أرجع درست;
    }

    فراغ بازدید(AstReplicate* nodep) override {
        إذا (nodep->user1_مجموعه_Once()) أرجع;  // Process once
        iterateChildren(nodep);
        إذا (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } وإلا {
            إذا (isImpure(nodep)) أرجع;
            File_سطر* ثابت fl = nodep->fileline();
            Ast_گره_عبارت* lhsp = nodep->srcp()->unlink_Fr_پشت();
            Ast_گره_عبارت* newp;
            ثابت صحيح lhswidth = lhsp->عرض_Min();
            إذا (lhswidth == 1) {
                did_الگو(VExpand_الگو::REPLICATE_ONE_بیت, nodep);
                newp = جديد AstNegate{fl, lhsp};
            } وإلا {
                did_الگو(VExpand_الگو::REPLICATE, nodep);
                ثابت AstConst* ثابت constp = VN_AS(nodep->countp(), Const);
                UASSERT_OBJ(constp, nodep,
                            "Replication value isn't a constant.  Checked earlier!");
                ثابت uint32_t times = constp->مقصد_UInt();
                إذا (nodep->isQuad() && !lhsp->isQuad()) lhsp = جديد AstCCast{fl, lhsp, nodep};
                newp = lhsp->clone_درخت_خالص(درست);
                لكل (بیعلامت repnum = 1; repnum < times; repnum++) {
                    ثابت صحيح rhsshift = repnum * lhswidth;
                    newp = جديد AstOr{
                        fl,
                        جديد Ast_لغزش_L{fl, lhsp->clone_درخت_خالص(درست),
                                      جديد AstConst{fl, static_cast<uint32_t>(rhsshift)},
                                      nodep->عرض()},
                        newp};
                    newp->dtypeFrom(nodep);  // Unsigned
                }
                VL_DO_DANGLING(lhsp->delete_درخت(), lhsp);  // Never used
            }
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    منطقي expandWide(Ast_گره_تخصیص_بده* nodep, AstReplicate* rhsp) {
        إذا (!doExpandWide(rhsp)) أرجع خطأ;
        did_الگو(VExpand_الگو::تخصیص_بده_REPLICATE, nodep);
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* ثابت lhsp = rhsp->srcp();
        ثابت صحيح lhswidth = lhsp->عرض_Min();
        ثابت AstConst* ثابت constp = VN_AS(rhsp->countp(), Const);
        UASSERT_OBJ(constp, rhsp, "Replication value isn't a constant.  Checked earlier!");
        ثابت uint32_t times = constp->مقصد_UInt();
        لكل (صحيح w = 0; w < rhsp->عرض_واژگان(); ++w) {
            Ast_گره_عبارت* newp;
            إذا (lhswidth == 1) {
                newp = جديد AstNegate{fl, lhsp->clone_درخت_خالص(درست)};
                // Replicate always unsigned
                newp->dtype_Chg_عرض_Signed(VL_EDATASIZE, VL_EDATASIZE, VSigning::UNSIGNED);
            } وإلا {
                newp = new_Ast_کلمه_Sel_Clone(lhsp, w);
                File_سطر* ثابت rfl = rhsp->fileline();
                لكل (بیعلامت repnum = 1; repnum < times; repnum++) {
                    newp = جديد AstOr{fl, new_کلمه_Grab_لغزش(rfl, w, lhsp, lhswidth * repnum), newp};
                }
            }
            جمع_کن_کلمه_تخصیص_بده(nodep, w, newp);
        }
        أرجع درست;
    }

    فراغ بازدید_Eq_Neq(Ast_گره_Biop* nodep) {
        إذا (nodep->user1_مجموعه_Once()) أرجع;  // Process once
        iterateChildren(nodep);
        إذا (nodep->lhsp()->isWide()
            && !(VN_IS(nodep->lhsp()->dtypep()->skipRefp(), جریان_DType)
                 || VN_IS(nodep->rhsp()->dtypep()->skipRefp(), جریان_DType))) {
            إذا (isImpure(nodep)) أرجع;
            إذا (!doExpandWide(nodep->lhsp())) أرجع;
            إذا (!doExpandWide(nodep->rhsp())) أرجع;
            did_الگو(VN_IS(nodep, Neq) ? VExpand_الگو::WORDIZE_NEQ
                                         : VExpand_الگو::WORDIZE_EQ,
                       nodep);
            // -> (0=={or{for each_word{WORDSEL(lhs,#)^WORDSEL(rhs,#)}}}
            File_سطر* ثابت fl = nodep->fileline();
            Ast_گره_عبارت* newp = اشاره_تهی;
            لكل (صحيح w = 0; w < nodep->lhsp()->عرض_واژگان(); ++w) {
                Ast_گره_عبارت* ثابت eqp = جديد AstXor{fl, new_Ast_کلمه_Sel_Clone(nodep->lhsp(), w),
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
    فراغ بازدید(AstEq* nodep) override { بازدید_Eq_Neq(nodep); }
    فراغ بازدید(AstNeq* nodep) override { بازدید_Eq_Neq(nodep); }

    فراغ بازدید(AstRedOr* nodep) override {
        إذا (nodep->user1_مجموعه_Once()) أرجع;  // Process once
        iterateChildren(nodep);
        File_سطر* ثابت fl = nodep->fileline();
        إذا (nodep->lhsp()->isWide()) {
            إذا (isImpure(nodep)) أرجع;
            إذا (!doExpandWide(nodep->lhsp())) أرجع;
            did_الگو(VExpand_الگو::WORDIZE_REDOR, nodep);
            // -> (0!={or{for each_word{WORDSEL(lhs,#)}}}
            Ast_گره_عبارت* newp = اشاره_تهی;
            لكل (صحيح w = 0; w < nodep->lhsp()->عرض_واژگان(); ++w) {
                Ast_گره_عبارت* ثابت eqp = new_Ast_کلمه_Sel_Clone(nodep->lhsp(), w);
                newp = newp ? جديد AstOr{fl, newp, eqp} : eqp;
            }
            newp = جديد AstNeq{fl, جديد AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } وإلا {
            // No isImpure() check - can handle side effects in below
            did_الگو(VExpand_الگو::REDOR_مقصد_NEQ, nodep);
            Ast_گره_عبارت* ثابت lhsp = nodep->lhsp()->unlink_Fr_پشت();
            Ast_گره_عبارت* ثابت newp = جديد AstNeq{
                fl, جديد AstConst{fl, AstConst::Widthed_مقدار{}, long_Or_Quad_عرض(nodep), 0}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    فراغ بازدید(AstRedAnd* nodep) override {
        إذا (nodep->user1_مجموعه_Once()) أرجع;  // Process once
        iterateChildren(nodep);
        File_سطر* ثابت fl = nodep->fileline();
        إذا (nodep->lhsp()->isWide()) {
            إذا (isImpure(nodep)) أرجع;
            إذا (!doExpandWide(nodep->lhsp())) أرجع;
            did_الگو(VExpand_الگو::WORDIZE_REDAND, nodep);
            // -> (0!={and{for each_word{WORDSEL(lhs,#)}}}
            Ast_گره_عبارت* newp = اشاره_تهی;
            لكل (صحيح w = 0; w < nodep->lhsp()->عرض_واژگان(); ++w) {
                Ast_گره_عبارت* eqp = new_Ast_کلمه_Sel_Clone(nodep->lhsp(), w);
                إذا (w == nodep->lhsp()->عرض_واژگان() - 1) {
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
            did_الگو(VExpand_الگو::REDAND_مقصد_EQ, nodep);
            Ast_گره_عبارت* ثابت lhsp = nodep->lhsp()->unlink_Fr_پشت();
            Ast_گره_عبارت* ثابت newp = جديد AstEq{fl, جديد AstConst{fl, کلمه_ماسک(lhsp)}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    فراغ بازدید(AstRedXor* nodep) override {
        إذا (nodep->user1_مجموعه_Once()) أرجع;  // Process once
        iterateChildren(nodep);
        إذا (nodep->lhsp()->isWide()) {
            إذا (isImpure(nodep)) أرجع;
            إذا (!doExpandWide(nodep->lhsp())) أرجع;
            did_الگو(VExpand_الگو::WORDIZE_REDXOR, nodep);
            // -> (0!={redxor{for each_word{XOR(WORDSEL(lhs,#))}}}
            File_سطر* ثابت fl = nodep->fileline();
            Ast_گره_عبارت* newp = اشاره_تهی;
            لكل (صحيح w = 0; w < nodep->lhsp()->عرض_واژگان(); ++w) {
                Ast_گره_عبارت* ثابت eqp = new_Ast_کلمه_Sel_Clone(nodep->lhsp(), w);
                newp = newp ? جديد AstXor{fl, newp, eqp} : eqp;
            }
            newp = جديد AstRedXor{fl, newp};
            UINFO(8, "    Wordize REDXORnew " << newp);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
        // We don't reduce non-wide XORs, as its more efficient to use a temp register,
        // which the inlined function does nicely.
    }

    فراغ بازدید(Ast_گره_Stmt* nodep) override {
        إذا (nodep->user1_مجموعه_Once()) أرجع;  // Process once
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    فراغ بازدید(Ast_گره_تخصیص_بده* nodep) override {
        إذا (nodep->user1_مجموعه_Once()) أرجع;  // Process once
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
            } وإلا إذا (Ast_لغزش_L* ثابت rhsp = VN_CAST(nodep->rhsp(), لغزش_L)) {
                did = expand_Wide_لغزش(nodep, rhsp, /* isLeftShift: */ درست);
            } وإلا إذا (Ast_لغزش_R* ثابت rhsp = VN_CAST(nodep->rhsp(), لغزش_R)) {
                did = expand_Wide_لغزش(nodep, rhsp, /* isLeftShift: */ خطأ);
            } وإلا إذا (AstCond* ثابت rhsp = VN_CAST(nodep->rhsp(), Cond)) {
                did = expandWide(nodep, rhsp);
            }
        } وإلا إذا (AstSel* ثابت lhsp = VN_CAST(nodep->lhsp(), Sel)) {
            did = expandLhs(nodep, lhsp);
        }
        // Cleanup common code
        إذا (did) VL_DO_DANGLING(nodep->unlink_Fr_پشت()->delete_درخت(), nodep);
    }

    //--------------------
    فراغ بازدید(AstVar*) override {}  // Don't hit varrefs under vars
    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    صریح ExpandVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ExpandVisitor() override {
        V3Stats::جمع_کن_Stat("Optimizations, Expand, expanded wides", m_statWides);
        V3Stats::جمع_کن_Stat("Optimizations, Expand, expanded wide words", m_stat_Wide_واژگان);
        V3Stats::جمع_کن_Stat("Optimizations, Expand, not expanded impure", m_statImpure);
        V3Stats::جمع_کن_Stat("Optimizations, Expand, not expanded limit", m_statWideLimited);
        V3Stats::جمع_کن_Stat("Optimizations, Expand, temporaries created", m_statTemps);
        ثابت خودکار emitStat = [هذا](VExpand_الگو الگو_2) {
            std::string str{الگو_2.ascii()};
            std::transform(str.begin(), str.انتها(), str.begin(), [](بیعلامت حرف c) {  //
                أرجع c == '_' ? ' ' : std::tolower(c);
            });
            V3Stats::جمع_کن_Stat("Optimizations, Expand, pattern " + str, m_statPatterns[الگو_2]);
        };
#تعریف_کن EXPAND_الگو_EMIT_STAT(id) emitStat(VExpand_الگو::id);
        FOR_EACH_EXPAND_الگو(EXPAND_الگو_EMIT_STAT)
#لغو_تعریف EXPAND_الگو_EMIT_STAT
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
    V3Global::dump_Check_Global_درخت("expand", 0, dump_درخت_Either_Level() >= 3);
}
