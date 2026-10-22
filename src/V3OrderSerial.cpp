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

Ast_گره_Stmt* V3_ترتیب::createSerial(ترتیب_Move_گراف& move_گراف, ثابت std::string& برچسب, منطقي slow) {

    UINFO(2, "  Constructing serial code for '" + برچسب + "'");

    // Serializer
    ترتیب_Move_گراف_Serializer serializer{move_گراف};

    // Add initially ready vertices (those with no dependencies) to the serializer as seeds
    لكل (V3_گراف_رأس& vtx : move_گراف.vertices()) {
        إذا (vtx.درون_خالی()) serializer.جمع_کن_دانه(vtx.as<ترتیب_Move_رأس>());
    }

    // Emit all logic as they become ready
    V3_ترتیب_CFunc_Emitter emitter{move_گراف.ترتیب_گراف_2().trigVscp(), برچسب, slow};
    ترتیب_Move_Dom_Scope* prevDomScopep = اشاره_تهی;
    طالما (ترتیب_Move_رأس* ثابت mVtxp = serializer.get_بعدی()) {
        // We only really care about logic vertices
        إذا (ترتیب_Logic_رأس* ثابت logicp = mVtxp->logicp()) {
            // Force a new function if the domain or scope changed, for better combining.
            ترتیب_Move_Dom_Scope* ثابت domScopep = &mVtxp->domScope();
            إذا (domScopep != prevDomScopep) emitter.forceNewFunction();
            prevDomScopep = domScopep;
            // Emit the logic under this vertex
            emitter.emitLogic(logicp);
        }
        // Can delete the vertex now
        VL_DO_DANGLING(mVtxp->unlinkDelete(&move_گراف), mVtxp);
    }

    // Delete the remaining variable vertices
    لكل (V3_گراف_رأس* ثابت vtxp : move_گراف.vertices().unlinkable()) {
        إذا (!vtxp->as<ترتیب_Move_رأس>()->logicp()) {
            VL_DO_DANGLING(vtxp->unlinkDelete(&move_گراف), vtxp);
        }
    }

    أرجع emitter.getStmts();
}
