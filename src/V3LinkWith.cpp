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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3LinkWith.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

বর্গ LinkWithVisitor final : সার্বজনিক VNVisitor {
    // NODE STATE

    // STATE
    // Below state needs to be preserved between each module call.
    ধ্রুব বর্ণ* m_randcIllegalWhyp = nullptr;  // Why randc illegal
    Ast_গ্রন্থি* m_randcIllegalp = nullptr;  // Node causing randc illegal
    Ast_গ্রন্থি_রাশিমালা* m_currentRandomizeSelectp = nullptr;  // fromp() of current `randomize()` call
    তার্কিক m_inRandomizeWith = অসত্য;  // If in randomize() with (and no other with afterwards)

    // VISITORS
    শূন্য visit(AstConstraint* nodep) override {
        // V3LinkDot moved the isExternDef into the class, the extern proto was
        // checked to exist, and now isn't needed
        nodep->isExternDef(অসত্য);
        যদি (nodep->isExternProto()) {
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
            প্রত্যাগম;
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstConstraintBefore* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        m_randcIllegalWhyp = "'solve before' (IEEE 1800-2023 18.5.9)";
        m_randcIllegalp = nodep;
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstDist* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        m_randcIllegalWhyp = "'constraint dist' (IEEE 1800-2023 18.5.3)";
        m_randcIllegalp = nodep;
        iterateChildrenConst(nodep);
    }
    শূন্য visit(Ast_Constraint_রাশিমালা* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        যদি (nodep->isSoft()) {
            m_randcIllegalWhyp = "'constraint soft' (IEEE 1800-2023 18.5.13.1)";
            m_randcIllegalp = nodep;
        }
        iterateChildrenConst(nodep);
    }

    শূন্য visit(Ast_গ্রন্থি_Var_Ref* nodep) override {
        যদি (nodep->varp()) {  // Else due to dead code, might not have var pointer
            যদি (nodep->varp()->isRandC() && m_randcIllegalp) {
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

    শূন্য visit(Ast_গ্রন্থি_FTask_Ref* nodep) override {
        VL_RESTORER(m_currentRandomizeSelectp);
        যদি (nodep->taskp()) {
            যদি (AstSequence* ধ্রুব seqp = VN_CAST(nodep->taskp(), Sequence))
                seqp->isReferenced(সত্য);
        }

        যদি (nodep->name() == "randomize") {
            যদি (ধ্রুব AstMethodCall* ধ্রুব methodcallp = VN_CAST(nodep, MethodCall)) {
                যদি (m_inRandomizeWith) {
                    nodep->v3warn(
                        E_UNSUPPORTED,
                        "Unsupported: randomize() nested in inline randomize() constraints");
                }
                m_currentRandomizeSelectp = methodcallp->fromp();
            }
        }
        iterateChildren(nodep);
    }

    শূন্য visit(AstMemberSel* nodep) override {
        যদি (m_inRandomizeWith && nodep->fromp()->same_বৃক্ষ(m_currentRandomizeSelectp)) {
            // Replace member selects to the element
            // on which the randomize() is called with LambdaArgRef
            // This allows V3Randomize to work properly when
            // constrained variables are referred using that object
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব prevFromp = nodep->fromp();
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব newp
                = নূতন AstLambdaArgRef{prevFromp->fileline(), prevFromp->name(), অসত্য};
            prevFromp->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(prevFromp), prevFromp);
        }
        iterateChildren(nodep);
    }

    শূন্য visit(AstWith* nodep) override {
        VL_RESTORER(m_inRandomizeWith);
        যদি (ধ্রুব AstMethodCall* ধ্রুব methodCallp = VN_CAST(nodep->backp(), MethodCall)) {
            m_inRandomizeWith = methodCallp->name() == "randomize";
        } অন্যথা {
            m_inRandomizeWith = অসত্য;
        }
        iterateChildren(nodep);
    }

    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সার্বজনিক:
    // CONSTRUCTORS
    explicit LinkWithVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkWithVisitor() override = স্বভাব;
};

//######################################################################
// V3LinkWith class functions

শূন্য V3LinkWith::linkWith(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { ধ্রুব LinkWithVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("linkwith", 0, dump_বৃক্ষ_Either_Level() >= 6);
}
