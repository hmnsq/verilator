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

#که_نه_تعریف_شوی VERILATOR_V3TASK_H_
#تعریف_کړه VERILATOR_V3TASK_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Container.h"
#ضمن "V3Error.h"

#ضمن <utility>
#ضمن <vector>

//============================================================================

using V3TaskConnect = std::pair<AstVar*, AstArg*>;  // [port, pin-connects-to]
using V3TaskConnects = std::ویکټور<V3TaskConnect>;  // [ [port, pin-connects-to] ... ]

class V3TaskConnectState final {
    VNDeleter m_deleter;  // Allow delayed deletion of nodes
    bool m_didWrap = false;  // Made a wrapper
    using WrapMap = std::map<std::pair<Ast_غوټه_FTask*, std::string>, Ast_غوټه_FTask*>;
    WrapMap m_wrapMap;  // Map of {old function, arguments} -> new function
عام:
    V3TaskConnectState() = افتراضي;
    ~V3TaskConnectState() = افتراضي;
    فراغ ټېل_Deletep(Ast_غوټه* nodep) { m_deleter.ټېل_Deletep(nodep); }
    bool didWrap() ثابت { أرجع m_didWrap; }
    فراغ سېټ_Did_Wrap() { m_didWrap = true; }
    WrapMap& wrapMap() { أرجع m_wrapMap; }
};

//============================================================================

class V3Task final {
    ساكن ثابت حرف* ثابت s_dpiTemporaryVarSuffix;

عام:
    ساكن فراغ taskAll(AstNetlist* nodep) VL_MT_DISABLED;
    /// Return vector of [port, pin-connects-to]  (SLOW)
    ساكن V3TaskConnects taskConnects(Ast_غوټه_FTask_Ref* nodep, Ast_غوټه* taskStmtsp,
                                       V3TaskConnectState* statep = nullptr,
                                       bool makeChanges = true) VL_MT_DISABLED;
    ساكن فراغ taskConnectWrap(Ast_غوټه_FTask_Ref* nodep, ثابت V3TaskConnects& tconnects,
                                V3TaskConnectState* statep,
                                ثابت VInsertion_سېټ<ثابت AstVar*>& argWrap) VL_MT_DISABLED;
    ساكن Ast_غوټه_FTask*
    taskConnectWrapNew(Ast_غوټه_FTask* taskp, ثابت string& newname, ثابت V3TaskConnects& tconnects,
                       ثابت VInsertion_سېټ<ثابت AstVar*>& argWrap) VL_MT_DISABLED;
    ساكن string ټاکل_دننی_منزل_Dpi(AstVar* portp, bool isPtr, ثابت string& frSuffix,
                                      ثابت string& منزل_Suffix,
                                      ثابت string& fr_مختاړی = "") VL_MT_DISABLED;
    ساكن string ټاکل_Dpi_منزل_دننی(ثابت string& lhsName, AstVar* rhsp) VL_MT_DISABLED;
    ساكن ثابت حرف* dpiTemporaryVarSuffix() VL_MT_خوندي { أرجع s_dpiTemporaryVarSuffix; }
};

#د_که_پای  // Guard
