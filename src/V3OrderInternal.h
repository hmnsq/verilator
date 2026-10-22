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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ "V3Order.h"
#অন্তর্ভাৱ "V3OrderGraph.h"
#অন্তর্ভাৱ "V3OrderMoveGraph.h"

#অন্তর্ভাৱ <string>
#অন্তর্ভাৱ <vector>

class AstVarScope;

namespace V3Sched {
class LogicByScope;
};  // namespace V3Sched

//============================================================================

namespace V3Order {

std::unique_ptr<OrderGraph> buildOrderGraph(ধ্রুৱ V3Sched::TriggerKit& trigKit,  //
                                            AstVarScope* trigVscp,  //
                                            ধ্রুৱ std::vector<V3Sched::LogicByScope*>& coll,  //
                                            ধ্রুৱ V3Sched::CovergroupRefBindings& cgRefBindings,
                                            bool parallel);

শূন্য orderOrderGraph(OrderGraph& graph, ধ্রুৱ std::string& tag);

শূন্য processDomains(OrderGraph& graph,  //
                    ধ্রুৱ std::string& tag,  //
                    ধ্রুৱ ExternalTriggersProvider& externalTriggers);

Ast_গ্ৰন্থি_Stmt* createSerial(OrderMoveGraph& moveGraph, ধ্রুৱ std::string& tag, bool slow);

Ast_গ্ৰন্থি_Stmt* createParallel(OrderMoveGraph& moveGraph, ধ্রুৱ std::string& tag, bool slow);

};  // namespace V3Order

#endif  // Guard
