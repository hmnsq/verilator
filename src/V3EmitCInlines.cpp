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

#అన్తర్భావ "V3PchAstMT.h"

#అన్తర్భావ "V3EmitC.h"
#అన్తర్భావ "V3EmitCBase.h"
#అన్తర్భావ "V3Stats.h"

#అన్తర్భావ <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

తరగతి EmitCInlines final : EmitCBaseVisitorConst {
    // STATE

    // METHODS

    // VISITORS
    శూన్య సందర్శించు(AstCNew* nodep) override {
        ఒకవేళ (v3Global.opt.savable())
            v3warn(E_UNSUPPORTED, "Unsupported: --savable with dynamic new");
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstDumpCtl* nodep) override {
        ఒకవేళ (v3Global.opt.trace()) v3Global.needTraceDumper(నిజం);
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Dist_Biop* nodep) override {
        v3Global.సమితి_ఉపయోగిస్తుంది_Prob_Dist();
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Dist_Triop* nodep) override {
        v3Global.సమితి_ఉపయోగిస్తుంది_Prob_Dist();
        iterateChildrenConst(nodep);
    }

    //---------------------------------------
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildrenConst(nodep); }

బహిరంగ:
    స్పష్టమైన EmitCInlines(AstNetlist* nodep) { iterateConst(nodep); }
};

//######################################################################
// EmitC class functions

శూన్య V3EmitC::emitcInlines() {
    UINFO(2, __FUNCTION__ << ":");
    { EmitCInlines{v3Global.rootp()}; }
}
