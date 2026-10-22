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
#تعریف_کرو FOR_EACH_EXPAND_نمونہ(macro) \
    macro(سونپو_AND) \
    macro(سونپو_ARRAYSEL) \
    macro(سونپو_CONCAT) \
    macro(سونپو_COND) \
    macro(سونپو_CONST) \
    macro(سونپو_EXTEND) \
    macro(سونپو_NOT) \
    macro(سونپو_OR) \
    macro(سونپو_REPLICATE) \
    macro(سونپو_SEL_ALIGNED) \
    macro(سونپو_SEL_UNALIGNED) \
    macro(سونپو_SHIFTL) \
    macro(سونپو_SHIFTR) \
    macro(سونپو_منزل_SEL_CONST_NARROW) \
    macro(سونپو_منزل_SEL_CONST_WIDE) \
    macro(سونپو_منزل_SEL_VAR_NARROW) \
    macro(سونپو_منزل_SEL_VAR_WIDE_ONE_ثنائی_ہندسہ) \
    macro(سونپو_VARREF) \
    macro(سونپو_XOR) \
    macro(CONCAT) \
    macro(REDAND_منزل_EQ) \
    macro(REDOR_منزل_NEQ) \
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

صنف VExpand_نمونہ final {
عام:
    تعداد en : uint8_t {
#تعریف_کرو EXPAND_نمونہ_ID(id) id,
        FOR_EACH_EXPAND_نمونہ(EXPAND_نمونہ_ID)
#تعریف_رد EXPAND_نمونہ_ID
            ENUM_سرا
    };
    تعداد en m_e;
    ثابت حرف* ascii() ثابت {
        ساكن ثابت حرف* ثابت ناں[] = {
#تعریف_کرو EXPAND_نمونہ_NAME(id) #id,
            FOR_EACH_EXPAND_نمونہ(EXPAND_نمونہ_NAME)
#تعریف_رد EXPAND_نمونہ_NAME
                "_ENUM_END"  //
        };
        أرجع ناں[m_e];
    }

    // cppcheck-suppress noExplicitConstructor
    VExpand_نمونہ(en _e)
        : m_e{_e} {}
    عامل en() ثابت { أرجع m_e; }
};

//######################################################################
// Find nodes with side effects, to mark as non-expandable

صنف ExpandOkVisitor final : عام VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> bool.  Is pure (along with all children)
    ثابت VNUser2_وچ_Use m_inuser2;

    // STATE - for current visit position (use VL_RESTORER)
    // Tracks similar to AstNode::isTreePureRecurse(), but avoid O(n^2)
    // False = pure, as nodes that ExpandVisitor inserts preserve pureness
    منطقي m_isImpure = سچ;  // Currently pure

    فراغ ویکھو(Ast_گرہ* nodep) override {
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
    واضح ExpandOkVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~ExpandOkVisitor() = افتراضي;
};

//######################################################################
// Expand state, as a visitor of each AstNode

