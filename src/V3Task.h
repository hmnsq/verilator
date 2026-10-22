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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Container.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"

#ಅನ್ತರ್ಭಾವ <utility>
#ಅನ್ತರ್ಭಾವ <vector>

//============================================================================

using V3TaskConnect = std::pair<AstVar*, AstArg*>;  // [port, pin-connects-to]
using V3TaskConnects = std::vector<V3TaskConnect>;  // [ [port, pin-connects-to] ... ]

class V3TaskConnectState final {
    VNDeleter m_deleter;  // Allow delayed deletion of nodes
    bool m_didWrap = false;  // Made a wrapper
    using WrapMap = std::map<std::pair<Ast_ಗಂಟು_FTask*, std::string>, Ast_ಗಂಟು_FTask*>;
    WrapMap m_wrapMap;  // Map of {old function, arguments} -> new function
ಸಾರ್ವಜನಿಕ:
    V3TaskConnectState() = ಸ್ವಭಾವ;
    ~V3TaskConnectState() = ಸ್ವಭಾವ;
    ಶೂನ್ಯ ತಳ್ಳು_Deletep(Ast_ಗಂಟು* nodep) { m_deleter.ತಳ್ಳು_Deletep(nodep); }
    bool didWrap() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_didWrap; }
    ಶೂನ್ಯ setDidWrap() { m_didWrap = true; }
    WrapMap& wrapMap() { ಪ್ರತ್ಯಾಗಮ m_wrapMap; }
};

//============================================================================

class V3Task final {
    ಸ್ಥಿರ ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ s_dpiTemporaryVarSuffix;

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ taskAll(AstNetlist* nodep) VL_MT_DISABLED;
    /// Return vector of [port, pin-connects-to]  (SLOW)
    ಸ್ಥಿರ V3TaskConnects taskConnects(Ast_ಗಂಟು_FTask_Ref* nodep, Ast_ಗಂಟು* taskStmtsp,
                                       V3TaskConnectState* statep = nullptr,
                                       bool makeChanges = true) VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ taskConnectWrap(Ast_ಗಂಟು_FTask_Ref* nodep, ಧ್ರುವ V3TaskConnects& tconnects,
                                V3TaskConnectState* statep,
                                ಧ್ರುವ VInsertionSet<ಧ್ರುವ AstVar*>& argWrap) VL_MT_DISABLED;
    ಸ್ಥಿರ Ast_ಗಂಟು_FTask*
    taskConnectWrapNew(Ast_ಗಂಟು_FTask* taskp, ಧ್ರುವ string& newname, ಧ್ರುವ V3TaskConnects& tconnects,
                       ಧ್ರುವ VInsertionSet<ಧ್ರುವ AstVar*>& argWrap) VL_MT_DISABLED;
    ಸ್ಥಿರ string assignInternalToDpi(AstVar* portp, bool isPtr, ಧ್ರುವ string& frSuffix,
                                      ಧ್ರುವ string& toSuffix,
                                      ಧ್ರುವ string& frPrefix = "") VL_MT_DISABLED;
    ಸ್ಥಿರ string assignDpiToInternal(ಧ್ರುವ string& lhsName, AstVar* rhsp) VL_MT_DISABLED;
    ಸ್ಥಿರ ಧ್ರುವ ವರ್ಣ* dpiTemporaryVarSuffix() VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ s_dpiTemporaryVarSuffix; }
};

#endif  // Guard
