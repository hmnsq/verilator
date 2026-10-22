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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3OrderCFuncEmitter.h"
#समावेश "V3OrderInternal.h"

#समावेश <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// OrderSerial class

Ast_गाँठ_Stmt* V3Order::createSerial(OrderMoveGraph& moveGraph, स्थिर std::string& tag, bool slow) {

    UINFO(2, "  Constructing serial code for '" + tag + "'");

    // Serializer
    OrderMoveGraphSerializer serializer{moveGraph};

    // Add initially ready vertices (those with no dependencies) to the serializer as seeds
    क्रम (V3GraphVertex& vtx : moveGraph.vertices()) {
        अगर (vtx.inEmpty()) serializer.addSeed(vtx.as<OrderMoveVertex>());
    }

    // Emit all logic as they become ready
    V3OrderCFuncEmitter emitter{moveGraph.orderGraph().trigVscp(), tag, slow};
    OrderMoveDomScope* पूर्व_Dom_Scopep = nullptr;
    जबतक (OrderMoveVertex* स्थिर mVtxp = serializer.get_अग्र()) {
        // We only really care about logic vertices
        अगर (OrderLogicVertex* स्थिर logicp = mVtxp->logicp()) {
            // Force a new function if the domain or scope changed, for better combining.
            OrderMoveDomScope* स्थिर domScopep = &mVtxp->domScope();
            अगर (domScopep != पूर्व_Dom_Scopep) emitter.forceNewFunction();
            पूर्व_Dom_Scopep = domScopep;
            // Emit the logic under this vertex
            emitter.emitLogic(logicp);
        }
        // Can delete the vertex now
        VL_DO_DANGLING(mVtxp->unlinkDelete(&moveGraph), mVtxp);
    }

    // Delete the remaining variable vertices
    क्रम (V3GraphVertex* स्थिर vtxp : moveGraph.vertices().unlinkable()) {
        अगर (!vtxp->as<OrderMoveVertex>()->logicp()) {
            VL_DO_DANGLING(vtxp->unlinkDelete(&moveGraph), vtxp);
        }
    }

    वापस emitter.getStmts();
}
