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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3CUse.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

// Visit within a module all nodes and data types they reference, finding
// any classes so we can make sure they are defined when Verilated code
// compiles
વર્ગ CUseVisitor final : જાહેર VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()     -> bool.  True if already visited
    અચલ VNUser1InUse m_inuser1;

    // MEMBERS
    Ast_ગાંઠ_Module* અચલ m_modp;  // Current module
    std::map<std::string, std::pair<File_લીટી*, VUseType>> m_didUse;  // What we already used

    // METHODS
    શૂન્ય addNewUse(અચલ Ast_ગાંઠ* nodep, VUseType useType, અચલ string& name) {
        auto e = m_didUse.emplace(name, std::make_pair(nodep->fileline(), useType));
        જો (e.second || ((e.first->second.second & useType) != useType)) {
            e.first->second.second = e.first->second.second | useType;
        }
    }

    // VISITORS
    શૂન્ય visit(AstClassRefDType* nodep) override {
        addNewUse(nodep, VUseType::INT_FWD_CLASS, nodep->classp()->name());
    }
    શૂન્ય visit(AstCFunc* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;
        iterateAndNextConstNull(nodep->argsp());
        iterateAndNextConstNull(nodep->stmtsp());
    }
    શૂન્ય visit(AstCCall* /*nodep*/) override {}
    શૂન્ય visit(AstCReturn* nodep) override {
        UASSERT_OBJ(!nodep->user1SetOnce(), nodep, "Visited same return twice");
        iterateConst(nodep->lhsp()->dtypep());
    }
    શૂન્ય visit(Ast_ગાંઠ_DType* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;  // Process once
        જો (nodep->virtRefDTypep()) iterateConst(nodep->virtRefDTypep());
        જો (nodep->virtRefDType2p()) iterateConst(nodep->virtRefDType2p());

        // Add a CUse for every struct that requires a declaration
        Ast_ગાંઠ_UOr_Struct_DType* અચલ stypep = VN_CAST(nodep->skipRefp(), ગાંઠ_UOr_Struct_DType);
        જો (stypep && stypep->classOrPackagep()) {
            addNewUse(nodep, VUseType::INT_INCLUDE, stypep->classOrPackagep()->name());
            iterateChildrenConst(stypep);
        } નહીંતર જો (અચલ AstClassRefDType* અચલ classp
                   = VN_CAST(nodep->skipRefp(), ClassRefDType)) {
            addNewUse(nodep, VUseType::INT_FWD_CLASS, classp->name());
        }
    }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;  // Process once
        જો (nodep->dtypep()) iterateConst(nodep->dtypep());
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstCell* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;  // Process once
        જો (nodep->modp()->isConstPool()) પાછા;  // Special emit rules
        // Currently no IMP_INCLUDE because we include __Syms which has them all
        addNewUse(nodep, VUseType::INT_FWD_CLASS, nodep->modp()->name());
        iterateChildrenConst(nodep);
    }

જાહેર:
    // CONSTRUCTORS
    explicit CUseVisitor(Ast_ગાંઠ_Module* modp)
        : m_modp{modp} {
        iterateConst(modp);

        માટે (અચલ auto& used : m_didUse) {
            AstCUse* અચલ newp = નવું AstCUse{used.second.first, used.second.second, used.first};
            m_modp->addStmtsp(newp);
            UINFO(8, "Insert " << newp);
        }
    }
    ~CUseVisitor() override = મૂળભૂત;
    VL_UNCOPYABLE(CUseVisitor);
};

//######################################################################
// Class class functions

શૂન્ય V3CUse::cUseAll() {
    UINFO(2, __FUNCTION__ << ":");
    // Call visitor separately for each module, so visitor state is cleared
    માટે (Ast_ગાંઠ_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), ગાંઠ_Module)) {
        જો (modp->isConstPool()) ચાલુ;  // Special emit rules
        // Insert under this module; someday we should e.g. make Ast
        // for each output file and put under that
        CUseVisitor{modp};
    }
    V3Global::dump_Check_Global_વૃક્ષ("cuse", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
