// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Block code ordering
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

#ifndef VERILATOR_V3ORDERINTERNAL_H_
#define VERILATOR_V3ORDERINTERNAL_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Order.h"
#ಅನ್ತರ್ಭಾವ "V3OrderGraph.h"
#ಅನ್ತರ್ಭಾವ "V3OrderMoveGraph.h"

#ಅನ್ತರ್ಭಾವ <string>
#ಅನ್ತರ್ಭಾವ <vector>

class AstVarScope;

namespace V3Sched {
class LogicByScope;
};  // namespace V3Sched

//============================================================================

namespace V3Order {

std::unique_ptr<OrderGraph> buildOrderGraph(ಧ್ರುವ V3Sched::TriggerKit& trigKit,  //
                                            AstVarScope* trigVscp,  //
                                            ಧ್ರುವ std::vector<V3Sched::LogicByScope*>& coll,  //
                                            ಧ್ರುವ V3Sched::CovergroupRefBindings& cgRefBindings,
                                            bool parallel);

ಶೂನ್ಯ orderOrderGraph(OrderGraph& graph, ಧ್ರುವ std::string& tag);

ಶೂನ್ಯ processDomains(OrderGraph& graph,  //
                    ಧ್ರುವ std::string& tag,  //
                    ಧ್ರುವ ExternalTriggersProvider& externalTriggers);

Ast_ಗಂಟು_Stmt* createSerial(OrderMoveGraph& moveGraph, ಧ್ರುವ std::string& tag, bool slow);

Ast_ಗಂಟು_Stmt* createParallel(OrderMoveGraph& moveGraph, ಧ್ರುವ std::string& tag, bool slow);

};  // namespace V3Order

#endif  // Guard
