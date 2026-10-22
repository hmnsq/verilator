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

#اگر_تعریف_نشده VERILATOR_V3LINKPARSE_H_
#تعریف_کن VERILATOR_V3LINKPARSE_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

class AstNetlist;

//============================================================================

class V3LinkParse final {
عام:
    ساكن فراغ linkParse(AstNetlist* rootp) VL_MT_DISABLED;
};

#پایان_اگر  // Guard
