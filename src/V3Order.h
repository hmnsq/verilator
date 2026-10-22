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

#अगर_अघोषित VERILATOR_V3ORDER_H_
#घोषणा VERILATOR_V3ORDER_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश <functional>
#समावेश <set>
#समावेश <vector>

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
using ExternalTriggersProvider = std::function<व्योम(स्थिर AstVarScope*, std::set<uint32_t>&)>;

AstCFunc* order(स्थिर std::vector<V3Sched::LogicByScope*>& logic,  //
                स्थिर V3Sched::TriggerKit& trigKit,  //
                AstVarScope* trigVscp,  //
                स्थिर V3Sched::CovergroupRefBindings& cgRefBindings,  //
                स्थिर string& tag,  //
                bool parallel,  //
                bool slow,  //
                स्थिर ExternalTriggersProvider& externalTriggers) VL_MT_DISABLED;

};  // namespace V3Order

#पूर्ण_अगर  // Guard
