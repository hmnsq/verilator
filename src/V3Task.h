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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3TASK_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3TASK_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Container.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"

#ಅನ್ತರ್ಭಾವ <utility>
#ಅನ್ತರ್ಭಾವ <vector>

//============================================================================

using V3TaskConnect = std::pair<AstVar*, AstArg*>;  // [port, pin-connects-to]
using V3TaskConnects = std::ಸದಿಶ<V3TaskConnect>;  // [ [port, pin-connects-to] ... ]

class V3TaskConnectState final {
    VNDeleter m_deleter;  // Allow delayed deletion of nodes
    bool m_didWrap = false;  // Made a wrapper
    using WrapMap = std::map<std::pair<Ast_ಗಂಟು_FTask*, std::string>, Ast_ಗಂಟು_FTask*>;
    WrapMap m_wrapMap;  // Map of {old function, arguments} -> new function
ಸಾರ್ವಜನಿಕ:
    V3TaskConnectState() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~V3TaskConnectState() = ಪೂರ್ವನಿಯೋಜಿತ;
    ಶೂನ್ಯ ತಳ್ಳು_Deletep(Ast_ಗಂಟು* nodep) { m_deleter.ತಳ್ಳು_Deletep(nodep); }
    bool didWrap() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_didWrap; }
    ಶೂನ್ಯ ಗಣ_Did_Wrap() { m_didWrap = true; }
    WrapMap& wrapMap() { ಹಿಂತಿರುಗಿ m_wrapMap; }
};

//============================================================================

class V3Task final {
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ s_dpiTemporaryVarSuffix;

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ taskAll(AstNetlist* nodep) VL_MT_DISABLED;
    /// Return vector of [port, pin-connects-to]  (SLOW)
    ಸ್ಥಿರ V3TaskConnects taskConnects(Ast_ಗಂಟು_FTask_Ref* nodep, Ast_ಗಂಟು* taskStmtsp,
                                       V3TaskConnectState* statep = nullptr,
                                       bool makeChanges = true) VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ taskConnectWrap(Ast_ಗಂಟು_FTask_Ref* nodep, ಸ್ಥಿರಾಂಕ V3TaskConnects& tconnects,
                                V3TaskConnectState* statep,
                                ಸ್ಥಿರಾಂಕ VInsertion_ಗಣ<ಸ್ಥಿರಾಂಕ AstVar*>& argWrap) VL_MT_DISABLED;
    ಸ್ಥಿರ Ast_ಗಂಟು_FTask*
    taskConnectWrapNew(Ast_ಗಂಟು_FTask* taskp, ಸ್ಥಿರಾಂಕ string& newname, ಸ್ಥಿರಾಂಕ V3TaskConnects& tconnects,
                       ಸ್ಥಿರಾಂಕ VInsertion_ಗಣ<ಸ್ಥಿರಾಂಕ AstVar*>& argWrap) VL_MT_DISABLED;
    ಸ್ಥಿರ string ನಿಯೋಜಿಸು_ಆಂತರಿಕ_ಗಮ್ಯ_Dpi(AstVar* portp, bool isPtr, ಸ್ಥಿರಾಂಕ string& frSuffix,
                                      ಸ್ಥಿರಾಂಕ string& ಗಮ್ಯ_Suffix,
                                      ಸ್ಥಿರಾಂಕ string& fr_ಪೂರ್ವಪ್ರತ್ಯಯ = "") VL_MT_DISABLED;
    ಸ್ಥಿರ string ನಿಯೋಜಿಸು_Dpi_ಗಮ್ಯ_ಆಂತರಿಕ(ಸ್ಥಿರಾಂಕ string& lhsName, AstVar* rhsp) VL_MT_DISABLED;
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* dpiTemporaryVarSuffix() VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ s_dpiTemporaryVarSuffix; }
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
