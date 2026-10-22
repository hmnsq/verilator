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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3LinkWith.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

ਵਰਗ LinkWithVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE

    // STATE
    // Below state needs to be preserved between each module call.
    ਅਚਲ ਅੱਖਰ* m_randcIllegalWhyp = ਖਾਲੀ_ਸੂਚਕ;  // Why randc illegal
    Ast_ਗੰਢ* m_randcIllegalp = ਖਾਲੀ_ਸੂਚਕ;  // Node causing randc illegal
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_currentRandomizeSelectp = ਖਾਲੀ_ਸੂਚਕ;  // fromp() of current `randomize()` call
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Randomize_With = ਝੂਠ;  // If in randomize() with (and no other with afterwards)

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstConstraint* nodep) override {
        // V3LinkDot moved the isExternDef into the class, the extern proto was
        // checked to exist, and now isn't needed
        nodep->isExternDef(ਝੂਠ);
        ਜੇ (nodep->isExternProto()) {
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
            ਵਾਪਸ;
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstConstraintBefore* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        m_randcIllegalWhyp = "'solve before' (IEEE 1800-2023 18.5.9)";
        m_randcIllegalp = nodep;
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstDist* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        m_randcIllegalWhyp = "'constraint dist' (IEEE 1800-2023 18.5.3)";
        m_randcIllegalp = nodep;
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Constraint_ਵਿਅੰਜਕ* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        ਜੇ (nodep->isSoft()) {
            m_randcIllegalWhyp = "'constraint soft' (IEEE 1800-2023 18.5.13.1)";
            m_randcIllegalp = nodep;
        }
        iterateChildrenConst(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Var_Ref* nodep) override {
        ਜੇ (nodep->varp()) {  // Else due to dead code, might not have var pointer
            ਜੇ (nodep->varp()->isRandC() && m_randcIllegalp) {
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

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask_Ref* nodep) override {
        VL_RESTORER(m_currentRandomizeSelectp);
        ਜੇ (nodep->taskp()) {
            ਜੇ (AstSequence* ਅਚਲ seqp = VN_CAST(nodep->taskp(), Sequence))
                seqp->isReferenced(ਸੱਚ);
        }

        ਜੇ (nodep->name() == "randomize") {
            ਜੇ (ਅਚਲ Ast_ਵਿਧੀ_Call* ਅਚਲ methodcallp = VN_CAST(nodep, ਵਿਧੀ_Call)) {
                ਜੇ (m_ਵਿੱਚ_Randomize_With) {
                    nodep->v3warn(
                        E_UNSUPPORTED,
                        "Unsupported: randomize() nested in inline randomize() constraints");
                }
                m_currentRandomizeSelectp = methodcallp->fromp();
            }
        }
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstMemberSel* nodep) override {
        ਜੇ (m_ਵਿੱਚ_Randomize_With && nodep->fromp()->same_ਰੁੱਖ(m_currentRandomizeSelectp)) {
            // Replace member selects to the element
            // on which the randomize() is called with LambdaArgRef
            // This allows V3Randomize to work properly when
            // constrained variables are referred using that object
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ prevFromp = nodep->fromp();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp
                = ਨਵਾਂ AstLambdaArgRef{prevFromp->fileline(), prevFromp->name(), ਝੂਠ};
            prevFromp->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(prevFromp), prevFromp);
        }
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstWith* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_Randomize_With);
        ਜੇ (ਅਚਲ Ast_ਵਿਧੀ_Call* ਅਚਲ ਵਿਧੀ_Callp = VN_CAST(nodep->backp(), ਵਿਧੀ_Call)) {
            m_ਵਿੱਚ_Randomize_With = ਵਿਧੀ_Callp->name() == "randomize";
        } ਨਹੀਂਤਾਂ {
            m_ਵਿੱਚ_Randomize_With = ਝੂਠ;
        }
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ LinkWithVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkWithVisitor() override = ਮੂਲ;
};

//######################################################################
// V3LinkWith class functions

ਖਾਲੀ V3LinkWith::linkWith(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { ਅਚਲ LinkWithVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("linkwith", 0, dump_ਰੁੱਖ_Either_Level() >= 6);
}
