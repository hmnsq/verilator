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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3CUse.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

// Visit within a module all nodes and data types they reference, finding
// any classes so we can make sure they are defined when Verilated code
// compiles
வகுப்பு CUseVisitor final : பொது VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()     -> bool.  True if already visited
    மாறிலி VNUser1InUse m_inuser1;

    // MEMBERS
    Ast_முனை_Module* மாறிலி m_modp;  // Current module
    std::map<std::string, std::pair<File_வரி*, VUseType>> m_didUse;  // What we already used

    // METHODS
    வெறுமை addNewUse(மாறிலி Ast_முனை* nodep, VUseType useType, மாறிலி string& name) {
        auto e = m_didUse.emplace(name, std::make_pair(nodep->fileline(), useType));
        எனில் (e.second || ((e.first->second.second & useType) != useType)) {
            e.first->second.second = e.first->second.second | useType;
        }
    }

    // VISITORS
    வெறுமை visit(AstClassRefDType* nodep) override {
        addNewUse(nodep, VUseType::INT_FWD_CLASS, nodep->classp()->name());
    }
    வெறுமை visit(AstCFunc* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;
        iterateAndNextConstNull(nodep->argsp());
        iterateAndNextConstNull(nodep->stmtsp());
    }
    வெறுமை visit(AstCCall* /*nodep*/) override {}
    வெறுமை visit(AstCReturn* nodep) override {
        UASSERT_OBJ(!nodep->user1SetOnce(), nodep, "Visited same return twice");
        iterateConst(nodep->lhsp()->dtypep());
    }
    வெறுமை visit(Ast_முனை_DType* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;  // Process once
        எனில் (nodep->virtRefDTypep()) iterateConst(nodep->virtRefDTypep());
        எனில் (nodep->virtRefDType2p()) iterateConst(nodep->virtRefDType2p());

        // Add a CUse for every struct that requires a declaration
        Ast_முனை_UOr_Struct_DType* மாறிலி stypep = VN_CAST(nodep->skipRefp(), முனை_UOr_Struct_DType);
        எனில் (stypep && stypep->classOrPackagep()) {
            addNewUse(nodep, VUseType::INT_INCLUDE, stypep->classOrPackagep()->name());
            iterateChildrenConst(stypep);
        } இல்லையெனில் எனில் (மாறிலி AstClassRefDType* மாறிலி classp
                   = VN_CAST(nodep->skipRefp(), ClassRefDType)) {
            addNewUse(nodep, VUseType::INT_FWD_CLASS, classp->name());
        }
    }
    வெறுமை visit(Ast_முனை* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;  // Process once
        எனில் (nodep->dtypep()) iterateConst(nodep->dtypep());
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstCell* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;  // Process once
        எனில் (nodep->modp()->isConstPool()) திரும்பு;  // Special emit rules
        // Currently no IMP_INCLUDE because we include __Syms which has them all
        addNewUse(nodep, VUseType::INT_FWD_CLASS, nodep->modp()->name());
        iterateChildrenConst(nodep);
    }

பொது:
    // CONSTRUCTORS
    explicit CUseVisitor(Ast_முனை_Module* modp)
        : m_modp{modp} {
        iterateConst(modp);

        க்கு (மாறிலி auto& used : m_didUse) {
            AstCUse* மாறிலி newp = புதிய AstCUse{used.second.first, used.second.second, used.first};
            m_modp->addStmtsp(newp);
            UINFO(8, "Insert " << newp);
        }
    }
    ~CUseVisitor() override = இயல்பு;
    VL_UNCOPYABLE(CUseVisitor);
};

//######################################################################
// Class class functions

வெறுமை V3CUse::cUseAll() {
    UINFO(2, __FUNCTION__ << ":");
    // Call visitor separately for each module, so visitor state is cleared
    க்கு (Ast_முனை_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), முனை_Module)) {
        எனில் (modp->isConstPool()) தொடர்;  // Special emit rules
        // Insert under this module; someday we should e.g. make Ast
        // for each output file and put under that
        CUseVisitor{modp};
    }
    V3Global::dump_Check_Global_மரம்("cuse", 0, dump_மரம்_Either_Level() >= 3);
}
