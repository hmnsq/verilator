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

#ifndef VERILATOR_V3TASK_H_
#define VERILATOR_V3TASK_H_

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3Container.h"
#અન્તર્ભાવ "V3Error.h"

#અન્તર્ભાવ <utility>
#અન્તર્ભાવ <vector>

//============================================================================

using V3TaskConnect = std::pair<AstVar*, AstArg*>;  // [port, pin-connects-to]
using V3TaskConnects = std::vector<V3TaskConnect>;  // [ [port, pin-connects-to] ... ]

class V3TaskConnectState final {
    VNDeleter m_deleter;  // Allow delayed deletion of nodes
    bool m_didWrap = false;  // Made a wrapper
    using WrapMap = std::map<std::pair<Ast_ગાંઠ_FTask*, std::string>, Ast_ગાંઠ_FTask*>;
    WrapMap m_wrapMap;  // Map of {old function, arguments} -> new function
જાહેર:
    V3TaskConnectState() = મૂળભૂત;
    ~V3TaskConnectState() = મૂળભૂત;
    શૂન્ય ધકેલો_Deletep(Ast_ગાંઠ* nodep) { m_deleter.ધકેલો_Deletep(nodep); }
    bool didWrap() અચલ { પાછા m_didWrap; }
    શૂન્ય setDidWrap() { m_didWrap = true; }
    WrapMap& wrapMap() { પાછા m_wrapMap; }
};

//============================================================================

class V3Task final {
    સ્થિર અચલ અક્ષર* અચલ s_dpiTemporaryVarSuffix;

જાહેર:
    સ્થિર શૂન્ય taskAll(AstNetlist* nodep) VL_MT_DISABLED;
    /// Return vector of [port, pin-connects-to]  (SLOW)
    સ્થિર V3TaskConnects taskConnects(Ast_ગાંઠ_FTask_Ref* nodep, Ast_ગાંઠ* taskStmtsp,
                                       V3TaskConnectState* statep = nullptr,
                                       bool makeChanges = true) VL_MT_DISABLED;
    સ્થિર શૂન્ય taskConnectWrap(Ast_ગાંઠ_FTask_Ref* nodep, અચલ V3TaskConnects& tconnects,
                                V3TaskConnectState* statep,
                                અચલ VInsertionSet<અચલ AstVar*>& argWrap) VL_MT_DISABLED;
    સ્થિર Ast_ગાંઠ_FTask*
    taskConnectWrapNew(Ast_ગાંઠ_FTask* taskp, અચલ string& newname, અચલ V3TaskConnects& tconnects,
                       અચલ VInsertionSet<અચલ AstVar*>& argWrap) VL_MT_DISABLED;
    સ્થિર string assignInternalToDpi(AstVar* portp, bool isPtr, અચલ string& frSuffix,
                                      અચલ string& toSuffix,
                                      અચલ string& frPrefix = "") VL_MT_DISABLED;
    સ્થિર string assignDpiToInternal(અચલ string& lhsName, AstVar* rhsp) VL_MT_DISABLED;
    સ્થિર અચલ અક્ષર* dpiTemporaryVarSuffix() VL_MT_SAFE { પાછા s_dpiTemporaryVarSuffix; }
};

#endif  // Guard
