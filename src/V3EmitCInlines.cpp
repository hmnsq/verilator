// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Emit C++ for tree
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

#ଅନ୍ତର୍ଭାଵ "V3PchAstMT.h"

#ଅନ୍ତର୍ଭାଵ "V3EmitC.h"
#ଅନ୍ତର୍ଭାଵ "V3EmitCBase.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

#ଅନ୍ତର୍ଭାଵ <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ଶ୍ରେଣୀ EmitCInlines final : EmitCBaseVisitorConst {
    // STATE

    // METHODS

    // VISITORS
    ଶୂନ୍ୟ visit(AstCNew* nodep) override {
        ଯଦି (v3Global.opt.savable())
            v3warn(E_UNSUPPORTED, "Unsupported: --savable with dynamic new");
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstDumpCtl* nodep) override {
        ଯଦି (v3Global.opt.trace()) v3Global.needTraceDumper(ସତ୍ୟ);
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Dist_Biop* nodep) override {
        v3Global.setUsesProbDist();
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Dist_Triop* nodep) override {
        v3Global.setUsesProbDist();
        iterateChildrenConst(nodep);
    }

    //---------------------------------------
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildrenConst(nodep); }

ସର୍ବସାଧାରଣ:
    explicit EmitCInlines(AstNetlist* nodep) { iterateConst(nodep); }
};

//######################################################################
// EmitC class functions

ଶୂନ୍ୟ V3EmitC::emitcInlines() {
    UINFO(2, __FUNCTION__ << ":");
    { EmitCInlines{v3Global.rootp()}; }
}
