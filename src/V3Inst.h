// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Break always into sensitivity inst domains
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

#جے_بنا_تعریف VERILATOR_V3INST_H_
#تعریف_کرو VERILATOR_V3INST_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

class Ast_سونپو_W;
class AstCell;
class AstNetlist;
class AstPin;

//============================================================================

class V3Inst final {
عام:
    ساكن فراغ instAll(AstNetlist* nodep) VL_MT_DISABLED;
    ساكن Ast_سونپو_W* pinReconnectSimple(AstPin* pinp, AstCell* cellp, bool forTristate,
                                          bool alwaysCvt = false) VL_MT_DISABLED;
    ساكن فراغ checkOutputShort(ثابت AstPin* nodep) VL_MT_DISABLED;
};

#جے_انت  // Guard
