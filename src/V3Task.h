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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3Container.h"
#അന്തര്ഭാവ "V3Error.h"

#അന്തര്ഭാവ <utility>
#അന്തര്ഭാവ <vector>

//============================================================================

using V3TaskConnect = std::pair<AstVar*, AstArg*>;  // [port, pin-connects-to]
using V3TaskConnects = std::vector<V3TaskConnect>;  // [ [port, pin-connects-to] ... ]

class V3TaskConnectState final {
    VNDeleter m_deleter;  // Allow delayed deletion of nodes
    bool m_didWrap = false;  // Made a wrapper
    using WrapMap = std::map<std::pair<Ast_കണ്ണി_FTask*, std::string>, Ast_കണ്ണി_FTask*>;
    WrapMap m_wrapMap;  // Map of {old function, arguments} -> new function
പൊതു:
    V3TaskConnectState() = സ്ഥിരസ്ഥിതി;
    ~V3TaskConnectState() = സ്ഥിരസ്ഥിതി;
    ശൂന്യം തള്ളുക_Deletep(Ast_കണ്ണി* nodep) { m_deleter.തള്ളുക_Deletep(nodep); }
    bool didWrap() സ്ഥിരം { തിരികെ m_didWrap; }
    ശൂന്യം setDidWrap() { m_didWrap = true; }
    WrapMap& wrapMap() { തിരികെ m_wrapMap; }
};

//============================================================================

class V3Task final {
    സ്ഥിതം സ്ഥിരം അക്ഷരം* സ്ഥിരം s_dpiTemporaryVarSuffix;

പൊതു:
    സ്ഥിതം ശൂന്യം taskAll(AstNetlist* nodep) VL_MT_DISABLED;
    /// Return vector of [port, pin-connects-to]  (SLOW)
    സ്ഥിതം V3TaskConnects taskConnects(Ast_കണ്ണി_FTask_Ref* nodep, Ast_കണ്ണി* taskStmtsp,
                                       V3TaskConnectState* statep = nullptr,
                                       bool makeChanges = true) VL_MT_DISABLED;
    സ്ഥിതം ശൂന്യം taskConnectWrap(Ast_കണ്ണി_FTask_Ref* nodep, സ്ഥിരം V3TaskConnects& tconnects,
                                V3TaskConnectState* statep,
                                സ്ഥിരം VInsertionSet<സ്ഥിരം AstVar*>& argWrap) VL_MT_DISABLED;
    സ്ഥിതം Ast_കണ്ണി_FTask*
    taskConnectWrapNew(Ast_കണ്ണി_FTask* taskp, സ്ഥിരം string& newname, സ്ഥിരം V3TaskConnects& tconnects,
                       സ്ഥിരം VInsertionSet<സ്ഥിരം AstVar*>& argWrap) VL_MT_DISABLED;
    സ്ഥിതം string assignInternalToDpi(AstVar* portp, bool isPtr, സ്ഥിരം string& frSuffix,
                                      സ്ഥിരം string& toSuffix,
                                      സ്ഥിരം string& frPrefix = "") VL_MT_DISABLED;
    സ്ഥിതം string assignDpiToInternal(സ്ഥിരം string& lhsName, AstVar* rhsp) VL_MT_DISABLED;
    സ്ഥിതം സ്ഥിരം അക്ഷരം* dpiTemporaryVarSuffix() VL_MT_SAFE { തിരികെ s_dpiTemporaryVarSuffix; }
};

#endif  // Guard
