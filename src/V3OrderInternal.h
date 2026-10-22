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

#अगर_अघोषित VERILATOR_V3ORDERINTERNAL_H_
#घोषणा VERILATOR_V3ORDERINTERNAL_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Order.h"
#समावेश "V3OrderGraph.h"
#समावेश "V3OrderMoveGraph.h"

#समावेश <string>
#समावेश <vector>

class AstVarScope;

namespace V3Sched {
class LogicByScope;
};  // namespace V3Sched

//============================================================================

namespace V3Order {

std::unique_ptr<OrderGraph> buildOrderGraph(स्थिर V3Sched::TriggerKit& trigKit,  //
                                            AstVarScope* trigVscp,  //
                                            स्थिर std::vector<V3Sched::LogicByScope*>& coll,  //
                                            स्थिर V3Sched::CovergroupRefBindings& cgRefBindings,
                                            bool parallel);

व्योम orderOrderGraph(OrderGraph& graph, स्थिर std::string& tag);

व्योम processDomains(OrderGraph& graph,  //
                    स्थिर std::string& tag,  //
                    स्थिर ExternalTriggersProvider& externalTriggers);

Ast_गाँठ_Stmt* createSerial(OrderMoveGraph& moveGraph, स्थिर std::string& tag, bool slow);

Ast_गाँठ_Stmt* createParallel(OrderMoveGraph& moveGraph, स्थिर std::string& tag, bool slow);

};  // namespace V3Order

#पूर्ण_अगर  // Guard
