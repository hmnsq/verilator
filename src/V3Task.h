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

#اگر_غیر_تعریف_شدہ VERILATOR_V3TASK_H_
#تعریف_کرو VERILATOR_V3TASK_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3Container.h"
#شامل "V3Error.h"

#شامل <utility>
#شامل <vector>

//============================================================================

using V3TaskConnect = std::pair<AstVar*, AstArg*>;  // [port, pin-connects-to]
using V3TaskConnects = std::سمتیہ<V3TaskConnect>;  // [ [port, pin-connects-to] ... ]

class V3TaskConnectState final {
    VNDeleter m_deleter;  // Allow delayed deletion of nodes
    bool m_didWrap = false;  // Made a wrapper
    using WrapMap = std::map<std::pair<Ast_گرہ_FTask*, std::string>, Ast_گرہ_FTask*>;
    WrapMap m_wrapMap;  // Map of {old function, arguments} -> new function
عوامی:
    V3TaskConnectState() = طےشدہ;
    ~V3TaskConnectState() = طےشدہ;
    خالی ڈالو_Deletep(Ast_گرہ* nodep) { m_deleter.ڈالو_Deletep(nodep); }
    bool didWrap() ثابت { واپس m_didWrap; }
    خالی سیٹ_Did_Wrap() { m_didWrap = true; }
    WrapMap& wrapMap() { واپس m_wrapMap; }
};

//============================================================================

class V3Task final {
    ساکن ثابت حرف* ثابت s_dpiTemporaryVarSuffix;

عوامی:
    ساکن خالی taskAll(AstNetlist* nodep) VL_MT_DISABLED;
    /// Return vector of [port, pin-connects-to]  (SLOW)
    ساکن V3TaskConnects taskConnects(Ast_گرہ_FTask_Ref* nodep, Ast_گرہ* taskStmtsp,
                                       V3TaskConnectState* statep = nullptr,
                                       bool makeChanges = true) VL_MT_DISABLED;
    ساکن خالی taskConnectWrap(Ast_گرہ_FTask_Ref* nodep, ثابت V3TaskConnects& tconnects,
                                V3TaskConnectState* statep,
                                ثابت VInsertion_سیٹ<ثابت AstVar*>& argWrap) VL_MT_DISABLED;
    ساکن Ast_گرہ_FTask*
    taskConnectWrapNew(Ast_گرہ_FTask* taskp, ثابت string& newname, ثابت V3TaskConnects& tconnects,
                       ثابت VInsertion_سیٹ<ثابت AstVar*>& argWrap) VL_MT_DISABLED;
    ساکن string تفویض_اندرونی_مقصد_Dpi(AstVar* portp, bool isPtr, ثابت string& frSuffix,
                                      ثابت string& مقصد_Suffix,
                                      ثابت string& fr_سابقہ = "") VL_MT_DISABLED;
    ساکن string تفویض_Dpi_مقصد_اندرونی(ثابت string& lhsName, AstVar* rhsp) VL_MT_DISABLED;
    ساکن ثابت حرف* dpiTemporaryVarSuffix() VL_MT_محفوظ { واپس s_dpiTemporaryVarSuffix; }
};

#اگر_ختم  // Guard
