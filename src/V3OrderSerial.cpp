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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3OrderCFuncEmitter.h"
#شامل "V3OrderInternal.h"

#شامل <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// OrderSerial class

Ast_گرہ_Stmt* V3_ترتیب::createSerial(ترتیب_Move_گراف& move_گراف, ثابت std::string& لیبل, منطقی slow) {

    UINFO(2, "  Constructing serial code for '" + لیبل + "'");

    // Serializer
    ترتیب_Move_گراف_Serializer serializer{move_گراف};

    // Add initially ready vertices (those with no dependencies) to the serializer as seeds
    برائے (V3_گراف_راس& vtx : move_گراف.vertices()) {
        اگر (vtx.اندر_خالی_خانے()) serializer.جوڑو_بیج(vtx.as<ترتیب_Move_راس>());
    }

    // Emit all logic as they become ready
    V3_ترتیب_CFunc_Emitter emitter{move_گراف.ترتیب_گراف_2().trigVscp(), لیبل, slow};
    ترتیب_Move_Dom_Scope* پچھلا_Dom_Scopep = خالی_اشاریہ;
    جب (ترتیب_Move_راس* ثابت mVtxp = serializer.get_اگلا()) {
        // We only really care about logic vertices
        اگر (ترتیب_Logic_راس* ثابت logicp = mVtxp->logicp()) {
            // Force a new function if the domain or scope changed, for better combining.
            ترتیب_Move_Dom_Scope* ثابت domScopep = &mVtxp->domScope();
            اگر (domScopep != پچھلا_Dom_Scopep) emitter.forceNewFunction();
            پچھلا_Dom_Scopep = domScopep;
            // Emit the logic under this vertex
            emitter.emitLogic(logicp);
        }
        // Can delete the vertex now
        VL_DO_DANGLING(mVtxp->unlinkDelete(&move_گراف), mVtxp);
    }

    // Delete the remaining variable vertices
    برائے (V3_گراف_راس* ثابت vtxp : move_گراف.vertices().unlinkable()) {
        اگر (!vtxp->as<ترتیب_Move_راس>()->logicp()) {
            VL_DO_DANGLING(vtxp->unlinkDelete(&move_گراف), vtxp);
        }
    }

    واپس emitter.getStmts();
}
