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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ "V3Ast.h"
#অন্তর্ভাৱ "V3Container.h"
#অন্তর্ভাৱ "V3Error.h"

#অন্তর্ভাৱ <utility>
#অন্তর্ভাৱ <vector>

//============================================================================

using V3TaskConnect = std::pair<AstVar*, AstArg*>;  // [port, pin-connects-to]
using V3TaskConnects = std::vector<V3TaskConnect>;  // [ [port, pin-connects-to] ... ]

class V3TaskConnectState final {
    VNDeleter m_deleter;  // Allow delayed deletion of nodes
    bool m_didWrap = false;  // Made a wrapper
    using WrapMap = std::map<std::pair<Ast_গ্ৰন্থি_FTask*, std::string>, Ast_গ্ৰন্থি_FTask*>;
    WrapMap m_wrapMap;  // Map of {old function, arguments} -> new function
সার্ৱজনিক:
    V3TaskConnectState() = স্ৱভাৱ;
    ~V3TaskConnectState() = স্ৱভাৱ;
    শূন্য ঠেলো_Deletep(Ast_গ্ৰন্থি* nodep) { m_deleter.ঠেলো_Deletep(nodep); }
    bool didWrap() ধ্রুৱ { ঘূৰাই m_didWrap; }
    শূন্য setDidWrap() { m_didWrap = true; }
    WrapMap& wrapMap() { ঘূৰাই m_wrapMap; }
};

//============================================================================

class V3Task final {
    স্থির ধ্রুৱ ৱর্ণ* ধ্রুৱ s_dpiTemporaryVarSuffix;

সার্ৱজনিক:
    স্থির শূন্য taskAll(AstNetlist* nodep) VL_MT_DISABLED;
    /// Return vector of [port, pin-connects-to]  (SLOW)
    স্থির V3TaskConnects taskConnects(Ast_গ্ৰন্থি_FTask_Ref* nodep, Ast_গ্ৰন্থি* taskStmtsp,
                                       V3TaskConnectState* statep = nullptr,
                                       bool makeChanges = true) VL_MT_DISABLED;
    স্থির শূন্য taskConnectWrap(Ast_গ্ৰন্থি_FTask_Ref* nodep, ধ্রুৱ V3TaskConnects& tconnects,
                                V3TaskConnectState* statep,
                                ধ্রুৱ VInsertionSet<ধ্রুৱ AstVar*>& argWrap) VL_MT_DISABLED;
    স্থির Ast_গ্ৰন্থি_FTask*
    taskConnectWrapNew(Ast_গ্ৰন্থি_FTask* taskp, ধ্রুৱ string& newname, ধ্রুৱ V3TaskConnects& tconnects,
                       ধ্রুৱ VInsertionSet<ধ্রুৱ AstVar*>& argWrap) VL_MT_DISABLED;
    স্থির string assignInternalToDpi(AstVar* portp, bool isPtr, ধ্রুৱ string& frSuffix,
                                      ধ্রুৱ string& toSuffix,
                                      ধ্রুৱ string& frPrefix = "") VL_MT_DISABLED;
    স্থির string assignDpiToInternal(ধ্রুৱ string& lhsName, AstVar* rhsp) VL_MT_DISABLED;
    স্থির ধ্রুৱ ৱর্ণ* dpiTemporaryVarSuffix() VL_MT_SAFE { ঘূৰাই s_dpiTemporaryVarSuffix; }
};

#endif  // Guard
