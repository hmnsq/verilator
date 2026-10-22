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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Order.h"
#அந்தர்பாவ "V3OrderGraph.h"
#அந்தர்பாவ "V3OrderMoveGraph.h"

#அந்தர்பாவ <string>
#அந்தர்பாவ <vector>

class AstVarScope;

namespace V3Sched {
class LogicByScope;
};  // namespace V3Sched

//============================================================================

namespace V3Order {

std::unique_ptr<OrderGraph> buildOrderGraph(மாறிலி V3Sched::TriggerKit& trigKit,  //
                                            AstVarScope* trigVscp,  //
                                            மாறிலி std::vector<V3Sched::LogicByScope*>& coll,  //
                                            மாறிலி V3Sched::CovergroupRefBindings& cgRefBindings,
                                            bool parallel);

வெறுமை orderOrderGraph(OrderGraph& graph, மாறிலி std::string& tag);

வெறுமை processDomains(OrderGraph& graph,  //
                    மாறிலி std::string& tag,  //
                    மாறிலி ExternalTriggersProvider& externalTriggers);

Ast_முனை_Stmt* createSerial(OrderMoveGraph& moveGraph, மாறிலி std::string& tag, bool slow);

Ast_முனை_Stmt* createParallel(OrderMoveGraph& moveGraph, மாறிலி std::string& tag, bool slow);

};  // namespace V3Order

#endif  // Guard
