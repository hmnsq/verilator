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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3LinkWith.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

વર્ગ LinkWithVisitor final : જાહેર VNVisitor {
    // NODE STATE

    // STATE
    // Below state needs to be preserved between each module call.
    અચલ અક્ષર* m_randcIllegalWhyp = nullptr;  // Why randc illegal
    Ast_ગાંઠ* m_randcIllegalp = nullptr;  // Node causing randc illegal
    Ast_ગાંઠ_પદાવલિ* m_currentRandomizeSelectp = nullptr;  // fromp() of current `randomize()` call
    તાર્કિક m_inRandomizeWith = ખોટું;  // If in randomize() with (and no other with afterwards)

    // VISITORS
    શૂન્ય visit(AstConstraint* nodep) override {
        // V3LinkDot moved the isExternDef into the class, the extern proto was
        // checked to exist, and now isn't needed
        nodep->isExternDef(ખોટું);
        જો (nodep->isExternProto()) {
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
            પાછા;
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstConstraintBefore* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        m_randcIllegalWhyp = "'solve before' (IEEE 1800-2023 18.5.9)";
        m_randcIllegalp = nodep;
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstDist* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        m_randcIllegalWhyp = "'constraint dist' (IEEE 1800-2023 18.5.3)";
        m_randcIllegalp = nodep;
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(Ast_Constraint_પદાવલિ* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        જો (nodep->isSoft()) {
            m_randcIllegalWhyp = "'constraint soft' (IEEE 1800-2023 18.5.13.1)";
            m_randcIllegalp = nodep;
        }
        iterateChildrenConst(nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ_Var_Ref* nodep) override {
        જો (nodep->varp()) {  // Else due to dead code, might not have var pointer
            જો (nodep->varp()->isRandC() && m_randcIllegalp) {
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

    શૂન્ય visit(Ast_ગાંઠ_FTask_Ref* nodep) override {
        VL_RESTORER(m_currentRandomizeSelectp);
        જો (nodep->taskp()) {
            જો (AstSequence* અચલ seqp = VN_CAST(nodep->taskp(), Sequence))
                seqp->isReferenced(સાચું);
        }

        જો (nodep->name() == "randomize") {
            જો (અચલ AstMethodCall* અચલ methodcallp = VN_CAST(nodep, MethodCall)) {
                જો (m_inRandomizeWith) {
                    nodep->v3warn(
                        E_UNSUPPORTED,
                        "Unsupported: randomize() nested in inline randomize() constraints");
                }
                m_currentRandomizeSelectp = methodcallp->fromp();
            }
        }
        iterateChildren(nodep);
    }

    શૂન્ય visit(AstMemberSel* nodep) override {
        જો (m_inRandomizeWith && nodep->fromp()->same_વૃક્ષ(m_currentRandomizeSelectp)) {
            // Replace member selects to the element
            // on which the randomize() is called with LambdaArgRef
            // This allows V3Randomize to work properly when
            // constrained variables are referred using that object
            Ast_ગાંઠ_પદાવલિ* અચલ prevFromp = nodep->fromp();
            Ast_ગાંઠ_પદાવલિ* અચલ newp
                = નવું AstLambdaArgRef{prevFromp->fileline(), prevFromp->name(), ખોટું};
            prevFromp->replaceWith(newp);
            VL_DO_DANGLING(ધકેલો_Deletep(prevFromp), prevFromp);
        }
        iterateChildren(nodep);
    }

    શૂન્ય visit(AstWith* nodep) override {
        VL_RESTORER(m_inRandomizeWith);
        જો (અચલ AstMethodCall* અચલ methodCallp = VN_CAST(nodep->backp(), MethodCall)) {
            m_inRandomizeWith = methodCallp->name() == "randomize";
        } નહીંતર {
            m_inRandomizeWith = ખોટું;
        }
        iterateChildren(nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit LinkWithVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkWithVisitor() override = મૂળભૂત;
};

//######################################################################
// V3LinkWith class functions

શૂન્ય V3LinkWith::linkWith(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { અચલ LinkWithVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("linkwith", 0, dump_વૃક્ષ_Either_Level() >= 6);
}
