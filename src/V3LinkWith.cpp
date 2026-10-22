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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3LinkWith.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

ଶ୍ରେଣୀ LinkWithVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE

    // STATE
    // Below state needs to be preserved between each module call.
    ଅଚଳ ଅକ୍ଷର* m_randcIllegalWhyp = nullptr;  // Why randc illegal
    Ast_ଗଣ୍ଠି* m_randcIllegalp = nullptr;  // Node causing randc illegal
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_currentRandomizeSelectp = nullptr;  // fromp() of current `randomize()` call
    ତାର୍କିକ m_inRandomizeWith = ମିଥ୍ୟା;  // If in randomize() with (and no other with afterwards)

    // VISITORS
    ଶୂନ୍ୟ visit(AstConstraint* nodep) override {
        // V3LinkDot moved the isExternDef into the class, the extern proto was
        // checked to exist, and now isn't needed
        nodep->isExternDef(ମିଥ୍ୟା);
        ଯଦି (nodep->isExternProto()) {
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
            ଫେରନ୍ତୁ;
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstConstraintBefore* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        m_randcIllegalWhyp = "'solve before' (IEEE 1800-2023 18.5.9)";
        m_randcIllegalp = nodep;
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstDist* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        m_randcIllegalWhyp = "'constraint dist' (IEEE 1800-2023 18.5.3)";
        m_randcIllegalp = nodep;
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_Constraint_ପଦସମଷ୍ଟି* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        ଯଦି (nodep->isSoft()) {
            m_randcIllegalWhyp = "'constraint soft' (IEEE 1800-2023 18.5.13.1)";
            m_randcIllegalp = nodep;
        }
        iterateChildrenConst(nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Var_Ref* nodep) override {
        ଯଦି (nodep->varp()) {  // Else due to dead code, might not have var pointer
            ଯଦି (nodep->varp()->isRandC() && m_randcIllegalp) {
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

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask_Ref* nodep) override {
        VL_RESTORER(m_currentRandomizeSelectp);
        ଯଦି (nodep->taskp()) {
            ଯଦି (AstSequence* ଅଚଳ seqp = VN_CAST(nodep->taskp(), Sequence))
                seqp->isReferenced(ସତ୍ୟ);
        }

        ଯଦି (nodep->name() == "randomize") {
            ଯଦି (ଅଚଳ AstMethodCall* ଅଚଳ methodcallp = VN_CAST(nodep, MethodCall)) {
                ଯଦି (m_inRandomizeWith) {
                    nodep->v3warn(
                        E_UNSUPPORTED,
                        "Unsupported: randomize() nested in inline randomize() constraints");
                }
                m_currentRandomizeSelectp = methodcallp->fromp();
            }
        }
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(AstMemberSel* nodep) override {
        ଯଦି (m_inRandomizeWith && nodep->fromp()->same_ବୃକ୍ଷ(m_currentRandomizeSelectp)) {
            // Replace member selects to the element
            // on which the randomize() is called with LambdaArgRef
            // This allows V3Randomize to work properly when
            // constrained variables are referred using that object
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ prevFromp = nodep->fromp();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp
                = ନୂତନ AstLambdaArgRef{prevFromp->fileline(), prevFromp->name(), ମିଥ୍ୟା};
            prevFromp->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(prevFromp), prevFromp);
        }
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(AstWith* nodep) override {
        VL_RESTORER(m_inRandomizeWith);
        ଯଦି (ଅଚଳ AstMethodCall* ଅଚଳ methodCallp = VN_CAST(nodep->backp(), MethodCall)) {
            m_inRandomizeWith = methodCallp->name() == "randomize";
        } ଅନ୍ୟଥା {
            m_inRandomizeWith = ମିଥ୍ୟା;
        }
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit LinkWithVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkWithVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// V3LinkWith class functions

ଶୂନ୍ୟ V3LinkWith::linkWith(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { ଅଚଳ LinkWithVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("linkwith", 0, dump_ବୃକ୍ଷ_Either_Level() >= 6);
}
