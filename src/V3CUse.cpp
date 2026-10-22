// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Handle SV classes
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
// V3CUse's Transformations:
//
// Each module:
//   Each cell:
//      Create CUse for cell forward declaration
//   Search for dtypes referencing class, and create CUse for forward declaration
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3CUse.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

// Visit within a module all nodes and data types they reference, finding
// any classes so we can make sure they are defined when Verilated code
// compiles
صنف CUseVisitor final : عام VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()     -> bool.  True if already visited
    ثابت VNUser1_دننه_Use m_inuser1;

    // MEMBERS
    Ast_غوټه_Module* ثابت m_modp;  // Current module
    std::map<std::string, std::pair<File_کرښه*, VUseType>> m_didUse;  // What we already used

    // METHODS
    فراغ جمع_کړه_New_Use(ثابت Ast_غوټه* nodep, VUseType useType, ثابت string& name) {
        خپلکاری e = m_didUse.emplace(name, std::make_pair(nodep->fileline(), useType));
        إذا (e.دوهم || ((e.لومړی->دوهم.دوهم & useType) != useType)) {
            e.لومړی->دوهم.دوهم = e.لومړی->دوهم.دوهم | useType;
        }
    }

    // VISITORS
    فراغ لیدنه(AstClassRefDType* nodep) override {
        جمع_کړه_New_Use(nodep, VUseType::INT_FWD_CLASS, nodep->classp()->name());
    }
    فراغ لیدنه(AstCFunc* nodep) override {
        إذا (nodep->user1_سېټ_Once()) أرجع;
        iterate_And_بل_Const_Null(nodep->argsp());
        iterate_And_بل_Const_Null(nodep->stmtsp());
    }
    فراغ لیدنه(AstCCall* /*nodep*/) override {}
    فراغ لیدنه(AstCReturn* nodep) override {
        UASSERT_OBJ(!nodep->user1_سېټ_Once(), nodep, "Visited same return twice");
        iterateConst(nodep->lhsp()->dtypep());
    }
    فراغ لیدنه(Ast_غوټه_DType* nodep) override {
        إذا (nodep->user1_سېټ_Once()) أرجع;  // Process once
        إذا (nodep->virtRefDTypep()) iterateConst(nodep->virtRefDTypep());
        إذا (nodep->virtRefDType2p()) iterateConst(nodep->virtRefDType2p());

        // Add a CUse for every struct that requires a declaration
        Ast_غوټه_UOr_Struct_DType* ثابت stypep = VN_CAST(nodep->skipRefp(), غوټه_UOr_Struct_DType);
        إذا (stypep && stypep->classOrPackagep()) {
            جمع_کړه_New_Use(nodep, VUseType::INT_INCLUDE, stypep->classOrPackagep()->name());
            iterateChildrenConst(stypep);
        } وإلا إذا (ثابت AstClassRefDType* ثابت classp
                   = VN_CAST(nodep->skipRefp(), ClassRefDType)) {
            جمع_کړه_New_Use(nodep, VUseType::INT_FWD_CLASS, classp->name());
        }
    }
    فراغ لیدنه(Ast_غوټه* nodep) override {
        إذا (nodep->user1_سېټ_Once()) أرجع;  // Process once
        إذا (nodep->dtypep()) iterateConst(nodep->dtypep());
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(AstCell* nodep) override {
        إذا (nodep->user1_سېټ_Once()) أرجع;  // Process once
        إذا (nodep->modp()->isConstPool()) أرجع;  // Special emit rules
        // Currently no IMP_INCLUDE because we include __Syms which has them all
        جمع_کړه_New_Use(nodep, VUseType::INT_FWD_CLASS, nodep->modp()->name());
        iterateChildrenConst(nodep);
    }

عام:
    // CONSTRUCTORS
    څرګند CUseVisitor(Ast_غوټه_Module* modp)
        : m_modp{modp} {
        iterateConst(modp);

        لكل (ثابت خپلکاری& کارول_شوی : m_didUse) {
            AstCUse* ثابت newp = جديد AstCUse{کارول_شوی.دوهم.لومړی, کارول_شوی.دوهم.دوهم, کارول_شوی.لومړی};
            m_modp->جمع_کړه_Stmtsp(newp);
            UINFO(8, "Insert " << newp);
        }
    }
    ~CUseVisitor() override = افتراضي;
    VL_UNCOPYABLE(CUseVisitor);
};

//######################################################################
// Class class functions

فراغ V3CUse::cUseAll() {
    UINFO(2, __FUNCTION__ << ":");
    // Call visitor separately for each module, so visitor state is cleared
    لكل (Ast_غوټه_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), غوټه_Module)) {
        إذا (modp->isConstPool()) استمر;  // Special emit rules
        // Insert under this module; someday we should e.g. make Ast
        // for each output file and put under that
        CUseVisitor{modp};
    }
    V3Global::dump_Check_Global_ونه("cuse", 0, dump_ونه_Either_Level() >= 3);
}
