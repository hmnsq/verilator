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

#ifndef VERILATOR_V3EMITC_H_
#define VERILATOR_V3EMITC_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

//============================================================================

class V3EmitC final {
عمومی:
    ایستا تهی emitcConstPool() VL_MT_DISABLED;
    ایستا تهی emitcExecGraph() VL_MT_DISABLED;
    ایستا تهی emitcFiles() VL_MT_DISABLED;
    ایستا تهی emitcHeaders() VL_MT_DISABLED;
    ایستا تهی emitcImp();
    ایستا تهی emitcInlines() VL_MT_DISABLED;
    ایستا تهی emitcModel() VL_MT_DISABLED;
    ایستا تهی emitcPch() VL_MT_DISABLED;
    ایستا تهی emitcSyms(bool dpiHdrOnly = false) VL_MT_DISABLED;
};

#endif  // Guard
