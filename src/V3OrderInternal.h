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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3ORDERINTERNAL_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3ORDERINTERNAL_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Order.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3OrderGraph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3OrderMoveGraph.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <string>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

class AstVarScope;

namespace V3Sched {
class LogicByScope;
};  // namespace V3Sched

//============================================================================

namespace V3_ਕ੍ਰਮ {

std::unique_ptr<ਕ੍ਰਮ_ਗ੍ਰਾਫ> build_ਕ੍ਰਮ_ਗ੍ਰਾਫ(ਅਚਲ V3Sched::TriggerKit& trigKit,  //
                                            AstVarScope* trigVscp,  //
                                            ਅਚਲ std::ਸਦਿਸ਼<V3Sched::LogicByScope*>& coll,  //
                                            ਅਚਲ V3Sched::CovergroupRefBindings& cgRefBindings,
                                            bool parallel);

ਖਾਲੀ ਕ੍ਰਮ_ਕ੍ਰਮ_ਗ੍ਰਾਫ(ਕ੍ਰਮ_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ, ਅਚਲ std::string& ਟੈਗ);

ਖਾਲੀ processDomains(ਕ੍ਰਮ_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ,  //
                    ਅਚਲ std::string& ਟੈਗ,  //
                    ਅਚਲ ExternalTriggersProvider& externalTriggers);

Ast_ਗੰਢ_Stmt* createSerial(ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ& move_ਗ੍ਰਾਫ, ਅਚਲ std::string& ਟੈਗ, bool slow);

Ast_ਗੰਢ_Stmt* createParallel(ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ& move_ਗ੍ਰਾਫ, ਅਚਲ std::string& ਟੈਗ, bool slow);

};  // namespace V3Order

#ਜੇ_ਅੰਤ  // Guard
