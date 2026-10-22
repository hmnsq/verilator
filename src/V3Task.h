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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3Container.h"
#شامل "V3Error.h"

#شامل <utility>
#شامل <vector>

//============================================================================

using V3TaskConnect = std::pair<AstVar*, AstArg*>;  // [port, pin-connects-to]
using V3TaskConnects = std::vector<V3TaskConnect>;  // [ [port, pin-connects-to] ... ]

class V3TaskConnectState final {
    VNDeleter m_deleter;  // Allow delayed deletion of nodes
    bool m_didWrap = false;  // Made a wrapper
    using WrapMap = std::map<std::pair<Ast_گره_FTask*, std::string>, Ast_گره_FTask*>;
    WrapMap m_wrapMap;  // Map of {old function, arguments} -> new function
عمومی:
    V3TaskConnectState() = پیشفرض;
    ~V3TaskConnectState() = پیشفرض;
    تهی افزودن_Deletep(Ast_گره* nodep) { m_deleter.افزودن_Deletep(nodep); }
    bool didWrap() ثابت { بازگشت m_didWrap; }
    تهی setDidWrap() { m_didWrap = true; }
    WrapMap& wrapMap() { بازگشت m_wrapMap; }
};

//============================================================================

class V3Task final {
    ایستا ثابت نویسه* ثابت s_dpiTemporaryVarSuffix;

عمومی:
    ایستا تهی taskAll(AstNetlist* nodep) VL_MT_DISABLED;
    /// Return vector of [port, pin-connects-to]  (SLOW)
    ایستا V3TaskConnects taskConnects(Ast_گره_FTask_Ref* nodep, Ast_گره* taskStmtsp,
                                       V3TaskConnectState* statep = nullptr,
                                       bool makeChanges = true) VL_MT_DISABLED;
    ایستا تهی taskConnectWrap(Ast_گره_FTask_Ref* nodep, ثابت V3TaskConnects& tconnects,
                                V3TaskConnectState* statep,
                                ثابت VInsertionSet<ثابت AstVar*>& argWrap) VL_MT_DISABLED;
    ایستا Ast_گره_FTask*
    taskConnectWrapNew(Ast_گره_FTask* taskp, ثابت string& newname, ثابت V3TaskConnects& tconnects,
                       ثابت VInsertionSet<ثابت AstVar*>& argWrap) VL_MT_DISABLED;
    ایستا string assignInternalToDpi(AstVar* portp, bool isPtr, ثابت string& frSuffix,
                                      ثابت string& toSuffix,
                                      ثابت string& frPrefix = "") VL_MT_DISABLED;
    ایستا string assignDpiToInternal(ثابت string& lhsName, AstVar* rhsp) VL_MT_DISABLED;
    ایستا ثابت نویسه* dpiTemporaryVarSuffix() VL_MT_SAFE { بازگشت s_dpiTemporaryVarSuffix; }
};

#endif  // Guard
