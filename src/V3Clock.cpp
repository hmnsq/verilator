// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Post scheduling transformations
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
// V3Clock's Transformations:
//
//  This pass is historic and does some arbitray post scheduling rewrites
//
//*************************************************************************

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Clock.h"

#அந்தர்பாவ "V3Const.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Convert every WRITE AstVarRef to a READ ref

வகுப்பு ConvertWriteRefsToRead final : பொது VNVisitor {
    // MEMBERS
    Ast_முனை_கோவை* m_result = nullptr;

    // CONSTRUCTORS
    explicit ConvertWriteRefsToRead(Ast_முனை_கோவை* nodep) {
        m_result = VN_AS(iterateSubtreeReturnEdits(nodep), முனை_கோவை);
    }

    // VISITORS
    வெறுமை visit(AstVarRef* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Cannot handle a READWRITE reference");
        எனில் (nodep->access().isWriteOnly()) {
            nodep->replaceWith(
                புதிய AstVarRef{nodep->fileline(), nodep->varScopep(), VAccess::READ});
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        }
    }

    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    நிலையான Ast_முனை_கோவை* முதன்மை(Ast_முனை_கோவை* nodep) { திரும்பு ConvertWriteRefsToRead{nodep}.m_result; }
};

//######################################################################
// Clock state, as a visitor of each AstNode

வகுப்பு ClockVisitor final : பொது VNVisitor {
    // VISITORS
    வெறுமை visit(AstCoverToggle* nodep) override {
        // UINFOTREE(1, nodep, "", "ct");
        // COVERTOGGLE(INC, ORIG, CHANGE) ->
        //   IF(ORIG ^ CHANGE) { INC; CHANGE = ORIG; }
        AstCoverInc* மாறிலி incp = nodep->incp()->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி origp = nodep->origp()->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி changeWrp = nodep->changep()->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி changeRdp = ConvertWriteRefsToRead::முதன்மை(changeWrp->clone_மரம்(பொய்));
        Ast_முனை_கோவை* comparedp = nullptr;
        incp->toggleExprp(origp->clone_மரம்(பொய்));
        incp->toggleCovExprp(changeRdp->clone_மரம்(பொய்));
        File_வரி* மாறிலி flp = nodep->fileline();
        // Xor will optimize better than Eq, when CoverToggle has bit selects,
        // but can only use Xor with non-opaque types
        எனில் (மாறிலி AstBasicDType* மாறிலி bdtypep
            = VN_CAST(origp->dtypep()->skipRefp(), BasicDType)) {
            எனில் (!bdtypep->isOpaque()) {
                Ast_முனை_கோவை* மாறிலி zp = புதிய AstConst{flp, AstConst::DTyped{}, origp->dtypep()};
                comparedp = புதிய AstNeq{flp, zp, புதிய AstXor{flp, origp, changeRdp}};
            }
        }
        எனில் (!comparedp) comparedp = AstNeq::newTyped(flp, origp, changeRdp);
        AstIf* மாறிலி newp = புதிய AstIf{flp, comparedp, incp};
        // We could add another IF to detect posedges, and only increment if so.
        // It's another whole branch though versus a potential memory miss.
        // We'll go with the miss.
        newp->addThensp(புதிய AstAssign{flp, changeWrp, origp->clone_மரம்(பொய்)});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
    }
    வெறுமை visit(Ast_Sen_மரம்* nodep) override {
        VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);  // No longer needed
    }

    //--------------------
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    explicit ClockVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    ~ClockVisitor() override = இயல்பு;
};

//######################################################################
// Clock class functions

வெறுமை V3Clock::clockAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClockVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("clock", 0, dump_மரம்_Either_Level() >= 3);
}
