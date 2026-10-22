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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Order.h"
#ଅନ୍ତର୍ଭାଵ "V3OrderGraph.h"
#ଅନ୍ତର୍ଭାଵ "V3OrderMoveGraph.h"

#ଅନ୍ତର୍ଭାଵ <string>
#ଅନ୍ତର୍ଭାଵ <vector>

class AstVarScope;

namespace V3Sched {
class LogicByScope;
};  // namespace V3Sched

//============================================================================

namespace V3Order {

std::unique_ptr<OrderGraph> buildOrderGraph(ଅଚଳ V3Sched::TriggerKit& trigKit,  //
                                            AstVarScope* trigVscp,  //
                                            ଅଚଳ std::vector<V3Sched::LogicByScope*>& coll,  //
                                            ଅଚଳ V3Sched::CovergroupRefBindings& cgRefBindings,
                                            bool parallel);

ଶୂନ୍ୟ orderOrderGraph(OrderGraph& graph, ଅଚଳ std::string& tag);

ଶୂନ୍ୟ processDomains(OrderGraph& graph,  //
                    ଅଚଳ std::string& tag,  //
                    ଅଚଳ ExternalTriggersProvider& externalTriggers);

Ast_ଗଣ୍ଠି_Stmt* createSerial(OrderMoveGraph& moveGraph, ଅଚଳ std::string& tag, bool slow);

Ast_ଗଣ୍ଠି_Stmt* createParallel(OrderMoveGraph& moveGraph, ଅଚଳ std::string& tag, bool slow);

};  // namespace V3Order

#endif  // Guard
