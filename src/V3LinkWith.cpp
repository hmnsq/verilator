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
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* m_randcIllegalWhyp = ಶೂನ್ಯಸೂಚಕ;  // Why randc illegal
    Ast_ಗಂಟು* m_randcIllegalp = ಶೂನ್ಯಸೂಚಕ;  // Node causing randc illegal
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_currentRandomizeSelectp = ಶೂನ್ಯಸೂಚಕ;  // fromp() of current `randomize()` call
    ತಾರ್ಕಿಕ m_ಒಳ_Randomize_With = ಸುಳ್ಳು;  // If in randomize() with (and no other with afterwards)

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstConstraint* nodep) override {
        // V3LinkDot moved the isExternDef into the class, the extern proto was
        // checked to exist, and now isn't needed
        nodep->isExternDef(ಸುಳ್ಳು);
        ಆದರೆ (nodep->isExternProto()) {
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConstraintBefore* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        m_randcIllegalWhyp = "'solve before' (IEEE 1800-2023 18.5.9)";
        m_randcIllegalp = nodep;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDist* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        m_randcIllegalWhyp = "'constraint dist' (IEEE 1800-2023 18.5.3)";
        m_randcIllegalp = nodep;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        ಆದರೆ (nodep->isSoft()) {
            m_randcIllegalWhyp = "'constraint soft' (IEEE 1800-2023 18.5.13.1)";
            m_randcIllegalp = nodep;
        }
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Var_Ref* nodep) override {
        ಆದರೆ (nodep->varp()) {  // Else due to dead code, might not have var pointer
            ಆದರೆ (nodep->varp()->isRandC() && m_randcIllegalp) {
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

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        VL_RESTORER(m_currentRandomizeSelectp);
        ಆದರೆ (nodep->taskp()) {
            ಆದರೆ (AstSequence* ಸ್ಥಿರಾಂಕ seqp = VN_CAST(nodep->taskp(), Sequence))
                seqp->isReferenced(ನಿಜ);
        }

        ಆದರೆ (nodep->name() == "randomize") {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ವಿಧಾನ_Call* ಸ್ಥಿರಾಂಕ methodcallp = VN_CAST(nodep, ವಿಧಾನ_Call)) {
                ಆದರೆ (m_ಒಳ_Randomize_With) {
                    nodep->v3warn(
                        E_UNSUPPORTED,
                        "Unsupported: randomize() nested in inline randomize() constraints");
                }
                m_currentRandomizeSelectp = methodcallp->fromp();
            }
        }
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstMemberSel* nodep) override {
        ಆದರೆ (m_ಒಳ_Randomize_With && nodep->fromp()->same_ವೃಕ್ಷ(m_currentRandomizeSelectp)) {
            // Replace member selects to the element
            // on which the randomize() is called with LambdaArgRef
            // This allows V3Randomize to work properly when
            // constrained variables are referred using that object
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ prevFromp = nodep->fromp();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp
                = ಹೊಸ AstLambdaArgRef{prevFromp->fileline(), prevFromp->name(), ಸುಳ್ಳು};
            prevFromp->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(prevFromp), prevFromp);
        }
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstWith* nodep) override {
        VL_RESTORER(m_ಒಳ_Randomize_With);
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ವಿಧಾನ_Call* ಸ್ಥಿರಾಂಕ ವಿಧಾನ_Callp = VN_CAST(nodep->backp(), ವಿಧಾನ_Call)) {
            m_ಒಳ_Randomize_With = ವಿಧಾನ_Callp->name() == "randomize";
        } ಇಲ್ಲದಿದ್ದರೆ {
            m_ಒಳ_Randomize_With = ಸುಳ್ಳು;
        }
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ LinkWithVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkWithVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// V3LinkWith class functions

ಶೂನ್ಯ V3LinkWith::linkWith(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { ಸ್ಥಿರಾಂಕ LinkWithVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("linkwith", 0, dump_ವೃಕ್ಷ_Either_Level() >= 6);
}
