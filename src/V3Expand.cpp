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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Expand.h"

#شامل "V3Const.h"
#شامل "V3Stats.h"

#شامل <algorithm>
#شامل <array>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// The distinct expansion patterns, for statistics. One entry for each
// unique transformation ExpandVisitor applies. Keep sorted.

// clang-format off
#تعریف_کرو FOR_EACH_EXPAND_نمونہ(macro) \
    macro(تفویض_AND) \
    macro(تفویض_ARRAYSEL) \
    macro(تفویض_CONCAT) \
    macro(تفویض_COND) \
    macro(تفویض_CONST) \
    macro(تفویض_EXTEND) \
    macro(تفویض_NOT) \
    macro(تفویض_OR) \
    macro(تفویض_REPLICATE) \
    macro(تفویض_SEL_ALIGNED) \
    macro(تفویض_SEL_UNALIGNED) \
    macro(تفویض_SHIFTL) \
    macro(تفویض_SHIFTR) \
    macro(تفویض_مقصد_SEL_CONST_NARROW) \
    macro(تفویض_مقصد_SEL_CONST_WIDE) \
    macro(تفویض_مقصد_SEL_VAR_NARROW) \
    macro(تفویض_مقصد_SEL_VAR_WIDE_ONE_ثنائی_ہندسہ) \
    macro(تفویض_VARREF) \
    macro(تفویض_XOR) \
    macro(CONCAT) \
    macro(REDAND_مقصد_EQ) \
    macro(REDOR_مقصد_NEQ) \
    macro(REPLICATE) \
    macro(REPLICATE_ONE_ثنائی_ہندسہ) \
    macro(SEL_FROM_NARROW) \
    macro(SEL_FROM_WIDE) \
    macro(WORDIZE_EQ) \
    macro(WORDIZE_NEQ) \
    macro(WORDIZE_REDAND) \
    macro(WORDIZE_REDOR) \
    macro(WORDIZE_REDXOR)
// clang-format on

جماعت VExpand_نمونہ final {
عوامی:
    شمار en : uint8_t {
#تعریف_کرو EXPAND_نمونہ_ID(id) id,
        FOR_EACH_EXPAND_نمونہ(EXPAND_نمونہ_ID)
#تعریف_منسوخ EXPAND_نمونہ_ID
            ENUM_انتہا
    };
    شمار en m_e;
    ثابت حرف* ascii() ثابت {
        ساکن ثابت حرف* ثابت نام[] = {
#تعریف_کرو EXPAND_نمونہ_NAME(id) #id,
            FOR_EACH_EXPAND_نمونہ(EXPAND_نمونہ_NAME)
#تعریف_منسوخ EXPAND_نمونہ_NAME
                "_ENUM_END"  //
        };
        واپس نام[m_e];
    }

    // cppcheck-suppress noExplicitConstructor
    VExpand_نمونہ(en _e)
        : m_e{_e} {}
    عامل en() ثابت { واپس m_e; }
};

//######################################################################
// Find nodes with side effects, to mark as non-expandable

جماعت ExpandOkVisitor final : عوامی VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> bool.  Is pure (along with all children)
    ثابت VNUser2_اندر_Use m_inuser2;

    // STATE - for current visit position (use VL_RESTORER)
    // Tracks similar to AstNode::isTreePureRecurse(), but avoid O(n^2)
    // False = pure, as nodes that ExpandVisitor inserts preserve pureness
    منطقی m_isImpure = سچ;  // Currently pure

    خالی دورہ(Ast_گرہ* nodep) override {
        منطقی selfImpure = !nodep->is_خالص();
        {
            VL_RESTORER(m_isImpure);
            m_isImpure = جھوٹ;
            iterateChildrenConst(nodep);
            selfImpure |= m_isImpure;
            nodep->user2(selfImpure);
        }
        m_isImpure |= selfImpure;
    }

عوامی:
    // CONSTRUCTORS
    صریح ExpandOkVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~ExpandOkVisitor() = طےشدہ;
};

//######################################################################
// Expand state, as a visitor of each AstNode

