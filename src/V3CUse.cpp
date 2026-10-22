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
رده CUseVisitor final : عمومی VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()     -> bool.  True if already visited
    ثابت VNUser1InUse m_inuser1;

    // MEMBERS
    Ast_گره_Module* ثابت m_modp;  // Current module
    std::map<std::string, std::pair<File_سطر*, VUseType>> m_didUse;  // What we already used

    // METHODS
    تهی addNewUse(ثابت Ast_گره* nodep, VUseType useType, ثابت string& name) {
        auto e = m_didUse.emplace(name, std::make_pair(nodep->fileline(), useType));
        اگر (e.second || ((e.first->second.second & useType) != useType)) {
            e.first->second.second = e.first->second.second | useType;
        }
    }

    // VISITORS
    تهی visit(AstClassRefDType* nodep) override {
        addNewUse(nodep, VUseType::INT_FWD_CLASS, nodep->classp()->name());
    }
    تهی visit(AstCFunc* nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;
        iterateAndNextConstNull(nodep->argsp());
        iterateAndNextConstNull(nodep->stmtsp());
    }
    تهی visit(AstCCall* /*nodep*/) override {}
    تهی visit(AstCReturn* nodep) override {
        UASSERT_OBJ(!nodep->user1SetOnce(), nodep, "Visited same return twice");
        iterateConst(nodep->lhsp()->dtypep());
    }
    تهی visit(Ast_گره_DType* nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;  // Process once
        اگر (nodep->virtRefDTypep()) iterateConst(nodep->virtRefDTypep());
        اگر (nodep->virtRefDType2p()) iterateConst(nodep->virtRefDType2p());

        // Add a CUse for every struct that requires a declaration
        Ast_گره_UOr_Struct_DType* ثابت stypep = VN_CAST(nodep->skipRefp(), گره_UOr_Struct_DType);
        اگر (stypep && stypep->classOrPackagep()) {
            addNewUse(nodep, VUseType::INT_INCLUDE, stypep->classOrPackagep()->name());
            iterateChildrenConst(stypep);
        } وگرنه اگر (ثابت AstClassRefDType* ثابت classp
                   = VN_CAST(nodep->skipRefp(), ClassRefDType)) {
            addNewUse(nodep, VUseType::INT_FWD_CLASS, classp->name());
        }
    }
    تهی visit(Ast_گره* nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;  // Process once
        اگر (nodep->dtypep()) iterateConst(nodep->dtypep());
        iterateChildrenConst(nodep);
    }
    تهی visit(AstCell* nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;  // Process once
        اگر (nodep->modp()->isConstPool()) بازگشت;  // Special emit rules
        // Currently no IMP_INCLUDE because we include __Syms which has them all
        addNewUse(nodep, VUseType::INT_FWD_CLASS, nodep->modp()->name());
        iterateChildrenConst(nodep);
    }

عمومی:
    // CONSTRUCTORS
    explicit CUseVisitor(Ast_گره_Module* modp)
        : m_modp{modp} {
        iterateConst(modp);

        برای (ثابت auto& used : m_didUse) {
            AstCUse* ثابت newp = جدید AstCUse{used.second.first, used.second.second, used.first};
            m_modp->addStmtsp(newp);
            UINFO(8, "Insert " << newp);
        }
    }
    ~CUseVisitor() override = پیشفرض;
    VL_UNCOPYABLE(CUseVisitor);
};

//######################################################################
// Class class functions

تهی V3CUse::cUseAll() {
    UINFO(2, __FUNCTION__ << ":");
    // Call visitor separately for each module, so visitor state is cleared
    برای (Ast_گره_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), گره_Module)) {
        اگر (modp->isConstPool()) ادامه;  // Special emit rules
        // Insert under this module; someday we should e.g. make Ast
        // for each output file and put under that
        CUseVisitor{modp};
    }
    V3Global::dump_Check_Global_درخت("cuse", 0, dump_درخت_Either_Level() >= 3);
}
