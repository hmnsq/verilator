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

#यदि_अपरिभाषितम् VERILATOR_V3_क्रमः_H
#परिभाषय VERILATOR_V3_क्रमः_H

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव <functional>
#अन्तर्भाव <set>
#अन्तर्भाव <vector>

class AstCFunc;
class AstVarScope;

namespace V3Sched {
class LogicByScope;
class CovergroupRefBindings;
class TriggerKit;
};  // namespace V3Sched

//============================================================================

namespace V3_क्रमः {

// Callable to add the indices of extra external triggers of a variable to the given set
using ExternalTriggersProvider = std::function<शून्य(ध्रुव AstVarScope*, std::समुच्चयः<uint32_t>&)>;

AstCFunc* क्रमः(ध्रुव std::सदिशम्<V3Sched::LogicByScope*>& logic,  //
                ध्रुव V3Sched::TriggerKit& trigKit,  //
                AstVarScope* trigVscp,  //
                ध्रुव V3Sched::CovergroupRefBindings& cgRefBindings,  //
                ध्रुव string& चिह्नकम्,  //
                bool parallel,  //
                bool slow,  //
                ध्रुव ExternalTriggersProvider& externalTriggers) VL_MT_DISABLED;

};  // namespace V3Order

#यदि_अन्तः  // Guard
