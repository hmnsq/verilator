// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Emit Makefile
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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3EMITMK_H_
#وصف_ڪريو VERILATOR_V3EMITMK_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

class V3_Hier_گراف;

//============================================================================

class V3EmitMk final {
عام:
    // Number of source files after which to use parallel compiles
    ساكن ثابت سائيز_t PARALLEL_FILE_CNT_حد = 128;

    ساكن فراغ emitmk() VL_MT_DISABLED;
    ساكن فراغ emitHierVerilation(ثابت V3_Hier_گراف* planp) VL_MT_DISABLED;
};

#جيڪڏهن_پورو  // Guard
