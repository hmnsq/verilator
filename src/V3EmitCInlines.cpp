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

#شامل "V3PchAstMT.h"

#شامل "V3EmitC.h"
#شامل "V3EmitCBase.h"
#شامل "V3Stats.h"

#شامل <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

جماعت EmitCInlines final : EmitCBaseVisitorConst {
    // STATE

    // METHODS

    // VISITORS
    خالی دورہ(AstCNew* nodep) override {
        اگر (v3Global.opt.savable())
            v3warn(E_UNSUPPORTED, "Unsupported: --savable with dynamic new");
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstDumpCtl* nodep) override {
        اگر (v3Global.opt.trace()) v3Global.needTraceDumper(سچ);
        iterateChildrenConst(nodep);
    }
    خالی دورہ(Ast_گرہ_Dist_Biop* nodep) override {
        v3Global.سیٹ_استعمال_کرتا_Prob_Dist();
        iterateChildrenConst(nodep);
    }
    خالی دورہ(Ast_گرہ_Dist_Triop* nodep) override {
        v3Global.سیٹ_استعمال_کرتا_Prob_Dist();
        iterateChildrenConst(nodep);
    }

    //---------------------------------------
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

عوامی:
    صریح EmitCInlines(AstNetlist* nodep) { iterateConst(nodep); }
};

//######################################################################
// EmitC class functions

خالی V3EmitC::emitcInlines() {
    UINFO(2, __FUNCTION__ << ":");
    { EmitCInlines{v3Global.rootp()}; }
}
