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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3CUse.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

// Visit within a module all nodes and data types they reference, finding
// any classes so we can make sure they are defined when Verilated code
// compiles
വർഗ്ഗം CUseVisitor final : പൊതു VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()     -> bool.  True if already visited
    സ്ഥിരം VNUser1InUse m_inuser1;

    // MEMBERS
    Ast_കണ്ണി_Module* സ്ഥിരം m_modp;  // Current module
    std::map<std::string, std::pair<File_വരി*, VUseType>> m_didUse;  // What we already used

    // METHODS
    ശൂന്യം addNewUse(സ്ഥിരം Ast_കണ്ണി* nodep, VUseType useType, സ്ഥിരം string& name) {
        auto e = m_didUse.emplace(name, std::make_pair(nodep->fileline(), useType));
        എങ്കിൽ (e.second || ((e.first->second.second & useType) != useType)) {
            e.first->second.second = e.first->second.second | useType;
        }
    }

    // VISITORS
    ശൂന്യം visit(AstClassRefDType* nodep) override {
        addNewUse(nodep, VUseType::INT_FWD_CLASS, nodep->classp()->name());
    }
    ശൂന്യം visit(AstCFunc* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;
        iterateAndNextConstNull(nodep->argsp());
        iterateAndNextConstNull(nodep->stmtsp());
    }
    ശൂന്യം visit(AstCCall* /*nodep*/) override {}
    ശൂന്യം visit(AstCReturn* nodep) override {
        UASSERT_OBJ(!nodep->user1SetOnce(), nodep, "Visited same return twice");
        iterateConst(nodep->lhsp()->dtypep());
    }
    ശൂന്യം visit(Ast_കണ്ണി_DType* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;  // Process once
        എങ്കിൽ (nodep->virtRefDTypep()) iterateConst(nodep->virtRefDTypep());
        എങ്കിൽ (nodep->virtRefDType2p()) iterateConst(nodep->virtRefDType2p());

        // Add a CUse for every struct that requires a declaration
        Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം stypep = VN_CAST(nodep->skipRefp(), കണ്ണി_UOr_Struct_DType);
        എങ്കിൽ (stypep && stypep->classOrPackagep()) {
            addNewUse(nodep, VUseType::INT_INCLUDE, stypep->classOrPackagep()->name());
            iterateChildrenConst(stypep);
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstClassRefDType* സ്ഥിരം classp
                   = VN_CAST(nodep->skipRefp(), ClassRefDType)) {
            addNewUse(nodep, VUseType::INT_FWD_CLASS, classp->name());
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;  // Process once
        എങ്കിൽ (nodep->dtypep()) iterateConst(nodep->dtypep());
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstCell* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;  // Process once
        എങ്കിൽ (nodep->modp()->isConstPool()) തിരികെ;  // Special emit rules
        // Currently no IMP_INCLUDE because we include __Syms which has them all
        addNewUse(nodep, VUseType::INT_FWD_CLASS, nodep->modp()->name());
        iterateChildrenConst(nodep);
    }

പൊതു:
    // CONSTRUCTORS
    explicit CUseVisitor(Ast_കണ്ണി_Module* modp)
        : m_modp{modp} {
        iterateConst(modp);

        വേണ്ടി (സ്ഥിരം auto& used : m_didUse) {
            AstCUse* സ്ഥിരം newp = പുതിയ AstCUse{used.second.first, used.second.second, used.first};
            m_modp->addStmtsp(newp);
            UINFO(8, "Insert " << newp);
        }
    }
    ~CUseVisitor() override = സ്ഥിരസ്ഥിതി;
    VL_UNCOPYABLE(CUseVisitor);
};

//######################################################################
// Class class functions

ശൂന്യം V3CUse::cUseAll() {
    UINFO(2, __FUNCTION__ << ":");
    // Call visitor separately for each module, so visitor state is cleared
    വേണ്ടി (Ast_കണ്ണി_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), കണ്ണി_Module)) {
        എങ്കിൽ (modp->isConstPool()) തുടരുക;  // Special emit rules
        // Insert under this module; someday we should e.g. make Ast
        // for each output file and put under that
        CUseVisitor{modp};
    }
    V3Global::dump_Check_Global_വൃക്ഷം("cuse", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
