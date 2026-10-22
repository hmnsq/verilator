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

#నిర్వచితం_కాకపోతే VERILATOR_V3ORDERINTERNAL_H_
#నిర్వచించు VERILATOR_V3ORDERINTERNAL_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Order.h"
#అన్తర్భావ "V3OrderGraph.h"
#అన్తర్భావ "V3OrderMoveGraph.h"

#అన్తర్భావ <string>
#అన్తర్భావ <vector>

class AstVarScope;

namespace V3Sched {
class LogicByScope;
};  // namespace V3Sched

//============================================================================

namespace V3_క్రమం {

std::unique_ptr<క్రమం_లేఖాచిత్రం> build_క్రమం_లేఖాచిత్రం(స్థిరరాశి V3Sched::TriggerKit& trigKit,  //
                                            AstVarScope* trigVscp,  //
                                            స్థిరరాశి std::సదిశ<V3Sched::LogicByScope*>& coll,  //
                                            స్థిరరాశి V3Sched::CovergroupRefBindings& cgRefBindings,
                                            bool parallel);

శూన్య క్రమం_క్రమం_లేఖాచిత్రం(క్రమం_లేఖాచిత్రం& లేఖాచిత్రం, స్థిరరాశి std::string& ట్యాగ్);

శూన్య processDomains(క్రమం_లేఖాచిత్రం& లేఖాచిత్రం,  //
                    స్థిరరాశి std::string& ట్యాగ్,  //
                    స్థిరరాశి ExternalTriggersProvider& externalTriggers);

Ast_కణుపు_Stmt* createSerial(క్రమం_Move_లేఖాచిత్రం& move_లేఖాచిత్రం, స్థిరరాశి std::string& ట్యాగ్, bool slow);

Ast_కణుపు_Stmt* createParallel(క్రమం_Move_లేఖాచిత్రం& move_లేఖాచిత్రం, స్థిరరాశి std::string& ట్యాగ్, bool slow);

};  // namespace V3Order

#అయితే_ముగింపు  // Guard
