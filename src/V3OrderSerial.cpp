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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3OrderCFuncEmitter.h"
#ضمن "V3OrderInternal.h"

#ضمن <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// OrderSerial class

Ast_ڳنڍ_Stmt* V3_ترتيب::createSerial(ترتيب_Move_گراف& move_گراف, ثابت std::string& ٽيگ, منطقي slow) {

    UINFO(2, "  Constructing serial code for '" + ٽيگ + "'");

    // Serializer
    ترتيب_Move_گراف_Serializer serializer{move_گراف};

    // Add initially ready vertices (those with no dependencies) to the serializer as seeds
    لكل (V3_گراف_راس& vtx : move_گراف.vertices()) {
        إذا (vtx.اندريون_خالي_جايون()) serializer.جوڙيو_ٻج(vtx.as<ترتيب_Move_راس>());
    }

    // Emit all logic as they become ready
    V3_ترتيب_CFunc_Emitter emitter{move_گراف.ترتيب_گراف_2().trigVscp(), ٽيگ, slow};
    ترتيب_Move_Dom_Scope* prevDomScopep = خالي_اشارو;
    طالما (ترتيب_Move_راس* ثابت mVtxp = serializer.get_ايندڙ()) {
        // We only really care about logic vertices
        إذا (ترتيب_Logic_راس* ثابت logicp = mVtxp->logicp()) {
            // Force a new function if the domain or scope changed, for better combining.
            ترتيب_Move_Dom_Scope* ثابت domScopep = &mVtxp->domScope();
            إذا (domScopep != prevDomScopep) emitter.forceNewFunction();
            prevDomScopep = domScopep;
            // Emit the logic under this vertex
            emitter.emitLogic(logicp);
        }
        // Can delete the vertex now
        VL_DO_DANGLING(mVtxp->unlinkDelete(&move_گراف), mVtxp);
    }

    // Delete the remaining variable vertices
    لكل (V3_گراف_راس* ثابت vtxp : move_گراف.vertices().unlinkable()) {
        إذا (!vtxp->as<ترتيب_Move_راس>()->logicp()) {
            VL_DO_DANGLING(vtxp->unlinkDelete(&move_گراف), vtxp);
        }
    }

    أرجع emitter.getStmts();
}
