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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3OrderCFuncEmitter.h"
#ଅନ୍ତର୍ଭାଵ "V3OrderInternal.h"

#ଅନ୍ତର୍ଭାଵ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// OrderSerial class

Ast_ଗଣ୍ଠି_Stmt* V3Order::createSerial(OrderMoveGraph& moveGraph, ଅଚଳ std::string& tag, ତାର୍କିକ slow) {

    UINFO(2, "  Constructing serial code for '" + tag + "'");

    // Serializer
    OrderMoveGraphSerializer serializer{moveGraph};

    // Add initially ready vertices (those with no dependencies) to the serializer as seeds
    ପାଇଁ (V3GraphVertex& vtx : moveGraph.vertices()) {
        ଯଦି (vtx.inEmpty()) serializer.addSeed(vtx.as<OrderMoveVertex>());
    }

    // Emit all logic as they become ready
    V3OrderCFuncEmitter emitter{moveGraph.orderGraph().trigVscp(), tag, slow};
    OrderMoveDomScope* prevDomScopep = nullptr;
    ଯେତେବେଳେ (OrderMoveVertex* ଅଚଳ mVtxp = serializer.getNext()) {
        // We only really care about logic vertices
        ଯଦି (OrderLogicVertex* ଅଚଳ logicp = mVtxp->logicp()) {
            // Force a new function if the domain or scope changed, for better combining.
            OrderMoveDomScope* ଅଚଳ domScopep = &mVtxp->domScope();
            ଯଦି (domScopep != prevDomScopep) emitter.forceNewFunction();
            prevDomScopep = domScopep;
            // Emit the logic under this vertex
            emitter.emitLogic(logicp);
        }
        // Can delete the vertex now
        VL_DO_DANGLING(mVtxp->unlinkDelete(&moveGraph), mVtxp);
    }

    // Delete the remaining variable vertices
    ପାଇଁ (V3GraphVertex* ଅଚଳ vtxp : moveGraph.vertices().unlinkable()) {
        ଯଦି (!vtxp->as<OrderMoveVertex>()->logicp()) {
            VL_DO_DANGLING(vtxp->unlinkDelete(&moveGraph), vtxp);
        }
    }

    ଫେରନ୍ତୁ emitter.getStmts();
}
