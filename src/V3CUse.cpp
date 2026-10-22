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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3CUse.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

// Visit within a module all nodes and data types they reference, finding
// any classes so we can make sure they are defined when Verilated code
// compiles
वर्ग CUseVisitor final : सार्वजनिक VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()     -> bool.  True if already visited
    अचर VNUser1InUse m_inuser1;

    // MEMBERS
    Ast_गाँठो_Module* अचर m_modp;  // Current module
    std::map<std::string, std::pair<File_पङ्क्ति*, VUseType>> m_didUse;  // What we already used

    // METHODS
    शून्य addNewUse(अचर Ast_गाँठो* nodep, VUseType useType, अचर string& name) {
        स्वतः e = m_didUse.emplace(name, std::make_pair(nodep->fileline(), useType));
        यदि (e.second || ((e.first->second.second & useType) != useType)) {
            e.first->second.second = e.first->second.second | useType;
        }
    }

    // VISITORS
    शून्य visit(AstClassRefDType* nodep) override {
        addNewUse(nodep, VUseType::INT_FWD_CLASS, nodep->classp()->name());
    }
    शून्य visit(AstCFunc* nodep) override {
        यदि (nodep->user1SetOnce()) फर्काऊ;
        iterateAndNextConstNull(nodep->argsp());
        iterateAndNextConstNull(nodep->stmtsp());
    }
    शून्य visit(AstCCall* /*nodep*/) override {}
    शून्य visit(AstCReturn* nodep) override {
        UASSERT_OBJ(!nodep->user1SetOnce(), nodep, "Visited same return twice");
        iterateConst(nodep->lhsp()->dtypep());
    }
    शून्य visit(Ast_गाँठो_DType* nodep) override {
        यदि (nodep->user1SetOnce()) फर्काऊ;  // Process once
        यदि (nodep->virtRefDTypep()) iterateConst(nodep->virtRefDTypep());
        यदि (nodep->virtRefDType2p()) iterateConst(nodep->virtRefDType2p());

        // Add a CUse for every struct that requires a declaration
        Ast_गाँठो_UOr_Struct_DType* अचर stypep = VN_CAST(nodep->skipRefp(), गाँठो_UOr_Struct_DType);
        यदि (stypep && stypep->classOrPackagep()) {
            addNewUse(nodep, VUseType::INT_INCLUDE, stypep->classOrPackagep()->name());
            iterateChildrenConst(stypep);
        } अन्यथा यदि (अचर AstClassRefDType* अचर classp
                   = VN_CAST(nodep->skipRefp(), ClassRefDType)) {
            addNewUse(nodep, VUseType::INT_FWD_CLASS, classp->name());
        }
    }
    शून्य visit(Ast_गाँठो* nodep) override {
        यदि (nodep->user1SetOnce()) फर्काऊ;  // Process once
        यदि (nodep->dtypep()) iterateConst(nodep->dtypep());
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstCell* nodep) override {
        यदि (nodep->user1SetOnce()) फर्काऊ;  // Process once
        यदि (nodep->modp()->isConstPool()) फर्काऊ;  // Special emit rules
        // Currently no IMP_INCLUDE because we include __Syms which has them all
        addNewUse(nodep, VUseType::INT_FWD_CLASS, nodep->modp()->name());
        iterateChildrenConst(nodep);
    }

सार्वजनिक:
    // CONSTRUCTORS
    explicit CUseVisitor(Ast_गाँठो_Module* modp)
        : m_modp{modp} {
        iterateConst(modp);

        लागि (अचर स्वतः& used : m_didUse) {
            AstCUse* अचर newp = नूतन AstCUse{used.second.first, used.second.second, used.first};
            m_modp->addStmtsp(newp);
            UINFO(8, "Insert " << newp);
        }
    }
    ~CUseVisitor() override = स्वभाव;
    VL_UNCOPYABLE(CUseVisitor);
};

//######################################################################
// Class class functions

शून्य V3CUse::cUseAll() {
    UINFO(2, __FUNCTION__ << ":");
    // Call visitor separately for each module, so visitor state is cleared
    लागि (Ast_गाँठो_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), गाँठो_Module)) {
        यदि (modp->isConstPool()) जारी;  // Special emit rules
        // Insert under this module; someday we should e.g. make Ast
        // for each output file and put under that
        CUseVisitor{modp};
    }
    V3Global::dump_Check_Global_रूख("cuse", 0, dump_रूख_Either_Level() >= 3);
}
