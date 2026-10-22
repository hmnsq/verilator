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

#که_نه_تعریف_شوی VERILATOR_V3_ترتیب_H
#تعریف_کړه VERILATOR_V3_ترتیب_H

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

namespace V3_ترتیب {

// Callable to add the indices of extra external triggers of a variable to the given set
using ExternalTriggersProvider = std::function<فراغ(ثابت AstVarScope*, std::سېټ<uint32_t>&)>;

AstCFunc* ترتیب(ثابت std::ویکټور<V3Sched::LogicByScope*>& logic,  //
                ثابت V3Sched::TriggerKit& trigKit,  //
                AstVarScope* trigVscp,  //
                ثابت V3Sched::CovergroupRefBindings& cgRefBindings,  //
                ثابت string& نښان,  //
                bool parallel,  //
                bool slow,  //
                ثابت ExternalTriggersProvider& externalTriggers) VL_MT_DISABLED;

};  // namespace V3Order

#د_که_پای  // Guard
