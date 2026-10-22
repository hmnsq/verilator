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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3LinkWith.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

מחלקה LinkWithVisitor final : عام VNVisitor {
    // NODE STATE

    // STATE
    // Below state needs to be preserved between each module call.
    ثابت حرف* m_randcIllegalWhyp = nullptr;  // Why randc illegal
    AstNode* m_randcIllegalp = nullptr;  // Node causing randc illegal
    AstNodeExpr* m_currentRandomizeSelectp = nullptr;  // fromp() of current `randomize()` call
    منطقي m_inRandomizeWith = שקר;  // If in randomize() with (and no other with afterwards)

    // VISITORS
    فراغ visit(AstConstraint* nodep) override {
        // V3LinkDot moved the isExternDef into the class, the extern proto was
        // checked to exist, and now isn't needed
        nodep->isExternDef(שקר);
        אם (nodep->isExternProto()) {
            VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
            החזר;
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstConstraintBefore* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        m_randcIllegalWhyp = "'solve before' (IEEE 1800-2023 18.5.9)";
        m_randcIllegalp = nodep;
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstDist* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        m_randcIllegalWhyp = "'constraint dist' (IEEE 1800-2023 18.5.3)";
        m_randcIllegalp = nodep;
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstConstraintExpr* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        אם (nodep->isSoft()) {
            m_randcIllegalWhyp = "'constraint soft' (IEEE 1800-2023 18.5.13.1)";
            m_randcIllegalp = nodep;
        }
        iterateChildrenConst(nodep);
    }

    فراغ visit(AstNodeVarRef* nodep) override {
        אם (nodep->varp()) {  // Else due to dead code, might not have var pointer
            אם (nodep->varp()->isRandC() && m_randcIllegalp) {
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

    فراغ visit(AstNodeFTaskRef* nodep) override {
        VL_RESTORER(m_currentRandomizeSelectp);
        אם (nodep->taskp()) {
            אם (AstSequence* ثابت seqp = VN_CAST(nodep->taskp(), Sequence))
                seqp->isReferenced(אמת);
        }

        אם (nodep->name() == "randomize") {
            אם (ثابت AstMethodCall* ثابت methodcallp = VN_CAST(nodep, MethodCall)) {
                אם (m_inRandomizeWith) {
                    nodep->v3warn(
                        E_UNSUPPORTED,
                        "Unsupported: randomize() nested in inline randomize() constraints");
                }
                m_currentRandomizeSelectp = methodcallp->fromp();
            }
        }
        iterateChildren(nodep);
    }

    فراغ visit(AstMemberSel* nodep) override {
        אם (m_inRandomizeWith && nodep->fromp()->sameTree(m_currentRandomizeSelectp)) {
            // Replace member selects to the element
            // on which the randomize() is called with LambdaArgRef
            // This allows V3Randomize to work properly when
            // constrained variables are referred using that object
            AstNodeExpr* ثابت prevFromp = nodep->fromp();
            AstNodeExpr* ثابت newp
                = جديد AstLambdaArgRef{prevFromp->fileline(), prevFromp->name(), שקר};
            prevFromp->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(prevFromp), prevFromp);
        }
        iterateChildren(nodep);
    }

    فراغ visit(AstWith* nodep) override {
        VL_RESTORER(m_inRandomizeWith);
        אם (ثابت AstMethodCall* ثابت methodCallp = VN_CAST(nodep->backp(), MethodCall)) {
            m_inRandomizeWith = methodCallp->name() == "randomize";
        } אחרת {
            m_inRandomizeWith = שקר;
        }
        iterateChildren(nodep);
    }

    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit LinkWithVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkWithVisitor() override = افتراضي;
};

//######################################################################
// V3LinkWith class functions

فراغ V3LinkWith::linkWith(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { ثابت LinkWithVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("linkwith", 0, dumpTreeEitherLevel() >= 6);
}
