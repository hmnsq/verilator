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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3OrderCFuncEmitter.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3OrderInternal.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// OrderSerial class

Ast_ਗੰਢ_Stmt* V3_ਕ੍ਰਮ::createSerial(ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ& move_ਗ੍ਰਾਫ, ਅਚਲ std::string& ਟੈਗ, ਤਾਰਕਿਕ slow) {

    UINFO(2, "  Constructing serial code for '" + ਟੈਗ + "'");

    // Serializer
    ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ_Serializer serializer{move_ਗ੍ਰਾਫ};

    // Add initially ready vertices (those with no dependencies) to the serializer as seeds
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : move_ਗ੍ਰਾਫ.vertices()) {
        ਜੇ (vtx.ਵਿੱਚ_ਖਾਲੀ_ਥਾਵਾਂ()) serializer.ਜੋੜੋ_ਬੀਜ(vtx.as<ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼>());
    }

    // Emit all logic as they become ready
    V3_ਕ੍ਰਮ_CFunc_Emitter emitter{move_ਗ੍ਰਾਫ.ਕ੍ਰਮ_ਗ੍ਰਾਫ_2().trigVscp(), ਟੈਗ, slow};
    ਕ੍ਰਮ_Move_Dom_Scope* prevDomScopep = ਖਾਲੀ_ਸੂਚਕ;
    ਜਦੋਂ (ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼* ਅਚਲ mVtxp = serializer.get_ਅਗਲਾ()) {
        // We only really care about logic vertices
        ਜੇ (ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ logicp = mVtxp->logicp()) {
            // Force a new function if the domain or scope changed, for better combining.
            ਕ੍ਰਮ_Move_Dom_Scope* ਅਚਲ domScopep = &mVtxp->domScope();
            ਜੇ (domScopep != prevDomScopep) emitter.forceNewFunction();
            prevDomScopep = domScopep;
            // Emit the logic under this vertex
            emitter.emitLogic(logicp);
        }
        // Can delete the vertex now
        VL_DO_DANGLING(mVtxp->unlinkDelete(&move_ਗ੍ਰਾਫ), mVtxp);
    }

    // Delete the remaining variable vertices
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : move_ਗ੍ਰਾਫ.vertices().unlinkable()) {
        ਜੇ (!vtxp->as<ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼>()->logicp()) {
            VL_DO_DANGLING(vtxp->unlinkDelete(&move_ਗ੍ਰਾਫ), vtxp);
        }
    }

    ਵਾਪਸ emitter.getStmts();
}
