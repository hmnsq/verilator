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

#اگر_غیر_تعریف_شدہ VERILATOR_V3ORDERINTERNAL_H_
#تعریف_کرو VERILATOR_V3ORDERINTERNAL_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Order.h"
#شامل "V3OrderGraph.h"
#شامل "V3OrderMoveGraph.h"

#شامل <string>
#شامل <vector>

class AstVarScope;

namespace V3Sched {
class LogicByScope;
};  // namespace V3Sched

//============================================================================

namespace V3_ترتیب {

std::unique_ptr<ترتیب_گراف> build_ترتیب_گراف(ثابت V3Sched::TriggerKit& trigKit,  //
                                            AstVarScope* trigVscp,  //
                                            ثابت std::سمتیہ<V3Sched::LogicByScope*>& coll,  //
                                            ثابت V3Sched::CovergroupRefBindings& cgRefBindings,
                                            bool parallel);

خالی ترتیب_ترتیب_گراف(ترتیب_گراف& گراف, ثابت std::string& لیبل);

خالی processDomains(ترتیب_گراف& گراف,  //
                    ثابت std::string& لیبل,  //
                    ثابت ExternalTriggersProvider& externalTriggers);

Ast_گرہ_Stmt* createSerial(ترتیب_Move_گراف& move_گراف, ثابت std::string& لیبل, bool slow);

Ast_گرہ_Stmt* createParallel(ترتیب_Move_گراف& move_گراف, ثابت std::string& لیبل, bool slow);

};  // namespace V3Order

#اگر_ختم  // Guard
