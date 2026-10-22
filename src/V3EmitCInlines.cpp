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

#अन्तर्भाव "V3PchAstMT.h"

#अन्तर्भाव "V3EmitC.h"
#अन्तर्भाव "V3EmitCBase.h"
#अन्तर्भाव "V3Stats.h"

#अन्तर्भाव <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

वर्ग EmitCInlines final : EmitCBaseVisitorConst {
    // STATE

    // METHODS

    // VISITORS
    शून्य दर्शय(AstCNew* nodep) override {
        यदि (v3Global.opt.savable())
            v3warn(E_UNSUPPORTED, "Unsupported: --savable with dynamic new");
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(AstDumpCtl* nodep) override {
        यदि (v3Global.opt.trace()) v3Global.needTraceDumper(सत्य);
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Dist_Biop* nodep) override {
        v3Global.समुच्चयः_उपयुङ्क्ते_Prob_Dist();
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Dist_Triop* nodep) override {
        v3Global.समुच्चयः_उपयुङ्क्ते_Prob_Dist();
        iterateChildrenConst(nodep);
    }

    //---------------------------------------
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    स्पष्ट EmitCInlines(AstNetlist* nodep) { iterateConst(nodep); }
};

//######################################################################
// EmitC class functions

शून्य V3EmitC::emitcInlines() {
    UINFO(2, __FUNCTION__ << ":");
    { EmitCInlines{v3Global.rootp()}; }
}
