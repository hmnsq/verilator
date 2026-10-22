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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Order.h"
#അന്തര്ഭാവ "V3OrderGraph.h"
#അന്തര്ഭാവ "V3OrderMoveGraph.h"

#അന്തര്ഭാവ <string>
#അന്തര്ഭാവ <vector>

class AstVarScope;

namespace V3Sched {
class LogicByScope;
};  // namespace V3Sched

//============================================================================

namespace V3Order {

std::unique_ptr<OrderGraph> buildOrderGraph(സ്ഥിരം V3Sched::TriggerKit& trigKit,  //
                                            AstVarScope* trigVscp,  //
                                            സ്ഥിരം std::vector<V3Sched::LogicByScope*>& coll,  //
                                            സ്ഥിരം V3Sched::CovergroupRefBindings& cgRefBindings,
                                            bool parallel);

ശൂന്യം orderOrderGraph(OrderGraph& graph, സ്ഥിരം std::string& tag);

ശൂന്യം processDomains(OrderGraph& graph,  //
                    സ്ഥിരം std::string& tag,  //
                    സ്ഥിരം ExternalTriggersProvider& externalTriggers);

Ast_കണ്ണി_Stmt* createSerial(OrderMoveGraph& moveGraph, സ്ഥിരം std::string& tag, bool slow);

Ast_കണ്ണി_Stmt* createParallel(OrderMoveGraph& moveGraph, സ്ഥിരം std::string& tag, bool slow);

};  // namespace V3Order

#endif  // Guard
