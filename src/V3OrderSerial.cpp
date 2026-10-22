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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3OrderCFuncEmitter.h"
#అన్తర్భావ "V3OrderInternal.h"

#అన్తర్భావ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// OrderSerial class

Ast_కణుపు_Stmt* V3_క్రమం::createSerial(క్రమం_Move_లేఖాచిత్రం& move_లేఖాచిత్రం, స్థిరరాశి std::string& ట్యాగ్, తార్కిక slow) {

    UINFO(2, "  Constructing serial code for '" + ట్యాగ్ + "'");

    // Serializer
    క్రమం_Move_లేఖాచిత్రం_Serializer serializer{move_లేఖాచిత్రం};

    // Add initially ready vertices (those with no dependencies) to the serializer as seeds
    కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : move_లేఖాచిత్రం.vertices()) {
        ఒకవేళ (vtx.లోపల_ఖాళీలు()) serializer.కూడు_విత్తనం(vtx.as<క్రమం_Move_శీర్షం>());
    }

    // Emit all logic as they become ready
    V3_క్రమం_CFunc_Emitter emitter{move_లేఖాచిత్రం.క్రమం_లేఖాచిత్రం_2().trigVscp(), ట్యాగ్, slow};
    క్రమం_Move_Dom_Scope* prevDomScopep = శూన్యసూచిక;
    అయితే (క్రమం_Move_శీర్షం* స్థిరరాశి mVtxp = serializer.get_తదుపరి()) {
        // We only really care about logic vertices
        ఒకవేళ (క్రమం_Logic_శీర్షం* స్థిరరాశి logicp = mVtxp->logicp()) {
            // Force a new function if the domain or scope changed, for better combining.
            క్రమం_Move_Dom_Scope* స్థిరరాశి domScopep = &mVtxp->domScope();
            ఒకవేళ (domScopep != prevDomScopep) emitter.forceNewFunction();
            prevDomScopep = domScopep;
            // Emit the logic under this vertex
            emitter.emitLogic(logicp);
        }
        // Can delete the vertex now
        VL_DO_DANGLING(mVtxp->unlinkDelete(&move_లేఖాచిత్రం), mVtxp);
    }

    // Delete the remaining variable vertices
    కొరకు (V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి vtxp : move_లేఖాచిత్రం.vertices().unlinkable()) {
        ఒకవేళ (!vtxp->as<క్రమం_Move_శీర్షం>()->logicp()) {
            VL_DO_DANGLING(vtxp->unlinkDelete(&move_లేఖాచిత్రం), vtxp);
        }
    }

    తిరిగి emitter.getStmts();
}
