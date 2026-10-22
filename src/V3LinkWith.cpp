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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3LinkWith.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

తరగతి LinkWithVisitor final : బహిరంగ VNVisitor {
    // NODE STATE

    // STATE
    // Below state needs to be preserved between each module call.
    స్థిరరాశి అక్షరం* m_randcIllegalWhyp = శూన్యసూచిక;  // Why randc illegal
    Ast_కణుపు* m_randcIllegalp = శూన్యసూచిక;  // Node causing randc illegal
    Ast_కణుపు_సమాసం* m_currentRandomizeSelectp = శూన్యసూచిక;  // fromp() of current `randomize()` call
    తార్కిక m_లోపల_Randomize_With = అబద్ధం;  // If in randomize() with (and no other with afterwards)

    // VISITORS
    శూన్య సందర్శించు(AstConstraint* nodep) override {
        // V3LinkDot moved the isExternDef into the class, the extern proto was
        // checked to exist, and now isn't needed
        nodep->isExternDef(అబద్ధం);
        ఒకవేళ (nodep->isExternProto()) {
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
            తిరిగి;
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstConstraintBefore* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        m_randcIllegalWhyp = "'solve before' (IEEE 1800-2023 18.5.9)";
        m_randcIllegalp = nodep;
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstDist* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        m_randcIllegalWhyp = "'constraint dist' (IEEE 1800-2023 18.5.3)";
        m_randcIllegalp = nodep;
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(Ast_Constraint_సమాసం* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        ఒకవేళ (nodep->isSoft()) {
            m_randcIllegalWhyp = "'constraint soft' (IEEE 1800-2023 18.5.13.1)";
            m_randcIllegalp = nodep;
        }
        iterateChildrenConst(nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు_Var_Ref* nodep) override {
        ఒకవేళ (nodep->varp()) {  // Else due to dead code, might not have var pointer
            ఒకవేళ (nodep->varp()->isRandC() && m_randcIllegalp) {
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

    శూన్య సందర్శించు(Ast_కణుపు_FTask_Ref* nodep) override {
        VL_RESTORER(m_currentRandomizeSelectp);
        ఒకవేళ (nodep->taskp()) {
            ఒకవేళ (AstSequence* స్థిరరాశి seqp = VN_CAST(nodep->taskp(), Sequence))
                seqp->isReferenced(నిజం);
        }

        ఒకవేళ (nodep->name() == "randomize") {
            ఒకవేళ (స్థిరరాశి Ast_పద్ధతి_Call* స్థిరరాశి methodcallp = VN_CAST(nodep, పద్ధతి_Call)) {
                ఒకవేళ (m_లోపల_Randomize_With) {
                    nodep->v3warn(
                        E_UNSUPPORTED,
                        "Unsupported: randomize() nested in inline randomize() constraints");
                }
                m_currentRandomizeSelectp = methodcallp->fromp();
            }
        }
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(AstMemberSel* nodep) override {
        ఒకవేళ (m_లోపల_Randomize_With && nodep->fromp()->same_వృక్షం(m_currentRandomizeSelectp)) {
            // Replace member selects to the element
            // on which the randomize() is called with LambdaArgRef
            // This allows V3Randomize to work properly when
            // constrained variables are referred using that object
            Ast_కణుపు_సమాసం* స్థిరరాశి prevFromp = nodep->fromp();
            Ast_కణుపు_సమాసం* స్థిరరాశి newp
                = కొత్త AstLambdaArgRef{prevFromp->fileline(), prevFromp->name(), అబద్ధం};
            prevFromp->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(prevFromp), prevFromp);
        }
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(AstWith* nodep) override {
        VL_RESTORER(m_లోపల_Randomize_With);
        ఒకవేళ (స్థిరరాశి Ast_పద్ధతి_Call* స్థిరరాశి పద్ధతి_Callp = VN_CAST(nodep->backp(), పద్ధతి_Call)) {
            m_లోపల_Randomize_With = పద్ధతి_Callp->name() == "randomize";
        } లేకపోతే {
            m_లోపల_Randomize_With = అబద్ధం;
        }
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన LinkWithVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkWithVisitor() override = అప్రమేయ;
};

//######################################################################
// V3LinkWith class functions

శూన్య V3LinkWith::linkWith(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { స్థిరరాశి LinkWithVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("linkwith", 0, dump_వృక్షం_Either_Level() >= 6);
}
