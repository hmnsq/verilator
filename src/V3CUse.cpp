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
    ثابت VNUser1_وچ_Use m_inuser1;

    // MEMBERS
    Ast_گرہ_Module* ثابت m_modp;  // Current module
    std::map<std::string, std::pair<File_سطر*, VUseType>> m_didUse;  // What we already used

    // METHODS
    فراغ جوڑو_New_Use(ثابت Ast_گرہ* nodep, VUseType useType, ثابت string& name) {
        خودکار e = m_didUse.emplace(name, std::make_pair(nodep->fileline(), useType));
        إذا (e.دوجا || ((e.پہلا->دوجا.دوجا & useType) != useType)) {
            e.پہلا->دوجا.دوجا = e.پہلا->دوجا.دوجا | useType;
        }
    }

    // VISITORS
    فراغ ویکھو(AstClassRefDType* nodep) override {
        جوڑو_New_Use(nodep, VUseType::INT_FWD_CLASS, nodep->classp()->name());
    }
    فراغ ویکھو(AstCFunc* nodep) override {
        إذا (nodep->user1_سیٹ_Once()) أرجع;
        iterate_And_اگلا_Const_Null(nodep->argsp());
        iterate_And_اگلا_Const_Null(nodep->stmtsp());
    }
    فراغ ویکھو(AstCCall* /*nodep*/) override {}
    فراغ ویکھو(AstCReturn* nodep) override {
        UASSERT_OBJ(!nodep->user1_سیٹ_Once(), nodep, "Visited same return twice");
        iterateConst(nodep->lhsp()->dtypep());
    }
    فراغ ویکھو(Ast_گرہ_DType* nodep) override {
        إذا (nodep->user1_سیٹ_Once()) أرجع;  // Process once
        إذا (nodep->virtRefDTypep()) iterateConst(nodep->virtRefDTypep());
        إذا (nodep->virtRefDType2p()) iterateConst(nodep->virtRefDType2p());

        // Add a CUse for every struct that requires a declaration
        Ast_گرہ_UOr_Struct_DType* ثابت stypep = VN_CAST(nodep->skipRefp(), گرہ_UOr_Struct_DType);
        إذا (stypep && stypep->classOrPackagep()) {
            جوڑو_New_Use(nodep, VUseType::INT_INCLUDE, stypep->classOrPackagep()->name());
            iterateChildrenConst(stypep);
        } وإلا إذا (ثابت AstClassRefDType* ثابت classp
                   = VN_CAST(nodep->skipRefp(), ClassRefDType)) {
            جوڑو_New_Use(nodep, VUseType::INT_FWD_CLASS, classp->name());
        }
    }
    فراغ ویکھو(Ast_گرہ* nodep) override {
        إذا (nodep->user1_سیٹ_Once()) أرجع;  // Process once
        إذا (nodep->dtypep()) iterateConst(nodep->dtypep());
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(AstCell* nodep) override {
        إذا (nodep->user1_سیٹ_Once()) أرجع;  // Process once
        إذا (nodep->modp()->isConstPool()) أرجع;  // Special emit rules
        // Currently no IMP_INCLUDE because we include __Syms which has them all
        جوڑو_New_Use(nodep, VUseType::INT_FWD_CLASS, nodep->modp()->name());
        iterateChildrenConst(nodep);
    }

عام:
    // CONSTRUCTORS
    واضح CUseVisitor(Ast_گرہ_Module* modp)
        : m_modp{modp} {
        iterateConst(modp);

        لكل (ثابت خودکار& ورتیا : m_didUse) {
            AstCUse* ثابت newp = جديد AstCUse{ورتیا.دوجا.پہلا, ورتیا.دوجا.دوجا, ورتیا.پہلا};
            m_modp->جوڑو_Stmtsp(newp);
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
    لكل (Ast_گرہ_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), گرہ_Module)) {
        إذا (modp->isConstPool()) استمر;  // Special emit rules
        // Insert under this module; someday we should e.g. make Ast
        // for each output file and put under that
        CUseVisitor{modp};
    }
    V3Global::dump_Check_Global_درخت("cuse", 0, dump_درخت_Either_Level() >= 3);
}
