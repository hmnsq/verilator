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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3LinkWith.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

வகுப்பு LinkWithVisitor final : பொது VNVisitor {
    // NODE STATE

    // STATE
    // Below state needs to be preserved between each module call.
    மாறிலி எழுத்து* m_randcIllegalWhyp = nullptr;  // Why randc illegal
    Ast_முனை* m_randcIllegalp = nullptr;  // Node causing randc illegal
    Ast_முனை_கோவை* m_currentRandomizeSelectp = nullptr;  // fromp() of current `randomize()` call
    தருக்க m_inRandomizeWith = பொய்;  // If in randomize() with (and no other with afterwards)

    // VISITORS
    வெறுமை visit(AstConstraint* nodep) override {
        // V3LinkDot moved the isExternDef into the class, the extern proto was
        // checked to exist, and now isn't needed
        nodep->isExternDef(பொய்);
        எனில் (nodep->isExternProto()) {
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
            திரும்பு;
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(AstConstraintBefore* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        m_randcIllegalWhyp = "'solve before' (IEEE 1800-2023 18.5.9)";
        m_randcIllegalp = nodep;
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstDist* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        m_randcIllegalWhyp = "'constraint dist' (IEEE 1800-2023 18.5.3)";
        m_randcIllegalp = nodep;
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(Ast_Constraint_கோவை* nodep) override {
        VL_RESTORER(m_randcIllegalWhyp);
        VL_RESTORER(m_randcIllegalp);
        எனில் (nodep->isSoft()) {
            m_randcIllegalWhyp = "'constraint soft' (IEEE 1800-2023 18.5.13.1)";
            m_randcIllegalp = nodep;
        }
        iterateChildrenConst(nodep);
    }

    வெறுமை visit(Ast_முனை_Var_Ref* nodep) override {
        எனில் (nodep->varp()) {  // Else due to dead code, might not have var pointer
            எனில் (nodep->varp()->isRandC() && m_randcIllegalp) {
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

    வெறுமை visit(Ast_முனை_FTask_Ref* nodep) override {
        VL_RESTORER(m_currentRandomizeSelectp);
        எனில் (nodep->taskp()) {
            எனில் (AstSequence* மாறிலி seqp = VN_CAST(nodep->taskp(), Sequence))
                seqp->isReferenced(உண்மை);
        }

        எனில் (nodep->name() == "randomize") {
            எனில் (மாறிலி AstMethodCall* மாறிலி methodcallp = VN_CAST(nodep, MethodCall)) {
                எனில் (m_inRandomizeWith) {
                    nodep->v3warn(
                        E_UNSUPPORTED,
                        "Unsupported: randomize() nested in inline randomize() constraints");
                }
                m_currentRandomizeSelectp = methodcallp->fromp();
            }
        }
        iterateChildren(nodep);
    }

    வெறுமை visit(AstMemberSel* nodep) override {
        எனில் (m_inRandomizeWith && nodep->fromp()->same_மரம்(m_currentRandomizeSelectp)) {
            // Replace member selects to the element
            // on which the randomize() is called with LambdaArgRef
            // This allows V3Randomize to work properly when
            // constrained variables are referred using that object
            Ast_முனை_கோவை* மாறிலி prevFromp = nodep->fromp();
            Ast_முனை_கோவை* மாறிலி newp
                = புதிய AstLambdaArgRef{prevFromp->fileline(), prevFromp->name(), பொய்};
            prevFromp->replaceWith(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(prevFromp), prevFromp);
        }
        iterateChildren(nodep);
    }

    வெறுமை visit(AstWith* nodep) override {
        VL_RESTORER(m_inRandomizeWith);
        எனில் (மாறிலி AstMethodCall* மாறிலி methodCallp = VN_CAST(nodep->backp(), MethodCall)) {
            m_inRandomizeWith = methodCallp->name() == "randomize";
        } இல்லையெனில் {
            m_inRandomizeWith = பொய்;
        }
        iterateChildren(nodep);
    }

    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    explicit LinkWithVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkWithVisitor() override = இயல்பு;
};

//######################################################################
// V3LinkWith class functions

வெறுமை V3LinkWith::linkWith(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { மாறிலி LinkWithVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("linkwith", 0, dump_மரம்_Either_Level() >= 6);
}
