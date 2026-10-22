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

#ضمن "V3PchAstMT.h"

#ضمن "V3EmitC.h"
#ضمن "V3EmitCBase.h"
#ضمن "V3Stats.h"

#ضمن <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

صنف EmitCInlines final : EmitCBaseVisitorConst {
    // STATE

    // METHODS

    // VISITORS
    فراغ بازدید(AstCNew* nodep) override {
        إذا (v3Global.opt.savable())
            v3warn(E_UNSUPPORTED, "Unsupported: --savable with dynamic new");
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(AstDumpCtl* nodep) override {
        إذا (v3Global.opt.trace()) v3Global.needTraceDumper(درست);
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(Ast_گره_Dist_Biop* nodep) override {
        v3Global.مجموعه_استفاده_میکند_Prob_Dist();
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(Ast_گره_Dist_Triop* nodep) override {
        v3Global.مجموعه_استفاده_میکند_Prob_Dist();
        iterateChildrenConst(nodep);
    }

    //---------------------------------------
    فراغ بازدید(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

عام:
    صریح EmitCInlines(AstNetlist* nodep) { iterateConst(nodep); }
};

//######################################################################
// EmitC class functions

فراغ V3EmitC::emitcInlines() {
    UINFO(2, __FUNCTION__ << ":");
    { EmitCInlines{v3Global.rootp()}; }
}
