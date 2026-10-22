// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Type system used by DFG
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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Dfg.h"
#అన్తర్భావ "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//------------------------------------------------------------------------------
// DfgDataType

స్థిరరాశి Dfg_దత్తాంశం_Type* Dfg_దత్తాంశం_Type::s_nullTypep{శూన్యసూచిక};
std::unordered_map<uint32_t, స్థిరరాశి Dfg_దత్తాంశం_Type*> Dfg_దత్తాంశం_Type::s_packedTypes{};

//------------------------------------------------------------------------------
// Type checker - for internal validation only

శూన్య V3DfgPasses::typeCheck(స్థిరరాశి Dfg_లేఖాచిత్రం& dfg) {
    dfg.for_Each_శీర్షం([&](స్థిరరాశి Dfg_శీర్షం& vtx) { vtx.typeCheck(dfg); });
}
