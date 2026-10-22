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

#اگر_غیر_تعریف_شدہ VERILATOR_V3LINKLEVEL_H_
#تعریف_کرو VERILATOR_V3LINKLEVEL_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3Error.h"

#شامل <vector>

//============================================================================

class V3LinkLevel final {
    using ModVec = std::سمتیہ<Ast_گرہ_Module*>;

    ساکن خالی timescaling(ثابت ModVec& mods) VL_MT_DISABLED;
    ساکن خالی wrap_چوٹی_Cell(AstNetlist* rootp) VL_MT_DISABLED;
    ساکن خالی wrap_چوٹی_Packages(AstNetlist* rootp) VL_MT_DISABLED;

عوامی:
    ساکن خالی mod_ترتیب_دو_By_Level() VL_MT_DISABLED;
    ساکن خالی wrap_چوٹی(AstNetlist* rootp) VL_MT_DISABLED;
    ساکن خالی non_Wrap_چوٹی(AstNetlist* rootp) VL_MT_DISABLED;
};

#اگر_ختم  // Guard
