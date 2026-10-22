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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3ORDERINTERNAL_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3ORDERINTERNAL_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Order.h"
#ಅನ್ತರ್ಭಾವ "V3OrderGraph.h"
#ಅನ್ತರ್ಭಾವ "V3OrderMoveGraph.h"

#ಅನ್ತರ್ಭಾವ <string>
#ಅನ್ತರ್ಭಾವ <vector>

class AstVarScope;

namespace V3Sched {
class LogicByScope;
};  // namespace V3Sched

//============================================================================

namespace V3_ಕ್ರಮ {

std::unique_ptr<ಕ್ರಮ_ನಕ್ಷೆ> build_ಕ್ರಮ_ನಕ್ಷೆ(ಸ್ಥಿರಾಂಕ V3Sched::TriggerKit& trigKit,  //
                                            AstVarScope* trigVscp,  //
                                            ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<V3Sched::LogicByScope*>& coll,  //
                                            ಸ್ಥಿರಾಂಕ V3Sched::CovergroupRefBindings& cgRefBindings,
                                            bool parallel);

ಶೂನ್ಯ ಕ್ರಮ_ಕ್ರಮ_ನಕ್ಷೆ(ಕ್ರಮ_ನಕ್ಷೆ& ನಕ್ಷೆ, ಸ್ಥಿರಾಂಕ std::string& ಟ್ಯಾಗ್);

ಶೂನ್ಯ processDomains(ಕ್ರಮ_ನಕ್ಷೆ& ನಕ್ಷೆ,  //
                    ಸ್ಥಿರಾಂಕ std::string& ಟ್ಯಾಗ್,  //
                    ಸ್ಥಿರಾಂಕ ExternalTriggersProvider& externalTriggers);

Ast_ಗಂಟು_Stmt* createSerial(ಕ್ರಮ_Move_ನಕ್ಷೆ& move_ನಕ್ಷೆ, ಸ್ಥಿರಾಂಕ std::string& ಟ್ಯಾಗ್, bool slow);

Ast_ಗಂಟು_Stmt* createParallel(ಕ್ರಮ_Move_ನಕ್ಷೆ& move_ನಕ್ಷೆ, ಸ್ಥಿರಾಂಕ std::string& ಟ್ಯಾಗ್, bool slow);

};  // namespace V3Order

#ಆದರೆ_ಅಂತ್ಯ  // Guard
