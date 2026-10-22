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

#جے_بنا_تعریف VERILATOR_V3LINKLVALUE_H_
#تعریف_کرو VERILATOR_V3LINKLVALUE_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"

//============================================================================

class V3LinkLValue final {
عام:
    ساكن فراغ linkLValue(AstNetlist* nodep) VL_MT_DISABLED;
    ساكن فراغ link_LValue_سیٹ(Ast_گرہ* nodep, VAccess access = VAccess::WRITE) VL_MT_DISABLED;
};

#جے_انت  // Guard
