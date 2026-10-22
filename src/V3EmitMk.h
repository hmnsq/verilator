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

#اگر_غیر_تعریف_شدہ VERILATOR_V3EMITMK_H_
#تعریف_کرو VERILATOR_V3EMITMK_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

class V3_Hier_گراف;

//============================================================================

class V3EmitMk final {
عوامی:
    // Number of source files after which to use parallel compiles
    ساکن ثابت سائز_t PARALLEL_FILE_CNT_حد = 128;

    ساکن خالی emitmk() VL_MT_DISABLED;
    ساکن خالی emitHierVerilation(ثابت V3_Hier_گراف* planp) VL_MT_DISABLED;
};

#اگر_ختم  // Guard
