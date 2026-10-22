// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Inlining of modules
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

#నిర్వచితం_కాకపోతే VERILATOR_V3TASK_H_
#నిర్వచించు VERILATOR_V3TASK_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3Container.h"
#అన్తర్భావ "V3Error.h"

#అన్తర్భావ <utility>
#అన్తర్భావ <vector>

//============================================================================

using V3TaskConnect = std::pair<AstVar*, AstArg*>;  // [port, pin-connects-to]
using V3TaskConnects = std::సదిశ<V3TaskConnect>;  // [ [port, pin-connects-to] ... ]

class V3TaskConnectState final {
    VNDeleter m_deleter;  // Allow delayed deletion of nodes
    bool m_didWrap = false;  // Made a wrapper
    using WrapMap = std::map<std::pair<Ast_కణుపు_FTask*, std::string>, Ast_కణుపు_FTask*>;
    WrapMap m_wrapMap;  // Map of {old function, arguments} -> new function
బహిరంగ:
    V3TaskConnectState() = అప్రమేయ;
    ~V3TaskConnectState() = అప్రమేయ;
    శూన్య నెట్టు_Deletep(Ast_కణుపు* nodep) { m_deleter.నెట్టు_Deletep(nodep); }
    bool didWrap() స్థిరరాశి { తిరిగి m_didWrap; }
    శూన్య సమితి_Did_Wrap() { m_didWrap = true; }
    WrapMap& wrapMap() { తిరిగి m_wrapMap; }
};

//============================================================================

class V3Task final {
    స్థిర స్థిరరాశి అక్షరం* స్థిరరాశి s_dpiTemporaryVarSuffix;

బహిరంగ:
    స్థిర శూన్య taskAll(AstNetlist* nodep) VL_MT_DISABLED;
    /// Return vector of [port, pin-connects-to]  (SLOW)
    స్థిర V3TaskConnects taskConnects(Ast_కణుపు_FTask_Ref* nodep, Ast_కణుపు* taskStmtsp,
                                       V3TaskConnectState* statep = nullptr,
                                       bool makeChanges = true) VL_MT_DISABLED;
    స్థిర శూన్య taskConnectWrap(Ast_కణుపు_FTask_Ref* nodep, స్థిరరాశి V3TaskConnects& tconnects,
                                V3TaskConnectState* statep,
                                స్థిరరాశి VInsertion_సమితి<స్థిరరాశి AstVar*>& argWrap) VL_MT_DISABLED;
    స్థిర Ast_కణుపు_FTask*
    taskConnectWrapNew(Ast_కణుపు_FTask* taskp, స్థిరరాశి string& newname, స్థిరరాశి V3TaskConnects& tconnects,
                       స్థిరరాశి VInsertion_సమితి<స్థిరరాశి AstVar*>& argWrap) VL_MT_DISABLED;
    స్థిర string కేటాయించు_అంతర్గత_గమ్యం_Dpi(AstVar* portp, bool isPtr, స్థిరరాశి string& frSuffix,
                                      స్థిరరాశి string& గమ్యం_Suffix,
                                      స్థిరరాశి string& fr_ఉపసర్గ = "") VL_MT_DISABLED;
    స్థిర string కేటాయించు_Dpi_గమ్యం_అంతర్గత(స్థిరరాశి string& lhsName, AstVar* rhsp) VL_MT_DISABLED;
    స్థిర స్థిరరాశి అక్షరం* dpiTemporaryVarSuffix() VL_MT_సురక్షిత { తిరిగి s_dpiTemporaryVarSuffix; }
};

#అయితే_ముగింపు  // Guard
