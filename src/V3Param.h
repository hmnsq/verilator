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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

class AstNetlist;

//============================================================================

class V3Param final {
જાહેર:
    સ્થિર શૂન્ય param(AstNetlist* rootp) VL_MT_DISABLED;
    સ્થિર શૂન્ય finalizeDeferredParams(AstNetlist* rootp) VL_MT_DISABLED;
};

#endif  // Guard
