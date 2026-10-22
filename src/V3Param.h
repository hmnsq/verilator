// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Param modules/signals together
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

#ifndef VERILATOR_V3PARAM_H_
#define VERILATOR_V3PARAM_H_

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

class AstNetlist;

//============================================================================

class V3Param final {
সার্ৱজনিক:
    স্থির শূন্য param(AstNetlist* rootp) VL_MT_DISABLED;
    স্থির শূন্য finalizeDeferredParams(AstNetlist* rootp) VL_MT_DISABLED;
};

#endif  // Guard
