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

#ifndef VERILATOR_V3ORDER_H_
#define VERILATOR_V3ORDER_H_

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ <functional>
#அந்தர்பாவ <set>
#அந்தர்பாவ <vector>

class AstCFunc;
class AstVarScope;

namespace V3Sched {
class LogicByScope;
class CovergroupRefBindings;
class TriggerKit;
};  // namespace V3Sched

//============================================================================

namespace V3Order {

// Callable to add the indices of extra external triggers of a variable to the given set
using ExternalTriggersProvider = std::function<வெறுமை(மாறிலி AstVarScope*, std::set<uint32_t>&)>;

AstCFunc* order(மாறிலி std::vector<V3Sched::LogicByScope*>& logic,  //
                மாறிலி V3Sched::TriggerKit& trigKit,  //
                AstVarScope* trigVscp,  //
                மாறிலி V3Sched::CovergroupRefBindings& cgRefBindings,  //
                மாறிலி string& tag,  //
                bool parallel,  //
                bool slow,  //
                மாறிலி ExternalTriggersProvider& externalTriggers) VL_MT_DISABLED;

};  // namespace V3Order

#endif  // Guard
