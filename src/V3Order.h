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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3_ಕ್ರಮ_H
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3_ಕ್ರಮ_H

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

namespace V3_ಕ್ರಮ {

// Callable to add the indices of extra external triggers of a variable to the given set
using ExternalTriggersProvider = std::function<ಶೂನ್ಯ(ಸ್ಥಿರಾಂಕ AstVarScope*, std::ಗಣ<uint32_t>&)>;

AstCFunc* ಕ್ರಮ(ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<V3Sched::LogicByScope*>& logic,  //
                ಸ್ಥಿರಾಂಕ V3Sched::TriggerKit& trigKit,  //
                AstVarScope* trigVscp,  //
                ಸ್ಥಿರಾಂಕ V3Sched::CovergroupRefBindings& cgRefBindings,  //
                ಸ್ಥಿರಾಂಕ string& ಟ್ಯಾಗ್,  //
                bool parallel,  //
                bool slow,  //
                ಸ್ಥಿರಾಂಕ ExternalTriggersProvider& externalTriggers) VL_MT_DISABLED;

};  // namespace V3Order

#ಆದರೆ_ಅಂತ್ಯ  // Guard
