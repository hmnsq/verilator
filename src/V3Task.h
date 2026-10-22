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
    using WrapMap = std::map<std::pair<Ast_ग्रन्थिः_FTask*, std::string>, Ast_ग्रन्थिः_FTask*>;
    WrapMap m_wrapMap;  // Map of {old function, arguments} -> new function
सार्वजनिक:
    V3TaskConnectState() = स्वभाव;
    ~V3TaskConnectState() = स्वभाव;
    शून्य प्रक्षिप_Deletep(Ast_ग्रन्थिः* nodep) { m_deleter.प्रक्षिप_Deletep(nodep); }
    bool didWrap() ध्रुव { प्रत्यागम m_didWrap; }
    शून्य setDidWrap() { m_didWrap = true; }
    WrapMap& wrapMap() { प्रत्यागम m_wrapMap; }
};

//============================================================================

class V3Task final {
    स्थिर ध्रुव वर्ण* ध्रुव s_dpiTemporaryVarSuffix;

सार्वजनिक:
    स्थिर शून्य taskAll(AstNetlist* nodep) VL_MT_DISABLED;
    /// Return vector of [port, pin-connects-to]  (SLOW)
    स्थिर V3TaskConnects taskConnects(Ast_ग्रन्थिः_FTask_Ref* nodep, Ast_ग्रन्थिः* taskStmtsp,
                                       V3TaskConnectState* statep = nullptr,
                                       bool makeChanges = true) VL_MT_DISABLED;
    स्थिर शून्य taskConnectWrap(Ast_ग्रन्थिः_FTask_Ref* nodep, ध्रुव V3TaskConnects& tconnects,
                                V3TaskConnectState* statep,
                                ध्रुव VInsertionSet<ध्रुव AstVar*>& argWrap) VL_MT_DISABLED;
    स्थिर Ast_ग्रन्थिः_FTask*
    taskConnectWrapNew(Ast_ग्रन्थिः_FTask* taskp, ध्रुव string& newname, ध्रुव V3TaskConnects& tconnects,
                       ध्रुव VInsertionSet<ध्रुव AstVar*>& argWrap) VL_MT_DISABLED;
    स्थिर string assignInternalToDpi(AstVar* portp, bool isPtr, ध्रुव string& frSuffix,
                                      ध्रुव string& toSuffix,
                                      ध्रुव string& frPrefix = "") VL_MT_DISABLED;
    स्थिर string assignDpiToInternal(ध्रुव string& lhsName, AstVar* rhsp) VL_MT_DISABLED;
    स्थिर ध्रुव वर्ण* dpiTemporaryVarSuffix() VL_MT_SAFE { प्रत्यागम s_dpiTemporaryVarSuffix; }
};

#endif  // Guard
