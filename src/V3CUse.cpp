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
    ثابت VNUser1InUse m_inuser1;

    // MEMBERS
    Ast_تۈگۈن_Module* ثابت m_modp;  // Current module
    std::map<std::string, std::pair<File_قۇر*, VUseType>> m_didUse;  // What we already used

    // METHODS
    فراغ addNewUse(ثابت Ast_تۈگۈن* nodep, VUseType useType, ثابت string& name) {
        auto e = m_didUse.emplace(name, std::make_pair(nodep->fileline(), useType));
        إذا (e.second || ((e.first->second.second & useType) != useType)) {
            e.first->second.second = e.first->second.second | useType;
        }
    }

    // VISITORS
    فراغ visit(AstClassRefDType* nodep) override {
        addNewUse(nodep, VUseType::INT_FWD_CLASS, nodep->classp()->name());
    }
    فراغ visit(AstCFunc* nodep) override {
        إذا (nodep->user1SetOnce()) أرجع;
        iterateAndNextConstNull(nodep->argsp());
        iterateAndNextConstNull(nodep->stmtsp());
    }
    فراغ visit(AstCCall* /*nodep*/) override {}
    فراغ visit(AstCReturn* nodep) override {
        UASSERT_OBJ(!nodep->user1SetOnce(), nodep, "Visited same return twice");
        iterateConst(nodep->lhsp()->dtypep());
    }
    فراغ visit(Ast_تۈگۈن_DType* nodep) override {
        إذا (nodep->user1SetOnce()) أرجع;  // Process once
        إذا (nodep->virtRefDTypep()) iterateConst(nodep->virtRefDTypep());
        إذا (nodep->virtRefDType2p()) iterateConst(nodep->virtRefDType2p());

        // Add a CUse for every struct that requires a declaration
        Ast_تۈگۈن_UOr_Struct_DType* ثابت stypep = VN_CAST(nodep->skipRefp(), تۈگۈن_UOr_Struct_DType);
        إذا (stypep && stypep->classOrPackagep()) {
            addNewUse(nodep, VUseType::INT_INCLUDE, stypep->classOrPackagep()->name());
            iterateChildrenConst(stypep);
        } وإلا إذا (ثابت AstClassRefDType* ثابت classp
                   = VN_CAST(nodep->skipRefp(), ClassRefDType)) {
            addNewUse(nodep, VUseType::INT_FWD_CLASS, classp->name());
        }
    }
    فراغ visit(Ast_تۈگۈن* nodep) override {
        إذا (nodep->user1SetOnce()) أرجع;  // Process once
        إذا (nodep->dtypep()) iterateConst(nodep->dtypep());
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstCell* nodep) override {
        إذا (nodep->user1SetOnce()) أرجع;  // Process once
        إذا (nodep->modp()->isConstPool()) أرجع;  // Special emit rules
        // Currently no IMP_INCLUDE because we include __Syms which has them all
        addNewUse(nodep, VUseType::INT_FWD_CLASS, nodep->modp()->name());
        iterateChildrenConst(nodep);
    }

عام:
    // CONSTRUCTORS
    explicit CUseVisitor(Ast_تۈگۈن_Module* modp)
        : m_modp{modp} {
        iterateConst(modp);

        لكل (ثابت auto& used : m_didUse) {
            AstCUse* ثابت newp = جديد AstCUse{used.second.first, used.second.second, used.first};
            m_modp->addStmtsp(newp);
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
    لكل (Ast_تۈگۈن_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), تۈگۈن_Module)) {
        إذا (modp->isConstPool()) استمر;  // Special emit rules
        // Insert under this module; someday we should e.g. make Ast
        // for each output file and put under that
        CUseVisitor{modp};
    }
    V3Global::dump_Check_Global_دەرەخ("cuse", 0, dump_دەرەخ_Either_Level() >= 3);
}
