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

Ast_عقدة_Stmt* V3Order::createSerial(OrderMoveGraph& moveGraph, ثابت std::string& tag, منطقي slow) {

    UINFO(2, "  Constructing serial code for '" + tag + "'");

    // Serializer
    OrderMoveGraphSerializer serializer{moveGraph};

    // Add initially ready vertices (those with no dependencies) to the serializer as seeds
    لكل (V3GraphVertex& vtx : moveGraph.vertices()) {
        إذا (vtx.inEmpty()) serializer.addSeed(vtx.as<OrderMoveVertex>());
    }

    // Emit all logic as they become ready
    V3OrderCFuncEmitter emitter{moveGraph.orderGraph().trigVscp(), tag, slow};
    OrderMoveDomScope* prevDomScopep = nullptr;
    بينما (OrderMoveVertex* ثابت mVtxp = serializer.getNext()) {
        // We only really care about logic vertices
        إذا (OrderLogicVertex* ثابت logicp = mVtxp->logicp()) {
            // Force a new function if the domain or scope changed, for better combining.
            OrderMoveDomScope* ثابت domScopep = &mVtxp->domScope();
            إذا (domScopep != prevDomScopep) emitter.forceNewFunction();
            prevDomScopep = domScopep;
            // Emit the logic under this vertex
            emitter.emitLogic(logicp);
        }
        // Can delete the vertex now
        VL_DO_DANGLING(mVtxp->unlinkDelete(&moveGraph), mVtxp);
    }

    // Delete the remaining variable vertices
    لكل (V3GraphVertex* ثابت vtxp : moveGraph.vertices().unlinkable()) {
        إذا (!vtxp->as<OrderMoveVertex>()->logicp()) {
            VL_DO_DANGLING(vtxp->unlinkDelete(&moveGraph), vtxp);
        }
    }

    إرجاع emitter.getStmts();
}
