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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3CUse.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

// Visit within a module all nodes and data types they reference, finding
// any classes so we can make sure they are defined when Verilated code
// compiles
ವರ್ಗ CUseVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()     -> bool.  True if already visited
    ಧ್ರುವ VNUser1InUse m_inuser1;

    // MEMBERS
    Ast_ಗಂಟು_Module* ಧ್ರುವ m_modp;  // Current module
    std::map<std::string, std::pair<File_ಸಾಲು*, VUseType>> m_didUse;  // What we already used

    // METHODS
    ಶೂನ್ಯ addNewUse(ಧ್ರುವ Ast_ಗಂಟು* nodep, VUseType useType, ಧ್ರುವ string& name) {
        auto e = m_didUse.emplace(name, std::make_pair(nodep->fileline(), useType));
        ಯದಿ (e.second || ((e.first->second.second & useType) != useType)) {
            e.first->second.second = e.first->second.second | useType;
        }
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstClassRefDType* nodep) override {
        addNewUse(nodep, VUseType::INT_FWD_CLASS, nodep->classp()->name());
    }
    ಶೂನ್ಯ visit(AstCFunc* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;
        iterateAndNextConstNull(nodep->argsp());
        iterateAndNextConstNull(nodep->stmtsp());
    }
    ಶೂನ್ಯ visit(AstCCall* /*nodep*/) override {}
    ಶೂನ್ಯ visit(AstCReturn* nodep) override {
        UASSERT_OBJ(!nodep->user1SetOnce(), nodep, "Visited same return twice");
        iterateConst(nodep->lhsp()->dtypep());
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_DType* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;  // Process once
        ಯದಿ (nodep->virtRefDTypep()) iterateConst(nodep->virtRefDTypep());
        ಯದಿ (nodep->virtRefDType2p()) iterateConst(nodep->virtRefDType2p());

        // Add a CUse for every struct that requires a declaration
        Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ stypep = VN_CAST(nodep->skipRefp(), ಗಂಟು_UOr_Struct_DType);
        ಯದಿ (stypep && stypep->classOrPackagep()) {
            addNewUse(nodep, VUseType::INT_INCLUDE, stypep->classOrPackagep()->name());
            iterateChildrenConst(stypep);
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstClassRefDType* ಧ್ರುವ classp
                   = VN_CAST(nodep->skipRefp(), ClassRefDType)) {
            addNewUse(nodep, VUseType::INT_FWD_CLASS, classp->name());
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;  // Process once
        ಯದಿ (nodep->dtypep()) iterateConst(nodep->dtypep());
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(AstCell* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;  // Process once
        ಯದಿ (nodep->modp()->isConstPool()) ಪ್ರತ್ಯಾಗಮ;  // Special emit rules
        // Currently no IMP_INCLUDE because we include __Syms which has them all
        addNewUse(nodep, VUseType::INT_FWD_CLASS, nodep->modp()->name());
        iterateChildrenConst(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit CUseVisitor(Ast_ಗಂಟು_Module* modp)
        : m_modp{modp} {
        iterateConst(modp);

        ಪ್ರತಿ (ಧ್ರುವ auto& used : m_didUse) {
            AstCUse* ಧ್ರುವ newp = ನೂತನ AstCUse{used.second.first, used.second.second, used.first};
            m_modp->addStmtsp(newp);
            UINFO(8, "Insert " << newp);
        }
    }
    ~CUseVisitor() override = ಸ್ವಭಾವ;
    VL_UNCOPYABLE(CUseVisitor);
};

//######################################################################
// Class class functions

ಶೂನ್ಯ V3CUse::cUseAll() {
    UINFO(2, __FUNCTION__ << ":");
    // Call visitor separately for each module, so visitor state is cleared
    ಪ್ರತಿ (Ast_ಗಂಟು_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), ಗಂಟು_Module)) {
        ಯದಿ (modp->isConstPool()) ಅನುವರ್ತ;  // Special emit rules
        // Insert under this module; someday we should e.g. make Ast
        // for each output file and put under that
        CUseVisitor{modp};
    }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("cuse", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
