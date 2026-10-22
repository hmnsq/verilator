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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Container.h"
#अन्तर्भाव "V3Error.h"

#अन्तर्भाव <utility>
#अन्तर्भाव <vector>

//============================================================================

using V3TaskConnect = std::pair<AstVar*, AstArg*>;  // [port, pin-connects-to]
using V3TaskConnects = std::vector<V3TaskConnect>;  // [ [port, pin-connects-to] ... ]

class V3TaskConnectState final {
    VNDeleter m_deleter;  // Allow delayed deletion of nodes
    bool m_didWrap = false;  // Made a wrapper
    using WrapMap = std::map<std::pair<Ast_गाठ_FTask*, std::string>, Ast_गाठ_FTask*>;
    WrapMap m_wrapMap;  // Map of {old function, arguments} -> new function
सार्वजनिक:
    V3TaskConnectState() = पूर्वनिर्धारित;
    ~V3TaskConnectState() = पूर्वनिर्धारित;
    शून्य ढकला_Deletep(Ast_गाठ* nodep) { m_deleter.ढकला_Deletep(nodep); }
    bool didWrap() स्थिरांक { परत m_didWrap; }
    शून्य setDidWrap() { m_didWrap = true; }
    WrapMap& wrapMap() { परत m_wrapMap; }
};

//============================================================================

class V3Task final {
    स्थिर स्थिरांक अक्षर* स्थिरांक s_dpiTemporaryVarSuffix;

सार्वजनिक:
    स्थिर शून्य taskAll(AstNetlist* nodep) VL_MT_DISABLED;
    /// Return vector of [port, pin-connects-to]  (SLOW)
    स्थिर V3TaskConnects taskConnects(Ast_गाठ_FTask_Ref* nodep, Ast_गाठ* taskStmtsp,
                                       V3TaskConnectState* statep = nullptr,
                                       bool makeChanges = true) VL_MT_DISABLED;
    स्थिर शून्य taskConnectWrap(Ast_गाठ_FTask_Ref* nodep, स्थिरांक V3TaskConnects& tconnects,
                                V3TaskConnectState* statep,
                                स्थिरांक VInsertionSet<स्थिरांक AstVar*>& argWrap) VL_MT_DISABLED;
    स्थिर Ast_गाठ_FTask*
    taskConnectWrapNew(Ast_गाठ_FTask* taskp, स्थिरांक string& newname, स्थिरांक V3TaskConnects& tconnects,
                       स्थिरांक VInsertionSet<स्थिरांक AstVar*>& argWrap) VL_MT_DISABLED;
    स्थिर string assignInternalToDpi(AstVar* portp, bool isPtr, स्थिरांक string& frSuffix,
                                      स्थिरांक string& toSuffix,
                                      स्थिरांक string& frPrefix = "") VL_MT_DISABLED;
    स्थिर string assignDpiToInternal(स्थिरांक string& lhsName, AstVar* rhsp) VL_MT_DISABLED;
    स्थिर स्थिरांक अक्षर* dpiTemporaryVarSuffix() VL_MT_SAFE { परत s_dpiTemporaryVarSuffix; }
};

#endif  // Guard
