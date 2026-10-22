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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3CUse.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

// Visit within a module all nodes and data types they reference, finding
// any classes so we can make sure they are defined when Verilated code
// compiles
ଶ୍ରେଣୀ CUseVisitor final : ସର୍ବସାଧାରଣ VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()     -> bool.  True if already visited
    ଅଚଳ VNUser1InUse m_inuser1;

    // MEMBERS
    Ast_ଗଣ୍ଠି_Module* ଅଚଳ m_modp;  // Current module
    std::map<std::string, std::pair<File_ଧାଡ଼ି*, VUseType>> m_didUse;  // What we already used

    // METHODS
    ଶୂନ୍ୟ addNewUse(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, VUseType useType, ଅଚଳ string& name) {
        auto e = m_didUse.emplace(name, std::make_pair(nodep->fileline(), useType));
        ଯଦି (e.second || ((e.first->second.second & useType) != useType)) {
            e.first->second.second = e.first->second.second | useType;
        }
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstClassRefDType* nodep) override {
        addNewUse(nodep, VUseType::INT_FWD_CLASS, nodep->classp()->name());
    }
    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;
        iterateAndNextConstNull(nodep->argsp());
        iterateAndNextConstNull(nodep->stmtsp());
    }
    ଶୂନ୍ୟ visit(AstCCall* /*nodep*/) override {}
    ଶୂନ୍ୟ visit(AstCReturn* nodep) override {
        UASSERT_OBJ(!nodep->user1SetOnce(), nodep, "Visited same return twice");
        iterateConst(nodep->lhsp()->dtypep());
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_DType* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;  // Process once
        ଯଦି (nodep->virtRefDTypep()) iterateConst(nodep->virtRefDTypep());
        ଯଦି (nodep->virtRefDType2p()) iterateConst(nodep->virtRefDType2p());

        // Add a CUse for every struct that requires a declaration
        Ast_ଗଣ୍ଠି_UOr_Struct_DType* ଅଚଳ stypep = VN_CAST(nodep->skipRefp(), ଗଣ୍ଠି_UOr_Struct_DType);
        ଯଦି (stypep && stypep->classOrPackagep()) {
            addNewUse(nodep, VUseType::INT_INCLUDE, stypep->classOrPackagep()->name());
            iterateChildrenConst(stypep);
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstClassRefDType* ଅଚଳ classp
                   = VN_CAST(nodep->skipRefp(), ClassRefDType)) {
            addNewUse(nodep, VUseType::INT_FWD_CLASS, classp->name());
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;  // Process once
        ଯଦି (nodep->dtypep()) iterateConst(nodep->dtypep());
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstCell* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;  // Process once
        ଯଦି (nodep->modp()->isConstPool()) ଫେରନ୍ତୁ;  // Special emit rules
        // Currently no IMP_INCLUDE because we include __Syms which has them all
        addNewUse(nodep, VUseType::INT_FWD_CLASS, nodep->modp()->name());
        iterateChildrenConst(nodep);
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit CUseVisitor(Ast_ଗଣ୍ଠି_Module* modp)
        : m_modp{modp} {
        iterateConst(modp);

        ପାଇଁ (ଅଚଳ auto& used : m_didUse) {
            AstCUse* ଅଚଳ newp = ନୂତନ AstCUse{used.second.first, used.second.second, used.first};
            m_modp->addStmtsp(newp);
            UINFO(8, "Insert " << newp);
        }
    }
    ~CUseVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    VL_UNCOPYABLE(CUseVisitor);
};

//######################################################################
// Class class functions

ଶୂନ୍ୟ V3CUse::cUseAll() {
    UINFO(2, __FUNCTION__ << ":");
    // Call visitor separately for each module, so visitor state is cleared
    ପାଇଁ (Ast_ଗଣ୍ଠି_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), ଗଣ୍ଠି_Module)) {
        ଯଦି (modp->isConstPool()) ଜାରି;  // Special emit rules
        // Insert under this module; someday we should e.g. make Ast
        // for each output file and put under that
        CUseVisitor{modp};
    }
    V3Global::dump_Check_Global_ବୃକ୍ଷ("cuse", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
