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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ <functional>
#অন্তর্ভাৱ <set>
#অন্তর্ভাৱ <vector>

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
using ExternalTriggersProvider = std::function<শূন্য(ধ্রুৱ AstVarScope*, std::set<uint32_t>&)>;

AstCFunc* order(ধ্রুৱ std::vector<V3Sched::LogicByScope*>& logic,  //
                ধ্রুৱ V3Sched::TriggerKit& trigKit,  //
                AstVarScope* trigVscp,  //
                ধ্রুৱ V3Sched::CovergroupRefBindings& cgRefBindings,  //
                ধ্রুৱ string& tag,  //
                bool parallel,  //
                bool slow,  //
                ধ্রুৱ ExternalTriggersProvider& externalTriggers) VL_MT_DISABLED;

};  // namespace V3Order

#endif  // Guard
