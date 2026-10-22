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

#جے_بنا_تعریف VERILATOR_V3LINKLEVEL_H_
#تعریف_کرو VERILATOR_V3LINKLEVEL_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Error.h"

#ضمن <vector>

//============================================================================

class V3LinkLevel final {
    using ModVec = std::سمتیہ<Ast_گرہ_Module*>;

    ساكن فراغ timescaling(ثابت ModVec& mods) VL_MT_DISABLED;
    ساكن فراغ wrap_چوٹی_Cell(AstNetlist* rootp) VL_MT_DISABLED;
    ساكن فراغ wrap_چوٹی_Packages(AstNetlist* rootp) VL_MT_DISABLED;

عام:
    ساكن فراغ mod_ترتیب_دیو_By_Level() VL_MT_DISABLED;
    ساكن فراغ wrap_چوٹی(AstNetlist* rootp) VL_MT_DISABLED;
    ساكن فراغ non_Wrap_چوٹی(AstNetlist* rootp) VL_MT_DISABLED;
};

#جے_انت  // Guard
