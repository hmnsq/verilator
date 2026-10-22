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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3CUse.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

// Visit within a module all nodes and data types they reference, finding
// any classes so we can make sure they are defined when Verilated code
// compiles
جماعت CUseVisitor final : عوامی VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()     -> bool.  True if already visited
    ثابت VNUser1_اندر_Use m_inuser1;

    // MEMBERS
    Ast_گرہ_Module* ثابت m_modp;  // Current module
    std::map<std::string, std::pair<File_سطر*, VUseType>> m_didUse;  // What we already used

    // METHODS
    خالی جوڑو_New_Use(ثابت Ast_گرہ* nodep, VUseType useType, ثابت string& name) {
        خودکار e = m_didUse.emplace(name, std::make_pair(nodep->fileline(), useType));
        اگر (e.دوسرا || ((e.پہلا->دوسرا.دوسرا & useType) != useType)) {
            e.پہلا->دوسرا.دوسرا = e.پہلا->دوسرا.دوسرا | useType;
        }
    }

    // VISITORS
    خالی دورہ(AstClassRefDType* nodep) override {
        جوڑو_New_Use(nodep, VUseType::INT_FWD_CLASS, nodep->classp()->name());
    }
    خالی دورہ(AstCFunc* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;
        iterate_And_اگلا_Const_Null(nodep->argsp());
        iterate_And_اگلا_Const_Null(nodep->stmtsp());
    }
    خالی دورہ(AstCCall* /*nodep*/) override {}
    خالی دورہ(AstCReturn* nodep) override {
        UASSERT_OBJ(!nodep->user1_سیٹ_Once(), nodep, "Visited same return twice");
        iterateConst(nodep->lhsp()->dtypep());
    }
    خالی دورہ(Ast_گرہ_DType* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;  // Process once
        اگر (nodep->virtRefDTypep()) iterateConst(nodep->virtRefDTypep());
        اگر (nodep->virtRefDType2p()) iterateConst(nodep->virtRefDType2p());

        // Add a CUse for every struct that requires a declaration
        Ast_گرہ_UOr_Struct_DType* ثابت stypep = VN_CAST(nodep->skipRefp(), گرہ_UOr_Struct_DType);
        اگر (stypep && stypep->classOrPackagep()) {
            جوڑو_New_Use(nodep, VUseType::INT_INCLUDE, stypep->classOrPackagep()->name());
            iterateChildrenConst(stypep);
        } ورنہ اگر (ثابت AstClassRefDType* ثابت classp
                   = VN_CAST(nodep->skipRefp(), ClassRefDType)) {
            جوڑو_New_Use(nodep, VUseType::INT_FWD_CLASS, classp->name());
        }
    }
    خالی دورہ(Ast_گرہ* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;  // Process once
        اگر (nodep->dtypep()) iterateConst(nodep->dtypep());
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstCell* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;  // Process once
        اگر (nodep->modp()->isConstPool()) واپس;  // Special emit rules
        // Currently no IMP_INCLUDE because we include __Syms which has them all
        جوڑو_New_Use(nodep, VUseType::INT_FWD_CLASS, nodep->modp()->name());
        iterateChildrenConst(nodep);
    }

عوامی:
    // CONSTRUCTORS
    صریح CUseVisitor(Ast_گرہ_Module* modp)
        : m_modp{modp} {
        iterateConst(modp);

        برائے (ثابت خودکار& مستعمل : m_didUse) {
            AstCUse* ثابت newp = نیا AstCUse{مستعمل.دوسرا.پہلا, مستعمل.دوسرا.دوسرا, مستعمل.پہلا};
            m_modp->جوڑو_Stmtsp(newp);
            UINFO(8, "Insert " << newp);
        }
    }
    ~CUseVisitor() override = طےشدہ;
    VL_UNCOPYABLE(CUseVisitor);
};

//######################################################################
// Class class functions

خالی V3CUse::cUseAll() {
    UINFO(2, __FUNCTION__ << ":");
    // Call visitor separately for each module, so visitor state is cleared
    برائے (Ast_گرہ_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), گرہ_Module)) {
        اگر (modp->isConstPool()) جاری;  // Special emit rules
        // Insert under this module; someday we should e.g. make Ast
        // for each output file and put under that
        CUseVisitor{modp};
    }
    V3Global::dump_Check_Global_درخت("cuse", 0, dump_درخت_Either_Level() >= 3);
}
