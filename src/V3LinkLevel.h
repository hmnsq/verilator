// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Link modules/signals together
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

#که_نه_تعریف_شوی VERILATOR_V3LINKLEVEL_H_
#تعریف_کړه VERILATOR_V3LINKLEVEL_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Error.h"

#ضمن <vector>

//============================================================================

class V3LinkLevel final {
    using ModVec = std::ویکټور<Ast_غوټه_Module*>;

    ساكن فراغ timescaling(ثابت ModVec& mods) VL_MT_DISABLED;
    ساكن فراغ wrap_سر_Cell(AstNetlist* rootp) VL_MT_DISABLED;
    ساكن فراغ wrap_سر_Packages(AstNetlist* rootp) VL_MT_DISABLED;

عام:
    ساكن فراغ mod_ترتیبول_By_Level() VL_MT_DISABLED;
    ساكن فراغ wrap_سر(AstNetlist* rootp) VL_MT_DISABLED;
    ساكن فراغ non_Wrap_سر(AstNetlist* rootp) VL_MT_DISABLED;
};

#د_که_پای  // Guard
