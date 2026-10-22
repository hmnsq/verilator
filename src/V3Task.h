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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3Container.h"
#அந்தர்பாவ "V3Error.h"

#அந்தர்பாவ <utility>
#அந்தர்பாவ <vector>

//============================================================================

using V3TaskConnect = std::pair<AstVar*, AstArg*>;  // [port, pin-connects-to]
using V3TaskConnects = std::vector<V3TaskConnect>;  // [ [port, pin-connects-to] ... ]

class V3TaskConnectState final {
    VNDeleter m_deleter;  // Allow delayed deletion of nodes
    bool m_didWrap = false;  // Made a wrapper
    using WrapMap = std::map<std::pair<Ast_முனை_FTask*, std::string>, Ast_முனை_FTask*>;
    WrapMap m_wrapMap;  // Map of {old function, arguments} -> new function
பொது:
    V3TaskConnectState() = இயல்பு;
    ~V3TaskConnectState() = இயல்பு;
    வெறுமை தள்ளு_Deletep(Ast_முனை* nodep) { m_deleter.தள்ளு_Deletep(nodep); }
    bool didWrap() மாறிலி { திரும்பு m_didWrap; }
    வெறுமை setDidWrap() { m_didWrap = true; }
    WrapMap& wrapMap() { திரும்பு m_wrapMap; }
};

//============================================================================

class V3Task final {
    நிலையான மாறிலி எழுத்து* மாறிலி s_dpiTemporaryVarSuffix;

பொது:
    நிலையான வெறுமை taskAll(AstNetlist* nodep) VL_MT_DISABLED;
    /// Return vector of [port, pin-connects-to]  (SLOW)
    நிலையான V3TaskConnects taskConnects(Ast_முனை_FTask_Ref* nodep, Ast_முனை* taskStmtsp,
                                       V3TaskConnectState* statep = nullptr,
                                       bool makeChanges = true) VL_MT_DISABLED;
    நிலையான வெறுமை taskConnectWrap(Ast_முனை_FTask_Ref* nodep, மாறிலி V3TaskConnects& tconnects,
                                V3TaskConnectState* statep,
                                மாறிலி VInsertionSet<மாறிலி AstVar*>& argWrap) VL_MT_DISABLED;
    நிலையான Ast_முனை_FTask*
    taskConnectWrapNew(Ast_முனை_FTask* taskp, மாறிலி string& newname, மாறிலி V3TaskConnects& tconnects,
                       மாறிலி VInsertionSet<மாறிலி AstVar*>& argWrap) VL_MT_DISABLED;
    நிலையான string assignInternalToDpi(AstVar* portp, bool isPtr, மாறிலி string& frSuffix,
                                      மாறிலி string& toSuffix,
                                      மாறிலி string& frPrefix = "") VL_MT_DISABLED;
    நிலையான string assignDpiToInternal(மாறிலி string& lhsName, AstVar* rhsp) VL_MT_DISABLED;
    நிலையான மாறிலி எழுத்து* dpiTemporaryVarSuffix() VL_MT_SAFE { திரும்பு s_dpiTemporaryVarSuffix; }
};

#endif  // Guard
