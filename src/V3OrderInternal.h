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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Order.h"
#અન્તર્ભાવ "V3OrderGraph.h"
#અન્તર્ભાવ "V3OrderMoveGraph.h"

#અન્તર્ભાવ <string>
#અન્તર્ભાવ <vector>

class AstVarScope;

namespace V3Sched {
class LogicByScope;
};  // namespace V3Sched

//============================================================================

namespace V3Order {

std::unique_ptr<OrderGraph> buildOrderGraph(અચલ V3Sched::TriggerKit& trigKit,  //
                                            AstVarScope* trigVscp,  //
                                            અચલ std::vector<V3Sched::LogicByScope*>& coll,  //
                                            અચલ V3Sched::CovergroupRefBindings& cgRefBindings,
                                            bool parallel);

શૂન્ય orderOrderGraph(OrderGraph& graph, અચલ std::string& tag);

શૂન્ય processDomains(OrderGraph& graph,  //
                    અચલ std::string& tag,  //
                    અચલ ExternalTriggersProvider& externalTriggers);

Ast_ગાંઠ_Stmt* createSerial(OrderMoveGraph& moveGraph, અચલ std::string& tag, bool slow);

Ast_ગાંઠ_Stmt* createParallel(OrderMoveGraph& moveGraph, અચલ std::string& tag, bool slow);

};  // namespace V3Order

#endif  // Guard
