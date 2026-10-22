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

#अगर_अघोषित VERILATOR_V3TASK_H_
#घोषणा VERILATOR_V3TASK_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Ast.h"
#समावेश "V3Container.h"
#समावेश "V3Error.h"

#समावेश <utility>
#समावेश <vector>

//============================================================================

using V3TaskConnect = std::pair<AstVar*, AstArg*>;  // [port, pin-connects-to]
using V3TaskConnects = std::vector<V3TaskConnect>;  // [ [port, pin-connects-to] ... ]

class V3TaskConnectState final {
    VNDeleter m_deleter;  // Allow delayed deletion of nodes
    bool m_didWrap = false;  // Made a wrapper
    using WrapMap = std::map<std::pair<Ast_गाँठ_FTask*, std::string>, Ast_गाँठ_FTask*>;
    WrapMap m_wrapMap;  // Map of {old function, arguments} -> new function
खुला:
    V3TaskConnectState() = शेष;
    ~V3TaskConnectState() = शेष;
    व्योम धकेलो_Deletep(Ast_गाँठ* nodep) { m_deleter.धकेलो_Deletep(nodep); }
    bool didWrap() स्थिर { वापस m_didWrap; }
    व्योम setDidWrap() { m_didWrap = true; }
    WrapMap& wrapMap() { वापस m_wrapMap; }
};

//============================================================================

class V3Task final {
    अटल स्थिर अक्षर* स्थिर s_dpiTemporaryVarSuffix;

खुला:
    अटल व्योम taskAll(AstNetlist* nodep) VL_MT_DISABLED;
    /// Return vector of [port, pin-connects-to]  (SLOW)
    अटल V3TaskConnects taskConnects(Ast_गाँठ_FTask_Ref* nodep, Ast_गाँठ* taskStmtsp,
                                       V3TaskConnectState* statep = nullptr,
                                       bool makeChanges = true) VL_MT_DISABLED;
    अटल व्योम taskConnectWrap(Ast_गाँठ_FTask_Ref* nodep, स्थिर V3TaskConnects& tconnects,
                                V3TaskConnectState* statep,
                                स्थिर VInsertionSet<स्थिर AstVar*>& argWrap) VL_MT_DISABLED;
    अटल Ast_गाँठ_FTask*
    taskConnectWrapNew(Ast_गाँठ_FTask* taskp, स्थिर string& newname, स्थिर V3TaskConnects& tconnects,
                       स्थिर VInsertionSet<स्थिर AstVar*>& argWrap) VL_MT_DISABLED;
    अटल string assignInternalToDpi(AstVar* portp, bool isPtr, स्थिर string& frSuffix,
                                      स्थिर string& toSuffix,
                                      स्थिर string& frPrefix = "") VL_MT_DISABLED;
    अटल string assignDpiToInternal(स्थिर string& lhsName, AstVar* rhsp) VL_MT_DISABLED;
    अटल स्थिर अक्षर* dpiTemporaryVarSuffix() VL_MT_SAFE { वापस s_dpiTemporaryVarSuffix; }
};

#पूर्ण_अगर  // Guard
