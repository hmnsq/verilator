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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3OrderCFuncEmitter.h"
#अन्तर्भाव "V3OrderInternal.h"

#अन्तर्भाव <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// OrderSerial class

Ast_ग्रन्थिः_Stmt* V3_क्रमः::createSerial(क्रमः_Move_Graph& moveGraph, ध्रुव std::string& चिह्नकम्, तार्किक slow) {

    UINFO(2, "  Constructing serial code for '" + चिह्नकम् + "'");

    // Serializer
    क्रमः_Move_Graph_Serializer serializer{moveGraph};

    // Add initially ready vertices (those with no dependencies) to the serializer as seeds
    प्रति (V3GraphVertex& vtx : moveGraph.vertices()) {
        यदि (vtx.अन्तर्_रिक्तानि()) serializer.सङ्कलय_बीजम्(vtx.as<क्रमः_Move_Vertex>());
    }

    // Emit all logic as they become ready
    V3_क्रमः_CFunc_Emitter emitter{moveGraph.क्रमः_Graph_2().trigVscp(), चिह्नकम्, slow};
    क्रमः_Move_Dom_Scope* पूर्व_Dom_Scopep = शून्यसूचकः;
    यावत् (क्रमः_Move_Vertex* ध्रुव mVtxp = serializer.get_अग्र()) {
        // We only really care about logic vertices
        यदि (क्रमः_Logic_Vertex* ध्रुव logicp = mVtxp->logicp()) {
            // Force a new function if the domain or scope changed, for better combining.
            क्रमः_Move_Dom_Scope* ध्रुव domScopep = &mVtxp->domScope();
            यदि (domScopep != पूर्व_Dom_Scopep) emitter.forceNewFunction();
            पूर्व_Dom_Scopep = domScopep;
            // Emit the logic under this vertex
            emitter.emitLogic(logicp);
        }
        // Can delete the vertex now
        VL_DO_DANGLING(mVtxp->unlinkDelete(&moveGraph), mVtxp);
    }

    // Delete the remaining variable vertices
    प्रति (V3GraphVertex* ध्रुव vtxp : moveGraph.vertices().unlinkable()) {
        यदि (!vtxp->as<क्रमः_Move_Vertex>()->logicp()) {
            VL_DO_DANGLING(vtxp->unlinkDelete(&moveGraph), vtxp);
        }
    }

    प्रत्यागम emitter.getStmts();
}
