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

#અન્તર્ભાવ "V3PchAstMT.h"

#અન્તર્ભાવ "V3EmitC.h"
#અન્તર્ભાવ "V3EmitCBase.h"
#અન્તર્ભાવ "V3Stats.h"

#અન્તર્ભાવ <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

વર્ગ EmitCInlines final : EmitCBaseVisitorConst {
    // STATE

    // METHODS

    // VISITORS
    શૂન્ય visit(AstCNew* nodep) override {
        જો (v3Global.opt.savable())
            v3warn(E_UNSUPPORTED, "Unsupported: --savable with dynamic new");
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstDumpCtl* nodep) override {
        જો (v3Global.opt.trace()) v3Global.needTraceDumper(સાચું);
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Dist_Biop* nodep) override {
        v3Global.setUsesProbDist();
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Dist_Triop* nodep) override {
        v3Global.setUsesProbDist();
        iterateChildrenConst(nodep);
    }

    //---------------------------------------
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildrenConst(nodep); }

જાહેર:
    explicit EmitCInlines(AstNetlist* nodep) { iterateConst(nodep); }
};

//######################################################################
// EmitC class functions

શૂન્ય V3EmitC::emitcInlines() {
    UINFO(2, __FUNCTION__ << ":");
    { EmitCInlines{v3Global.rootp()}; }
}
