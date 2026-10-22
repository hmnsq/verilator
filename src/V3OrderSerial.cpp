// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Block code ordering
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
//
//  Serial code ordering
//
//*************************************************************************

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3OrderCFuncEmitter.h"
#ಅನ್ತರ್ಭಾವ "V3OrderInternal.h"

#ಅನ್ತರ್ಭಾವ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// OrderSerial class

Ast_ಗಂಟು_Stmt* V3_ಕ್ರಮ::createSerial(ಕ್ರಮ_Move_ನಕ್ಷೆ& move_ನಕ್ಷೆ, ಸ್ಥಿರಾಂಕ std::string& ಟ್ಯಾಗ್, ತಾರ್ಕಿಕ slow) {

    UINFO(2, "  Constructing serial code for '" + ಟ್ಯಾಗ್ + "'");

    // Serializer
    ಕ್ರಮ_Move_ನಕ್ಷೆ_Serializer serializer{move_ನಕ್ಷೆ};

    // Add initially ready vertices (those with no dependencies) to the serializer as seeds
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : move_ನಕ್ಷೆ.vertices()) {
        ಆದರೆ (vtx.ಒಳ_ಖಾಲಿಗಳು()) serializer.ಕೂಡಿಸು_ಬೀಜ(vtx.as<ಕ್ರಮ_Move_ಶೃಂಗ>());
    }

    // Emit all logic as they become ready
    V3_ಕ್ರಮ_CFunc_Emitter emitter{move_ನಕ್ಷೆ.ಕ್ರಮ_ನಕ್ಷೆ_2().trigVscp(), ಟ್ಯಾಗ್, slow};
    ಕ್ರಮ_Move_Dom_Scope* prevDomScopep = ಶೂನ್ಯಸೂಚಕ;
    ಆಗ (ಕ್ರಮ_Move_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ mVtxp = serializer.get_ಮುಂದಿನ()) {
        // We only really care about logic vertices
        ಆದರೆ (ಕ್ರಮ_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ logicp = mVtxp->logicp()) {
            // Force a new function if the domain or scope changed, for better combining.
            ಕ್ರಮ_Move_Dom_Scope* ಸ್ಥಿರಾಂಕ domScopep = &mVtxp->domScope();
            ಆದರೆ (domScopep != prevDomScopep) emitter.forceNewFunction();
            prevDomScopep = domScopep;
            // Emit the logic under this vertex
            emitter.emitLogic(logicp);
        }
        // Can delete the vertex now
        VL_DO_DANGLING(mVtxp->unlinkDelete(&move_ನಕ್ಷೆ), mVtxp);
    }

    // Delete the remaining variable vertices
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : move_ನಕ್ಷೆ.vertices().unlinkable()) {
        ಆದರೆ (!vtxp->as<ಕ್ರಮ_Move_ಶೃಂಗ>()->logicp()) {
            VL_DO_DANGLING(vtxp->unlinkDelete(&move_ನಕ್ಷೆ), vtxp);
        }
    }

    ಹಿಂತಿರುಗಿ emitter.getStmts();
}
