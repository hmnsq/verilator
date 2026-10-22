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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Order.h"
#ضمن "V3OrderGraph.h"
#ضمن "V3OrderMoveGraph.h"

#ضمن <string>
#ضمن <vector>

class AstVarScope;

namespace V3Sched {
class LogicByScope;
};  // namespace V3Sched

//============================================================================

namespace V3Order {

std::unique_ptr<OrderGraph> buildOrderGraph(ثابت V3Sched::TriggerKit& trigKit,  //
                                            AstVarScope* trigVscp,  //
                                            ثابت std::vector<V3Sched::LogicByScope*>& coll,  //
                                            ثابت V3Sched::CovergroupRefBindings& cgRefBindings,
                                            bool parallel);

فراغ orderOrderGraph(OrderGraph& graph, ثابت std::string& tag);

فراغ processDomains(OrderGraph& graph,  //
                    ثابت std::string& tag,  //
                    ثابت ExternalTriggersProvider& externalTriggers);

Ast_نود_Stmt* createSerial(OrderMoveGraph& moveGraph, ثابت std::string& tag, bool slow);

Ast_نود_Stmt* createParallel(OrderMoveGraph& moveGraph, ثابت std::string& tag, bool slow);

};  // namespace V3Order

#endif  // Guard
