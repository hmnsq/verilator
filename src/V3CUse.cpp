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
    ध्रुव VNUser1_अन्तर्_Use m_inuser1;

    // MEMBERS
    Ast_ग्रन्थिः_Module* ध्रुव m_modp;  // Current module
    std::map<std::string, std::pair<File_पङ्क्तिः*, VUseType>> m_didUse;  // What we already used

    // METHODS
    शून्य सङ्कलय_New_Use(ध्रुव Ast_ग्रन्थिः* nodep, VUseType useType, ध्रुव string& name) {
        स्वचालित e = m_didUse.emplace(name, std::make_pair(nodep->fileline(), useType));
        यदि (e.second || ((e.first->second.second & useType) != useType)) {
            e.first->second.second = e.first->second.second | useType;
        }
    }

    // VISITORS
    शून्य दर्शय(AstClassRefDType* nodep) override {
        सङ्कलय_New_Use(nodep, VUseType::INT_FWD_CLASS, nodep->classp()->name());
    }
    शून्य दर्शय(AstCFunc* nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;
        iterate_And_अग्र_Const_Null(nodep->argsp());
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
    }
    शून्य दर्शय(AstCCall* /*nodep*/) override {}
    शून्य दर्शय(AstCReturn* nodep) override {
        UASSERT_OBJ(!nodep->user1_समुच्चयः_Once(), nodep, "Visited same return twice");
        iterateConst(nodep->lhsp()->dtypep());
    }
    शून्य दर्शय(Ast_ग्रन्थिः_DType* nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;  // Process once
        यदि (nodep->virtRefDTypep()) iterateConst(nodep->virtRefDTypep());
        यदि (nodep->virtRefDType2p()) iterateConst(nodep->virtRefDType2p());

        // Add a CUse for every struct that requires a declaration
        Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव stypep = VN_CAST(nodep->skipRefp(), ग्रन्थिः_UOr_Struct_DType);
        यदि (stypep && stypep->classOrPackagep()) {
            सङ्कलय_New_Use(nodep, VUseType::INT_INCLUDE, stypep->classOrPackagep()->name());
            iterateChildrenConst(stypep);
        } अन्यथा यदि (ध्रुव AstClassRefDType* ध्रुव classp
                   = VN_CAST(nodep->skipRefp(), ClassRefDType)) {
            सङ्कलय_New_Use(nodep, VUseType::INT_FWD_CLASS, classp->name());
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;  // Process once
        यदि (nodep->dtypep()) iterateConst(nodep->dtypep());
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(AstCell* nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;  // Process once
        यदि (nodep->modp()->isConstPool()) प्रत्यागम;  // Special emit rules
        // Currently no IMP_INCLUDE because we include __Syms which has them all
        सङ्कलय_New_Use(nodep, VUseType::INT_FWD_CLASS, nodep->modp()->name());
        iterateChildrenConst(nodep);
    }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट CUseVisitor(Ast_ग्रन्थिः_Module* modp)
        : m_modp{modp} {
        iterateConst(modp);

        प्रति (ध्रुव स्वचालित& प्रयुक्तम् : m_didUse) {
            AstCUse* ध्रुव newp = नूतन AstCUse{प्रयुक्तम्.second.first, प्रयुक्तम्.second.second, प्रयुक्तम्.first};
            m_modp->सङ्कलय_Stmtsp(newp);
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
    प्रति (Ast_ग्रन्थिः_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), ग्रन्थिः_Module)) {
        यदि (modp->isConstPool()) अनुवर्त;  // Special emit rules
        // Insert under this module; someday we should e.g. make Ast
        // for each output file and put under that
        CUseVisitor{modp};
    }
    V3Global::dump_Check_Global_वृक्षः("cuse", 0, dump_वृक्षः_Either_Level() >= 3);
}
