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

#که_نه_تعریف_شوی VERILATOR_V3ORDERINTERNAL_H_
#تعریف_کړه VERILATOR_V3ORDERINTERNAL_H_

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

std::unique_ptr<ترتیب_ګراف> build_ترتیب_ګراف(ثابت V3Sched::TriggerKit& trigKit,  //
                                            AstVarScope* trigVscp,  //
                                            ثابت std::ویکټور<V3Sched::LogicByScope*>& coll,  //
                                            ثابت V3Sched::CovergroupRefBindings& cgRefBindings,
                                            bool parallel);

فراغ ترتیب_ترتیب_ګراف(ترتیب_ګراف& ګراف, ثابت std::string& نښان);

فراغ processDomains(ترتیب_ګراف& ګراف,  //
                    ثابت std::string& نښان,  //
                    ثابت ExternalTriggersProvider& externalTriggers);

Ast_غوټه_Stmt* createSerial(ترتیب_Move_ګراف& move_ګراف, ثابت std::string& نښان, bool slow);

Ast_غوټه_Stmt* createParallel(ترتیب_Move_ګراف& move_ګراف, ثابت std::string& نښان, bool slow);

};  // namespace V3Order

#د_که_پای  // Guard
