// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Resolve module/signal name references
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
// LinkResolve TRANSFORMATIONS:
//      Top-down traversal
//          With vars: Fixup LambdaRefs
//*************************************************************************

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3LinkWith.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

رده LinkWithVisitor final : عمومی VNVisitor {
    // NODE STATE

    // STATE
    // Below state needs to be preserved between each module call.
    ثابت نویسه* m_randcIllegalWhyp = nullptr;  // Why randc illegal
    Ast_گره* m_randcIllegalp = nullptr;  // Node causing randc illegal
    Ast_گره_عبارت* m_currentRandomizeSelectp = nullptr;  // fromp() of current `randomize()` call
    منطقی m_inRandomizeWith = نادرست;  // If in randomize() with (and no other with afterwards)

    // VISITORS
    تهی visit(AstConstraint* nodep) override {
        // V3LinkDot moved the isExternDef into the class, the extern proto was
        // checked to exist, and now isn't needed
        nodep->isExternDef(نادرست);
        اگر (nodep->isExternProto()) {
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
            بازگشت;
        }
        iterateChildren(nodep);
    }
    تهی visit(AstConstraintBefore* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        m_randcIllegalWhyp = "'solve before' (IEEE 1800-2023 18.5.9)";
        m_randcIllegalp = nodep;
        iterateChildrenConst(nodep);
    }
    تهی visit(AstDist* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        m_randcIllegalWhyp = "'constraint dist' (IEEE 1800-2023 18.5.3)";
        m_randcIllegalp = nodep;
        iterateChildrenConst(nodep);
    }
    تهی visit(Ast_Constraint_عبارت* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        اگر (nodep->isSoft()) {
            m_randcIllegalWhyp = "'constraint soft' (IEEE 1800-2023 18.5.13.1)";
            m_randcIllegalp = nodep;
        }
        iterateChildrenConst(nodep);
    }

    تهی visit(Ast_گره_Var_Ref* nodep) override {
        اگر (nodep->varp()) {  // Else due to dead code, might not have var pointer
            اگر (nodep->varp()->isRandC() && m_randcIllegalp) {
                nodep->v3error("Randc variables not allowed in "
                               << m_randcIllegalWhyp << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << m_randcIllegalp->warnOther()
                               << "... Location of restricting expression\n"
                               << m_randcIllegalp->warnContextSecondary());
            }
        }
        iterateChildren(nodep);
    }

    تهی visit(Ast_گره_FTask_Ref* nodep) override {
        VL_RESTORER(m_currentRandomizeSelectp);
        اگر (nodep->taskp()) {
            اگر (AstSequence* ثابت seqp = VN_CAST(nodep->taskp(), Sequence))
                seqp->isReferenced(درست);
        }

        اگر (nodep->name() == "randomize") {
            اگر (ثابت AstMethodCall* ثابت methodcallp = VN_CAST(nodep, MethodCall)) {
                اگر (m_inRandomizeWith) {
                    nodep->v3warn(
                        E_UNSUPPORTED,
                        "Unsupported: randomize() nested in inline randomize() constraints");
                }
                m_currentRandomizeSelectp = methodcallp->fromp();
            }
        }
        iterateChildren(nodep);
    }

    تهی visit(AstMemberSel* nodep) override {
        اگر (m_inRandomizeWith && nodep->fromp()->same_درخت(m_currentRandomizeSelectp)) {
            // Replace member selects to the element
            // on which the randomize() is called with LambdaArgRef
            // This allows V3Randomize to work properly when
            // constrained variables are referred using that object
            Ast_گره_عبارت* ثابت prevFromp = nodep->fromp();
            Ast_گره_عبارت* ثابت newp
                = جدید AstLambdaArgRef{prevFromp->fileline(), prevFromp->name(), نادرست};
            prevFromp->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(prevFromp), prevFromp);
        }
        iterateChildren(nodep);
    }

    تهی visit(AstWith* nodep) override {
        VL_RESTORER(m_inRandomizeWith);
        اگر (ثابت AstMethodCall* ثابت methodCallp = VN_CAST(nodep->backp(), MethodCall)) {
            m_inRandomizeWith = methodCallp->name() == "randomize";
        } وگرنه {
            m_inRandomizeWith = نادرست;
        }
        iterateChildren(nodep);
    }

    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit LinkWithVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkWithVisitor() override = پیشفرض;
};

//######################################################################
// V3LinkWith class functions

تهی V3LinkWith::linkWith(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { ثابت LinkWithVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("linkwith", 0, dump_درخت_Either_Level() >= 6);
}
