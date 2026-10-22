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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ <functional>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <vector>

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
using ExternalTriggersProvider = std::function<ಶೂನ್ಯ(ಧ್ರುವ AstVarScope*, std::set<uint32_t>&)>;

AstCFunc* order(ಧ್ರುವ std::vector<V3Sched::LogicByScope*>& logic,  //
                ಧ್ರುವ V3Sched::TriggerKit& trigKit,  //
                AstVarScope* trigVscp,  //
                ಧ್ರುವ V3Sched::CovergroupRefBindings& cgRefBindings,  //
                ಧ್ರುವ string& tag,  //
                bool parallel,  //
                bool slow,  //
                ಧ್ರುವ ExternalTriggersProvider& externalTriggers) VL_MT_DISABLED;

};  // namespace V3Order

#endif  // Guard
