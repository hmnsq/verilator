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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3ORDERINTERNAL_H_
#وصف_ڪريو VERILATOR_V3ORDERINTERNAL_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Order.h"
#ضمن "V3OrderGraph.h"
#ضمن "V3OrderMoveGraph.h"

#ضمن <string>
#ضمن <vector>

class AstVarScope;

namespace V3Sched {
class LogicByScope;
};  // namespace V3Sched

//============================================================================

namespace V3_ترتيب {

std::unique_ptr<ترتيب_گراف> build_ترتيب_گراف(ثابت V3Sched::TriggerKit& trigKit,  //
                                            AstVarScope* trigVscp,  //
                                            ثابت std::ويڪٽر<V3Sched::LogicByScope*>& coll,  //
                                            ثابت V3Sched::CovergroupRefBindings& cgRefBindings,
                                            bool parallel);

فراغ ترتيب_ترتيب_گراف(ترتيب_گراف& گراف, ثابت std::string& ٽيگ);

فراغ processDomains(ترتيب_گراف& گراف,  //
                    ثابت std::string& ٽيگ,  //
                    ثابت ExternalTriggersProvider& externalTriggers);

Ast_ڳنڍ_Stmt* createSerial(ترتيب_Move_گراف& move_گراف, ثابت std::string& ٽيگ, bool slow);

Ast_ڳنڍ_Stmt* createParallel(ترتيب_Move_گراف& move_گراف, ثابت std::string& ٽيگ, bool slow);

};  // namespace V3Order

#جيڪڏهن_پورو  // Guard
