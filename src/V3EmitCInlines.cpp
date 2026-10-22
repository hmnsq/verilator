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

#ಅನ್ತರ್ಭಾವ "V3PchAstMT.h"

#ಅನ್ತರ್ಭಾವ "V3EmitC.h"
#ಅನ್ತರ್ಭಾವ "V3EmitCBase.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

#ಅನ್ತರ್ಭಾವ <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ವರ್ಗ EmitCInlines final : EmitCBaseVisitorConst {
    // STATE

    // METHODS

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstCNew* nodep) override {
        ಆದರೆ (v3Global.opt.savable())
            v3warn(E_UNSUPPORTED, "Unsupported: --savable with dynamic new");
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDumpCtl* nodep) override {
        ಆದರೆ (v3Global.opt.trace()) v3Global.needTraceDumper(ನಿಜ);
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Dist_Biop* nodep) override {
        v3Global.ಗಣ_ಬಳಸುತ್ತದೆ_Prob_Dist();
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Dist_Triop* nodep) override {
        v3Global.ಗಣ_ಬಳಸುತ್ತದೆ_Prob_Dist();
        iterateChildrenConst(nodep);
    }

    //---------------------------------------
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ EmitCInlines(AstNetlist* nodep) { iterateConst(nodep); }
};

//######################################################################
// EmitC class functions

ಶೂನ್ಯ V3EmitC::emitcInlines() {
    UINFO(2, __FUNCTION__ << ":");
    { EmitCInlines{v3Global.rootp()}; }
}
