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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3CUse.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

// Visit within a module all nodes and data types they reference, finding
// any classes so we can make sure they are defined when Verilated code
// compiles
ਵਰਗ CUseVisitor final : ਜਨਤਕ VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()     -> bool.  True if already visited
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;

    // MEMBERS
    Ast_ਗੰਢ_Module* ਅਚਲ m_modp;  // Current module
    std::map<std::string, std::pair<File_ਸਤਰ*, VUseType>> m_didUse;  // What we already used

    // METHODS
    ਖਾਲੀ ਜੋੜੋ_New_Use(ਅਚਲ Ast_ਗੰਢ* nodep, VUseType useType, ਅਚਲ string& name) {
        ਸਵੈਚਾਲਿਤ e = m_didUse.emplace(name, std::make_pair(nodep->fileline(), useType));
        ਜੇ (e.ਦੂਜਾ || ((e.ਪਹਿਲਾ->ਦੂਜਾ.ਦੂਜਾ & useType) != useType)) {
            e.ਪਹਿਲਾ->ਦੂਜਾ.ਦੂਜਾ = e.ਪਹਿਲਾ->ਦੂਜਾ.ਦੂਜਾ | useType;
        }
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstClassRefDType* nodep) override {
        ਜੋੜੋ_New_Use(nodep, VUseType::INT_FWD_CLASS, nodep->classp()->name());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;
        iterate_And_ਅਗਲਾ_Const_Null(nodep->argsp());
        iterate_And_ਅਗਲਾ_Const_Null(nodep->stmtsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCCall* /*nodep*/) override {}
    ਖਾਲੀ ਦੇਖੋ(AstCReturn* nodep) override {
        UASSERT_OBJ(!nodep->user1_ਸੈੱਟ_Once(), nodep, "Visited same return twice");
        iterateConst(nodep->lhsp()->dtypep());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_DType* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;  // Process once
        ਜੇ (nodep->virtRefDTypep()) iterateConst(nodep->virtRefDTypep());
        ਜੇ (nodep->virtRefDType2p()) iterateConst(nodep->virtRefDType2p());

        // Add a CUse for every struct that requires a declaration
        Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ stypep = VN_CAST(nodep->skipRefp(), ਗੰਢ_UOr_Struct_DType);
        ਜੇ (stypep && stypep->classOrPackagep()) {
            ਜੋੜੋ_New_Use(nodep, VUseType::INT_INCLUDE, stypep->classOrPackagep()->name());
            iterateChildrenConst(stypep);
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstClassRefDType* ਅਚਲ classp
                   = VN_CAST(nodep->skipRefp(), ClassRefDType)) {
            ਜੋੜੋ_New_Use(nodep, VUseType::INT_FWD_CLASS, classp->name());
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;  // Process once
        ਜੇ (nodep->dtypep()) iterateConst(nodep->dtypep());
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCell* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;  // Process once
        ਜੇ (nodep->modp()->isConstPool()) ਵਾਪਸ;  // Special emit rules
        // Currently no IMP_INCLUDE because we include __Syms which has them all
        ਜੋੜੋ_New_Use(nodep, VUseType::INT_FWD_CLASS, nodep->modp()->name());
        iterateChildrenConst(nodep);
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ CUseVisitor(Ast_ਗੰਢ_Module* modp)
        : m_modp{modp} {
        iterateConst(modp);

        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਵਰਤਿਆ : m_didUse) {
            AstCUse* ਅਚਲ newp = ਨਵਾਂ AstCUse{ਵਰਤਿਆ.ਦੂਜਾ.ਪਹਿਲਾ, ਵਰਤਿਆ.ਦੂਜਾ.ਦੂਜਾ, ਵਰਤਿਆ.ਪਹਿਲਾ};
            m_modp->ਜੋੜੋ_Stmtsp(newp);
            UINFO(8, "Insert " << newp);
        }
    }
    ~CUseVisitor() override = ਮੂਲ;
    VL_UNCOPYABLE(CUseVisitor);
};

//######################################################################
// Class class functions

ਖਾਲੀ V3CUse::cUseAll() {
    UINFO(2, __FUNCTION__ << ":");
    // Call visitor separately for each module, so visitor state is cleared
    ਲਈ (Ast_ਗੰਢ_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), ਗੰਢ_Module)) {
        ਜੇ (modp->isConstPool()) ਜਾਰੀ;  // Special emit rules
        // Insert under this module; someday we should e.g. make Ast
        // for each output file and put under that
        CUseVisitor{modp};
    }
    V3Global::dump_Check_Global_ਰੁੱਖ("cuse", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
