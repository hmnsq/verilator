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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3LINKLEVEL_H_
#وصف_ڪريو VERILATOR_V3LINKLEVEL_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Error.h"

#ضمن <vector>

//============================================================================

class V3LinkLevel final {
    using ModVec = std::ويڪٽر<Ast_ڳنڍ_Module*>;

    ساكن فراغ timescaling(ثابت ModVec& mods) VL_MT_DISABLED;
    ساكن فراغ wrap_چوٽي_Cell(AstNetlist* rootp) VL_MT_DISABLED;
    ساكن فراغ wrap_چوٽي_Packages(AstNetlist* rootp) VL_MT_DISABLED;

عام:
    ساكن فراغ mod_ترتيب_ڏيو_By_Level() VL_MT_DISABLED;
    ساكن فراغ wrap_چوٽي(AstNetlist* rootp) VL_MT_DISABLED;
    ساكن فراغ non_Wrap_چوٽي(AstNetlist* rootp) VL_MT_DISABLED;
};

#جيڪڏهن_پورو  // Guard
