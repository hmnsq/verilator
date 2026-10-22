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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3CUse.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

// Visit within a module all nodes and data types they reference, finding
// any classes so we can make sure they are defined when Verilated code
// compiles
বর্গ CUseVisitor final : সার্বজনিক VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()     -> bool.  True if already visited
    ধ্রুব VNUser1InUse m_inuser1;

    // MEMBERS
    Ast_গ্রন্থি_Module* ধ্রুব m_modp;  // Current module
    std::map<std::string, std::pair<File_সারি*, VUseType>> m_didUse;  // What we already used

    // METHODS
    শূন্য addNewUse(ধ্রুব Ast_গ্রন্থি* nodep, VUseType useType, ধ্রুব string& name) {
        auto e = m_didUse.emplace(name, std::make_pair(nodep->fileline(), useType));
        যদি (e.second || ((e.first->second.second & useType) != useType)) {
            e.first->second.second = e.first->second.second | useType;
        }
    }

    // VISITORS
    শূন্য visit(AstClassRefDType* nodep) override {
        addNewUse(nodep, VUseType::INT_FWD_CLASS, nodep->classp()->name());
    }
    শূন্য visit(AstCFunc* nodep) override {
        যদি (nodep->user1SetOnce()) প্রত্যাগম;
        iterateAndNextConstNull(nodep->argsp());
        iterateAndNextConstNull(nodep->stmtsp());
    }
    শূন্য visit(AstCCall* /*nodep*/) override {}
    শূন্য visit(AstCReturn* nodep) override {
        UASSERT_OBJ(!nodep->user1SetOnce(), nodep, "Visited same return twice");
        iterateConst(nodep->lhsp()->dtypep());
    }
    শূন্য visit(Ast_গ্রন্থি_DType* nodep) override {
        যদি (nodep->user1SetOnce()) প্রত্যাগম;  // Process once
        যদি (nodep->virtRefDTypep()) iterateConst(nodep->virtRefDTypep());
        যদি (nodep->virtRefDType2p()) iterateConst(nodep->virtRefDType2p());

        // Add a CUse for every struct that requires a declaration
        Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুব stypep = VN_CAST(nodep->skipRefp(), গ্রন্থি_UOr_Struct_DType);
        যদি (stypep && stypep->classOrPackagep()) {
            addNewUse(nodep, VUseType::INT_INCLUDE, stypep->classOrPackagep()->name());
            iterateChildrenConst(stypep);
        } অন্যথা যদি (ধ্রুব AstClassRefDType* ধ্রুব classp
                   = VN_CAST(nodep->skipRefp(), ClassRefDType)) {
            addNewUse(nodep, VUseType::INT_FWD_CLASS, classp->name());
        }
    }
    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        যদি (nodep->user1SetOnce()) প্রত্যাগম;  // Process once
        যদি (nodep->dtypep()) iterateConst(nodep->dtypep());
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstCell* nodep) override {
        যদি (nodep->user1SetOnce()) প্রত্যাগম;  // Process once
        যদি (nodep->modp()->isConstPool()) প্রত্যাগম;  // Special emit rules
        // Currently no IMP_INCLUDE because we include __Syms which has them all
        addNewUse(nodep, VUseType::INT_FWD_CLASS, nodep->modp()->name());
        iterateChildrenConst(nodep);
    }

সার্বজনিক:
    // CONSTRUCTORS
    explicit CUseVisitor(Ast_গ্রন্থি_Module* modp)
        : m_modp{modp} {
        iterateConst(modp);

        প্রতি (ধ্রুব auto& used : m_didUse) {
            AstCUse* ধ্রুব newp = নূতন AstCUse{used.second.first, used.second.second, used.first};
            m_modp->addStmtsp(newp);
            UINFO(8, "Insert " << newp);
        }
    }
    ~CUseVisitor() override = স্বভাব;
    VL_UNCOPYABLE(CUseVisitor);
};

//######################################################################
// Class class functions

শূন্য V3CUse::cUseAll() {
    UINFO(2, __FUNCTION__ << ":");
    // Call visitor separately for each module, so visitor state is cleared
    প্রতি (Ast_গ্রন্থি_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), গ্রন্থি_Module)) {
        যদি (modp->isConstPool()) অনুবর্ত;  // Special emit rules
        // Insert under this module; someday we should e.g. make Ast
        // for each output file and put under that
        CUseVisitor{modp};
    }
    V3Global::dump_Check_Global_বৃক্ষ("cuse", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
