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

#اگر_تعریف_نشده VERILATOR_V3ORDERINTERNAL_H_
#تعریف_کن VERILATOR_V3ORDERINTERNAL_H_

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

namespace V3_ترتیب {

std::unique_ptr<ترتیب_گراف> build_ترتیب_گراف(ثابت V3Sched::TriggerKit& trigKit,  //
                                            AstVarScope* trigVscp,  //
                                            ثابت std::وکتور<V3Sched::LogicByScope*>& coll,  //
                                            ثابت V3Sched::CovergroupRefBindings& cgRefBindings,
                                            bool parallel);

فراغ ترتیب_ترتیب_گراف(ترتیب_گراف& گراف, ثابت std::string& برچسب);

فراغ processDomains(ترتیب_گراف& گراف,  //
                    ثابت std::string& برچسب,  //
                    ثابت ExternalTriggersProvider& externalTriggers);

Ast_گره_Stmt* createSerial(ترتیب_Move_گراف& move_گراف, ثابت std::string& برچسب, bool slow);

Ast_گره_Stmt* createParallel(ترتیب_Move_گراف& move_گراف, ثابت std::string& برچسب, bool slow);

};  // namespace V3Order

#پایان_اگر  // Guard
