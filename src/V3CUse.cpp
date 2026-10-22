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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3CUse.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

// Visit within a module all nodes and data types they reference, finding
// any classes so we can make sure they are defined when Verilated code
// compiles
श्रेणी CUseVisitor final : खुला VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()     -> bool.  True if already visited
    स्थिर VNUser1InUse m_inuser1;

    // MEMBERS
    Ast_गाँठ_Module* स्थिर m_modp;  // Current module
    std::map<std::string, std::pair<File_पंक्ति*, VUseType>> m_didUse;  // What we already used

    // METHODS
    व्योम addNewUse(स्थिर Ast_गाँठ* nodep, VUseType useType, स्थिर string& name) {
        स्वतः e = m_didUse.emplace(name, std::make_pair(nodep->fileline(), useType));
        अगर (e.second || ((e.first->second.second & useType) != useType)) {
            e.first->second.second = e.first->second.second | useType;
        }
    }

    // VISITORS
    व्योम visit(AstClassRefDType* nodep) override {
        addNewUse(nodep, VUseType::INT_FWD_CLASS, nodep->classp()->name());
    }
    व्योम visit(AstCFunc* nodep) override {
        अगर (nodep->user1SetOnce()) वापस;
        iterate_And_अग्र_Const_Null(nodep->argsp());
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
    }
    व्योम visit(AstCCall* /*nodep*/) override {}
    व्योम visit(AstCReturn* nodep) override {
        UASSERT_OBJ(!nodep->user1SetOnce(), nodep, "Visited same return twice");
        iterateConst(nodep->lhsp()->dtypep());
    }
    व्योम visit(Ast_गाँठ_DType* nodep) override {
        अगर (nodep->user1SetOnce()) वापस;  // Process once
        अगर (nodep->virtRefDTypep()) iterateConst(nodep->virtRefDTypep());
        अगर (nodep->virtRefDType2p()) iterateConst(nodep->virtRefDType2p());

        // Add a CUse for every struct that requires a declaration
        Ast_गाँठ_UOr_Struct_DType* स्थिर stypep = VN_CAST(nodep->skipRefp(), गाँठ_UOr_Struct_DType);
        अगर (stypep && stypep->classOrPackagep()) {
            addNewUse(nodep, VUseType::INT_INCLUDE, stypep->classOrPackagep()->name());
            iterateChildrenConst(stypep);
        } अन्यथा अगर (स्थिर AstClassRefDType* स्थिर classp
                   = VN_CAST(nodep->skipRefp(), ClassRefDType)) {
            addNewUse(nodep, VUseType::INT_FWD_CLASS, classp->name());
        }
    }
    व्योम visit(Ast_गाँठ* nodep) override {
        अगर (nodep->user1SetOnce()) वापस;  // Process once
        अगर (nodep->dtypep()) iterateConst(nodep->dtypep());
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstCell* nodep) override {
        अगर (nodep->user1SetOnce()) वापस;  // Process once
        अगर (nodep->modp()->isConstPool()) वापस;  // Special emit rules
        // Currently no IMP_INCLUDE because we include __Syms which has them all
        addNewUse(nodep, VUseType::INT_FWD_CLASS, nodep->modp()->name());
        iterateChildrenConst(nodep);
    }

खुला:
    // CONSTRUCTORS
    explicit CUseVisitor(Ast_गाँठ_Module* modp)
        : m_modp{modp} {
        iterateConst(modp);

        क्रम (स्थिर स्वतः& used : m_didUse) {
            AstCUse* स्थिर newp = नया AstCUse{used.second.first, used.second.second, used.first};
            m_modp->addStmtsp(newp);
            UINFO(8, "Insert " << newp);
        }
    }
    ~CUseVisitor() override = शेष;
    VL_UNCOPYABLE(CUseVisitor);
};

//######################################################################
// Class class functions

व्योम V3CUse::cUseAll() {
    UINFO(2, __FUNCTION__ << ":");
    // Call visitor separately for each module, so visitor state is cleared
    क्रम (Ast_गाँठ_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), गाँठ_Module)) {
        अगर (modp->isConstPool()) जारी;  // Special emit rules
        // Insert under this module; someday we should e.g. make Ast
        // for each output file and put under that
        CUseVisitor{modp};
    }
    V3Global::dump_Check_Global_वृक्ष("cuse", 0, dump_वृक्ष_Either_Level() >= 3);
}
