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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3LinkWith.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

श्रेणी LinkWithVisitor final : खुला VNVisitor {
    // NODE STATE

    // STATE
    // Below state needs to be preserved between each module call.
    स्थिर अक्षर* m_randcIllegalWhyp = nullptr;  // Why randc illegal
    Ast_गाँठ* m_randcIllegalp = nullptr;  // Node causing randc illegal
    Ast_गाँठ_व्यंजक* m_currentRandomizeSelectp = nullptr;  // fromp() of current `randomize()` call
    bool m_inRandomizeWith = false;  // If in randomize() with (and no other with afterwards)

    // VISITORS
    व्योम visit(AstConstraint* nodep) override {
        // V3LinkDot moved the isExternDef into the class, the extern proto was
        // checked to exist, and now isn't needed
        nodep->isExternDef(false);
        अगर (nodep->isExternProto()) {
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
            वापस;
        }
        iterateChildren(nodep);
    }
    व्योम visit(AstConstraintBefore* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        m_randcIllegalWhyp = "'solve before' (IEEE 1800-2023 18.5.9)";
        m_randcIllegalp = nodep;
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstDist* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        m_randcIllegalWhyp = "'constraint dist' (IEEE 1800-2023 18.5.3)";
        m_randcIllegalp = nodep;
        iterateChildrenConst(nodep);
    }
    व्योम visit(Ast_Constraint_व्यंजक* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        अगर (nodep->isSoft()) {
            m_randcIllegalWhyp = "'constraint soft' (IEEE 1800-2023 18.5.13.1)";
            m_randcIllegalp = nodep;
        }
        iterateChildrenConst(nodep);
    }

    व्योम visit(Ast_गाँठ_Var_Ref* nodep) override {
        अगर (nodep->varp()) {  // Else due to dead code, might not have var pointer
            अगर (nodep->varp()->isRandC() && m_randcIllegalp) {
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

    व्योम visit(Ast_गाँठ_FTask_Ref* nodep) override {
        VL_RESTORER(m_currentRandomizeSelectp);
        अगर (nodep->taskp()) {
            अगर (AstSequence* स्थिर seqp = VN_CAST(nodep->taskp(), Sequence))
                seqp->isReferenced(true);
        }

        अगर (nodep->name() == "randomize") {
            अगर (स्थिर AstMethodCall* स्थिर methodcallp = VN_CAST(nodep, MethodCall)) {
                अगर (m_inRandomizeWith) {
                    nodep->v3warn(
                        E_UNSUPPORTED,
                        "Unsupported: randomize() nested in inline randomize() constraints");
                }
                m_currentRandomizeSelectp = methodcallp->fromp();
            }
        }
        iterateChildren(nodep);
    }

    व्योम visit(AstMemberSel* nodep) override {
        अगर (m_inRandomizeWith && nodep->fromp()->same_वृक्ष(m_currentRandomizeSelectp)) {
            // Replace member selects to the element
            // on which the randomize() is called with LambdaArgRef
            // This allows V3Randomize to work properly when
            // constrained variables are referred using that object
            Ast_गाँठ_व्यंजक* स्थिर पूर्व_Fromp = nodep->fromp();
            Ast_गाँठ_व्यंजक* स्थिर newp
                = नया AstLambdaArgRef{पूर्व_Fromp->fileline(), पूर्व_Fromp->name(), false};
            पूर्व_Fromp->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(पूर्व_Fromp), पूर्व_Fromp);
        }
        iterateChildren(nodep);
    }

    व्योम visit(AstWith* nodep) override {
        VL_RESTORER(m_inRandomizeWith);
        अगर (स्थिर AstMethodCall* स्थिर methodCallp = VN_CAST(nodep->backp(), MethodCall)) {
            m_inRandomizeWith = methodCallp->name() == "randomize";
        } अन्यथा {
            m_inRandomizeWith = false;
        }
        iterateChildren(nodep);
    }

    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    // CONSTRUCTORS
    explicit LinkWithVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkWithVisitor() override = शेष;
};

//######################################################################
// V3LinkWith class functions

व्योम V3LinkWith::linkWith(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { स्थिर LinkWithVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("linkwith", 0, dump_वृक्ष_Either_Level() >= 6);
}
