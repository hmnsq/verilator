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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3CUse.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

// Visit within a module all nodes and data types they reference, finding
// any classes so we can make sure they are defined when Verilated code
// compiles
తరగతి CUseVisitor final : బహిరంగ VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()     -> bool.  True if already visited
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;

    // MEMBERS
    Ast_కణుపు_Module* స్థిరరాశి m_modp;  // Current module
    std::map<std::string, std::pair<File_పంక్తి*, VUseType>> m_didUse;  // What we already used

    // METHODS
    శూన్య కూడు_New_Use(స్థిరరాశి Ast_కణుపు* nodep, VUseType useType, స్థిరరాశి string& name) {
        స్వయంచాలక e = m_didUse.emplace(name, std::make_pair(nodep->fileline(), useType));
        ఒకవేళ (e.రెండవ || ((e.మొదటి->రెండవ.రెండవ & useType) != useType)) {
            e.మొదటి->రెండవ.రెండవ = e.మొదటి->రెండవ.రెండవ | useType;
        }
    }

    // VISITORS
    శూన్య సందర్శించు(AstClassRefDType* nodep) override {
        కూడు_New_Use(nodep, VUseType::INT_FWD_CLASS, nodep->classp()->name());
    }
    శూన్య సందర్శించు(AstCFunc* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;
        iterate_And_తదుపరి_Const_Null(nodep->argsp());
        iterate_And_తదుపరి_Const_Null(nodep->stmtsp());
    }
    శూన్య సందర్శించు(AstCCall* /*nodep*/) override {}
    శూన్య సందర్శించు(AstCReturn* nodep) override {
        UASSERT_OBJ(!nodep->user1_సమితి_Once(), nodep, "Visited same return twice");
        iterateConst(nodep->lhsp()->dtypep());
    }
    శూన్య సందర్శించు(Ast_కణుపు_DType* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;  // Process once
        ఒకవేళ (nodep->virtRefDTypep()) iterateConst(nodep->virtRefDTypep());
        ఒకవేళ (nodep->virtRefDType2p()) iterateConst(nodep->virtRefDType2p());

        // Add a CUse for every struct that requires a declaration
        Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి stypep = VN_CAST(nodep->skipRefp(), కణుపు_UOr_Struct_DType);
        ఒకవేళ (stypep && stypep->classOrPackagep()) {
            కూడు_New_Use(nodep, VUseType::INT_INCLUDE, stypep->classOrPackagep()->name());
            iterateChildrenConst(stypep);
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstClassRefDType* స్థిరరాశి classp
                   = VN_CAST(nodep->skipRefp(), ClassRefDType)) {
            కూడు_New_Use(nodep, VUseType::INT_FWD_CLASS, classp->name());
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;  // Process once
        ఒకవేళ (nodep->dtypep()) iterateConst(nodep->dtypep());
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstCell* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;  // Process once
        ఒకవేళ (nodep->modp()->isConstPool()) తిరిగి;  // Special emit rules
        // Currently no IMP_INCLUDE because we include __Syms which has them all
        కూడు_New_Use(nodep, VUseType::INT_FWD_CLASS, nodep->modp()->name());
        iterateChildrenConst(nodep);
    }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన CUseVisitor(Ast_కణుపు_Module* modp)
        : m_modp{modp} {
        iterateConst(modp);

        కొరకు (స్థిరరాశి స్వయంచాలక& వాడిన : m_didUse) {
            AstCUse* స్థిరరాశి newp = కొత్త AstCUse{వాడిన.రెండవ.మొదటి, వాడిన.రెండవ.రెండవ, వాడిన.మొదటి};
            m_modp->కూడు_Stmtsp(newp);
            UINFO(8, "Insert " << newp);
        }
    }
    ~CUseVisitor() override = అప్రమేయ;
    VL_UNCOPYABLE(CUseVisitor);
};

//######################################################################
// Class class functions

శూన్య V3CUse::cUseAll() {
    UINFO(2, __FUNCTION__ << ":");
    // Call visitor separately for each module, so visitor state is cleared
    కొరకు (Ast_కణుపు_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), కణుపు_Module)) {
        ఒకవేళ (modp->isConstPool()) కొనసాగు;  // Special emit rules
        // Insert under this module; someday we should e.g. make Ast
        // for each output file and put under that
        CUseVisitor{modp};
    }
    V3Global::dump_Check_Global_వృక్షం("cuse", 0, dump_వృక్షం_Either_Level() >= 3);
}
