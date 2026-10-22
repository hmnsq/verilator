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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3LinkWith.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

വർഗ്ഗം LinkWithVisitor final : പൊതു VNVisitor {
    // NODE STATE

    // STATE
    // Below state needs to be preserved between each module call.
    സ്ഥിരം അക്ഷരം* m_randcIllegalWhyp = nullptr;  // Why randc illegal
    Ast_കണ്ണി* m_randcIllegalp = nullptr;  // Node causing randc illegal
    Ast_കണ്ണി_വ്യഞ്ജകം* m_currentRandomizeSelectp = nullptr;  // fromp() of current `randomize()` call
    തർക്കം m_inRandomizeWith = അസത്യം;  // If in randomize() with (and no other with afterwards)

    // VISITORS
    ശൂന്യം visit(AstConstraint* nodep) override {
        // V3LinkDot moved the isExternDef into the class, the extern proto was
        // checked to exist, and now isn't needed
        nodep->isExternDef(അസത്യം);
        എങ്കിൽ (nodep->isExternProto()) {
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
            തിരികെ;
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstConstraintBefore* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        m_randcIllegalWhyp = "'solve before' (IEEE 1800-2023 18.5.9)";
        m_randcIllegalp = nodep;
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstDist* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        m_randcIllegalWhyp = "'constraint dist' (IEEE 1800-2023 18.5.3)";
        m_randcIllegalp = nodep;
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(Ast_Constraint_വ്യഞ്ജകം* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        എങ്കിൽ (nodep->isSoft()) {
            m_randcIllegalWhyp = "'constraint soft' (IEEE 1800-2023 18.5.13.1)";
            m_randcIllegalp = nodep;
        }
        iterateChildrenConst(nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി_Var_Ref* nodep) override {
        എങ്കിൽ (nodep->varp()) {  // Else due to dead code, might not have var pointer
            എങ്കിൽ (nodep->varp()->isRandC() && m_randcIllegalp) {
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

    ശൂന്യം visit(Ast_കണ്ണി_FTask_Ref* nodep) override {
        VL_RESTORER(m_currentRandomizeSelectp);
        എങ്കിൽ (nodep->taskp()) {
            എങ്കിൽ (AstSequence* സ്ഥിരം seqp = VN_CAST(nodep->taskp(), Sequence))
                seqp->isReferenced(സത്യം);
        }

        എങ്കിൽ (nodep->name() == "randomize") {
            എങ്കിൽ (സ്ഥിരം AstMethodCall* സ്ഥിരം methodcallp = VN_CAST(nodep, MethodCall)) {
                എങ്കിൽ (m_inRandomizeWith) {
                    nodep->v3warn(
                        E_UNSUPPORTED,
                        "Unsupported: randomize() nested in inline randomize() constraints");
                }
                m_currentRandomizeSelectp = methodcallp->fromp();
            }
        }
        iterateChildren(nodep);
    }

    ശൂന്യം visit(AstMemberSel* nodep) override {
        എങ്കിൽ (m_inRandomizeWith && nodep->fromp()->same_വൃക്ഷം(m_currentRandomizeSelectp)) {
            // Replace member selects to the element
            // on which the randomize() is called with LambdaArgRef
            // This allows V3Randomize to work properly when
            // constrained variables are referred using that object
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം prevFromp = nodep->fromp();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp
                = പുതിയ AstLambdaArgRef{prevFromp->fileline(), prevFromp->name(), അസത്യം};
            prevFromp->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(prevFromp), prevFromp);
        }
        iterateChildren(nodep);
    }

    ശൂന്യം visit(AstWith* nodep) override {
        VL_RESTORER(m_inRandomizeWith);
        എങ്കിൽ (സ്ഥിരം AstMethodCall* സ്ഥിരം methodCallp = VN_CAST(nodep->backp(), MethodCall)) {
            m_inRandomizeWith = methodCallp->name() == "randomize";
        } അല്ലെങ്കിൽ {
            m_inRandomizeWith = അസത്യം;
        }
        iterateChildren(nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit LinkWithVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkWithVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// V3LinkWith class functions

ശൂന്യം V3LinkWith::linkWith(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { സ്ഥിരം LinkWithVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("linkwith", 0, dump_വൃക്ഷം_Either_Level() >= 6);
}
