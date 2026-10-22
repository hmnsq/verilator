// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Inline small CFuncs into their callers
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

#ifndef VERILATOR_V3INLINECFUNCS_H_
#define VERILATOR_V3INLINECFUNCS_H_

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

class AstNetlist;

class V3InlineCFuncs final {
সার্বজনিক:
    স্থির শূন্য inlineAll(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // Guard
