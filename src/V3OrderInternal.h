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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Order.h"
#अन्तर्भाव "V3OrderGraph.h"
#अन्तर्भाव "V3OrderMoveGraph.h"

#अन्तर्भाव <string>
#अन्तर्भाव <vector>

class AstVarScope;

namespace V3Sched {
class LogicByScope;
};  // namespace V3Sched

//============================================================================

namespace V3Order {

std::unique_ptr<OrderGraph> buildOrderGraph(अचर V3Sched::TriggerKit& trigKit,  //
                                            AstVarScope* trigVscp,  //
                                            अचर std::vector<V3Sched::LogicByScope*>& coll,  //
                                            अचर V3Sched::CovergroupRefBindings& cgRefBindings,
                                            bool parallel);

शून्य orderOrderGraph(OrderGraph& graph, अचर std::string& tag);

शून्य processDomains(OrderGraph& graph,  //
                    अचर std::string& tag,  //
                    अचर ExternalTriggersProvider& externalTriggers);

Ast_गाँठो_Stmt* createSerial(OrderMoveGraph& moveGraph, अचर std::string& tag, bool slow);

Ast_गाँठो_Stmt* createParallel(OrderMoveGraph& moveGraph, अचर std::string& tag, bool slow);

};  // namespace V3Order

#endif  // Guard