صنف ExpandVisitor final : عام VNVisitor {
    // NODE STATE
    //  AstNode::user1()        -> bool.  Processed
    //  AstNode::user2()        -> See ExpandOkVisitor
    ثابت VNUser1_وچ_Use m_inuser1;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_گرہ* m_stmtp = خالی_سوچک;  // Current statement

    // STATE - across all visitors
    AstCFunc* m_funcp = خالی_سوچک;  // Current function
    VDouble0 m_statImpure;  // Statistic tracking
    VDouble0 m_statWides;  // Statistic tracking
    VDouble0 m_stat_Wide_الفاظ;  // Statistic tracking
    VDouble0 m_statWideLimited;  // Statistic tracking
    VDouble0 m_statTemps;  // Statistic tracking
    // Number of applications of each expansion pattern for statistics
    std::array<VDouble0, VExpand_نمونہ::ENUM_سرا> m_statPatterns;

    // STATE - for current function
    ناپ_t m_nTmps = 0;  // Sequence numbers for temopraries

    // METHODS
    // Note the application of the given expansion pattern to 'nodep'
    فراغ did_نمونہ(VExpand_نمونہ نمونہ_2, Ast_گرہ* nodep) {
        UINFO(8, "    " << نمونہ_2.ascii() << " " << nodep);
        ++m_statPatterns[نمونہ_2];
    }

    // Use state that ExpandOkVisitor calculated
    ساكن منطقي isImpure(Ast_گرہ* nodep) {
        ثابت منطقي impure = nodep->user2();
        إذا (impure) UINFO(9, "      impure " << nodep);
        أرجع impure;
    }

    منطقي doExpandWide(Ast_گرہ* nodep) {
        إذا (isImpure(nodep)) {
            ++m_statImpure;
            أرجع خطأ;
        }
        إذا (nodep->چوڑائی_الفاظ() <= v3Global.opt.expand_حد_بندی()) {
            ++m_statWides;
            m_stat_Wide_الفاظ += nodep->چوڑائی_الفاظ();
            أرجع سچ;
        } وإلا {
            ++m_statWideLimited;
            أرجع خطأ;
        }
    }

    ساكن صحيح long_Or_Quad_چوڑائی(Ast_گرہ* nodep) {
        أرجع (nodep->چوڑائی() + (VL_EDATASIZE - 1)) & ~(VL_EDATASIZE - 1);
    }
    ساكن V3_نمبر not_Wide_نقاب(Ast_گرہ* nodep) {
        أرجع V3_نمبر{nodep, VL_EDATASIZE, ~VL_نقاب_E(nodep->چوڑائی_Min())};
    }
    ساكن V3_نمبر لفظ_نقاب(Ast_گرہ* nodep) {
        إذا (nodep->isWide()) {
            أرجع V3_نمبر{nodep, VL_EDATASIZE, VL_نقاب_E(nodep->چوڑائی_Min())};
        } وإلا {
            V3_نمبر نقاب{nodep, long_Or_Quad_چوڑائی(nodep)};
            نقاب.سیٹ_نقاب(nodep->چوڑائی_Min());
            أرجع نقاب;
        }
    }

    ساكن فراغ داخل_کرو_Before(Ast_گرہ* placep, Ast_گرہ* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        placep->جوڑو_ایتھے_This_As_اگلا(newp);
    }
    ساكن فراغ replaceWithDelete(Ast_گرہ* nodep, Ast_گرہ* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        إذا (newp->چوڑائی() != nodep->چوڑائی()) {
            UASSERT_OBJ(newp->چوڑائی_Min() == nodep->چوڑائی_Min(), nodep,
                        "Replacement width mismatch");
            newp->dtype_Chg_چوڑائی(nodep->چوڑائی(), nodep->چوڑائی_Min());
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    ساكن Ast_گرہ* new_لفظ_سونپو(Ast_گرہ_سونپو* placep, صحيح لفظ, Ast_گرہ_اظہاریہ* lhsp,
                                  Ast_گرہ_اظہاریہ* rhsp) {
        File_سطر* ثابت fl = placep->fileline();
        أرجع جديد Ast_سونپو{fl,
                             جديد Ast_لفظ_Sel{fl, lhsp->clone_درخت_خالص(سچ),
                                            جديد AstConst{fl, static_cast<uint32_t>(لفظ)}},
                             rhsp};
    }
    ساكن فراغ جوڑو_لفظ_سونپو(Ast_گرہ_سونپو* placep, صحيح لفظ, Ast_گرہ_اظہاریہ* lhsp,
                              Ast_گرہ_اظہاریہ* rhsp) {
        داخل_کرو_Before(placep, new_لفظ_سونپو(placep, لفظ, lhsp, rhsp));
    }
    ساكن فراغ جوڑو_لفظ_سونپو(Ast_گرہ_سونپو* placep, صحيح لفظ, Ast_گرہ_اظہاریہ* rhsp) {
        جوڑو_لفظ_سونپو(placep, لفظ, placep->lhsp(), rhsp);
    }

    AstVar* جوڑو_Local_Tmp(Ast_گرہ* placep, ثابت حرف* namep, Ast_گرہ_اظہاریہ* valuep) {
        ثابت std::string name = "__V"s + namep + "_"s + std::منزل_string(m_nTmps);
        File_سطر* ثابت flp = placep->fileline();
        AstVar* ثابت tmpp = جديد AstVar{flp, VVarType::STMTTEMP, name, valuep->dtypep()};
        tmpp->funcLocal(سچ);
        tmpp->is_اندرونی(سچ);
        tmpp->noReset(سچ);
        tmpp->substConstOnly(سچ);
        m_funcp->جوڑو_Varsp(tmpp);
        داخل_کرو_Before(placep, جديد Ast_سونپو{flp, جديد AstVarRef{flp, tmpp, VAccess::WRITE}, valuep});
        ++m_statTemps;
        أرجع tmpp;
    }

    ساكن فراغ fixCloneLvalue(Ast_گرہ* nodep) {
        // In AstSel transforms, we call clone() on VarRefs that were lvalues,
        // but are now being used on the RHS of the assignment
        إذا (VN_IS(nodep, VarRef)) VN_AS(nodep, VarRef)->access(VAccess::READ);
        // Iterate
        إذا (Ast_گرہ* ثابت refp = nodep->op1p()) fixCloneLvalue(refp);
        إذا (Ast_گرہ* ثابت refp = nodep->op2p()) fixCloneLvalue(refp);
        إذا (Ast_گرہ* ثابت refp = nodep->op3p()) fixCloneLvalue(refp);
        إذا (Ast_گرہ* ثابت refp = nodep->op4p()) fixCloneLvalue(refp);
    }

    ساكن Ast_گرہ_اظہاریہ* new_Ast_لفظ_Sel_Clone(Ast_گرہ_اظہاریہ* nodep, صحيح لفظ) {
        // Get the specified word number from a wide array
        // Or, if it's a long/quad, do appropriate conversion to wide
        // Concat may pass negative word numbers, that means it wants a zero
        File_سطر* ثابت fl = nodep->fileline();
        إذا (nodep->isWide() && لفظ >= 0 && لفظ < nodep->چوڑائی_الفاظ()) {
            أرجع جديد Ast_لفظ_Sel{fl, nodep->clone_درخت_خالص(سچ),
                                  جديد AstConst{fl, static_cast<uint32_t>(لفظ)}};
        } وإلا إذا (nodep->isQuad() && لفظ == 0) {
            Ast_گرہ_اظہاریہ* ثابت quadfromp = nodep->clone_درخت_خالص(سچ);
            quadfromp->dtype_سیٹ_ثنائی_ہندسہ_Unsized(VL_QUADSIZE, quadfromp->چوڑائی_Min(), VSigning::UNSIGNED);
            أرجع جديد AstCCast{fl, quadfromp, VL_EDATASIZE};
        } وإلا إذا (nodep->isQuad() && لفظ == 1) {
            Ast_گرہ_اظہاریہ* ثابت quadfromp = nodep->clone_درخت_خالص(سچ);
            quadfromp->dtype_سیٹ_ثنائی_ہندسہ_Unsized(VL_QUADSIZE, quadfromp->چوڑائی_Min(), VSigning::UNSIGNED);
            أرجع جديد AstCCast{
                fl, جديد Ast_کھسکاؤ_R{fl, quadfromp, جديد AstConst{fl, VL_EDATASIZE}, VL_EDATASIZE},
                VL_EDATASIZE};
        } وإلا إذا (!nodep->isWide() && !nodep->isQuad() && لفظ == 0) {
            أرجع nodep->clone_درخت_خالص(سچ);
        } وإلا {  // Out of bounds
            أرجع جديد AstConst{fl, 0};
        }
    }

    ساكن Ast_گرہ_اظہاریہ* new_لفظ_Grab_کھسکاؤ(File_سطر* fl, صحيح لفظ, Ast_گرہ_اظہاریہ* lhsp, صحيح کھسکاؤ_2) {
        // Extract the expression to grab the value for the specified word, if it's the shift
        // of shift bits from lhsp
        // Negative word numbers requested for lhs when it's "before" what we want.
        // We get a 0 then.
        ثابت صحيح othword = لفظ - کھسکاؤ_2 / VL_EDATASIZE;
        Ast_گرہ_اظہاریہ* ثابت lop = new_Ast_لفظ_Sel_Clone(lhsp, othword);
        ثابت uint32_t lo_کھسکاؤ = VL_BITBIT_E(کھسکاؤ_2);
        إذا (!lo_کھسکاؤ) أرجع lop;
        Ast_گرہ_اظہاریہ* ثابت hip = new_Ast_لفظ_Sel_Clone(lhsp, othword - 1);
        ثابت uint32_t hi_کھسکاؤ = VL_EDATASIZE - lo_کھسکاؤ;  // Complement offset
        Ast_گرہ_اظہاریہ* ثابت newp
            = جديد AstOr{fl,  //
                        جديد Ast_کھسکاؤ_R{fl, hip, جديد AstConst{fl, hi_کھسکاؤ}, VL_EDATASIZE},
                        جديد Ast_کھسکاؤ_L{fl, lop, جديد AstConst{fl, lo_کھسکاؤ}, VL_EDATASIZE}};
        أرجع V3Const::constifyEditCpp(newp);
    }

    // Return expression indexing the word that contains 'lsbp' + the given word offset
    ساكن Ast_گرہ_اظہاریہ* new_لفظ_اشاریہ(Ast_گرہ_اظہاریہ* lsbp, uint32_t لفظ_وتھ = 0) {
        // This is indexing a WordSel, so a 32 bit constants are fine
        File_سطر* ثابت flp = lsbp->fileline();
        إذا (ثابت AstConst* constp = VN_CAST(lsbp, Const)) {
            أرجع جديد AstConst{flp, لفظ_وتھ + VL_BITWORD_E(constp->منزل_UInt())};
        }

        إذا (lsbp->backp()) lsbp = lsbp->clone_درخت_خالص(خطأ);
        Ast_گرہ_اظہاریہ* ثابت wwl2p = جديد AstConst{flp, VL_EDATASIZE_LOG2};  // Word width log 2
        Ast_گرہ_اظہاریہ* indexp = جديد Ast_کھسکاؤ_R{flp, lsbp, wwl2p, VL_EDATASIZE};
        إذا (لفظ_وتھ) indexp = جديد Ast_جوڑو{flp, جديد AstConst{flp, لفظ_وتھ}, indexp};
        أرجع indexp;
    }

    // Return word of fromp that contains word indexp + the given word offset.
    ساكن Ast_گرہ_اظہاریہ* new_لفظ_Sel_لفظ(File_سطر* flp, Ast_گرہ_اظہاریہ* fromp, Ast_گرہ_اظہاریہ* indexp,
                                       uint32_t لفظ_وتھ = 0) {
        UASSERT_OBJ(fromp->isWide(), fromp, "Only need AstWordSel on wide from's");

        إذا (ثابت AstConst* ثابت constp = VN_CAST(indexp, Const)) {
            indexp = خالی_سوچک;
            لفظ_وتھ += constp->منزل_UInt();
        }

        // e.g. "logic [95:0] var[0]; logic [0] sel; out = var[sel];"
        // Squash before C++ to avoid getting a C++ compiler warning
        // (even though code would be unreachable as presumably a
        // AstCond is protecting above this node.
        إذا (لفظ_وتھ >= static_cast<uint32_t>(fromp->چوڑائی_الفاظ())) {
            أرجع جديد AstConst{flp, AstConst::SizedEData{}, 0};
        }

        إذا (fromp->backp()) fromp = fromp->clone_درخت_خالص(خطأ);

        // If indexp was constant, just use it
        إذا (!indexp) أرجع جديد Ast_لفظ_Sel{flp, fromp, جديد AstConst{flp, لفظ_وتھ}};

        // Otherwise compute at runtime
        إذا (indexp->backp()) indexp = indexp->clone_درخت_خالص(خطأ);
        إذا (لفظ_وتھ) indexp = جديد Ast_جوڑو{flp, جديد AstConst{flp, لفظ_وتھ}, indexp};
        أرجع جديد Ast_لفظ_Sel{flp, fromp, indexp};
    }

    // Return word of fromp that contains bit lsbp + the given word offset.
    ساكن Ast_گرہ_اظہاریہ* new_لفظ_Sel_ثنائی_ہندسہ(File_سطر* flp, Ast_گرہ_اظہاریہ* fromp, Ast_گرہ_اظہاریہ* lsbp,
                                      uint32_t لفظ_وتھ = 0) {
        Ast_گرہ_اظہاریہ* ثابت indexp = new_لفظ_اشاریہ(lsbp, لفظ_وتھ);
        Ast_گرہ_اظہاریہ* ثابت لفظ_Selp = new_لفظ_Sel_لفظ(flp, fromp, indexp);
        إذا (!indexp->backp()) VL_DO_DANGLING(indexp->delete_درخت(), indexp);
        أرجع لفظ_Selp;
    }

    ساكن Ast_گرہ_اظہاریہ* new_Sel_ثنائی_ہندسہ_ثنائی_ہندسہ(Ast_گرہ_اظہاریہ* lsbp) {
        // Return equation to get the VL_BITBIT of a constant or non-constant
        File_سطر* ثابت fl = lsbp->fileline();
        إذا (VN_IS(lsbp, Const)) {
            أرجع جديد AstConst{fl, VL_BITBIT_E(VN_AS(lsbp, Const)->منزل_UInt())};
        } وإلا {
            أرجع جديد AstAnd{fl, جديد AstConst{fl, VL_EDATASIZE - 1}, lsbp->clone_درخت_خالص(سچ)};
        }
    }

    //====================

    منطقي expandWide(Ast_گرہ_سونپو* nodep, AstConst* rhsp) {
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        did_نمونہ(VExpand_نمونہ::سونپو_CONST, nodep);
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),WORDSEL(CONST,#))}}
        إذا (rhsp->عدد().isFourState()) {
            rhsp->v3warn(E_UNSUPPORTED,  // LCOV_EXCL_LINE  // impossible?
                         "Unsupported: 4-state numbers in this context");
        }
        File_سطر* ثابت fl = nodep->fileline();
        لكل (صحيح w = 0; w < nodep->چوڑائی_الفاظ(); ++w) {
            جوڑو_لفظ_سونپو(nodep, w,
                          جديد AstConst{fl, AstConst::SizedEData{}, rhsp->عدد().edata_لفظ(w)});
        }
        أرجع سچ;
    }
    //-------- Uniops
    منطقي expandWide(Ast_گرہ_سونپو* nodep, AstVarRef* rhsp) {
        // Special case: do not expand assignment of constant pool variables.
        // V3Subst undestands these directly.
        إذا (rhsp->varp()->const_Pool_اندراج()) أرجع خطأ;
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        did_نمونہ(VExpand_نمونہ::سونپو_VARREF, nodep);
        لكل (صحيح w = 0; w < nodep->چوڑائی_الفاظ(); ++w) {
            جوڑو_لفظ_سونپو(nodep, w, new_Ast_لفظ_Sel_Clone(rhsp, w));
        }
        أرجع سچ;
    }
    منطقي expandWide(Ast_گرہ_سونپو* nodep, AstArraySel* rhsp) {
        UASSERT_OBJ(!VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType), nodep,
                    "ArraySel with unpacked arrays should have been removed in V3Slice");
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        did_نمونہ(VExpand_نمونہ::سونپو_ARRAYSEL, nodep);
        لكل (صحيح w = 0; w < nodep->چوڑائی_الفاظ(); ++w) {
            جوڑو_لفظ_سونپو(nodep, w, new_Ast_لفظ_Sel_Clone(rhsp, w));
        }
        أرجع سچ;
    }
    منطقي expandWide(Ast_گرہ_سونپو* nodep, AstNot* rhsp) {
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),NOT(WORDSEL(lhs,#))) }}
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        did_نمونہ(VExpand_نمونہ::سونپو_NOT, nodep);
        File_سطر* ثابت fl = rhsp->fileline();
        لكل (صحيح w = 0; w < nodep->چوڑائی_الفاظ(); ++w) {
            جوڑو_لفظ_سونپو(nodep, w, جديد AstNot{fl, new_Ast_لفظ_Sel_Clone(rhsp->lhsp(), w)});
        }
        أرجع سچ;
    }
    //-------- Biops
    منطقي expandWide(Ast_گرہ_سونپو* nodep, AstAnd* rhsp) {
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        did_نمونہ(VExpand_نمونہ::سونپو_AND, nodep);
        File_سطر* ثابت fl = nodep->fileline();
        لكل (صحيح w = 0; w < nodep->چوڑائی_الفاظ(); ++w) {
            جوڑو_لفظ_سونپو(nodep, w,
                          جديد AstAnd{fl, new_Ast_لفظ_Sel_Clone(rhsp->lhsp(), w),
                                     new_Ast_لفظ_Sel_Clone(rhsp->rhsp(), w)});
        }
        أرجع سچ;
    }
    منطقي expandWide(Ast_گرہ_سونپو* nodep, AstOr* rhsp) {
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        did_نمونہ(VExpand_نمونہ::سونپو_OR, nodep);
        File_سطر* ثابت fl = nodep->fileline();
        لكل (صحيح w = 0; w < nodep->چوڑائی_الفاظ(); ++w) {
            جوڑو_لفظ_سونپو(nodep, w,
                          جديد AstOr{fl, new_Ast_لفظ_Sel_Clone(rhsp->lhsp(), w),
                                    new_Ast_لفظ_Sel_Clone(rhsp->rhsp(), w)});
        }
        أرجع سچ;
    }
    منطقي expandWide(Ast_گرہ_سونپو* nodep, AstXor* rhsp) {
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        did_نمونہ(VExpand_نمونہ::سونپو_XOR, nodep);
        File_سطر* ثابت fl = nodep->fileline();
        لكل (صحيح w = 0; w < nodep->چوڑائی_الفاظ(); ++w) {
            جوڑو_لفظ_سونپو(nodep, w,
                          جديد AstXor{fl, new_Ast_لفظ_Sel_Clone(rhsp->lhsp(), w),
                                     new_Ast_لفظ_Sel_Clone(rhsp->rhsp(), w)});
        }
        أرجع سچ;
    }
    منطقي expand_Wide_کھسکاؤ(Ast_گرہ_سونپو* nodep, Ast_گرہ_Biop* rhsp, منطقي is_بایاں_کھسکاؤ) {
        إذا (!doExpandWide(nodep)) أرجع خطأ;

        // Simplify the shift amount, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->rhsp());

        // If it's a constant shift by whole words, expand it so V3Subst can substitute it
        إذا (ثابت AstConst* ثابت rhsConstp = VN_CAST(rhsp->rhsp(), Const)) {
            ثابت uint32_t کھسکاؤ_بٹ = rhsConstp->منزل_UInt();
            إذا (VL_BITBIT_E(کھسکاؤ_بٹ) == 0) {
                ثابت صحيح چوڑائی_الفاظ = nodep->چوڑائی_الفاظ();
                ثابت صحيح کھسکاؤ_الفاظ = std::min<صحيح>(VL_BITWORD_E(کھسکاؤ_بٹ), چوڑائی_الفاظ);
                File_سطر* ثابت flp = rhsp->fileline();
                إذا (is_بایاں_کھسکاؤ) {
                    did_نمونہ(VExpand_نمونہ::سونپو_SHIFTL, nodep);
                    // Low words of the result are zero
                    لكل (صحيح w = 0; w < کھسکاؤ_الفاظ; ++w) {
                        جوڑو_لفظ_سونپو(nodep, w, جديد AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                    // High words of the result are copied from higher words of the source
                    لكل (صحيح w = کھسکاؤ_الفاظ; w < چوڑائی_الفاظ; ++w) {
                        جوڑو_لفظ_سونپو(nodep, w, new_Ast_لفظ_Sel_Clone(rhsp->lhsp(), w - کھسکاؤ_الفاظ));
                    }
                } وإلا {
                    did_نمونہ(VExpand_نمونہ::سونپو_SHIFTR, nodep);
                    // Low words of the result are copied from higher words of the source
                    لكل (صحيح w = 0; w < چوڑائی_الفاظ - کھسکاؤ_الفاظ; ++w) {
                        جوڑو_لفظ_سونپو(nodep, w, new_Ast_لفظ_Sel_Clone(rhsp->lhsp(), w + کھسکاؤ_الفاظ));
                    }
                    // High words of the result are zero
                    لكل (صحيح w = چوڑائی_الفاظ - کھسکاؤ_الفاظ; w < چوڑائی_الفاظ; ++w) {
                        جوڑو_لفظ_سونپو(nodep, w, جديد AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                }
                أرجع سچ;
            }
        }

        أرجع خطأ;
    }

    //-------- Triops
    منطقي expandWide(Ast_گرہ_سونپو* nodep, AstCond* rhsp) {
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        did_نمونہ(VExpand_نمونہ::سونپو_COND, nodep);
        File_سطر* ثابت fl = nodep->fileline();
        لكل (صحيح w = 0; w < nodep->چوڑائی_الفاظ(); ++w) {
            جوڑو_لفظ_سونپو(nodep, w,
                          جديد AstCond{fl, rhsp->condp()->clone_درخت_خالص(سچ),
                                      new_Ast_لفظ_Sel_Clone(rhsp->thenp(), w),
                                      new_Ast_لفظ_Sel_Clone(rhsp->elsep(), w)});
        }
        أرجع سچ;
    }

    // VISITORS
    فراغ ویکھو(AstNetlist* nodep) override { iterate_And_اگلا_Null(nodep->modulesp()); }

    فراغ ویکھو(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_nTmps);
        m_funcp = nodep;
        m_nTmps = 0;
        ثابت VDouble0 statWidesBefore = m_statWides;
        iterateChildren(nodep);

        // Constant fold here if anything was expanded, as Ast size can likely be reduced
        إذا (v3Global.opt.fConstEager() && m_statWides != statWidesBefore) {
            Ast_گرہ* ثابت editedp = V3Const::constifyEditCpp(nodep);
            UASSERT_OBJ(editedp == nodep, editedp, "Should not have replaced CFunc");
        }
    }

    فراغ ویکھو(AstExtend* nodep) override {
        إذا (nodep->user1_سیٹ_Once()) أرجع;  // Process once
        iterateChildren(nodep);
        إذا (nodep->isWide()) {
            // See under ASSIGN(EXTEND)
        } وإلا {
            إذا (isImpure(nodep)) أرجع;
            Ast_گرہ_اظہاریہ* ثابت lhsp = nodep->lhsp()->unlink_Fr_پٹھ();
            Ast_گرہ_اظہاریہ* newp = lhsp;
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
    منطقي expandWide(Ast_گرہ_سونپو* nodep, AstExtend* rhsp) {
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        did_نمونہ(VExpand_نمونہ::سونپو_EXTEND, nodep);
        Ast_گرہ_اظہاریہ* ثابت rlhsp = rhsp->lhsp();
        لكل (صحيح w = 0; w < rlhsp->چوڑائی_الفاظ(); ++w) {
            جوڑو_لفظ_سونپو(nodep, w, new_Ast_لفظ_Sel_Clone(rlhsp, w));
        }
        لكل (صحيح w = rlhsp->چوڑائی_الفاظ(); w < nodep->چوڑائی_الفاظ(); ++w) {
            جوڑو_لفظ_سونپو(nodep, w, جديد AstConst{nodep->fileline(), AstConst::SizedEData{}, 0});
        }
        أرجع سچ;
    }

    فراغ ویکھو(AstSel* nodep) override {
        إذا (nodep->user1_سیٹ_Once()) أرجع;  // Process once
        iterateChildren(nodep);

        ثابت uint32_t چوڑائی = static_cast<uint32_t>(nodep->چوڑائی_Const());
        UASSERT_OBJ(nodep->چوڑائی_Min() == static_cast<صحيح>(چوڑائی), nodep, "Width mismatch");

        // Skip if Sel is an LHS assignment select
        إذا (Ast_گرہ_سونپو* ثابت assignp = VN_CAST(nodep->backp(), گرہ_سونپو)) {
            إذا (nodep == assignp->lhsp()) أرجع;
        }

        // Skip if wide: See under ASSIGN(WIDE)
        إذا (nodep->isWide()) أرجع;

        // Skip Sel from stream or queue
        إذا (VN_IS(nodep->fromp()->dtypep(), دھارا_DType)) أرجع;
        إذا (VN_IS(nodep->fromp()->dtypep(), قطار_DType)) أرجع;

        // Result must be non-wide after skipping all the above
        UASSERT_OBJ(چوڑائی <= 64, nodep, "Inconsistent result width");

        إذا (nodep->fromp()->isWide()) {  // Long/Quad from Wide
            إذا (isImpure(nodep)) أرجع;
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
                ساكن constexpr صحيح EXPAND_SEL_LSB_حد_بندی = 8;
                // Simplify
                exprp = V3Const::constifyEditCpp(exprp);
                // Accept if not larger than the limit
                إذا (!exprp->isLargerThan(EXPAND_SEL_LSB_حد_بندی)) أرجع exprp;
                // Delete if rejected
                VL_DO_DANGLING(exprp->delete_درخت(), exprp);
                أرجع خالی_سوچک;
            };

            // Return the index of the word holding the bit 'offset' bits above the LSB
            ثابت خودکار لفظ_Idx = [&](uint32_t وتھ) -> Ast_گرہ_اظہاریہ* {
                Ast_گرہ_اظہاریہ* ثابت msbp = جديد Ast_جوڑو{lfl, جديد AstConst{lfl, وتھ},
                                                     nodep->lsbp()->clone_درخت_خالص(خطأ)};
                Ast_گرہ_اظہاریہ* ثابت idxp = new_لفظ_اشاریہ(msbp);
                إذا (!msbp->backp()) VL_DO_DANGLING(msbp->delete_درخت(), msbp);
                أرجع idxp;
            };

            // Bit index of the selected LSB within its word
            Ast_گرہ_اظہاریہ* lBitp = tryFold(new_Sel_ثنائی_ہندسہ_ثنائی_ہندسہ(nodep->lsbp()));
            ثابت منطقي aligned = lBitp && lBitp->isZero();

            // Which of the other 2 words the expansion might need
            ثابت منطقي mNeeded = چوڑائی > 1 && !aligned;
            ثابت منطقي hNeeded = چوڑائی > VL_EDATASIZE;
            UASSERT_OBJ(!hNeeded || nodep->isQuad(), nodep, "Width mismatch");

            // Word indices
            ثابت uint32_t m_Msb_وتھ = std::min<uint32_t>(چوڑائی, VL_EDATASIZE) - 1;
            ثابت uint32_t h_Msb_وتھ = چوڑائی - 1;
            Ast_گرہ_اظہاریہ* lIdxp = tryFold(new_لفظ_اشاریہ(nodep->lsbp()));
            Ast_گرہ_اظہاریہ* mIdxp = mNeeded ? tryFold(لفظ_Idx(m_Msb_وتھ)) : خالی_سوچک;
            Ast_گرہ_اظہاریہ* hIdxp = hNeeded ? tryFold(لفظ_Idx(h_Msb_وتھ)) : خالی_سوچک;

            // If the LSB expression is needed multiple times, evaluate it into a temporary
            ثابت صحيح nCopies = !lBitp + !lIdxp + (mNeeded && !mIdxp) + (hNeeded && !hIdxp);
            إذا (nCopies > 1 && m_funcp && m_stmtp && !isImpure(m_stmtp)) {
                ++m_nTmps;  // Use fresh set of temporaries
                Ast_گرہ_اظہاریہ* ثابت lsbp = nodep->lsbp()->unlink_Fr_پٹھ();
                AstVar* ثابت tmpp = جوڑو_Local_Tmp(m_stmtp, "ExpandSel_Lsb", lsbp);
                nodep->lsbp(جديد AstVarRef{lfl, tmpp, VAccess::READ});
            }

            // Rebuild remaining expressions derived from LSB if needed, now using the temporary
            إذا (!lBitp) lBitp = new_Sel_ثنائی_ہندسہ_ثنائی_ہندسہ(nodep->lsbp());
            إذا (!lIdxp) lIdxp = new_لفظ_اشاریہ(nodep->lsbp());
            إذا (!mIdxp) mIdxp = mNeeded ? لفظ_Idx(m_Msb_وتھ) : خالی_سوچک;
            إذا (!hIdxp) hIdxp = hNeeded ? لفظ_Idx(h_Msb_وتھ) : خالی_سوچک;

            // Return word 'idxp' of 'fromp', without consuming 'idxp'
            ثابت خودکار لفظ_Sel_2 = [&](Ast_گرہ_اظہاریہ* idxp) -> Ast_گرہ_اظہاریہ* {
                Ast_گرہ_اظہاریہ* ثابت clonep = idxp->clone_درخت_خالص(خطأ);
                Ast_گرہ_اظہاریہ* ثابت wordp = new_لفظ_Sel_لفظ(ffl, fromp, clonep);
                إذا (!clonep->backp()) VL_DO_DANGLING(clonep->delete_درخت(), clonep);
                أرجع wordp;
            };

            // Construct term containing the bits of the low word - always needed
            Ast_گرہ_اظہاریہ* ثابت lTermp = [&]() -> Ast_گرہ_اظہاریہ* {
                Ast_گرہ_اظہاریہ* lWordp = لفظ_Sel_2(lIdxp);
                إذا (nodep->isQuad()) lWordp = جديد AstCCast{nfl, lWordp, nodep};
                أرجع جديد Ast_کھسکاؤ_R{nfl, lWordp, lBitp->clone_درخت_خالص(خطأ), nodep->چوڑائی()};
            }();

            // Construct term containing the bits of the middle word - if needed
            Ast_گرہ_اظہاریہ* ثابت mTermp = [&]() -> Ast_گرہ_اظہاریہ* {
                إذا (!mNeeded) أرجع خالی_سوچک;

                Ast_گرہ_اظہاریہ* mWordp = لفظ_Sel_2(mIdxp);
                إذا (nodep->isQuad()) mWordp = جديد AstCCast{nfl, mWordp, nodep};
                Ast_گرہ_اظہاریہ* ثابت mShiftp = جديد AstSub{lfl, جديد AstConst{lfl, VL_EDATASIZE},
                                                        lBitp->clone_درخت_خالص(خطأ)};
                // If we're selecting bit zero, then all 32 bits in the mid word
                // get shifted << by 32 bits, so ignore them.
                ثابت V3_نمبر zero{nodep, long_Or_Quad_چوڑائی(nodep)};
                أرجع جديد AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    جديد AstEq{nfl, جديد AstConst{nfl, 0}, lBitp->clone_درخت_خالص(خطأ)},
                    // 0 :
                    جديد AstConst{nfl, zero},
                    //  midword >> (VL_EDATASIZE - (lbs % VL_EDATASIZE))
                    جديد Ast_کھسکاؤ_L{nfl, mWordp, mShiftp, nodep->چوڑائی()}};
            }();

            // Construct term containing the bits of the high word - if needed
            Ast_گرہ_اظہاریہ* ثابت hTermp = [&]() -> Ast_گرہ_اظہاریہ* {
                إذا (!hNeeded) أرجع خالی_سوچک;

                Ast_گرہ_اظہاریہ* hWordp = لفظ_Sel_2(hIdxp);
                hWordp = جديد AstCCast{nfl, hWordp, nodep};
                Ast_گرہ_اظہاریہ* ثابت hShiftp = جديد AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    جديد AstEq{nfl, جديد AstConst{nfl, 0}, lBitp->clone_درخت_خالص(خطأ)},
                    // VL_EDATASIZE :
                    جديد AstConst{lfl, VL_EDATASIZE},
                    // 64 - (lbs % VL_EDATASIZE)
                    جديد AstSub{lfl, جديد AstConst{lfl, 64}, lBitp->clone_درخت_خالص(خطأ)}};
                أرجع جديد Ast_کھسکاؤ_L{nfl, hWordp, hShiftp, nodep->چوڑائی()};
            }();

            // Delete parts not captured during construction
            VL_DO_DANGLING(lBitp->delete_درخت(), lBitp);
            VL_DO_DANGLING(lIdxp->delete_درخت(), lIdxp);
            إذا (mIdxp) VL_DO_DANGLING(mIdxp->delete_درخت(), mIdxp);
            إذا (hIdxp) VL_DO_DANGLING(hIdxp->delete_درخت(), hIdxp);

            // Or reduce the terms
            Ast_گرہ_اظہاریہ* newp = lTermp;
            إذا (mTermp) newp = جديد AstOr{nfl, mTermp, newp};
            إذا (hTermp) newp = جديد AstOr{nfl, hTermp, newp};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } وإلا {  // Long/Quad from Long/Quad
            // No isImpure() check - can handle side effects in below
            did_نمونہ(VExpand_نمونہ::SEL_FROM_NARROW, nodep);
            File_سطر* ثابت fl = nodep->fileline();
            Ast_گرہ_اظہاریہ* fromp = nodep->fromp()->unlink_Fr_پٹھ();
            Ast_گرہ_اظہاریہ* ثابت lsbp = nodep->lsbp()->unlink_Fr_پٹھ();
            إذا (nodep->isQuad() && !fromp->isQuad()) fromp = جديد AstCCast{fl, fromp, nodep};
            // {large}>>32 requires 64-bit shift operation; then cast
            Ast_گرہ_اظہاریہ* newp = جديد Ast_کھسکاؤ_R{fl, fromp, lsbp, fromp->چوڑائی()};
            newp->dtypeFrom(fromp);
            إذا (!nodep->isQuad() && fromp->isQuad()) newp = جديد AstCCast{fl, newp, nodep};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }

    منطقي expandWide(Ast_گرہ_سونپو* nodep, AstSel* rhsp) {
        UASSERT_OBJ(nodep->چوڑائی_Min() == rhsp->چوڑائی_Const(), nodep, "Width mismatch");
        إذا (!doExpandWide(nodep)) أرجع خطأ;

        // Simplify the index, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->lsbp());

        // If it's a constant select and aligned, we can just copy the words
        إذا (ثابت AstConst* ثابت lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            ثابت uint32_t lsb = lsbConstp->منزل_UInt();
            إذا (VL_BITBIT_E(lsb) == 0) {
                did_نمونہ(VExpand_نمونہ::سونپو_SEL_ALIGNED, nodep);
                ثابت uint32_t لفظ = VL_BITWORD_E(lsb);
                لكل (صحيح w = 0; w < nodep->چوڑائی_الفاظ(); ++w) {
                    جوڑو_لفظ_سونپو(nodep, w, new_Ast_لفظ_Sel_Clone(rhsp->fromp(), w + لفظ));
                }
                أرجع سچ;
            }
        }

        did_نمونہ(VExpand_نمونہ::سونپو_SEL_UNALIGNED, nodep);
        File_سطر* ثابت flp = rhsp->fileline();

        // Use fresh set of temporaries
        ++m_nTmps;

        // Compute word index of LSB, store to temporary if not constant
        Ast_گرہ_اظہاریہ* ثابت لفظ_Lsbp = rhsp->lsbp()->clone_درخت_خالص(خطأ);
        Ast_گرہ_اظہاریہ* لفظ_Idxp = new_لفظ_اشاریہ(لفظ_Lsbp);
        إذا (!لفظ_Lsbp->backp()) VL_DO_DANGLING(لفظ_Lsbp->delete_درخت(), لفظ_Lsbp);
        لفظ_Idxp = V3Const::constifyEditCpp(لفظ_Idxp);
        إذا (!VN_IS(لفظ_Idxp, Const)) {
            AstVar* ثابت tmpp = جوڑو_Local_Tmp(nodep, "ExpandSel_WordIdx", لفظ_Idxp);
            لفظ_Idxp = جديد AstVarRef{flp, tmpp, VAccess::READ};
        }

        // Compute shift amounts and mask, store to temporaries if not constants
        Ast_گرہ_اظہاریہ* loShftp = خالی_سوچک;
        Ast_گرہ_اظہاریہ* hiShftp = خالی_سوچک;
        Ast_گرہ_اظہاریہ* hiMaskp = خالی_سوچک;
        إذا (ثابت AstConst* ثابت lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            ثابت uint32_t ثنائی_ہندسہ_وتھ = VL_BITBIT_E(lsbConstp->منزل_UInt());
            // Must be unaligned, otherwise we would have handled it above
            UASSERT_OBJ(ثنائی_ہندسہ_وتھ, nodep, "Missed aligned wide select");
            loShftp = جديد AstConst{flp, ثنائی_ہندسہ_وتھ};
            hiShftp = جديد AstConst{flp, VL_EDATASIZE - ثنائی_ہندسہ_وتھ};
            hiMaskp = خالی_سوچک;
        } وإلا {
            // Compute Low word shift amount: bottom bits of lsb index
            Ast_گرہ_اظہاریہ* ثابت lsbp = rhsp->lsbp()->clone_درخت_خالص(خطأ);
            loShftp = جديد AstAnd{flp, جديد AstConst{flp, VL_SIZEBITS_E}, lsbp};
            AstVar* ثابت loTmpp = جوڑو_Local_Tmp(nodep, "ExpandSel_LoShift", loShftp);
            loShftp = جديد AstVarRef{flp, loTmpp, VAccess::READ};

            // Compute if aligned: Low word shift amount is 0
            Ast_گرہ_اظہاریہ* ثابت zerop = جديد AstConst{flp, 0};
            Ast_گرہ_اظہاریہ* alignedp = جديد AstEq{flp, zerop, loShftp->clone_درخت_خالص(خطأ)};
            AstVar* ثابت alignedTmpp = جوڑو_Local_Tmp(nodep, "ExpandSel_Aligned", alignedp);
            alignedp = جديد AstVarRef{flp, alignedTmpp, VAccess::READ};

            // Computed High word shift amount: 0 if aligned else VL_EDATASIZE - Low word shift
            Ast_گرہ_اظہاریہ* ثابت edsp = جديد AstConst{flp, VL_EDATASIZE};
            Ast_گرہ_اظہاریہ* ثابت subp = جديد AstSub{flp, edsp, loShftp->clone_درخت_خالص(خطأ)};
            hiShftp = جديد AstCond{flp, alignedp, جديد AstConst{flp, 0}, subp};
            AstVar* ثابت hiTmpp = جوڑو_Local_Tmp(nodep, "ExpandSel_HiShift", hiShftp);
            hiShftp = جديد AstVarRef{flp, hiTmpp, VAccess::READ};

            // Compute the High word mask: 0 if aligned, ones otherwise
            hiMaskp = جديد AstCond{flp, alignedp->clone_درخت_خالص(خطأ),
                                  جديد AstConst{flp, AstConst::SizedEData{}, 0},
                                  جديد AstConst{flp, AstConst::SizedEData{}, -1ULL}};
            AstVar* ثابت نقاب_Tmpp = جوڑو_Local_Tmp(nodep, "ExpandSel_HiMask", hiMaskp);
            hiMaskp = جديد AstVarRef{flp, نقاب_Tmpp, VAccess::READ};
        }

        // Create each word of the selected result
        Ast_گرہ_اظہاریہ* ثابت fromp = rhsp->fromp();
        ثابت صحيح sel_الفاظ = nodep->چوڑائی_الفاظ();
        لكل (صحيح w = 0; w < sel_الفاظ; ++w) {
            // Grab bits from word 'VL_BITWORD_E(lsb) + w'
            Ast_گرہ_اظہاریہ* ثابت loWordp = new_لفظ_Sel_لفظ(fromp->fileline(), fromp, لفظ_Idxp, w);
            Ast_گرہ_اظہاریہ* ثابت loShftClonep = loShftp->clone_درخت_خالص(خطأ);
            Ast_گرہ_اظہاریہ* ثابت lop = جديد Ast_کھسکاؤ_R{flp, loWordp, loShftClonep, VL_EDATASIZE};
            // Grab bits from word 'VL_BITWORD_E(lsb) + w + 1'
            Ast_گرہ_اظہاریہ* hiWordp = new_لفظ_Sel_لفظ(fromp->fileline(), fromp, لفظ_Idxp, w + 1);
            // For the last word of the result, avoid an OOB access when the Sel is not OOB
            إذا (w == sel_الفاظ - 1) {
                ثابت uint32_t max = fromp->چوڑائی_الفاظ() - w - 1;
                Ast_گرہ_اظہاریہ* ثابت maxp = جديد AstConst{flp, max};
                Ast_گرہ_اظہاریہ* ثابت condp = جديد AstGte{flp, لفظ_Idxp->clone_درخت_خالص(خطأ), maxp};
                Ast_گرہ_اظہاریہ* ثابت zerop = جديد AstConst{flp, AstConst::SizedEData{}, 0};
                hiWordp = جديد AstCond{flp, condp, zerop, hiWordp};
            }
            Ast_گرہ_اظہاریہ* ثابت hiShftClonep = hiShftp->clone_درخت_خالص(خطأ);
            Ast_گرہ_اظہاریہ* ثابت hiPartp = جديد Ast_کھسکاؤ_L{flp, hiWordp, hiShftClonep, VL_EDATASIZE};
            Ast_گرہ_اظہاریہ* ثابت hip
                = hiMaskp ? جديد AstAnd{flp, hiPartp, hiMaskp->clone_درخت_خالص(خطأ)} : hiPartp;
            // Combine them
            جوڑو_لفظ_سونپو(nodep, w, جديد AstOr{nodep->fileline(), hip, lop});
        }

        // Delete parts not captured during construction
        إذا (!لفظ_Idxp->backp()) VL_DO_DANGLING(لفظ_Idxp->delete_درخت(), لفظ_Idxp);
        إذا (!loShftp->backp()) VL_DO_DANGLING(loShftp->delete_درخت(), loShftp);
        إذا (!hiShftp->backp()) VL_DO_DANGLING(hiShftp->delete_درخت(), hiShftp);
        إذا (hiMaskp && !hiMaskp->backp()) VL_DO_DANGLING(hiMaskp->delete_درخت(), hiMaskp);

        أرجع سچ;
    }

    منطقي expandLhs(Ast_گرہ_سونپو* nodep, AstSel* lhsp) {
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
            Ast_گرہ_اظہاریہ* rhsp = nodep->rhsp()->unlink_Fr_پٹھ();
            Ast_گرہ_اظہاریہ* ثابت destp = lhsp->fromp()->unlink_Fr_پٹھ();
            ثابت صحيح lsb = lhsp->lsbConst();
            ثابت صحيح msb = lhsp->msbConst();
            V3_نمبر maskset{nodep, destp->چوڑائی_Min()};
            maskset.سیٹ_نقاب(msb + 1 - lsb, lsb);
            V3_نمبر maskold{nodep, destp->چوڑائی_Min()};
            maskold.opNot(maskset);
            إذا (destwide) {
                did_نمونہ(VExpand_نمونہ::سونپو_منزل_SEL_CONST_WIDE, nodep);
                لكل (صحيح w = 0; w < destp->چوڑائی_الفاظ(); ++w) {
                    إذا (w >= VL_BITWORD_E(lsb) && w <= VL_BITWORD_E(msb)) {
                        // else we would just be setting it to the same exact value
                        Ast_گرہ_اظہاریہ* oldvalp = new_Ast_لفظ_Sel_Clone(destp, w);
                        fixCloneLvalue(oldvalp);
                        إذا (!ones) {
                            oldvalp = جديد AstAnd{
                                lfl,
                                جديد AstConst{lfl, AstConst::SizedEData{}, maskold.edata_لفظ(w)},
                                oldvalp};
                        }

                        // Appropriate word of new value to insert:
                        Ast_گرہ_اظہاریہ* newp = new_لفظ_Grab_کھسکاؤ(lfl, w, rhsp, lsb);

                        // Apply cleaning at the top word of the destination
                        // (no cleaning to do if dst's width is a whole number
                        // of words).
                        إذا (w == destp->چوڑائی_الفاظ() - 1 && VL_BITBIT_E(destp->چوڑائی_Min()) != 0) {
                            V3_نمبر cleanmask{nodep, VL_EDATASIZE};
                            cleanmask.سیٹ_نقاب(VL_BITBIT_E(destp->چوڑائی_Min()));
                            newp = جديد AstAnd{lfl, newp, جديد AstConst{lfl, cleanmask}};
                        }
                        Ast_گرہ_اظہاریہ* ثابت orp
                            = V3Const::constifyEditCpp(جديد AstOr{lfl, oldvalp, newp});
                        جوڑو_لفظ_سونپو(nodep, w, destp, orp);
                    }
                }
                VL_DO_DANGLING(rhsp->delete_درخت(), rhsp);
                VL_DO_DANGLING(destp->delete_درخت(), destp);
            } وإلا {
                did_نمونہ(VExpand_نمونہ::سونپو_منزل_SEL_CONST_NARROW, nodep);
                إذا (destp->isQuad() && !rhsp->isQuad()) rhsp = جديد AstCCast{nfl, rhsp, nodep};
                Ast_گرہ_اظہاریہ* oldvalp = destp->clone_درخت_خالص(سچ);
                fixCloneLvalue(oldvalp);
                إذا (!ones) oldvalp = جديد AstAnd{lfl, جديد AstConst{lfl, maskold}, oldvalp};

                // The bit-select can refer to bits outside the width of nodep
                // which we aren't allowed to assign to.  This is a mask of the
                // valid range of nodep which we apply to the new shifted RHS.
                V3_نمبر cleanmask{nodep, destp->چوڑائی_Min()};
                cleanmask.سیٹ_نقاب(destp->چوڑائی_Min());
                Ast_گرہ_اظہاریہ* ثابت shifted = جديد Ast_کھسکاؤ_L{
                    lfl, rhsp, جديد AstConst{lfl, static_cast<uint32_t>(lsb)}, destp->چوڑائی()};
                Ast_گرہ_اظہاریہ* ثابت cleaned
                    = جديد AstAnd{lfl, shifted, جديد AstConst{lfl, cleanmask}};
                Ast_گرہ_اظہاریہ* ثابت orp
                    = V3Const::constifyEditCpp(جديد AstOr{lfl, oldvalp, cleaned});
                داخل_کرو_Before(nodep, جديد Ast_سونپو{nfl, destp, orp});
            }
            أرجع سچ;
        } وإلا {  // non-const select offset
            إذا (destwide && lhsp->چوڑائی_Const() == 1) {
                did_نمونہ(VExpand_نمونہ::سونپو_منزل_SEL_VAR_WIDE_ONE_ثنائی_ہندسہ, nodep);
                Ast_گرہ_اظہاریہ* ثابت rhsp = nodep->rhsp()->unlink_Fr_پٹھ();
                Ast_گرہ_اظہاریہ* ثابت destp = lhsp->fromp()->unlink_Fr_پٹھ();
                Ast_گرہ_اظہاریہ* oldvalp
                    = new_لفظ_Sel_ثنائی_ہندسہ(lfl, destp->clone_درخت_خالص(خطأ), lhsp->lsbp());
                fixCloneLvalue(oldvalp);
                إذا (!ones) {
                    oldvalp = جديد AstAnd{
                        lfl,
                        جديد AstNot{
                            lfl, جديد Ast_کھسکاؤ_L{lfl, جديد AstConst{nfl, 1},
                                               // newSelBitBit may exceed the MSB of this variable.
                                               // That's ok as we'd just AND with a larger value,
                                               // but oldval would clip the upper bits to sanity
                                               new_Sel_ثنائی_ہندسہ_ثنائی_ہندسہ(lhsp->lsbp()), VL_EDATASIZE}},
                        oldvalp};
                }
                // Restrict the shift amount to 0-31, see bug804.
                Ast_گرہ_اظہاریہ* ثابت shiftp = جديد AstAnd{nfl, lhsp->lsbp()->clone_درخت_خالص(سچ),
                                                       جديد AstConst{nfl, VL_EDATASIZE - 1}};
                Ast_گرہ* ثابت newp = جديد Ast_سونپو{
                    nfl, new_لفظ_Sel_ثنائی_ہندسہ(nfl, destp, lhsp->lsbp()),
                    جديد AstOr{lfl, oldvalp, جديد Ast_کھسکاؤ_L{lfl, rhsp, shiftp, VL_EDATASIZE}}};
                داخل_کرو_Before(nodep, newp);
                أرجع سچ;
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
                did_نمونہ(VExpand_نمونہ::سونپو_منزل_SEL_VAR_NARROW, nodep);
                // nodep->dumpTree("-  old: ");
                Ast_گرہ_اظہاریہ* rhsp = nodep->rhsp()->unlink_Fr_پٹھ();
                Ast_گرہ_اظہاریہ* ثابت destp = lhsp->fromp()->unlink_Fr_پٹھ();
                Ast_گرہ_اظہاریہ* oldvalp = destp->clone_درخت_خالص(سچ);
                fixCloneLvalue(oldvalp);

                V3_نمبر maskwidth{nodep, destp->چوڑائی_Min()};
                maskwidth.سیٹ_نقاب(lhsp->چوڑائی_Const());

                إذا (destp->isQuad() && !rhsp->isQuad()) rhsp = جديد AstCCast{nfl, rhsp, nodep};
                إذا (!ones) {
                    oldvalp = جديد AstAnd{
                        lfl,
                        جديد AstNot{lfl, جديد Ast_کھسکاؤ_L{lfl, جديد AstConst{nfl, maskwidth},
                                                      lhsp->lsbp()->clone_درخت_خالص(سچ),
                                                      destp->چوڑائی()}},
                        oldvalp};
                }
                Ast_گرہ_اظہاریہ* newp
                    = جديد Ast_کھسکاؤ_L{lfl, rhsp, lhsp->lsbp()->clone_درخت_خالص(سچ), destp->چوڑائی()};
                // Apply cleaning to the new value being inserted.  Mask is
                // slightly wider than necessary to avoid an AND with all ones
                // being optimized out.  No need to clean if destp is
                // quad-sized as there are no extra bits to contaminate
                إذا (destp->چوڑائی_Min() != 64) {
                    V3_نمبر cleanmask{nodep, destp->چوڑائی_Min() + 1};
                    cleanmask.سیٹ_نقاب(destp->چوڑائی_Min());
                    newp = جديد AstAnd{lfl, newp, جديد AstConst{lfl, cleanmask}};
                }

                داخل_کرو_Before(nodep, جديد Ast_سونپو{nfl, destp, جديد AstOr{lfl, oldvalp, newp}});
                أرجع سچ;
            }
        }
    }

    فراغ ویکھو(AstConcat* nodep) override {
        إذا (nodep->user1_سیٹ_Once()) أرجع;  // Process once
        iterateChildren(nodep);
        إذا (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } وإلا {
            // No isImpure() check - can handle side effects in below
            did_نمونہ(VExpand_نمونہ::CONCAT, nodep);
            File_سطر* ثابت fl = nodep->fileline();
            Ast_گرہ_اظہاریہ* lhsp = nodep->lhsp()->unlink_Fr_پٹھ();
            Ast_گرہ_اظہاریہ* rhsp = nodep->rhsp()->unlink_Fr_پٹھ();
            ثابت uint32_t rhsshift = rhsp->چوڑائی_Min();
            إذا (nodep->isQuad() && !lhsp->isQuad()) lhsp = جديد AstCCast{fl, lhsp, nodep};
            إذا (nodep->isQuad() && !rhsp->isQuad()) rhsp = جديد AstCCast{fl, rhsp, nodep};
            Ast_گرہ_اظہاریہ* ثابت newp = جديد AstOr{
                fl, جديد Ast_کھسکاؤ_L{fl, lhsp, جديد AstConst{fl, rhsshift}, nodep->چوڑائی()}, rhsp};
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    منطقي expandWide(Ast_گرہ_سونپو* nodep, AstConcat* rhsp) {
        إذا (!doExpandWide(rhsp)) أرجع خطأ;
        did_نمونہ(VExpand_نمونہ::سونپو_CONCAT, nodep);
        File_سطر* ثابت fl = rhsp->fileline();
        // Lhs or Rhs may be word, long, or quad.
        // newAstWordSelClone nicely abstracts the difference.
        ثابت صحيح rhsshift = rhsp->rhsp()->چوڑائی_Min();
        // Sometimes doing the words backwards is preferable.
        // When we have x={x,foo} backwards is better, when x={foo,x} forward is better
        // However V3Subst tends to rip this up, so not worth optimizing now.
        لكل (صحيح w = 0; w < rhsp->چوڑائی_الفاظ(); ++w) {
            جوڑو_لفظ_سونپو(nodep, w,
                          جديد AstOr{fl, new_لفظ_Grab_کھسکاؤ(fl, w, rhsp->lhsp(), rhsshift),
                                    new_Ast_لفظ_Sel_Clone(rhsp->rhsp(), w)});
        }
        أرجع سچ;
    }

    فراغ ویکھو(AstReplicate* nodep) override {
        إذا (nodep->user1_سیٹ_Once()) أرجع;  // Process once
        iterateChildren(nodep);
        إذا (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } وإلا {
            إذا (isImpure(nodep)) أرجع;
            File_سطر* ثابت fl = nodep->fileline();
            Ast_گرہ_اظہاریہ* lhsp = nodep->srcp()->unlink_Fr_پٹھ();
            Ast_گرہ_اظہاریہ* newp;
            ثابت صحيح lhswidth = lhsp->چوڑائی_Min();
            إذا (lhswidth == 1) {
                did_نمونہ(VExpand_نمونہ::REPLICATE_ONE_ثنائی_ہندسہ, nodep);
                newp = جديد AstNegate{fl, lhsp};
            } وإلا {
                did_نمونہ(VExpand_نمونہ::REPLICATE, nodep);
                ثابت AstConst* ثابت constp = VN_AS(nodep->countp(), Const);
                UASSERT_OBJ(constp, nodep,
                            "Replication value isn't a constant.  Checked earlier!");
                ثابت uint32_t times = constp->منزل_UInt();
                إذا (nodep->isQuad() && !lhsp->isQuad()) lhsp = جديد AstCCast{fl, lhsp, nodep};
                newp = lhsp->clone_درخت_خالص(سچ);
                لكل (بنانشانی repnum = 1; repnum < times; repnum++) {
                    ثابت صحيح rhsshift = repnum * lhswidth;
                    newp = جديد AstOr{
                        fl,
                        جديد Ast_کھسکاؤ_L{fl, lhsp->clone_درخت_خالص(سچ),
                                      جديد AstConst{fl, static_cast<uint32_t>(rhsshift)},
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
    منطقي expandWide(Ast_گرہ_سونپو* nodep, AstReplicate* rhsp) {
        إذا (!doExpandWide(rhsp)) أرجع خطأ;
        did_نمونہ(VExpand_نمونہ::سونپو_REPLICATE, nodep);
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گرہ_اظہاریہ* ثابت lhsp = rhsp->srcp();
        ثابت صحيح lhswidth = lhsp->چوڑائی_Min();
        ثابت AstConst* ثابت constp = VN_AS(rhsp->countp(), Const);
        UASSERT_OBJ(constp, rhsp, "Replication value isn't a constant.  Checked earlier!");
        ثابت uint32_t times = constp->منزل_UInt();
        لكل (صحيح w = 0; w < rhsp->چوڑائی_الفاظ(); ++w) {
            Ast_گرہ_اظہاریہ* newp;
            إذا (lhswidth == 1) {
                newp = جديد AstNegate{fl, lhsp->clone_درخت_خالص(سچ)};
                // Replicate always unsigned
                newp->dtype_Chg_چوڑائی_Signed(VL_EDATASIZE, VL_EDATASIZE, VSigning::UNSIGNED);
            } وإلا {
                newp = new_Ast_لفظ_Sel_Clone(lhsp, w);
                File_سطر* ثابت rfl = rhsp->fileline();
                لكل (بنانشانی repnum = 1; repnum < times; repnum++) {
                    newp = جديد AstOr{fl, new_لفظ_Grab_کھسکاؤ(rfl, w, lhsp, lhswidth * repnum), newp};
                }
            }
            جوڑو_لفظ_سونپو(nodep, w, newp);
        }
        أرجع سچ;
    }

    فراغ ویکھو_Eq_Neq(Ast_گرہ_Biop* nodep) {
        إذا (nodep->user1_سیٹ_Once()) أرجع;  // Process once
        iterateChildren(nodep);
        إذا (nodep->lhsp()->isWide()
            && !(VN_IS(nodep->lhsp()->dtypep()->skipRefp(), دھارا_DType)
                 || VN_IS(nodep->rhsp()->dtypep()->skipRefp(), دھارا_DType))) {
            إذا (isImpure(nodep)) أرجع;
            إذا (!doExpandWide(nodep->lhsp())) أرجع;
            إذا (!doExpandWide(nodep->rhsp())) أرجع;
            did_نمونہ(VN_IS(nodep, Neq) ? VExpand_نمونہ::WORDIZE_NEQ
                                         : VExpand_نمونہ::WORDIZE_EQ,
                       nodep);
            // -> (0=={or{for each_word{WORDSEL(lhs,#)^WORDSEL(rhs,#)}}}
            File_سطر* ثابت fl = nodep->fileline();
            Ast_گرہ_اظہاریہ* newp = خالی_سوچک;
            لكل (صحيح w = 0; w < nodep->lhsp()->چوڑائی_الفاظ(); ++w) {
                Ast_گرہ_اظہاریہ* ثابت eqp = جديد AstXor{fl, new_Ast_لفظ_Sel_Clone(nodep->lhsp(), w),
                                                    new_Ast_لفظ_Sel_Clone(nodep->rhsp(), w)};
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
    فراغ ویکھو(AstEq* nodep) override { ویکھو_Eq_Neq(nodep); }
    فراغ ویکھو(AstNeq* nodep) override { ویکھو_Eq_Neq(nodep); }

    فراغ ویکھو(AstRedOr* nodep) override {
        إذا (nodep->user1_سیٹ_Once()) أرجع;  // Process once
        iterateChildren(nodep);
        File_سطر* ثابت fl = nodep->fileline();
        إذا (nodep->lhsp()->isWide()) {
            إذا (isImpure(nodep)) أرجع;
            إذا (!doExpandWide(nodep->lhsp())) أرجع;
            did_نمونہ(VExpand_نمونہ::WORDIZE_REDOR, nodep);
            // -> (0!={or{for each_word{WORDSEL(lhs,#)}}}
            Ast_گرہ_اظہاریہ* newp = خالی_سوچک;
            لكل (صحيح w = 0; w < nodep->lhsp()->چوڑائی_الفاظ(); ++w) {
                Ast_گرہ_اظہاریہ* ثابت eqp = new_Ast_لفظ_Sel_Clone(nodep->lhsp(), w);
                newp = newp ? جديد AstOr{fl, newp, eqp} : eqp;
            }
            newp = جديد AstNeq{fl, جديد AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } وإلا {
            // No isImpure() check - can handle side effects in below
            did_نمونہ(VExpand_نمونہ::REDOR_منزل_NEQ, nodep);
            Ast_گرہ_اظہاریہ* ثابت lhsp = nodep->lhsp()->unlink_Fr_پٹھ();
            Ast_گرہ_اظہاریہ* ثابت newp = جديد AstNeq{
                fl, جديد AstConst{fl, AstConst::Widthed_قدر{}, long_Or_Quad_چوڑائی(nodep), 0}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    فراغ ویکھو(AstRedAnd* nodep) override {
        إذا (nodep->user1_سیٹ_Once()) أرجع;  // Process once
        iterateChildren(nodep);
        File_سطر* ثابت fl = nodep->fileline();
        إذا (nodep->lhsp()->isWide()) {
            إذا (isImpure(nodep)) أرجع;
            إذا (!doExpandWide(nodep->lhsp())) أرجع;
            did_نمونہ(VExpand_نمونہ::WORDIZE_REDAND, nodep);
            // -> (0!={and{for each_word{WORDSEL(lhs,#)}}}
            Ast_گرہ_اظہاریہ* newp = خالی_سوچک;
            لكل (صحيح w = 0; w < nodep->lhsp()->چوڑائی_الفاظ(); ++w) {
                Ast_گرہ_اظہاریہ* eqp = new_Ast_لفظ_Sel_Clone(nodep->lhsp(), w);
                إذا (w == nodep->lhsp()->چوڑائی_الفاظ() - 1) {
                    // Rather than doing a (slowish) ==##, we OR in the
                    // bits that aren't part of the mask
                    eqp = جديد AstOr{fl, جديد AstConst{fl, not_Wide_نقاب(nodep->lhsp())},
                                    // Bug in cppcheck
                                    // cppcheck-suppress memleak
                                    eqp};
                }
                newp = newp ? جديد AstAnd{fl, newp, eqp} : eqp;
            }
            newp = جديد AstEq{fl, جديد AstConst{fl, AstConst::SizedEData{}, VL_نقاب_E(VL_EDATASIZE)},
                             newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } وإلا {
            // No isImpure() check - can handle side effects in below
            did_نمونہ(VExpand_نمونہ::REDAND_منزل_EQ, nodep);
            Ast_گرہ_اظہاریہ* ثابت lhsp = nodep->lhsp()->unlink_Fr_پٹھ();
            Ast_گرہ_اظہاریہ* ثابت newp = جديد AstEq{fl, جديد AstConst{fl, لفظ_نقاب(lhsp)}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    فراغ ویکھو(AstRedXor* nodep) override {
        إذا (nodep->user1_سیٹ_Once()) أرجع;  // Process once
        iterateChildren(nodep);
        إذا (nodep->lhsp()->isWide()) {
            إذا (isImpure(nodep)) أرجع;
            إذا (!doExpandWide(nodep->lhsp())) أرجع;
            did_نمونہ(VExpand_نمونہ::WORDIZE_REDXOR, nodep);
            // -> (0!={redxor{for each_word{XOR(WORDSEL(lhs,#))}}}
            File_سطر* ثابت fl = nodep->fileline();
            Ast_گرہ_اظہاریہ* newp = خالی_سوچک;
            لكل (صحيح w = 0; w < nodep->lhsp()->چوڑائی_الفاظ(); ++w) {
                Ast_گرہ_اظہاریہ* ثابت eqp = new_Ast_لفظ_Sel_Clone(nodep->lhsp(), w);
                newp = newp ? جديد AstXor{fl, newp, eqp} : eqp;
            }
            newp = جديد AstRedXor{fl, newp};
            UINFO(8, "    Wordize REDXORnew " << newp);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
        // We don't reduce non-wide XORs, as its more efficient to use a temp register,
        // which the inlined function does nicely.
    }

    فراغ ویکھو(Ast_گرہ_Stmt* nodep) override {
        إذا (nodep->user1_سیٹ_Once()) أرجع;  // Process once
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    فراغ ویکھو(Ast_گرہ_سونپو* nodep) override {
        إذا (nodep->user1_سیٹ_Once()) أرجع;  // Process once
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
            } وإلا إذا (Ast_کھسکاؤ_L* ثابت rhsp = VN_CAST(nodep->rhsp(), کھسکاؤ_L)) {
                did = expand_Wide_کھسکاؤ(nodep, rhsp, /* isLeftShift: */ سچ);
            } وإلا إذا (Ast_کھسکاؤ_R* ثابت rhsp = VN_CAST(nodep->rhsp(), کھسکاؤ_R)) {
                did = expand_Wide_کھسکاؤ(nodep, rhsp, /* isLeftShift: */ خطأ);
            } وإلا إذا (AstCond* ثابت rhsp = VN_CAST(nodep->rhsp(), Cond)) {
                did = expandWide(nodep, rhsp);
            }
        } وإلا إذا (AstSel* ثابت lhsp = VN_CAST(nodep->lhsp(), Sel)) {
            did = expandLhs(nodep, lhsp);
        }
        // Cleanup common code
        إذا (did) VL_DO_DANGLING(nodep->unlink_Fr_پٹھ()->delete_درخت(), nodep);
    }

    //--------------------
    فراغ ویکھو(AstVar*) override {}  // Don't hit varrefs under vars
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح ExpandVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ExpandVisitor() override {
        V3Stats::جوڑو_Stat("Optimizations, Expand, expanded wides", m_statWides);
        V3Stats::جوڑو_Stat("Optimizations, Expand, expanded wide words", m_stat_Wide_الفاظ);
        V3Stats::جوڑو_Stat("Optimizations, Expand, not expanded impure", m_statImpure);
        V3Stats::جوڑو_Stat("Optimizations, Expand, not expanded limit", m_statWideLimited);
        V3Stats::جوڑو_Stat("Optimizations, Expand, temporaries created", m_statTemps);
        ثابت خودکار emitStat = [هذا](VExpand_نمونہ نمونہ_2) {
            std::string str{نمونہ_2.ascii()};
            std::transform(str.begin(), str.سرا(), str.begin(), [](بنانشانی حرف c) {  //
                أرجع c == '_' ? ' ' : std::tolower(c);
            });
            V3Stats::جوڑو_Stat("Optimizations, Expand, pattern " + str, m_statPatterns[نمونہ_2]);
        };
#تعریف_کرو EXPAND_نمونہ_EMIT_STAT(id) emitStat(VExpand_نمونہ::id);
        FOR_EACH_EXPAND_نمونہ(EXPAND_نمونہ_EMIT_STAT)
#تعریف_رد EXPAND_نمونہ_EMIT_STAT
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
