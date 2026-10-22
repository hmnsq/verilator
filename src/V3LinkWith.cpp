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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3LinkWith.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

ವರ್ಗ LinkWithVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE

    // STATE
    // Below state needs to be preserved between each module call.
    ಧ್ರುವ ವರ್ಣ* m_randcIllegalWhyp = nullptr;  // Why randc illegal
    Ast_ಗಂಟು* m_randcIllegalp = nullptr;  // Node causing randc illegal
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_currentRandomizeSelectp = nullptr;  // fromp() of current `randomize()` call
    ತಾರ್ಕಿಕ m_inRandomizeWith = ಅಸತ್ಯ;  // If in randomize() with (and no other with afterwards)

    // VISITORS
    ಶೂನ್ಯ visit(AstConstraint* nodep) override {
        // V3LinkDot moved the isExternDef into the class, the extern proto was
        // checked to exist, and now isn't needed
        nodep->isExternDef(ಅಸತ್ಯ);
        ಯದಿ (nodep->isExternProto()) {
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstConstraintBefore* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        m_randcIllegalWhyp = "'solve before' (IEEE 1800-2023 18.5.9)";
        m_randcIllegalp = nodep;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(AstDist* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        m_randcIllegalWhyp = "'constraint dist' (IEEE 1800-2023 18.5.3)";
        m_randcIllegalp = nodep;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        ಯದಿ (nodep->isSoft()) {
            m_randcIllegalWhyp = "'constraint soft' (IEEE 1800-2023 18.5.13.1)";
            m_randcIllegalp = nodep;
        }
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Var_Ref* nodep) override {
        ಯದಿ (nodep->varp()) {  // Else due to dead code, might not have var pointer
            ಯದಿ (nodep->varp()->isRandC() && m_randcIllegalp) {
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

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        VL_RESTORER(m_currentRandomizeSelectp);
        ಯದಿ (nodep->taskp()) {
            ಯದಿ (AstSequence* ಧ್ರುವ seqp = VN_CAST(nodep->taskp(), Sequence))
                seqp->isReferenced(ಸತ್ಯ);
        }

        ಯದಿ (nodep->name() == "randomize") {
            ಯದಿ (ಧ್ರುವ AstMethodCall* ಧ್ರುವ methodcallp = VN_CAST(nodep, MethodCall)) {
                ಯದಿ (m_inRandomizeWith) {
                    nodep->v3warn(
                        E_UNSUPPORTED,
                        "Unsupported: randomize() nested in inline randomize() constraints");
                }
                m_currentRandomizeSelectp = methodcallp->fromp();
            }
        }
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(AstMemberSel* nodep) override {
        ಯದಿ (m_inRandomizeWith && nodep->fromp()->same_ವೃಕ್ಷ(m_currentRandomizeSelectp)) {
            // Replace member selects to the element
            // on which the randomize() is called with LambdaArgRef
            // This allows V3Randomize to work properly when
            // constrained variables are referred using that object
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ prevFromp = nodep->fromp();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp
                = ನೂತನ AstLambdaArgRef{prevFromp->fileline(), prevFromp->name(), ಅಸತ್ಯ};
            prevFromp->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(prevFromp), prevFromp);
        }
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(AstWith* nodep) override {
        VL_RESTORER(m_inRandomizeWith);
        ಯದಿ (ಧ್ರುವ AstMethodCall* ಧ್ರುವ methodCallp = VN_CAST(nodep->backp(), MethodCall)) {
            m_inRandomizeWith = methodCallp->name() == "randomize";
        } ಅನ್ಯಥಾ {
            m_inRandomizeWith = ಅಸತ್ಯ;
        }
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit LinkWithVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkWithVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// V3LinkWith class functions

ಶೂನ್ಯ V3LinkWith::linkWith(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { ಧ್ರುವ LinkWithVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("linkwith", 0, dump_ವೃಕ್ಷ_Either_Level() >= 6);
}
