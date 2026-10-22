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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3TASK_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3TASK_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Container.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <utility>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

//============================================================================

using V3TaskConnect = std::pair<AstVar*, AstArg*>;  // [port, pin-connects-to]
using V3TaskConnects = std::ਸਦਿਸ਼<V3TaskConnect>;  // [ [port, pin-connects-to] ... ]

class V3TaskConnectState final {
    VNDeleter m_deleter;  // Allow delayed deletion of nodes
    bool m_didWrap = false;  // Made a wrapper
    using WrapMap = std::map<std::pair<Ast_ਗੰਢ_FTask*, std::string>, Ast_ਗੰਢ_FTask*>;
    WrapMap m_wrapMap;  // Map of {old function, arguments} -> new function
ਜਨਤਕ:
    V3TaskConnectState() = ਮੂਲ;
    ~V3TaskConnectState() = ਮੂਲ;
    ਖਾਲੀ ਧੱਕੋ_Deletep(Ast_ਗੰਢ* nodep) { m_deleter.ਧੱਕੋ_Deletep(nodep); }
    bool didWrap() ਅਚਲ { ਵਾਪਸ m_didWrap; }
    ਖਾਲੀ ਸੈੱਟ_Did_Wrap() { m_didWrap = true; }
    WrapMap& wrapMap() { ਵਾਪਸ m_wrapMap; }
};

//============================================================================

class V3Task final {
    ਸਥਿਰ ਅਚਲ ਅੱਖਰ* ਅਚਲ s_dpiTemporaryVarSuffix;

ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ taskAll(AstNetlist* nodep) VL_MT_DISABLED;
    /// Return vector of [port, pin-connects-to]  (SLOW)
    ਸਥਿਰ V3TaskConnects taskConnects(Ast_ਗੰਢ_FTask_Ref* nodep, Ast_ਗੰਢ* taskStmtsp,
                                       V3TaskConnectState* statep = nullptr,
                                       bool makeChanges = true) VL_MT_DISABLED;
    ਸਥਿਰ ਖਾਲੀ taskConnectWrap(Ast_ਗੰਢ_FTask_Ref* nodep, ਅਚਲ V3TaskConnects& tconnects,
                                V3TaskConnectState* statep,
                                ਅਚਲ VInsertion_ਸੈੱਟ<ਅਚਲ AstVar*>& argWrap) VL_MT_DISABLED;
    ਸਥਿਰ Ast_ਗੰਢ_FTask*
    taskConnectWrapNew(Ast_ਗੰਢ_FTask* taskp, ਅਚਲ string& newname, ਅਚਲ V3TaskConnects& tconnects,
                       ਅਚਲ VInsertion_ਸੈੱਟ<ਅਚਲ AstVar*>& argWrap) VL_MT_DISABLED;
    ਸਥਿਰ string ਸੌਂਪੋ_ਅੰਦਰੂਨੀ_ਮੰਜ਼ਿਲ_Dpi(AstVar* portp, bool isPtr, ਅਚਲ string& frSuffix,
                                      ਅਚਲ string& ਮੰਜ਼ਿਲ_Suffix,
                                      ਅਚਲ string& fr_ਅਗੇਤਰ = "") VL_MT_DISABLED;
    ਸਥਿਰ string ਸੌਂਪੋ_Dpi_ਮੰਜ਼ਿਲ_ਅੰਦਰੂਨੀ(ਅਚਲ string& lhsName, AstVar* rhsp) VL_MT_DISABLED;
    ਸਥਿਰ ਅਚਲ ਅੱਖਰ* dpiTemporaryVarSuffix() VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ s_dpiTemporaryVarSuffix; }
};

#ਜੇ_ਅੰਤ  // Guard
