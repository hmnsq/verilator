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
    ثابت VNUser1_درون_Use m_inuser1;

    // MEMBERS
    Ast_گره_Module* ثابت m_modp;  // Current module
    std::map<std::string, std::pair<File_سطر*, VUseType>> m_didUse;  // What we already used

    // METHODS
    فراغ جمع_کن_New_Use(ثابت Ast_گره* nodep, VUseType useType, ثابت string& name) {
        خودکار e = m_didUse.emplace(name, std::make_pair(nodep->fileline(), useType));
        إذا (e.دومی || ((e.اولی->دومی.دومی & useType) != useType)) {
            e.اولی->دومی.دومی = e.اولی->دومی.دومی | useType;
        }
    }

    // VISITORS
    فراغ بازدید(AstClassRefDType* nodep) override {
        جمع_کن_New_Use(nodep, VUseType::INT_FWD_CLASS, nodep->classp()->name());
    }
    فراغ بازدید(AstCFunc* nodep) override {
        إذا (nodep->user1_مجموعه_Once()) أرجع;
        iterate_And_بعدی_Const_Null(nodep->argsp());
        iterate_And_بعدی_Const_Null(nodep->stmtsp());
    }
    فراغ بازدید(AstCCall* /*nodep*/) override {}
    فراغ بازدید(AstCReturn* nodep) override {
        UASSERT_OBJ(!nodep->user1_مجموعه_Once(), nodep, "Visited same return twice");
        iterateConst(nodep->lhsp()->dtypep());
    }
    فراغ بازدید(Ast_گره_DType* nodep) override {
        إذا (nodep->user1_مجموعه_Once()) أرجع;  // Process once
        إذا (nodep->virtRefDTypep()) iterateConst(nodep->virtRefDTypep());
        إذا (nodep->virtRefDType2p()) iterateConst(nodep->virtRefDType2p());

        // Add a CUse for every struct that requires a declaration
        Ast_گره_UOr_Struct_DType* ثابت stypep = VN_CAST(nodep->skipRefp(), گره_UOr_Struct_DType);
        إذا (stypep && stypep->classOrPackagep()) {
            جمع_کن_New_Use(nodep, VUseType::INT_INCLUDE, stypep->classOrPackagep()->name());
            iterateChildrenConst(stypep);
        } وإلا إذا (ثابت AstClassRefDType* ثابت classp
                   = VN_CAST(nodep->skipRefp(), ClassRefDType)) {
            جمع_کن_New_Use(nodep, VUseType::INT_FWD_CLASS, classp->name());
        }
    }
    فراغ بازدید(Ast_گره* nodep) override {
        إذا (nodep->user1_مجموعه_Once()) أرجع;  // Process once
        إذا (nodep->dtypep()) iterateConst(nodep->dtypep());
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(AstCell* nodep) override {
        إذا (nodep->user1_مجموعه_Once()) أرجع;  // Process once
        إذا (nodep->modp()->isConstPool()) أرجع;  // Special emit rules
        // Currently no IMP_INCLUDE because we include __Syms which has them all
        جمع_کن_New_Use(nodep, VUseType::INT_FWD_CLASS, nodep->modp()->name());
        iterateChildrenConst(nodep);
    }

عام:
    // CONSTRUCTORS
    صریح CUseVisitor(Ast_گره_Module* modp)
        : m_modp{modp} {
        iterateConst(modp);

        لكل (ثابت خودکار& استفاده_شده : m_didUse) {
            AstCUse* ثابت newp = جديد AstCUse{استفاده_شده.دومی.اولی, استفاده_شده.دومی.دومی, استفاده_شده.اولی};
            m_modp->جمع_کن_Stmtsp(newp);
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
    لكل (Ast_گره_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), گره_Module)) {
        إذا (modp->isConstPool()) استمر;  // Special emit rules
        // Insert under this module; someday we should e.g. make Ast
        // for each output file and put under that
        CUseVisitor{modp};
    }
    V3Global::dump_Check_Global_درخت("cuse", 0, dump_درخت_Either_Level() >= 3);
}
