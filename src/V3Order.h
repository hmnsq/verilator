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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3_ਕ੍ਰਮ_H
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3_ਕ੍ਰਮ_H

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <functional>
#ਅਨ੍ਤਰ੍ਭਾਵ <set>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

class AstCFunc;
class AstVarScope;

namespace V3Sched {
class LogicByScope;
class CovergroupRefBindings;
class TriggerKit;
};  // namespace V3Sched

//============================================================================

namespace V3_ਕ੍ਰਮ {

// Callable to add the indices of extra external triggers of a variable to the given set
using ExternalTriggersProvider = std::function<ਖਾਲੀ(ਅਚਲ AstVarScope*, std::ਸੈੱਟ<uint32_t>&)>;

AstCFunc* ਕ੍ਰਮ(ਅਚਲ std::ਸਦਿਸ਼<V3Sched::LogicByScope*>& logic,  //
                ਅਚਲ V3Sched::TriggerKit& trigKit,  //
                AstVarScope* trigVscp,  //
                ਅਚਲ V3Sched::CovergroupRefBindings& cgRefBindings,  //
                ਅਚਲ string& ਟੈਗ,  //
                bool parallel,  //
                bool slow,  //
                ਅਚਲ ExternalTriggersProvider& externalTriggers) VL_MT_DISABLED;

};  // namespace V3Order

#ਜੇ_ਅੰਤ  // Guard
