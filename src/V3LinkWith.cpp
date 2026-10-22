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

جماعت LinkWithVisitor final : عوامی VNVisitor {
    // NODE STATE

    // STATE
    // Below state needs to be preserved between each module call.
    ثابت حرف* m_randcIllegalWhyp = خالی_اشاریہ;  // Why randc illegal
    Ast_گرہ* m_randcIllegalp = خالی_اشاریہ;  // Node causing randc illegal
    Ast_گرہ_اظہاریہ* m_currentRandomizeSelectp = خالی_اشاریہ;  // fromp() of current `randomize()` call
    منطقی m_اندر_Randomize_With = جھوٹ;  // If in randomize() with (and no other with afterwards)

    // VISITORS
    خالی دورہ(AstConstraint* nodep) override {
        // V3LinkDot moved the isExternDef into the class, the extern proto was
        // checked to exist, and now isn't needed
        nodep->isExternDef(جھوٹ);
        اگر (nodep->isExternProto()) {
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
            واپس;
        }
        iterateChildren(nodep);
    }
    خالی دورہ(AstConstraintBefore* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        m_randcIllegalWhyp = "'solve before' (IEEE 1800-2023 18.5.9)";
        m_randcIllegalp = nodep;
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstDist* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        m_randcIllegalWhyp = "'constraint dist' (IEEE 1800-2023 18.5.3)";
        m_randcIllegalp = nodep;
        iterateChildrenConst(nodep);
    }
    خالی دورہ(Ast_Constraint_اظہاریہ* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        اگر (nodep->isSoft()) {
            m_randcIllegalWhyp = "'constraint soft' (IEEE 1800-2023 18.5.13.1)";
            m_randcIllegalp = nodep;
        }
        iterateChildrenConst(nodep);
    }

    خالی دورہ(Ast_گرہ_Var_Ref* nodep) override {
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

    خالی دورہ(Ast_گرہ_FTask_Ref* nodep) override {
        VL_RESTORER(m_currentRandomizeSelectp);
        اگر (nodep->taskp()) {
            اگر (AstSequence* ثابت seqp = VN_CAST(nodep->taskp(), Sequence))
                seqp->isReferenced(سچ);
        }

        اگر (nodep->name() == "randomize") {
            اگر (ثابت Ast_طریقہ_Call* ثابت methodcallp = VN_CAST(nodep, طریقہ_Call)) {
                اگر (m_اندر_Randomize_With) {
                    nodep->v3warn(
                        E_UNSUPPORTED,
                        "Unsupported: randomize() nested in inline randomize() constraints");
                }
                m_currentRandomizeSelectp = methodcallp->fromp();
            }
        }
        iterateChildren(nodep);
    }

    خالی دورہ(AstMemberSel* nodep) override {
        اگر (m_اندر_Randomize_With && nodep->fromp()->same_درخت(m_currentRandomizeSelectp)) {
            // Replace member selects to the element
            // on which the randomize() is called with LambdaArgRef
            // This allows V3Randomize to work properly when
            // constrained variables are referred using that object
            Ast_گرہ_اظہاریہ* ثابت پچھلا_Fromp = nodep->fromp();
            Ast_گرہ_اظہاریہ* ثابت newp
                = نیا AstLambdaArgRef{پچھلا_Fromp->fileline(), پچھلا_Fromp->name(), جھوٹ};
            پچھلا_Fromp->replaceWith(newp);
            VL_DO_DANGLING(ڈالو_Deletep(پچھلا_Fromp), پچھلا_Fromp);
        }
        iterateChildren(nodep);
    }

    خالی دورہ(AstWith* nodep) override {
        VL_RESTORER(m_اندر_Randomize_With);
        اگر (ثابت Ast_طریقہ_Call* ثابت طریقہ_Callp = VN_CAST(nodep->backp(), طریقہ_Call)) {
            m_اندر_Randomize_With = طریقہ_Callp->name() == "randomize";
        } ورنہ {
            m_اندر_Randomize_With = جھوٹ;
        }
        iterateChildren(nodep);
    }

    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح LinkWithVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkWithVisitor() override = طےشدہ;
};

//######################################################################
// V3LinkWith class functions

خالی V3LinkWith::linkWith(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { ثابت LinkWithVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("linkwith", 0, dump_درخت_Either_Level() >= 6);
}
