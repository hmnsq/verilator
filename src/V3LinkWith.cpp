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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3LinkWith.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

वर्ग LinkWithVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE

    // STATE
    // Below state needs to be preserved between each module call.
    ध्रुव वर्ण* m_randcIllegalWhyp = शून्यसूचकः;  // Why randc illegal
    Ast_ग्रन्थिः* m_randcIllegalp = शून्यसूचकः;  // Node causing randc illegal
    Ast_ग्रन्थिः_व्यञ्जकम्* m_currentRandomizeSelectp = शून्यसूचकः;  // fromp() of current `randomize()` call
    तार्किक m_अन्तर्_Randomize_With = असत्य;  // If in randomize() with (and no other with afterwards)

    // VISITORS
    शून्य दर्शय(AstConstraint* nodep) override {
        // V3LinkDot moved the isExternDef into the class, the extern proto was
        // checked to exist, and now isn't needed
        nodep->isExternDef(असत्य);
        यदि (nodep->isExternProto()) {
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
            प्रत्यागम;
        }
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstConstraintBefore* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        m_randcIllegalWhyp = "'solve before' (IEEE 1800-2023 18.5.9)";
        m_randcIllegalp = nodep;
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(AstDist* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        m_randcIllegalWhyp = "'constraint dist' (IEEE 1800-2023 18.5.3)";
        m_randcIllegalp = nodep;
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(Ast_Constraint_व्यञ्जकम्* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        यदि (nodep->isSoft()) {
            m_randcIllegalWhyp = "'constraint soft' (IEEE 1800-2023 18.5.13.1)";
            m_randcIllegalp = nodep;
        }
        iterateChildrenConst(nodep);
    }

    शून्य दर्शय(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        यदि (nodep->varp()) {  // Else due to dead code, might not have var pointer
            यदि (nodep->varp()->isRandC() && m_randcIllegalp) {
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

    शून्य दर्शय(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        VL_RESTORER(m_currentRandomizeSelectp);
        यदि (nodep->taskp()) {
            यदि (AstSequence* ध्रुव seqp = VN_CAST(nodep->taskp(), Sequence))
                seqp->isReferenced(सत्य);
        }

        यदि (nodep->name() == "randomize") {
            यदि (ध्रुव Ast_विधानम्_Call* ध्रुव methodcallp = VN_CAST(nodep, विधानम्_Call)) {
                यदि (m_अन्तर्_Randomize_With) {
                    nodep->v3warn(
                        E_UNSUPPORTED,
                        "Unsupported: randomize() nested in inline randomize() constraints");
                }
                m_currentRandomizeSelectp = methodcallp->fromp();
            }
        }
        iterateChildren(nodep);
    }

    शून्य दर्शय(AstMemberSel* nodep) override {
        यदि (m_अन्तर्_Randomize_With && nodep->fromp()->same_वृक्षः(m_currentRandomizeSelectp)) {
            // Replace member selects to the element
            // on which the randomize() is called with LambdaArgRef
            // This allows V3Randomize to work properly when
            // constrained variables are referred using that object
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव पूर्व_Fromp = nodep->fromp();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp
                = नूतन AstLambdaArgRef{पूर्व_Fromp->fileline(), पूर्व_Fromp->name(), असत्य};
            पूर्व_Fromp->replaceWith(newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(पूर्व_Fromp), पूर्व_Fromp);
        }
        iterateChildren(nodep);
    }

    शून्य दर्शय(AstWith* nodep) override {
        VL_RESTORER(m_अन्तर्_Randomize_With);
        यदि (ध्रुव Ast_विधानम्_Call* ध्रुव विधानम्_Callp = VN_CAST(nodep->backp(), विधानम्_Call)) {
            m_अन्तर्_Randomize_With = विधानम्_Callp->name() == "randomize";
        } अन्यथा {
            m_अन्तर्_Randomize_With = असत्य;
        }
        iterateChildren(nodep);
    }

    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट LinkWithVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkWithVisitor() override = स्वभाव;
};

//######################################################################
// V3LinkWith class functions

शून्य V3LinkWith::linkWith(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { ध्रुव LinkWithVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("linkwith", 0, dump_वृक्षः_Either_Level() >= 6);
}
