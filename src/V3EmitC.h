// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Emit C++ code for module tree
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

#اگر_غیر_تعریف_شدہ VERILATOR_V3EMITC_H_
#تعریف_کرو VERILATOR_V3EMITC_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

//============================================================================

class V3EmitC final {
عوامی:
    ساکن خالی emitcConstPool() VL_MT_DISABLED;
    ساکن خالی emitc_Exec_گراف() VL_MT_DISABLED;
    ساکن خالی emitcFiles() VL_MT_DISABLED;
    ساکن خالی emitc_سرخیاں() VL_MT_DISABLED;
    ساکن خالی emitcImp();
    ساکن خالی emitcInlines() VL_MT_DISABLED;
    ساکن خالی emitcModel() VL_MT_DISABLED;
    ساکن خالی emitcPch() VL_MT_DISABLED;
    ساکن خالی emitcSyms(bool dpiHdrOnly = false) VL_MT_DISABLED;
};

#اگر_ختم  // Guard
