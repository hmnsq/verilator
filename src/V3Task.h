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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3Container.h"
#ଅନ୍ତର୍ଭାଵ "V3Error.h"

#ଅନ୍ତର୍ଭାଵ <utility>
#ଅନ୍ତର୍ଭାଵ <vector>

//============================================================================

using V3TaskConnect = std::pair<AstVar*, AstArg*>;  // [port, pin-connects-to]
using V3TaskConnects = std::vector<V3TaskConnect>;  // [ [port, pin-connects-to] ... ]

class V3TaskConnectState final {
    VNDeleter m_deleter;  // Allow delayed deletion of nodes
    bool m_didWrap = false;  // Made a wrapper
    using WrapMap = std::map<std::pair<Ast_ଗଣ୍ଠି_FTask*, std::string>, Ast_ଗଣ୍ଠି_FTask*>;
    WrapMap m_wrapMap;  // Map of {old function, arguments} -> new function
ସର୍ବସାଧାରଣ:
    V3TaskConnectState() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~V3TaskConnectState() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ଶୂନ୍ୟ ଠେଲ_Deletep(Ast_ଗଣ୍ଠି* nodep) { m_deleter.ଠେଲ_Deletep(nodep); }
    bool didWrap() ଅଚଳ { ଫେରନ୍ତୁ m_didWrap; }
    ଶୂନ୍ୟ setDidWrap() { m_didWrap = true; }
    WrapMap& wrapMap() { ଫେରନ୍ତୁ m_wrapMap; }
};

//============================================================================

class V3Task final {
    ସ୍ଥିର ଅଚଳ ଅକ୍ଷର* ଅଚଳ s_dpiTemporaryVarSuffix;

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ taskAll(AstNetlist* nodep) VL_MT_DISABLED;
    /// Return vector of [port, pin-connects-to]  (SLOW)
    ସ୍ଥିର V3TaskConnects taskConnects(Ast_ଗଣ୍ଠି_FTask_Ref* nodep, Ast_ଗଣ୍ଠି* taskStmtsp,
                                       V3TaskConnectState* statep = nullptr,
                                       bool makeChanges = true) VL_MT_DISABLED;
    ସ୍ଥିର ଶୂନ୍ୟ taskConnectWrap(Ast_ଗଣ୍ଠି_FTask_Ref* nodep, ଅଚଳ V3TaskConnects& tconnects,
                                V3TaskConnectState* statep,
                                ଅଚଳ VInsertionSet<ଅଚଳ AstVar*>& argWrap) VL_MT_DISABLED;
    ସ୍ଥିର Ast_ଗଣ୍ଠି_FTask*
    taskConnectWrapNew(Ast_ଗଣ୍ଠି_FTask* taskp, ଅଚଳ string& newname, ଅଚଳ V3TaskConnects& tconnects,
                       ଅଚଳ VInsertionSet<ଅଚଳ AstVar*>& argWrap) VL_MT_DISABLED;
    ସ୍ଥିର string assignInternalToDpi(AstVar* portp, bool isPtr, ଅଚଳ string& frSuffix,
                                      ଅଚଳ string& toSuffix,
                                      ଅଚଳ string& frPrefix = "") VL_MT_DISABLED;
    ସ୍ଥିର string assignDpiToInternal(ଅଚଳ string& lhsName, AstVar* rhsp) VL_MT_DISABLED;
    ସ୍ଥିର ଅଚଳ ଅକ୍ଷର* dpiTemporaryVarSuffix() VL_MT_SAFE { ଫେରନ୍ତୁ s_dpiTemporaryVarSuffix; }
};

#endif  // Guard
