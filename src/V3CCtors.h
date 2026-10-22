// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Emit CFunc's for class construction and configuration
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

#که_نه_تعریف_شوی VERILATOR_V3CCTORS_H_
#تعریف_کړه VERILATOR_V3CCTORS_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

//============================================================================

class V3CCtors final {
عام:
    ساكن فراغ cctorsAll() VL_MT_DISABLED;

خاص:
    ساكن فراغ evalAsserts() VL_MT_DISABLED;
};

#د_که_پای  // Guard
