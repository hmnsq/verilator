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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstMT.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitC.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitCBase.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ਵਰਗ EmitCInlines final : EmitCBaseVisitorConst {
    // STATE

    // METHODS

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstCNew* nodep) override {
        ਜੇ (v3Global.opt.savable())
            v3warn(E_UNSUPPORTED, "Unsupported: --savable with dynamic new");
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstDumpCtl* nodep) override {
        ਜੇ (v3Global.opt.trace()) v3Global.needTraceDumper(ਸੱਚ);
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Dist_Biop* nodep) override {
        v3Global.ਸੈੱਟ_ਵਰਤਦਾ_Prob_Dist();
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Dist_Triop* nodep) override {
        v3Global.ਸੈੱਟ_ਵਰਤਦਾ_Prob_Dist();
        iterateChildrenConst(nodep);
    }

    //---------------------------------------
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildrenConst(nodep); }

ਜਨਤਕ:
    ਸਪਸ਼ਟ EmitCInlines(AstNetlist* nodep) { iterateConst(nodep); }
};

//######################################################################
// EmitC class functions

ਖਾਲੀ V3EmitC::emitcInlines() {
    UINFO(2, __FUNCTION__ << ":");
    { EmitCInlines{v3Global.rootp()}; }
}
