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

#నిర్వచితం_కాకపోతే VERILATOR_V3_క్రమం_H
#నిర్వచించు VERILATOR_V3_క్రమం_H

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ <functional>
#అన్తర్భావ <set>
#అన్తర్భావ <vector>

class AstCFunc;
class AstVarScope;

namespace V3Sched {
class LogicByScope;
class CovergroupRefBindings;
class TriggerKit;
};  // namespace V3Sched

//============================================================================

namespace V3_క్రమం {

// Callable to add the indices of extra external triggers of a variable to the given set
using ExternalTriggersProvider = std::function<శూన్య(స్థిరరాశి AstVarScope*, std::సమితి<uint32_t>&)>;

AstCFunc* క్రమం(స్థిరరాశి std::సదిశ<V3Sched::LogicByScope*>& logic,  //
                స్థిరరాశి V3Sched::TriggerKit& trigKit,  //
                AstVarScope* trigVscp,  //
                స్థిరరాశి V3Sched::CovergroupRefBindings& cgRefBindings,  //
                స్థిరరాశి string& ట్యాగ్,  //
                bool parallel,  //
                bool slow,  //
                స్థిరరాశి ExternalTriggersProvider& externalTriggers) VL_MT_DISABLED;

};  // namespace V3Order

#అయితే_ముగింపు  // Guard
