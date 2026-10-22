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
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;

    // MEMBERS
    Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ m_modp;  // Current module
    std::map<std::string, std::pair<File_ಸಾಲು*, VUseType>> m_didUse;  // What we already used

    // METHODS
    ಶೂನ್ಯ ಕೂಡಿಸು_New_Use(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, VUseType useType, ಸ್ಥಿರಾಂಕ string& name) {
        ಸ್ವಯಂಚಾಲಿತ e = m_didUse.emplace(name, std::make_pair(nodep->fileline(), useType));
        ಆದರೆ (e.ಎರಡನೇ || ((e.ಮೊದಲ->ಎರಡನೇ.ಎರಡನೇ & useType) != useType)) {
            e.ಮೊದಲ->ಎರಡನೇ.ಎರಡನೇ = e.ಮೊದಲ->ಎರಡನೇ.ಎರಡನೇ | useType;
        }
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstClassRefDType* nodep) override {
        ಕೂಡಿಸು_New_Use(nodep, VUseType::INT_FWD_CLASS, nodep->classp()->name());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->argsp());
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->stmtsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCCall* /*nodep*/) override {}
    ಶೂನ್ಯ ಭೇಟಿ(AstCReturn* nodep) override {
        UASSERT_OBJ(!nodep->user1_ಗಣ_Once(), nodep, "Visited same return twice");
        iterateConst(nodep->lhsp()->dtypep());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_DType* nodep) override {
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;  // Process once
        ಆದರೆ (nodep->virtRefDTypep()) iterateConst(nodep->virtRefDTypep());
        ಆದರೆ (nodep->virtRefDType2p()) iterateConst(nodep->virtRefDType2p());

        // Add a CUse for every struct that requires a declaration
        Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ stypep = VN_CAST(nodep->skipRefp(), ಗಂಟು_UOr_Struct_DType);
        ಆದರೆ (stypep && stypep->classOrPackagep()) {
            ಕೂಡಿಸು_New_Use(nodep, VUseType::INT_INCLUDE, stypep->classOrPackagep()->name());
            iterateChildrenConst(stypep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstClassRefDType* ಸ್ಥಿರಾಂಕ classp
                   = VN_CAST(nodep->skipRefp(), ClassRefDType)) {
            ಕೂಡಿಸು_New_Use(nodep, VUseType::INT_FWD_CLASS, classp->name());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;  // Process once
        ಆದರೆ (nodep->dtypep()) iterateConst(nodep->dtypep());
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCell* nodep) override {
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;  // Process once
        ಆದರೆ (nodep->modp()->isConstPool()) ಹಿಂತಿರುಗಿ;  // Special emit rules
        // Currently no IMP_INCLUDE because we include __Syms which has them all
        ಕೂಡಿಸು_New_Use(nodep, VUseType::INT_FWD_CLASS, nodep->modp()->name());
        iterateChildrenConst(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ CUseVisitor(Ast_ಗಂಟು_Module* modp)
        : m_modp{modp} {
        iterateConst(modp);

        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ಬಳಸಿದ : m_didUse) {
            AstCUse* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstCUse{ಬಳಸಿದ.ಎರಡನೇ.ಮೊದಲ, ಬಳಸಿದ.ಎರಡನೇ.ಎರಡನೇ, ಬಳಸಿದ.ಮೊದಲ};
            m_modp->ಕೂಡಿಸು_Stmtsp(newp);
            UINFO(8, "Insert " << newp);
        }
    }
    ~CUseVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
    VL_UNCOPYABLE(CUseVisitor);
};

//######################################################################
// Class class functions

ಶೂನ್ಯ V3CUse::cUseAll() {
    UINFO(2, __FUNCTION__ << ":");
    // Call visitor separately for each module, so visitor state is cleared
    ಗಾಗಿ (Ast_ಗಂಟು_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), ಗಂಟು_Module)) {
        ಆದರೆ (modp->isConstPool()) ಮುಂದುವರಿ;  // Special emit rules
        // Insert under this module; someday we should e.g. make Ast
        // for each output file and put under that
        CUseVisitor{modp};
    }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("cuse", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
