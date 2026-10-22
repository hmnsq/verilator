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

#அந்தர்பாவ "V3PchAstMT.h"

#அந்தர்பாவ "V3EmitC.h"
#அந்தர்பாவ "V3EmitCBase.h"
#அந்தர்பாவ "V3Stats.h"

#அந்தர்பாவ <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

வகுப்பு EmitCInlines final : EmitCBaseVisitorConst {
    // STATE

    // METHODS

    // VISITORS
    வெறுமை visit(AstCNew* nodep) override {
        எனில் (v3Global.opt.savable())
            v3warn(E_UNSUPPORTED, "Unsupported: --savable with dynamic new");
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstDumpCtl* nodep) override {
        எனில் (v3Global.opt.trace()) v3Global.needTraceDumper(உண்மை);
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(Ast_முனை_Dist_Biop* nodep) override {
        v3Global.setUsesProbDist();
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(Ast_முனை_Dist_Triop* nodep) override {
        v3Global.setUsesProbDist();
        iterateChildrenConst(nodep);
    }

    //---------------------------------------
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildrenConst(nodep); }

பொது:
    explicit EmitCInlines(AstNetlist* nodep) { iterateConst(nodep); }
};

//######################################################################
// EmitC class functions

வெறுமை V3EmitC::emitcInlines() {
    UINFO(2, __FUNCTION__ << ":");
    { EmitCInlines{v3Global.rootp()}; }
}