جماعت ExpandVisitor final : عوامی VNVisitor {
    // NODE STATE
    //  AstNode::user1()        -> bool.  Processed
    //  AstNode::user2()        -> See ExpandOkVisitor
    ثابت VNUser1_اندر_Use m_inuser1;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_گرہ* m_stmtp = خالی_اشاریہ;  // Current statement

    // STATE - across all visitors
    AstCFunc* m_funcp = خالی_اشاریہ;  // Current function
    VDouble0 m_statImpure;  // Statistic tracking
    VDouble0 m_statWides;  // Statistic tracking
    VDouble0 m_stat_Wide_الفاظ;  // Statistic tracking
    VDouble0 m_statWideLimited;  // Statistic tracking
    VDouble0 m_statTemps;  // Statistic tracking
    // Number of applications of each expansion pattern for statistics
    std::array<VDouble0, VExpand_نمونہ::ENUM_انتہا> m_statPatterns;

    // STATE - for current function
    سائز_t m_nTmps = 0;  // Sequence numbers for temopraries

    // METHODS
    // Note the application of the given expansion pattern to 'nodep'
    خالی did_نمونہ(VExpand_نمونہ نمونہ_2, Ast_گرہ* nodep) {
        UINFO(8, "    " << نمونہ_2.ascii() << " " << nodep);
        ++m_statPatterns[نمونہ_2];
    }

    // Use state that ExpandOkVisitor calculated
    ساکن منطقی isImpure(Ast_گرہ* nodep) {
        ثابت منطقی impure = nodep->user2();
        اگر (impure) UINFO(9, "      impure " << nodep);
        واپس impure;
    }

    منطقی doExpandWide(Ast_گرہ* nodep) {
        اگر (isImpure(nodep)) {
            ++m_statImpure;
            واپس جھوٹ;
        }
        اگر (nodep->چوڑائی_الفاظ() <= v3Global.opt.expand_حد_بندی()) {
            ++m_statWides;
            m_stat_Wide_الفاظ += nodep->چوڑائی_الفاظ();
            واپس سچ;
        } ورنہ {
            ++m_statWideLimited;
            واپس جھوٹ;
        }
    }

    ساکن صحیح long_Or_Quad_چوڑائی(Ast_گرہ* nodep) {
        واپس (nodep->چوڑائی() + (VL_EDATASIZE - 1)) & ~(VL_EDATASIZE - 1);
    }
    ساکن V3_نمبر not_Wide_نقاب(Ast_گرہ* nodep) {
        واپس V3_نمبر{nodep, VL_EDATASIZE, ~VL_نقاب_E(nodep->چوڑائی_Min())};
    }
    ساکن V3_نمبر لفظ_نقاب(Ast_گرہ* nodep) {
        اگر (nodep->isWide()) {
            واپس V3_نمبر{nodep, VL_EDATASIZE, VL_نقاب_E(nodep->چوڑائی_Min())};
        } ورنہ {
            V3_نمبر نقاب{nodep, long_Or_Quad_چوڑائی(nodep)};
            نقاب.سیٹ_نقاب(nodep->چوڑائی_Min());
            واپس نقاب;
        }
    }

    ساکن خالی داخل_کرو_Before(Ast_گرہ* placep, Ast_گرہ* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        placep->جوڑو_یہاں_This_As_اگلا(newp);
    }
    ساکن خالی replaceWithDelete(Ast_گرہ* nodep, Ast_گرہ* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        اگر (newp->چوڑائی() != nodep->چوڑائی()) {
            UASSERT_OBJ(newp->چوڑائی_Min() == nodep->چوڑائی_Min(), nodep,
                        "Replacement width mismatch");
            newp->dtype_Chg_چوڑائی(nodep->چوڑائی(), nodep->چوڑائی_Min());
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    ساکن Ast_گرہ* new_لفظ_تفویض(Ast_گرہ_تفویض* placep, صحیح لفظ, Ast_گرہ_اظہاریہ* lhsp,
                                  Ast_گرہ_اظہاریہ* rhsp) {
        File_سطر* ثابت fl = placep->fileline();
        واپس نیا Ast_تفویض{fl,
                             نیا Ast_لفظ_Sel{fl, lhsp->clone_درخت_خالص(سچ),
                                            نیا AstConst{fl, static_cast<uint32_t>(لفظ)}},
                             rhsp};
    }
    ساکن خالی جوڑو_لفظ_تفویض(Ast_گرہ_تفویض* placep, صحیح لفظ, Ast_گرہ_اظہاریہ* lhsp,
                              Ast_گرہ_اظہاریہ* rhsp) {
        داخل_کرو_Before(placep, new_لفظ_تفویض(placep, لفظ, lhsp, rhsp));
    }
    ساکن خالی جوڑو_لفظ_تفویض(Ast_گرہ_تفویض* placep, صحیح لفظ, Ast_گرہ_اظہاریہ* rhsp) {
        جوڑو_لفظ_تفویض(placep, لفظ, placep->lhsp(), rhsp);
    }

    AstVar* جوڑو_Local_Tmp(Ast_گرہ* placep, ثابت حرف* namep, Ast_گرہ_اظہاریہ* valuep) {
        ثابت std::string name = "__V"s + namep + "_"s + std::مقصد_string(m_nTmps);
        File_سطر* ثابت flp = placep->fileline();
        AstVar* ثابت tmpp = نیا AstVar{flp, VVarType::STMTTEMP, name, valuep->dtypep()};
        tmpp->funcLocal(سچ);
        tmpp->is_اندرونی(سچ);
        tmpp->noReset(سچ);
        tmpp->substConstOnly(سچ);
        m_funcp->جوڑو_Varsp(tmpp);
        داخل_کرو_Before(placep, نیا Ast_تفویض{flp, نیا AstVarRef{flp, tmpp, VAccess::WRITE}, valuep});
        ++m_statTemps;
        واپس tmpp;
    }

    ساکن خالی fixCloneLvalue(Ast_گرہ* nodep) {
        // In AstSel transforms, we call clone() on VarRefs that were lvalues,
        // but are now being used on the RHS of the assignment
        اگر (VN_IS(nodep, VarRef)) VN_AS(nodep, VarRef)->access(VAccess::READ);
        // Iterate
        اگر (Ast_گرہ* ثابت refp = nodep->op1p()) fixCloneLvalue(refp);
        اگر (Ast_گرہ* ثابت refp = nodep->op2p()) fixCloneLvalue(refp);
        اگر (Ast_گرہ* ثابت refp = nodep->op3p()) fixCloneLvalue(refp);
        اگر (Ast_گرہ* ثابت refp = nodep->op4p()) fixCloneLvalue(refp);
    }

    ساکن Ast_گرہ_اظہاریہ* new_Ast_لفظ_Sel_Clone(Ast_گرہ_اظہاریہ* nodep, صحیح لفظ) {
        // Get the specified word number from a wide array
        // Or, if it's a long/quad, do appropriate conversion to wide
        // Concat may pass negative word numbers, that means it wants a zero
        File_سطر* ثابت fl = nodep->fileline();
        اگر (nodep->isWide() && لفظ >= 0 && لفظ < nodep->چوڑائی_الفاظ()) {
            واپس نیا Ast_لفظ_Sel{fl, nodep->clone_درخت_خالص(سچ),
                                  نیا AstConst{fl, static_cast<uint32_t>(لفظ)}};
        } ورنہ اگر (nodep->isQuad() && لفظ == 0) {
            Ast_گرہ_اظہاریہ* ثابت quadfromp = nodep->clone_درخت_خالص(سچ);
            quadfromp->dtype_سیٹ_ثنائی_ہندسہ_Unsized(VL_QUADSIZE, quadfromp->چوڑائی_Min(), VSigning::UNSIGNED);
            واپس نیا AstCCast{fl, quadfromp, VL_EDATASIZE};
        } ورنہ اگر (nodep->isQuad() && لفظ == 1) {
            Ast_گرہ_اظہاریہ* ثابت quadfromp = nodep->clone_درخت_خالص(سچ);
            quadfromp->dtype_سیٹ_ثنائی_ہندسہ_Unsized(VL_QUADSIZE, quadfromp->چوڑائی_Min(), VSigning::UNSIGNED);
            واپس نیا AstCCast{
                fl, نیا Ast_سرکاؤ_R{fl, quadfromp, نیا AstConst{fl, VL_EDATASIZE}, VL_EDATASIZE},
                VL_EDATASIZE};
        } ورنہ اگر (!nodep->isWide() && !nodep->isQuad() && لفظ == 0) {
            واپس nodep->clone_درخت_خالص(سچ);
        } ورنہ {  // Out of bounds
            واپس نیا AstConst{fl, 0};
        }
    }

    ساکن Ast_گرہ_اظہاریہ* new_لفظ_Grab_سرکاؤ(File_سطر* fl, صحیح لفظ, Ast_گرہ_اظہاریہ* lhsp, صحیح سرکاؤ_2) {
        // Extract the expression to grab the value for the specified word, if it's the shift
        // of shift bits from lhsp
        // Negative word numbers requested for lhs when it's "before" what we want.
        // We get a 0 then.
        ثابت صحیح othword = لفظ - سرکاؤ_2 / VL_EDATASIZE;
        Ast_گرہ_اظہاریہ* ثابت lop = new_Ast_لفظ_Sel_Clone(lhsp, othword);
        ثابت uint32_t lo_سرکاؤ = VL_BITBIT_E(سرکاؤ_2);
        اگر (!lo_سرکاؤ) واپس lop;
        Ast_گرہ_اظہاریہ* ثابت hip = new_Ast_لفظ_Sel_Clone(lhsp, othword - 1);
        ثابت uint32_t hi_سرکاؤ = VL_EDATASIZE - lo_سرکاؤ;  // Complement offset
        Ast_گرہ_اظہاریہ* ثابت newp
            = نیا AstOr{fl,  //
                        نیا Ast_سرکاؤ_R{fl, hip, نیا AstConst{fl, hi_سرکاؤ}, VL_EDATASIZE},
                        نیا Ast_سرکاؤ_L{fl, lop, نیا AstConst{fl, lo_سرکاؤ}, VL_EDATASIZE}};
        واپس V3Const::constifyEditCpp(newp);
    }

    // Return expression indexing the word that contains 'lsbp' + the given word offset
    ساکن Ast_گرہ_اظہاریہ* new_لفظ_اشاریہ(Ast_گرہ_اظہاریہ* lsbp, uint32_t لفظ_انحراف = 0) {
        // This is indexing a WordSel, so a 32 bit constants are fine
        File_سطر* ثابت flp = lsbp->fileline();
        اگر (ثابت AstConst* constp = VN_CAST(lsbp, Const)) {
            واپس نیا AstConst{flp, لفظ_انحراف + VL_BITWORD_E(constp->مقصد_UInt())};
        }

        اگر (lsbp->backp()) lsbp = lsbp->clone_درخت_خالص(جھوٹ);
        Ast_گرہ_اظہاریہ* ثابت wwl2p = نیا AstConst{flp, VL_EDATASIZE_LOG2};  // Word width log 2
        Ast_گرہ_اظہاریہ* indexp = نیا Ast_سرکاؤ_R{flp, lsbp, wwl2p, VL_EDATASIZE};
        اگر (لفظ_انحراف) indexp = نیا Ast_جوڑو{flp, نیا AstConst{flp, لفظ_انحراف}, indexp};
        واپس indexp;
    }

    // Return word of fromp that contains word indexp + the given word offset.
    ساکن Ast_گرہ_اظہاریہ* new_لفظ_Sel_لفظ(File_سطر* flp, Ast_گرہ_اظہاریہ* fromp, Ast_گرہ_اظہاریہ* indexp,
                                       uint32_t لفظ_انحراف = 0) {
        UASSERT_OBJ(fromp->isWide(), fromp, "Only need AstWordSel on wide from's");

        اگر (ثابت AstConst* ثابت constp = VN_CAST(indexp, Const)) {
            indexp = خالی_اشاریہ;
            لفظ_انحراف += constp->مقصد_UInt();
        }

        // e.g. "logic [95:0] var[0]; logic [0] sel; out = var[sel];"
        // Squash before C++ to avoid getting a C++ compiler warning
        // (even though code would be unreachable as presumably a
        // AstCond is protecting above this node.
        اگر (لفظ_انحراف >= static_cast<uint32_t>(fromp->چوڑائی_الفاظ())) {
            واپس نیا AstConst{flp, AstConst::SizedEData{}, 0};
        }

        اگر (fromp->backp()) fromp = fromp->clone_درخت_خالص(جھوٹ);

        // If indexp was constant, just use it
        اگر (!indexp) واپس نیا Ast_لفظ_Sel{flp, fromp, نیا AstConst{flp, لفظ_انحراف}};

        // Otherwise compute at runtime
        اگر (indexp->backp()) indexp = indexp->clone_درخت_خالص(جھوٹ);
        اگر (لفظ_انحراف) indexp = نیا Ast_جوڑو{flp, نیا AstConst{flp, لفظ_انحراف}, indexp};
        واپس نیا Ast_لفظ_Sel{flp, fromp, indexp};
    }

    // Return word of fromp that contains bit lsbp + the given word offset.
    ساکن Ast_گرہ_اظہاریہ* new_لفظ_Sel_ثنائی_ہندسہ(File_سطر* flp, Ast_گرہ_اظہاریہ* fromp, Ast_گرہ_اظہاریہ* lsbp,
                                      uint32_t لفظ_انحراف = 0) {
        Ast_گرہ_اظہاریہ* ثابت indexp = new_لفظ_اشاریہ(lsbp, لفظ_انحراف);
        Ast_گرہ_اظہاریہ* ثابت لفظ_Selp = new_لفظ_Sel_لفظ(flp, fromp, indexp);
        اگر (!indexp->backp()) VL_DO_DANGLING(indexp->delete_درخت(), indexp);
        واپس لفظ_Selp;
    }

    ساکن Ast_گرہ_اظہاریہ* new_Sel_ثنائی_ہندسہ_ثنائی_ہندسہ(Ast_گرہ_اظہاریہ* lsbp) {
        // Return equation to get the VL_BITBIT of a constant or non-constant
        File_سطر* ثابت fl = lsbp->fileline();
        اگر (VN_IS(lsbp, Const)) {
            واپس نیا AstConst{fl, VL_BITBIT_E(VN_AS(lsbp, Const)->مقصد_UInt())};
        } ورنہ {
            واپس نیا AstAnd{fl, نیا AstConst{fl, VL_EDATASIZE - 1}, lsbp->clone_درخت_خالص(سچ)};
        }
    }

    //====================

    منطقی expandWide(Ast_گرہ_تفویض* nodep, AstConst* rhsp) {
        اگر (!doExpandWide(nodep)) واپس جھوٹ;
        did_نمونہ(VExpand_نمونہ::تفویض_CONST, nodep);
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),WORDSEL(CONST,#))}}
        اگر (rhsp->عدد().isFourState()) {
            rhsp->v3warn(E_UNSUPPORTED,  // LCOV_EXCL_LINE  // impossible?
                         "Unsupported: 4-state numbers in this context");
        }
        File_سطر* ثابت fl = nodep->fileline();
        برائے (صحیح w = 0; w < nodep->چوڑائی_الفاظ(); ++w) {
            جوڑو_لفظ_تفویض(nodep, w,
                          نیا AstConst{fl, AstConst::SizedEData{}, rhsp->عدد().edata_لفظ(w)});
        }
        واپس سچ;
    }
    //-------- Uniops
    منطقی expandWide(Ast_گرہ_تفویض* nodep, AstVarRef* rhsp) {
        // Special case: do not expand assignment of constant pool variables.
        // V3Subst undestands these directly.
        اگر (rhsp->varp()->const_Pool_اندراج()) واپس جھوٹ;
        اگر (!doExpandWide(nodep)) واپس جھوٹ;
        did_نمونہ(VExpand_نمونہ::تفویض_VARREF, nodep);
        برائے (صحیح w = 0; w < nodep->چوڑائی_الفاظ(); ++w) {
            جوڑو_لفظ_تفویض(nodep, w, new_Ast_لفظ_Sel_Clone(rhsp, w));
        }
        واپس سچ;
    }
    منطقی expandWide(Ast_گرہ_تفویض* nodep, AstArraySel* rhsp) {
        UASSERT_OBJ(!VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType), nodep,
                    "ArraySel with unpacked arrays should have been removed in V3Slice");
        اگر (!doExpandWide(nodep)) واپس جھوٹ;
        did_نمونہ(VExpand_نمونہ::تفویض_ARRAYSEL, nodep);
        برائے (صحیح w = 0; w < nodep->چوڑائی_الفاظ(); ++w) {
            جوڑو_لفظ_تفویض(nodep, w, new_Ast_لفظ_Sel_Clone(rhsp, w));
        }
        واپس سچ;
    }
    منطقی expandWide(Ast_گرہ_تفویض* nodep, AstNot* rhsp) {
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),NOT(WORDSEL(lhs,#))) }}
        اگر (!doExpandWide(nodep)) واپس جھوٹ;
        did_نمونہ(VExpand_نمونہ::تفویض_NOT, nodep);
        File_سطر* ثابت fl = rhsp->fileline();
        برائے (صحیح w = 0; w < nodep->چوڑائی_الفاظ(); ++w) {
            جوڑو_لفظ_تفویض(nodep, w, نیا AstNot{fl, new_Ast_لفظ_Sel_Clone(rhsp->lhsp(), w)});
        }
        واپس سچ;
    }
    //-------- Biops
    منطقی expandWide(Ast_گرہ_تفویض* nodep, AstAnd* rhsp) {
        اگر (!doExpandWide(nodep)) واپس جھوٹ;
        did_نمونہ(VExpand_نمونہ::تفویض_AND, nodep);
        File_سطر* ثابت fl = nodep->fileline();
        برائے (صحیح w = 0; w < nodep->چوڑائی_الفاظ(); ++w) {
            جوڑو_لفظ_تفویض(nodep, w,
                          نیا AstAnd{fl, new_Ast_لفظ_Sel_Clone(rhsp->lhsp(), w),
                                     new_Ast_لفظ_Sel_Clone(rhsp->rhsp(), w)});
        }
        واپس سچ;
    }
    منطقی expandWide(Ast_گرہ_تفویض* nodep, AstOr* rhsp) {
        اگر (!doExpandWide(nodep)) واپس جھوٹ;
        did_نمونہ(VExpand_نمونہ::تفویض_OR, nodep);
        File_سطر* ثابت fl = nodep->fileline();
        برائے (صحیح w = 0; w < nodep->چوڑائی_الفاظ(); ++w) {
            جوڑو_لفظ_تفویض(nodep, w,
                          نیا AstOr{fl, new_Ast_لفظ_Sel_Clone(rhsp->lhsp(), w),
                                    new_Ast_لفظ_Sel_Clone(rhsp->rhsp(), w)});
        }
        واپس سچ;
    }
    منطقی expandWide(Ast_گرہ_تفویض* nodep, AstXor* rhsp) {
        اگر (!doExpandWide(nodep)) واپس جھوٹ;
        did_نمونہ(VExpand_نمونہ::تفویض_XOR, nodep);
        File_سطر* ثابت fl = nodep->fileline();
        برائے (صحیح w = 0; w < nodep->چوڑائی_الفاظ(); ++w) {
            جوڑو_لفظ_تفویض(nodep, w,
                          نیا AstXor{fl, new_Ast_لفظ_Sel_Clone(rhsp->lhsp(), w),
                                     new_Ast_لفظ_Sel_Clone(rhsp->rhsp(), w)});
        }
        واپس سچ;
    }
    منطقی expand_Wide_سرکاؤ(Ast_گرہ_تفویض* nodep, Ast_گرہ_Biop* rhsp, منطقی is_بایاں_سرکاؤ) {
        اگر (!doExpandWide(nodep)) واپس جھوٹ;

        // Simplify the shift amount, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->rhsp());

        // If it's a constant shift by whole words, expand it so V3Subst can substitute it
        اگر (ثابت AstConst* ثابت rhsConstp = VN_CAST(rhsp->rhsp(), Const)) {
            ثابت uint32_t سرکاؤ_بٹ = rhsConstp->مقصد_UInt();
            اگر (VL_BITBIT_E(سرکاؤ_بٹ) == 0) {
                ثابت صحیح چوڑائی_الفاظ = nodep->چوڑائی_الفاظ();
                ثابت صحیح سرکاؤ_الفاظ = std::min<صحیح>(VL_BITWORD_E(سرکاؤ_بٹ), چوڑائی_الفاظ);
                File_سطر* ثابت flp = rhsp->fileline();
                اگر (is_بایاں_سرکاؤ) {
                    did_نمونہ(VExpand_نمونہ::تفویض_SHIFTL, nodep);
                    // Low words of the result are zero
                    برائے (صحیح w = 0; w < سرکاؤ_الفاظ; ++w) {
                        جوڑو_لفظ_تفویض(nodep, w, نیا AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                    // High words of the result are copied from higher words of the source
                    برائے (صحیح w = سرکاؤ_الفاظ; w < چوڑائی_الفاظ; ++w) {
                        جوڑو_لفظ_تفویض(nodep, w, new_Ast_لفظ_Sel_Clone(rhsp->lhsp(), w - سرکاؤ_الفاظ));
                    }
                } ورنہ {
                    did_نمونہ(VExpand_نمونہ::تفویض_SHIFTR, nodep);
                    // Low words of the result are copied from higher words of the source
                    برائے (صحیح w = 0; w < چوڑائی_الفاظ - سرکاؤ_الفاظ; ++w) {
                        جوڑو_لفظ_تفویض(nodep, w, new_Ast_لفظ_Sel_Clone(rhsp->lhsp(), w + سرکاؤ_الفاظ));
                    }
                    // High words of the result are zero
                    برائے (صحیح w = چوڑائی_الفاظ - سرکاؤ_الفاظ; w < چوڑائی_الفاظ; ++w) {
                        جوڑو_لفظ_تفویض(nodep, w, نیا AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                }
                واپس سچ;
            }
        }

        واپس جھوٹ;
    }

    //-------- Triops
    منطقی expandWide(Ast_گرہ_تفویض* nodep, AstCond* rhsp) {
        اگر (!doExpandWide(nodep)) واپس جھوٹ;
        did_نمونہ(VExpand_نمونہ::تفویض_COND, nodep);
        File_سطر* ثابت fl = nodep->fileline();
        برائے (صحیح w = 0; w < nodep->چوڑائی_الفاظ(); ++w) {
            جوڑو_لفظ_تفویض(nodep, w,
                          نیا AstCond{fl, rhsp->condp()->clone_درخت_خالص(سچ),
                                      new_Ast_لفظ_Sel_Clone(rhsp->thenp(), w),
                                      new_Ast_لفظ_Sel_Clone(rhsp->elsep(), w)});
        }
        واپس سچ;
    }

    // VISITORS
    خالی دورہ(AstNetlist* nodep) override { iterate_And_اگلا_Null(nodep->modulesp()); }

    خالی دورہ(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_nTmps);
        m_funcp = nodep;
        m_nTmps = 0;
        ثابت VDouble0 statWidesBefore = m_statWides;
        iterateChildren(nodep);

        // Constant fold here if anything was expanded, as Ast size can likely be reduced
        اگر (v3Global.opt.fConstEager() && m_statWides != statWidesBefore) {
            Ast_گرہ* ثابت editedp = V3Const::constifyEditCpp(nodep);
            UASSERT_OBJ(editedp == nodep, editedp, "Should not have replaced CFunc");
        }
    }

    خالی دورہ(AstExtend* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;  // Process once
        iterateChildren(nodep);
        اگر (nodep->isWide()) {
            // See under ASSIGN(EXTEND)
        } ورنہ {
            اگر (isImpure(nodep)) واپس;
            Ast_گرہ_اظہاریہ* ثابت lhsp = nodep->lhsp()->unlink_Fr_عقب();
            Ast_گرہ_اظہاریہ* newp = lhsp;
            اگر (nodep->isQuad()) {
                اگر (lhsp->isQuad()) {
                    lhsp->dtypeFrom(nodep);  // Just mark it, else nop
                } ورنہ اگر (lhsp->isWide()) {
                    nodep->v3fatalSrc("extending larger thing into smaller?");
                } ورنہ {
                    newp = نیا AstCCast{nodep->fileline(), lhsp, nodep};
                }
            } ورنہ {  // Long
                UASSERT_OBJ(!(lhsp->isQuad() || lhsp->isWide()), nodep,
                            "extending larger thing into smaller?");
                lhsp->dtypeFrom(nodep);  // Just mark it, else nop
            }
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    منطقی expandWide(Ast_گرہ_تفویض* nodep, AstExtend* rhsp) {
        اگر (!doExpandWide(nodep)) واپس جھوٹ;
        did_نمونہ(VExpand_نمونہ::تفویض_EXTEND, nodep);
        Ast_گرہ_اظہاریہ* ثابت rlhsp = rhsp->lhsp();
        برائے (صحیح w = 0; w < rlhsp->چوڑائی_الفاظ(); ++w) {
            جوڑو_لفظ_تفویض(nodep, w, new_Ast_لفظ_Sel_Clone(rlhsp, w));
        }
        برائے (صحیح w = rlhsp->چوڑائی_الفاظ(); w < nodep->چوڑائی_الفاظ(); ++w) {
            جوڑو_لفظ_تفویض(nodep, w, نیا AstConst{nodep->fileline(), AstConst::SizedEData{}, 0});
        }
        واپس سچ;
    }

    خالی دورہ(AstSel* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;  // Process once
        iterateChildren(nodep);

        ثابت uint32_t چوڑائی = static_cast<uint32_t>(nodep->چوڑائی_Const());
        UASSERT_OBJ(nodep->چوڑائی_Min() == static_cast<صحیح>(چوڑائی), nodep, "Width mismatch");

        // Skip if Sel is an LHS assignment select
        اگر (Ast_گرہ_تفویض* ثابت assignp = VN_CAST(nodep->backp(), گرہ_تفویض)) {
            اگر (nodep == assignp->lhsp()) واپس;
        }

        // Skip if wide: See under ASSIGN(WIDE)
        اگر (nodep->isWide()) واپس;

        // Skip Sel from stream or queue
        اگر (VN_IS(nodep->fromp()->dtypep(), دھارا_DType)) واپس;
        اگر (VN_IS(nodep->fromp()->dtypep(), قطار_DType)) واپس;

        // Result must be non-wide after skipping all the above
        UASSERT_OBJ(چوڑائی <= 64, nodep, "Inconsistent result width");

        اگر (nodep->fromp()->isWide()) {  // Long/Quad from Wide
            اگر (isImpure(nodep)) واپس;
            did_نمونہ(VExpand_نمونہ::SEL_FROM_WIDE, nodep);
            File_سطر* ثابت nfl = nodep->fileline();
            File_سطر* ثابت lfl = nodep->lsbp()->fileline();
            File_سطر* ثابت ffl = nodep->fromp()->fileline();
            Ast_گرہ_اظہاریہ* ثابت fromp = nodep->fromp();

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
            ثابت خودکار tryFold = [&](Ast_گرہ_اظہاریہ* exprp) -> Ast_گرہ_اظہاریہ* {
                // Size limit for inlining expressions (any arbitrary value results in
                // a bounded expansion. Picked a value to allow common simple forms.)
                ساکن constexpr صحیح EXPAND_SEL_LSB_حد_بندی = 8;
                // Simplify
                exprp = V3Const::constifyEditCpp(exprp);
                // Accept if not larger than the limit
                اگر (!exprp->isLargerThan(EXPAND_SEL_LSB_حد_بندی)) واپس exprp;
                // Delete if rejected
                VL_DO_DANGLING(exprp->delete_درخت(), exprp);
                واپس خالی_اشاریہ;
            };

            // Return the index of the word holding the bit 'offset' bits above the LSB
            ثابت خودکار لفظ_Idx = [&](uint32_t انحراف) -> Ast_گرہ_اظہاریہ* {
                Ast_گرہ_اظہاریہ* ثابت msbp = نیا Ast_جوڑو{lfl, نیا AstConst{lfl, انحراف},
                                                     nodep->lsbp()->clone_درخت_خالص(جھوٹ)};
                Ast_گرہ_اظہاریہ* ثابت idxp = new_لفظ_اشاریہ(msbp);
                اگر (!msbp->backp()) VL_DO_DANGLING(msbp->delete_درخت(), msbp);
                واپس idxp;
            };

            // Bit index of the selected LSB within its word
            Ast_گرہ_اظہاریہ* lBitp = tryFold(new_Sel_ثنائی_ہندسہ_ثنائی_ہندسہ(nodep->lsbp()));
            ثابت منطقی aligned = lBitp && lBitp->isZero();

            // Which of the other 2 words the expansion might need
            ثابت منطقی mNeeded = چوڑائی > 1 && !aligned;
            ثابت منطقی hNeeded = چوڑائی > VL_EDATASIZE;
            UASSERT_OBJ(!hNeeded || nodep->isQuad(), nodep, "Width mismatch");

            // Word indices
            ثابت uint32_t m_Msb_انحراف = std::min<uint32_t>(چوڑائی, VL_EDATASIZE) - 1;
            ثابت uint32_t h_Msb_انحراف = چوڑائی - 1;
            Ast_گرہ_اظہاریہ* lIdxp = tryFold(new_لفظ_اشاریہ(nodep->lsbp()));
            Ast_گرہ_اظہاریہ* mIdxp = mNeeded ? tryFold(لفظ_Idx(m_Msb_انحراف)) : خالی_اشاریہ;
            Ast_گرہ_اظہاریہ* hIdxp = hNeeded ? tryFold(لفظ_Idx(h_Msb_انحراف)) : خالی_اشاریہ;

            // If the LSB expression is needed multiple times, evaluate it into a temporary
            ثابت صحیح nCopies = !lBitp + !lIdxp + (mNeeded && !mIdxp) + (hNeeded && !hIdxp);
            اگر (nCopies > 1 && m_funcp && m_stmtp && !isImpure(m_stmtp)) {
                ++m_nTmps;  // Use fresh set of temporaries
                Ast_گرہ_اظہاریہ* ثابت lsbp = nodep->lsbp()->unlink_Fr_عقب();
                AstVar* ثابت tmpp = جوڑو_Local_Tmp(m_stmtp, "ExpandSel_Lsb", lsbp);
                nodep->lsbp(نیا AstVarRef{lfl, tmpp, VAccess::READ});
            }

            // Rebuild remaining expressions derived from LSB if needed, now using the temporary
            اگر (!lBitp) lBitp = new_Sel_ثنائی_ہندسہ_ثنائی_ہندسہ(nodep->lsbp());
            اگر (!lIdxp) lIdxp = new_لفظ_اشاریہ(nodep->lsbp());
            اگر (!mIdxp) mIdxp = mNeeded ? لفظ_Idx(m_Msb_انحراف) : خالی_اشاریہ;
            اگر (!hIdxp) hIdxp = hNeeded ? لفظ_Idx(h_Msb_انحراف) : خالی_اشاریہ;

            // Return word 'idxp' of 'fromp', without consuming 'idxp'
            ثابت خودکار لفظ_Sel_2 = [&](Ast_گرہ_اظہاریہ* idxp) -> Ast_گرہ_اظہاریہ* {
                Ast_گرہ_اظہاریہ* ثابت clonep = idxp->clone_درخت_خالص(جھوٹ);
                Ast_گرہ_اظہاریہ* ثابت wordp = new_لفظ_Sel_لفظ(ffl, fromp, clonep);
                اگر (!clonep->backp()) VL_DO_DANGLING(clonep->delete_درخت(), clonep);
                واپس wordp;
            };

            // Construct term containing the bits of the low word - always needed
            Ast_گرہ_اظہاریہ* ثابت lTermp = [&]() -> Ast_گرہ_اظہاریہ* {
                Ast_گرہ_اظہاریہ* lWordp = لفظ_Sel_2(lIdxp);
                اگر (nodep->isQuad()) lWordp = نیا AstCCast{nfl, lWordp, nodep};
                واپس نیا Ast_سرکاؤ_R{nfl, lWordp, lBitp->clone_درخت_خالص(جھوٹ), nodep->چوڑائی()};
            }();

            // Construct term containing the bits of the middle word - if needed
            Ast_گرہ_اظہاریہ* ثابت mTermp = [&]() -> Ast_گرہ_اظہاریہ* {
                اگر (!mNeeded) واپس خالی_اشاریہ;

                Ast_گرہ_اظہاریہ* mWordp = لفظ_Sel_2(mIdxp);
                اگر (nodep->isQuad()) mWordp = نیا AstCCast{nfl, mWordp, nodep};
                Ast_گرہ_اظہاریہ* ثابت mShiftp = نیا AstSub{lfl, نیا AstConst{lfl, VL_EDATASIZE},
                                                        lBitp->clone_درخت_خالص(جھوٹ)};
                // If we're selecting bit zero, then all 32 bits in the mid word
                // get shifted << by 32 bits, so ignore them.
                ثابت V3_نمبر zero{nodep, long_Or_Quad_چوڑائی(nodep)};
                واپس نیا AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    نیا AstEq{nfl, نیا AstConst{nfl, 0}, lBitp->clone_درخت_خالص(جھوٹ)},
                    // 0 :
                    نیا AstConst{nfl, zero},
                    //  midword >> (VL_EDATASIZE - (lbs % VL_EDATASIZE))
                    نیا Ast_سرکاؤ_L{nfl, mWordp, mShiftp, nodep->چوڑائی()}};
            }();

            // Construct term containing the bits of the high word - if needed
            Ast_گرہ_اظہاریہ* ثابت hTermp = [&]() -> Ast_گرہ_اظہاریہ* {
                اگر (!hNeeded) واپس خالی_اشاریہ;

                Ast_گرہ_اظہاریہ* hWordp = لفظ_Sel_2(hIdxp);
                hWordp = نیا AstCCast{nfl, hWordp, nodep};
                Ast_گرہ_اظہاریہ* ثابت hShiftp = نیا AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    نیا AstEq{nfl, نیا AstConst{nfl, 0}, lBitp->clone_درخت_خالص(جھوٹ)},
                    // VL_EDATASIZE :
                    نیا AstConst{lfl, VL_EDATASIZE},
                    // 64 - (lbs % VL_EDATASIZE)
                    نیا AstSub{lfl, نیا AstConst{lfl, 64}, lBitp->clone_درخت_خالص(جھوٹ)}};
                واپس نیا Ast_سرکاؤ_L{nfl, hWordp, hShiftp, nodep->چوڑائی()};
            }();

            // Delete parts not captured during construction
            VL_DO_DANGLING(lBitp->delete_درخت(), lBitp);
            VL_DO_DANGLING(lIdxp->delete_درخت(), lIdxp);
            اگر (mIdxp) VL_DO_DANGLING(mIdxp->delete_درخت(), mIdxp);
            اگر (hIdxp) VL_DO_DANGLING(hIdxp->delete_درخت(), hIdxp);

            // Or reduce the terms
            Ast_گرہ_اظہاریہ* newp = lTermp;
            اگر (mTermp) newp = نیا AstOr{nfl, mTermp, newp};
            اگر (hTermp) newp = نیا AstOr{nfl, hTermp, newp};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } ورنہ {  // Long/Quad from Long/Quad
            // No isImpure() check - can handle side effects in below
            did_نمونہ(VExpand_نمونہ::SEL_FROM_NARROW, nodep);
            File_سطر* ثابت fl = nodep->fileline();
            Ast_گرہ_اظہاریہ* fromp = nodep->fromp()->unlink_Fr_عقب();
            Ast_گرہ_اظہاریہ* ثابت lsbp = nodep->lsbp()->unlink_Fr_عقب();
            اگر (nodep->isQuad() && !fromp->isQuad()) fromp = نیا AstCCast{fl, fromp, nodep};
            // {large}>>32 requires 64-bit shift operation; then cast
            Ast_گرہ_اظہاریہ* newp = نیا Ast_سرکاؤ_R{fl, fromp, lsbp, fromp->چوڑائی()};
            newp->dtypeFrom(fromp);
            اگر (!nodep->isQuad() && fromp->isQuad()) newp = نیا AstCCast{fl, newp, nodep};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }

    منطقی expandWide(Ast_گرہ_تفویض* nodep, AstSel* rhsp) {
        UASSERT_OBJ(nodep->چوڑائی_Min() == rhsp->چوڑائی_Const(), nodep, "Width mismatch");
        اگر (!doExpandWide(nodep)) واپس جھوٹ;

        // Simplify the index, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->lsbp());

        // If it's a constant select and aligned, we can just copy the words
        اگر (ثابت AstConst* ثابت lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            ثابت uint32_t lsb = lsbConstp->مقصد_UInt();
            اگر (VL_BITBIT_E(lsb) == 0) {
                did_نمونہ(VExpand_نمونہ::تفویض_SEL_ALIGNED, nodep);
                ثابت uint32_t لفظ = VL_BITWORD_E(lsb);
                برائے (صحیح w = 0; w < nodep->چوڑائی_الفاظ(); ++w) {
                    جوڑو_لفظ_تفویض(nodep, w, new_Ast_لفظ_Sel_Clone(rhsp->fromp(), w + لفظ));
                }
                واپس سچ;
            }
        }

        did_نمونہ(VExpand_نمونہ::تفویض_SEL_UNALIGNED, nodep);
        File_سطر* ثابت flp = rhsp->fileline();

        // Use fresh set of temporaries
        ++m_nTmps;

        // Compute word index of LSB, store to temporary if not constant
        Ast_گرہ_اظہاریہ* ثابت لفظ_Lsbp = rhsp->lsbp()->clone_درخت_خالص(جھوٹ);
        Ast_گرہ_اظہاریہ* لفظ_Idxp = new_لفظ_اشاریہ(لفظ_Lsbp);
        اگر (!لفظ_Lsbp->backp()) VL_DO_DANGLING(لفظ_Lsbp->delete_درخت(), لفظ_Lsbp);
        لفظ_Idxp = V3Const::constifyEditCpp(لفظ_Idxp);
        اگر (!VN_IS(لفظ_Idxp, Const)) {
            AstVar* ثابت tmpp = جوڑو_Local_Tmp(nodep, "ExpandSel_WordIdx", لفظ_Idxp);
            لفظ_Idxp = نیا AstVarRef{flp, tmpp, VAccess::READ};
        }

        // Compute shift amounts and mask, store to temporaries if not constants
        Ast_گرہ_اظہاریہ* loShftp = خالی_اشاریہ;
        Ast_گرہ_اظہاریہ* hiShftp = خالی_اشاریہ;
        Ast_گرہ_اظہاریہ* hiMaskp = خالی_اشاریہ;
        اگر (ثابت AstConst* ثابت lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            ثابت uint32_t ثنائی_ہندسہ_انحراف = VL_BITBIT_E(lsbConstp->مقصد_UInt());
            // Must be unaligned, otherwise we would have handled it above
            UASSERT_OBJ(ثنائی_ہندسہ_انحراف, nodep, "Missed aligned wide select");
            loShftp = نیا AstConst{flp, ثنائی_ہندسہ_انحراف};
            hiShftp = نیا AstConst{flp, VL_EDATASIZE - ثنائی_ہندسہ_انحراف};
            hiMaskp = خالی_اشاریہ;
        } ورنہ {
            // Compute Low word shift amount: bottom bits of lsb index
            Ast_گرہ_اظہاریہ* ثابت lsbp = rhsp->lsbp()->clone_درخت_خالص(جھوٹ);
            loShftp = نیا AstAnd{flp, نیا AstConst{flp, VL_SIZEBITS_E}, lsbp};
            AstVar* ثابت loTmpp = جوڑو_Local_Tmp(nodep, "ExpandSel_LoShift", loShftp);
            loShftp = نیا AstVarRef{flp, loTmpp, VAccess::READ};

            // Compute if aligned: Low word shift amount is 0
            Ast_گرہ_اظہاریہ* ثابت zerop = نیا AstConst{flp, 0};
            Ast_گرہ_اظہاریہ* alignedp = نیا AstEq{flp, zerop, loShftp->clone_درخت_خالص(جھوٹ)};
            AstVar* ثابت alignedTmpp = جوڑو_Local_Tmp(nodep, "ExpandSel_Aligned", alignedp);
            alignedp = نیا AstVarRef{flp, alignedTmpp, VAccess::READ};

            // Computed High word shift amount: 0 if aligned else VL_EDATASIZE - Low word shift
            Ast_گرہ_اظہاریہ* ثابت edsp = نیا AstConst{flp, VL_EDATASIZE};
            Ast_گرہ_اظہاریہ* ثابت subp = نیا AstSub{flp, edsp, loShftp->clone_درخت_خالص(جھوٹ)};
            hiShftp = نیا AstCond{flp, alignedp, نیا AstConst{flp, 0}, subp};
            AstVar* ثابت hiTmpp = جوڑو_Local_Tmp(nodep, "ExpandSel_HiShift", hiShftp);
            hiShftp = نیا AstVarRef{flp, hiTmpp, VAccess::READ};

            // Compute the High word mask: 0 if aligned, ones otherwise
            hiMaskp = نیا AstCond{flp, alignedp->clone_درخت_خالص(جھوٹ),
                                  نیا AstConst{flp, AstConst::SizedEData{}, 0},
                                  نیا AstConst{flp, AstConst::SizedEData{}, -1ULL}};
            AstVar* ثابت نقاب_Tmpp = جوڑو_Local_Tmp(nodep, "ExpandSel_HiMask", hiMaskp);
            hiMaskp = نیا AstVarRef{flp, نقاب_Tmpp, VAccess::READ};
        }

        // Create each word of the selected result
        Ast_گرہ_اظہاریہ* ثابت fromp = rhsp->fromp();
        ثابت صحیح sel_الفاظ = nodep->چوڑائی_الفاظ();
        برائے (صحیح w = 0; w < sel_الفاظ; ++w) {
            // Grab bits from word 'VL_BITWORD_E(lsb) + w'
            Ast_گرہ_اظہاریہ* ثابت loWordp = new_لفظ_Sel_لفظ(fromp->fileline(), fromp, لفظ_Idxp, w);
            Ast_گرہ_اظہاریہ* ثابت loShftClonep = loShftp->clone_درخت_خالص(جھوٹ);
            Ast_گرہ_اظہاریہ* ثابت lop = نیا Ast_سرکاؤ_R{flp, loWordp, loShftClonep, VL_EDATASIZE};
            // Grab bits from word 'VL_BITWORD_E(lsb) + w + 1'
            Ast_گرہ_اظہاریہ* hiWordp = new_لفظ_Sel_لفظ(fromp->fileline(), fromp, لفظ_Idxp, w + 1);
            // For the last word of the result, avoid an OOB access when the Sel is not OOB
            اگر (w == sel_الفاظ - 1) {
                ثابت uint32_t max = fromp->چوڑائی_الفاظ() - w - 1;
                Ast_گرہ_اظہاریہ* ثابت maxp = نیا AstConst{flp, max};
                Ast_گرہ_اظہاریہ* ثابت condp = نیا AstGte{flp, لفظ_Idxp->clone_درخت_خالص(جھوٹ), maxp};
                Ast_گرہ_اظہاریہ* ثابت zerop = نیا AstConst{flp, AstConst::SizedEData{}, 0};
                hiWordp = نیا AstCond{flp, condp, zerop, hiWordp};
            }
            Ast_گرہ_اظہاریہ* ثابت hiShftClonep = hiShftp->clone_درخت_خالص(جھوٹ);
            Ast_گرہ_اظہاریہ* ثابت hiPartp = نیا Ast_سرکاؤ_L{flp, hiWordp, hiShftClonep, VL_EDATASIZE};
            Ast_گرہ_اظہاریہ* ثابت hip
                = hiMaskp ? نیا AstAnd{flp, hiPartp, hiMaskp->clone_درخت_خالص(جھوٹ)} : hiPartp;
            // Combine them
            جوڑو_لفظ_تفویض(nodep, w, نیا AstOr{nodep->fileline(), hip, lop});
        }

        // Delete parts not captured during construction
        اگر (!لفظ_Idxp->backp()) VL_DO_DANGLING(لفظ_Idxp->delete_درخت(), لفظ_Idxp);
        اگر (!loShftp->backp()) VL_DO_DANGLING(loShftp->delete_درخت(), loShftp);
        اگر (!hiShftp->backp()) VL_DO_DANGLING(hiShftp->delete_درخت(), hiShftp);
        اگر (hiMaskp && !hiMaskp->backp()) VL_DO_DANGLING(hiMaskp->delete_درخت(), hiMaskp);

        واپس سچ;
    }

    منطقی expandLhs(Ast_گرہ_تفویض* nodep, AstSel* lhsp) {
        // Possibilities
        //      destp: wide or narrow
        //      rhsp:  wide (destp must be wide), narrow, or 1 bit wide
        //      rhsp:  may be allones and can remove AND NOT gate
        //      lsbp:  constant or variable
        // Yuk.
        اگر (isImpure(nodep)) واپس جھوٹ;
        File_سطر* ثابت nfl = nodep->fileline();
        File_سطر* ثابت lfl = lhsp->fileline();
        ثابت منطقی destwide = lhsp->fromp()->isWide();
        ثابت منطقی ones = nodep->rhsp()->isAllOnesV();
        اگر (VN_IS(lhsp->lsbp(), Const)) {
            // The code should work without this constant test, but it won't
            // constify as nicely as we'd like.
            Ast_گرہ_اظہاریہ* rhsp = nodep->rhsp()->unlink_Fr_عقب();
            Ast_گرہ_اظہاریہ* ثابت destp = lhsp->fromp()->unlink_Fr_عقب();
            ثابت صحیح lsb = lhsp->lsbConst();
            ثابت صحیح msb = lhsp->msbConst();
            V3_نمبر maskset{nodep, destp->چوڑائی_Min()};
            maskset.سیٹ_نقاب(msb + 1 - lsb, lsb);
            V3_نمبر maskold{nodep, destp->چوڑائی_Min()};
            maskold.opNot(maskset);
            اگر (destwide) {
                did_نمونہ(VExpand_نمونہ::تفویض_مقصد_SEL_CONST_WIDE, nodep);
                برائے (صحیح w = 0; w < destp->چوڑائی_الفاظ(); ++w) {
                    اگر (w >= VL_BITWORD_E(lsb) && w <= VL_BITWORD_E(msb)) {
                        // else we would just be setting it to the same exact value
                        Ast_گرہ_اظہاریہ* oldvalp = new_Ast_لفظ_Sel_Clone(destp, w);
                        fixCloneLvalue(oldvalp);
                        اگر (!ones) {
                            oldvalp = نیا AstAnd{
                                lfl,
                                نیا AstConst{lfl, AstConst::SizedEData{}, maskold.edata_لفظ(w)},
                                oldvalp};
                        }

                        // Appropriate word of new value to insert:
                        Ast_گرہ_اظہاریہ* newp = new_لفظ_Grab_سرکاؤ(lfl, w, rhsp, lsb);

                        // Apply cleaning at the top word of the destination
                        // (no cleaning to do if dst's width is a whole number
                        // of words).
                        اگر (w == destp->چوڑائی_الفاظ() - 1 && VL_BITBIT_E(destp->چوڑائی_Min()) != 0) {
                            V3_نمبر cleanmask{nodep, VL_EDATASIZE};
                            cleanmask.سیٹ_نقاب(VL_BITBIT_E(destp->چوڑائی_Min()));
                            newp = نیا AstAnd{lfl, newp, نیا AstConst{lfl, cleanmask}};
                        }
                        Ast_گرہ_اظہاریہ* ثابت orp
                            = V3Const::constifyEditCpp(نیا AstOr{lfl, oldvalp, newp});
                        جوڑو_لفظ_تفویض(nodep, w, destp, orp);
                    }
                }
                VL_DO_DANGLING(rhsp->delete_درخت(), rhsp);
                VL_DO_DANGLING(destp->delete_درخت(), destp);
            } ورنہ {
                did_نمونہ(VExpand_نمونہ::تفویض_مقصد_SEL_CONST_NARROW, nodep);
                اگر (destp->isQuad() && !rhsp->isQuad()) rhsp = نیا AstCCast{nfl, rhsp, nodep};
                Ast_گرہ_اظہاریہ* oldvalp = destp->clone_درخت_خالص(سچ);
                fixCloneLvalue(oldvalp);
                اگر (!ones) oldvalp = نیا AstAnd{lfl, نیا AstConst{lfl, maskold}, oldvalp};

                // The bit-select can refer to bits outside the width of nodep
                // which we aren't allowed to assign to.  This is a mask of the
                // valid range of nodep which we apply to the new shifted RHS.
                V3_نمبر cleanmask{nodep, destp->چوڑائی_Min()};
                cleanmask.سیٹ_نقاب(destp->چوڑائی_Min());
                Ast_گرہ_اظہاریہ* ثابت shifted = نیا Ast_سرکاؤ_L{
                    lfl, rhsp, نیا AstConst{lfl, static_cast<uint32_t>(lsb)}, destp->چوڑائی()};
                Ast_گرہ_اظہاریہ* ثابت cleaned
                    = نیا AstAnd{lfl, shifted, نیا AstConst{lfl, cleanmask}};
                Ast_گرہ_اظہاریہ* ثابت orp
                    = V3Const::constifyEditCpp(نیا AstOr{lfl, oldvalp, cleaned});
                داخل_کرو_Before(nodep, نیا Ast_تفویض{nfl, destp, orp});
            }
            واپس سچ;
        } ورنہ {  // non-const select offset
            اگر (destwide && lhsp->چوڑائی_Const() == 1) {
                did_نمونہ(VExpand_نمونہ::تفویض_مقصد_SEL_VAR_WIDE_ONE_ثنائی_ہندسہ, nodep);
                Ast_گرہ_اظہاریہ* ثابت rhsp = nodep->rhsp()->unlink_Fr_عقب();
                Ast_گرہ_اظہاریہ* ثابت destp = lhsp->fromp()->unlink_Fr_عقب();
                Ast_گرہ_اظہاریہ* oldvalp
                    = new_لفظ_Sel_ثنائی_ہندسہ(lfl, destp->clone_درخت_خالص(جھوٹ), lhsp->lsbp());
                fixCloneLvalue(oldvalp);
                اگر (!ones) {
                    oldvalp = نیا AstAnd{
                        lfl,
                        نیا AstNot{
                            lfl, نیا Ast_سرکاؤ_L{lfl, نیا AstConst{nfl, 1},
                                               // newSelBitBit may exceed the MSB of this variable.
                                               // That's ok as we'd just AND with a larger value,
                                               // but oldval would clip the upper bits to sanity
                                               new_Sel_ثنائی_ہندسہ_ثنائی_ہندسہ(lhsp->lsbp()), VL_EDATASIZE}},
                        oldvalp};
                }
                // Restrict the shift amount to 0-31, see bug804.
                Ast_گرہ_اظہاریہ* ثابت shiftp = نیا AstAnd{nfl, lhsp->lsbp()->clone_درخت_خالص(سچ),
                                                       نیا AstConst{nfl, VL_EDATASIZE - 1}};
                Ast_گرہ* ثابت newp = نیا Ast_تفویض{
                    nfl, new_لفظ_Sel_ثنائی_ہندسہ(nfl, destp, lhsp->lsbp()),
                    نیا AstOr{lfl, oldvalp, نیا Ast_سرکاؤ_L{lfl, rhsp, shiftp, VL_EDATASIZE}}};
                داخل_کرو_Before(nodep, newp);
                واپس سچ;
            } ورنہ اگر (destwide) {
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
                واپس جھوٹ;
            } ورنہ {
                did_نمونہ(VExpand_نمونہ::تفویض_مقصد_SEL_VAR_NARROW, nodep);
                // nodep->dumpTree("-  old: ");
                Ast_گرہ_اظہاریہ* rhsp = nodep->rhsp()->unlink_Fr_عقب();
                Ast_گرہ_اظہاریہ* ثابت destp = lhsp->fromp()->unlink_Fr_عقب();
                Ast_گرہ_اظہاریہ* oldvalp = destp->clone_درخت_خالص(سچ);
                fixCloneLvalue(oldvalp);

                V3_نمبر maskwidth{nodep, destp->چوڑائی_Min()};
                maskwidth.سیٹ_نقاب(lhsp->چوڑائی_Const());

                اگر (destp->isQuad() && !rhsp->isQuad()) rhsp = نیا AstCCast{nfl, rhsp, nodep};
                اگر (!ones) {
                    oldvalp = نیا AstAnd{
                        lfl,
                        نیا AstNot{lfl, نیا Ast_سرکاؤ_L{lfl, نیا AstConst{nfl, maskwidth},
                                                      lhsp->lsbp()->clone_درخت_خالص(سچ),
                                                      destp->چوڑائی()}},
                        oldvalp};
                }
                Ast_گرہ_اظہاریہ* newp
                    = نیا Ast_سرکاؤ_L{lfl, rhsp, lhsp->lsbp()->clone_درخت_خالص(سچ), destp->چوڑائی()};
                // Apply cleaning to the new value being inserted.  Mask is
                // slightly wider than necessary to avoid an AND with all ones
                // being optimized out.  No need to clean if destp is
                // quad-sized as there are no extra bits to contaminate
                اگر (destp->چوڑائی_Min() != 64) {
                    V3_نمبر cleanmask{nodep, destp->چوڑائی_Min() + 1};
                    cleanmask.سیٹ_نقاب(destp->چوڑائی_Min());
                    newp = نیا AstAnd{lfl, newp, نیا AstConst{lfl, cleanmask}};
                }

                داخل_کرو_Before(nodep, نیا Ast_تفویض{nfl, destp, نیا AstOr{lfl, oldvalp, newp}});
                واپس سچ;
            }
        }
    }

    خالی دورہ(AstConcat* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;  // Process once
        iterateChildren(nodep);
        اگر (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } ورنہ {
            // No isImpure() check - can handle side effects in below
            did_نمونہ(VExpand_نمونہ::CONCAT, nodep);
            File_سطر* ثابت fl = nodep->fileline();
            Ast_گرہ_اظہاریہ* lhsp = nodep->lhsp()->unlink_Fr_عقب();
            Ast_گرہ_اظہاریہ* rhsp = nodep->rhsp()->unlink_Fr_عقب();
            ثابت uint32_t rhsshift = rhsp->چوڑائی_Min();
            اگر (nodep->isQuad() && !lhsp->isQuad()) lhsp = نیا AstCCast{fl, lhsp, nodep};
            اگر (nodep->isQuad() && !rhsp->isQuad()) rhsp = نیا AstCCast{fl, rhsp, nodep};
            Ast_گرہ_اظہاریہ* ثابت newp = نیا AstOr{
                fl, نیا Ast_سرکاؤ_L{fl, lhsp, نیا AstConst{fl, rhsshift}, nodep->چوڑائی()}, rhsp};
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    منطقی expandWide(Ast_گرہ_تفویض* nodep, AstConcat* rhsp) {
        اگر (!doExpandWide(rhsp)) واپس جھوٹ;
        did_نمونہ(VExpand_نمونہ::تفویض_CONCAT, nodep);
        File_سطر* ثابت fl = rhsp->fileline();
        // Lhs or Rhs may be word, long, or quad.
        // newAstWordSelClone nicely abstracts the difference.
        ثابت صحیح rhsshift = rhsp->rhsp()->چوڑائی_Min();
        // Sometimes doing the words backwards is preferable.
        // When we have x={x,foo} backwards is better, when x={foo,x} forward is better
        // However V3Subst tends to rip this up, so not worth optimizing now.
        برائے (صحیح w = 0; w < rhsp->چوڑائی_الفاظ(); ++w) {
            جوڑو_لفظ_تفویض(nodep, w,
                          نیا AstOr{fl, new_لفظ_Grab_سرکاؤ(fl, w, rhsp->lhsp(), rhsshift),
                                    new_Ast_لفظ_Sel_Clone(rhsp->rhsp(), w)});
        }
        واپس سچ;
    }

    خالی دورہ(AstReplicate* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;  // Process once
        iterateChildren(nodep);
        اگر (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } ورنہ {
            اگر (isImpure(nodep)) واپس;
            File_سطر* ثابت fl = nodep->fileline();
            Ast_گرہ_اظہاریہ* lhsp = nodep->srcp()->unlink_Fr_عقب();
            Ast_گرہ_اظہاریہ* newp;
            ثابت صحیح lhswidth = lhsp->چوڑائی_Min();
            اگر (lhswidth == 1) {
                did_نمونہ(VExpand_نمونہ::REPLICATE_ONE_ثنائی_ہندسہ, nodep);
                newp = نیا AstNegate{fl, lhsp};
            } ورنہ {
                did_نمونہ(VExpand_نمونہ::REPLICATE, nodep);
                ثابت AstConst* ثابت constp = VN_AS(nodep->countp(), Const);
                UASSERT_OBJ(constp, nodep,
                            "Replication value isn't a constant.  Checked earlier!");
                ثابت uint32_t times = constp->مقصد_UInt();
                اگر (nodep->isQuad() && !lhsp->isQuad()) lhsp = نیا AstCCast{fl, lhsp, nodep};
                newp = lhsp->clone_درخت_خالص(سچ);
                برائے (غیرعلامتی repnum = 1; repnum < times; repnum++) {
                    ثابت صحیح rhsshift = repnum * lhswidth;
                    newp = نیا AstOr{
                        fl,
                        نیا Ast_سرکاؤ_L{fl, lhsp->clone_درخت_خالص(سچ),
                                      نیا AstConst{fl, static_cast<uint32_t>(rhsshift)},
                                      nodep->چوڑائی()},
                        newp};
                    newp->dtypeFrom(nodep);  // Unsigned
                }
                VL_DO_DANGLING(lhsp->delete_درخت(), lhsp);  // Never used
            }
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    منطقی expandWide(Ast_گرہ_تفویض* nodep, AstReplicate* rhsp) {
        اگر (!doExpandWide(rhsp)) واپس جھوٹ;
        did_نمونہ(VExpand_نمونہ::تفویض_REPLICATE, nodep);
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گرہ_اظہاریہ* ثابت lhsp = rhsp->srcp();
        ثابت صحیح lhswidth = lhsp->چوڑائی_Min();
        ثابت AstConst* ثابت constp = VN_AS(rhsp->countp(), Const);
        UASSERT_OBJ(constp, rhsp, "Replication value isn't a constant.  Checked earlier!");
        ثابت uint32_t times = constp->مقصد_UInt();
        برائے (صحیح w = 0; w < rhsp->چوڑائی_الفاظ(); ++w) {
            Ast_گرہ_اظہاریہ* newp;
            اگر (lhswidth == 1) {
                newp = نیا AstNegate{fl, lhsp->clone_درخت_خالص(سچ)};
                // Replicate always unsigned
                newp->dtype_Chg_چوڑائی_Signed(VL_EDATASIZE, VL_EDATASIZE, VSigning::UNSIGNED);
            } ورنہ {
                newp = new_Ast_لفظ_Sel_Clone(lhsp, w);
                File_سطر* ثابت rfl = rhsp->fileline();
                برائے (غیرعلامتی repnum = 1; repnum < times; repnum++) {
                    newp = نیا AstOr{fl, new_لفظ_Grab_سرکاؤ(rfl, w, lhsp, lhswidth * repnum), newp};
                }
            }
            جوڑو_لفظ_تفویض(nodep, w, newp);
        }
        واپس سچ;
    }

    خالی دورہ_Eq_Neq(Ast_گرہ_Biop* nodep) {
        اگر (nodep->user1_سیٹ_Once()) واپس;  // Process once
        iterateChildren(nodep);
        اگر (nodep->lhsp()->isWide()
            && !(VN_IS(nodep->lhsp()->dtypep()->skipRefp(), دھارا_DType)
                 || VN_IS(nodep->rhsp()->dtypep()->skipRefp(), دھارا_DType))) {
            اگر (isImpure(nodep)) واپس;
            اگر (!doExpandWide(nodep->lhsp())) واپس;
            اگر (!doExpandWide(nodep->rhsp())) واپس;
            did_نمونہ(VN_IS(nodep, Neq) ? VExpand_نمونہ::WORDIZE_NEQ
                                         : VExpand_نمونہ::WORDIZE_EQ,
                       nodep);
            // -> (0=={or{for each_word{WORDSEL(lhs,#)^WORDSEL(rhs,#)}}}
            File_سطر* ثابت fl = nodep->fileline();
            Ast_گرہ_اظہاریہ* newp = خالی_اشاریہ;
            برائے (صحیح w = 0; w < nodep->lhsp()->چوڑائی_الفاظ(); ++w) {
                Ast_گرہ_اظہاریہ* ثابت eqp = نیا AstXor{fl, new_Ast_لفظ_Sel_Clone(nodep->lhsp(), w),
                                                    new_Ast_لفظ_Sel_Clone(nodep->rhsp(), w)};
                newp = newp ? نیا AstOr{fl, newp, eqp} : eqp;
            }
            اگر (VN_IS(nodep, Neq)) {
                newp = نیا AstNeq{fl, نیا AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            } ورنہ {
                newp = نیا AstEq{fl, نیا AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            }
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    خالی دورہ(AstEq* nodep) override { دورہ_Eq_Neq(nodep); }
    خالی دورہ(AstNeq* nodep) override { دورہ_Eq_Neq(nodep); }

    خالی دورہ(AstRedOr* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;  // Process once
        iterateChildren(nodep);
        File_سطر* ثابت fl = nodep->fileline();
        اگر (nodep->lhsp()->isWide()) {
            اگر (isImpure(nodep)) واپس;
            اگر (!doExpandWide(nodep->lhsp())) واپس;
            did_نمونہ(VExpand_نمونہ::WORDIZE_REDOR, nodep);
            // -> (0!={or{for each_word{WORDSEL(lhs,#)}}}
            Ast_گرہ_اظہاریہ* newp = خالی_اشاریہ;
            برائے (صحیح w = 0; w < nodep->lhsp()->چوڑائی_الفاظ(); ++w) {
                Ast_گرہ_اظہاریہ* ثابت eqp = new_Ast_لفظ_Sel_Clone(nodep->lhsp(), w);
                newp = newp ? نیا AstOr{fl, newp, eqp} : eqp;
            }
            newp = نیا AstNeq{fl, نیا AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } ورنہ {
            // No isImpure() check - can handle side effects in below
            did_نمونہ(VExpand_نمونہ::REDOR_مقصد_NEQ, nodep);
            Ast_گرہ_اظہاریہ* ثابت lhsp = nodep->lhsp()->unlink_Fr_عقب();
            Ast_گرہ_اظہاریہ* ثابت newp = نیا AstNeq{
                fl, نیا AstConst{fl, AstConst::Widthed_قدر{}, long_Or_Quad_چوڑائی(nodep), 0}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    خالی دورہ(AstRedAnd* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;  // Process once
        iterateChildren(nodep);
        File_سطر* ثابت fl = nodep->fileline();
        اگر (nodep->lhsp()->isWide()) {
            اگر (isImpure(nodep)) واپس;
            اگر (!doExpandWide(nodep->lhsp())) واپس;
            did_نمونہ(VExpand_نمونہ::WORDIZE_REDAND, nodep);
            // -> (0!={and{for each_word{WORDSEL(lhs,#)}}}
            Ast_گرہ_اظہاریہ* newp = خالی_اشاریہ;
            برائے (صحیح w = 0; w < nodep->lhsp()->چوڑائی_الفاظ(); ++w) {
                Ast_گرہ_اظہاریہ* eqp = new_Ast_لفظ_Sel_Clone(nodep->lhsp(), w);
                اگر (w == nodep->lhsp()->چوڑائی_الفاظ() - 1) {
                    // Rather than doing a (slowish) ==##, we OR in the
                    // bits that aren't part of the mask
                    eqp = نیا AstOr{fl, نیا AstConst{fl, not_Wide_نقاب(nodep->lhsp())},
                                    // Bug in cppcheck
                                    // cppcheck-suppress memleak
                                    eqp};
                }
                newp = newp ? نیا AstAnd{fl, newp, eqp} : eqp;
            }
            newp = نیا AstEq{fl, نیا AstConst{fl, AstConst::SizedEData{}, VL_نقاب_E(VL_EDATASIZE)},
                             newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } ورنہ {
            // No isImpure() check - can handle side effects in below
            did_نمونہ(VExpand_نمونہ::REDAND_مقصد_EQ, nodep);
            Ast_گرہ_اظہاریہ* ثابت lhsp = nodep->lhsp()->unlink_Fr_عقب();
            Ast_گرہ_اظہاریہ* ثابت newp = نیا AstEq{fl, نیا AstConst{fl, لفظ_نقاب(lhsp)}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    خالی دورہ(AstRedXor* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;  // Process once
        iterateChildren(nodep);
        اگر (nodep->lhsp()->isWide()) {
            اگر (isImpure(nodep)) واپس;
            اگر (!doExpandWide(nodep->lhsp())) واپس;
            did_نمونہ(VExpand_نمونہ::WORDIZE_REDXOR, nodep);
            // -> (0!={redxor{for each_word{XOR(WORDSEL(lhs,#))}}}
            File_سطر* ثابت fl = nodep->fileline();
            Ast_گرہ_اظہاریہ* newp = خالی_اشاریہ;
            برائے (صحیح w = 0; w < nodep->lhsp()->چوڑائی_الفاظ(); ++w) {
                Ast_گرہ_اظہاریہ* ثابت eqp = new_Ast_لفظ_Sel_Clone(nodep->lhsp(), w);
                newp = newp ? نیا AstXor{fl, newp, eqp} : eqp;
            }
            newp = نیا AstRedXor{fl, newp};
            UINFO(8, "    Wordize REDXORnew " << newp);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
        // We don't reduce non-wide XORs, as its more efficient to use a temp register,
        // which the inlined function does nicely.
    }

    خالی دورہ(Ast_گرہ_Stmt* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;  // Process once
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ_تفویض* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;  // Process once
        اگر (VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            واپس;  // Skip for UnpackArrayDType
        }

        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
        منطقی did = جھوٹ;
        اگر (nodep->isWide()  //
            && ((VN_IS(nodep->lhsp(), VarRef) || VN_IS(nodep->lhsp(), ArraySel)))
            && !AstVar::scVarRecurse(nodep->lhsp())  // Need special function for SC
            && !AstVar::scVarRecurse(nodep->rhsp())) {
            اگر (AstConst* ثابت rhsp = VN_CAST(nodep->rhsp(), Const)) {
                did = expandWide(nodep, rhsp);
            } ورنہ اگر (AstVarRef* ثابت rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                did = expandWide(nodep, rhsp);
            } ورنہ اگر (AstSel* ثابت rhsp = VN_CAST(nodep->rhsp(), Sel)) {
                did = expandWide(nodep, rhsp);
            } ورنہ اگر (AstArraySel* ثابت rhsp = VN_CAST(nodep->rhsp(), ArraySel)) {
                did = expandWide(nodep, rhsp);
            } ورنہ اگر (AstConcat* ثابت rhsp = VN_CAST(nodep->rhsp(), Concat)) {
                did = expandWide(nodep, rhsp);
            } ورنہ اگر (AstExtend* ثابت rhsp = VN_CAST(nodep->rhsp(), Extend)) {
                did = expandWide(nodep, rhsp);
            } ورنہ اگر (AstReplicate* ثابت rhsp = VN_CAST(nodep->rhsp(), Replicate)) {
                did = expandWide(nodep, rhsp);
            } ورنہ اگر (AstAnd* ثابت rhsp = VN_CAST(nodep->rhsp(), And)) {
                did = expandWide(nodep, rhsp);
            } ورنہ اگر (AstOr* ثابت rhsp = VN_CAST(nodep->rhsp(), Or)) {
                did = expandWide(nodep, rhsp);
            } ورنہ اگر (AstNot* ثابت rhsp = VN_CAST(nodep->rhsp(), Not)) {
                did = expandWide(nodep, rhsp);
            } ورنہ اگر (AstXor* ثابت rhsp = VN_CAST(nodep->rhsp(), Xor)) {
                did = expandWide(nodep, rhsp);
            } ورنہ اگر (Ast_سرکاؤ_L* ثابت rhsp = VN_CAST(nodep->rhsp(), سرکاؤ_L)) {
                did = expand_Wide_سرکاؤ(nodep, rhsp, /* isLeftShift: */ سچ);
            } ورنہ اگر (Ast_سرکاؤ_R* ثابت rhsp = VN_CAST(nodep->rhsp(), سرکاؤ_R)) {
                did = expand_Wide_سرکاؤ(nodep, rhsp, /* isLeftShift: */ جھوٹ);
            } ورنہ اگر (AstCond* ثابت rhsp = VN_CAST(nodep->rhsp(), Cond)) {
                did = expandWide(nodep, rhsp);
            }
        } ورنہ اگر (AstSel* ثابت lhsp = VN_CAST(nodep->lhsp(), Sel)) {
            did = expandLhs(nodep, lhsp);
        }
        // Cleanup common code
        اگر (did) VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
    }

    //--------------------
    خالی دورہ(AstVar*) override {}  // Don't hit varrefs under vars
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح ExpandVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ExpandVisitor() override {
        V3Stats::جوڑو_Stat("Optimizations, Expand, expanded wides", m_statWides);
        V3Stats::جوڑو_Stat("Optimizations, Expand, expanded wide words", m_stat_Wide_الفاظ);
        V3Stats::جوڑو_Stat("Optimizations, Expand, not expanded impure", m_statImpure);
        V3Stats::جوڑو_Stat("Optimizations, Expand, not expanded limit", m_statWideLimited);
        V3Stats::جوڑو_Stat("Optimizations, Expand, temporaries created", m_statTemps);
        ثابت خودکار emitStat = [یہ](VExpand_نمونہ نمونہ_2) {
            std::string str{نمونہ_2.ascii()};
            std::transform(str.begin(), str.انتہا(), str.begin(), [](غیرعلامتی حرف c) {  //
                واپس c == '_' ? ' ' : std::tolower(c);
            });
            V3Stats::جوڑو_Stat("Optimizations, Expand, pattern " + str, m_statPatterns[نمونہ_2]);
        };
#تعریف_کرو EXPAND_نمونہ_EMIT_STAT(id) emitStat(VExpand_نمونہ::id);
        FOR_EACH_EXPAND_نمونہ(EXPAND_نمونہ_EMIT_STAT)
#تعریف_منسوخ EXPAND_نمونہ_EMIT_STAT
    }
};

//######################################################################
// Expand class functions

خالی V3Expand::expandAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ثابت ExpandOkVisitor okVisitor{nodep};
        ExpandVisitor{nodep};
    }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("expand", 0, dump_درخت_Either_Level() >= 3);
}
