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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3_ترتيب_H
#وصف_ڪريو VERILATOR_V3_ترتيب_H

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن <functional>
#ضمن <set>
#ضمن <vector>

class AstCFunc;
class AstVarScope;

namespace V3Sched {
class LogicByScope;
class CovergroupRefBindings;
class TriggerKit;
};  // namespace V3Sched

//============================================================================

namespace V3_ترتيب {

// Callable to add the indices of extra external triggers of a variable to the given set
using ExternalTriggersProvider = std::function<فراغ(ثابت AstVarScope*, std::سيٽ<uint32_t>&)>;

AstCFunc* ترتيب(ثابت std::ويڪٽر<V3Sched::LogicByScope*>& logic,  //
                ثابت V3Sched::TriggerKit& trigKit,  //
                AstVarScope* trigVscp,  //
                ثابت V3Sched::CovergroupRefBindings& cgRefBindings,  //
                ثابت string& ٽيگ,  //
                bool parallel,  //
                bool slow,  //
                ثابت ExternalTriggersProvider& externalTriggers) VL_MT_DISABLED;

};  // namespace V3Order

#جيڪڏهن_پورو  // Guard
