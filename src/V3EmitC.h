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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

//============================================================================

class V3EmitC final {
பொது:
    நிலையான வெறுமை emitcConstPool() VL_MT_DISABLED;
    நிலையான வெறுமை emitcExecGraph() VL_MT_DISABLED;
    நிலையான வெறுமை emitcFiles() VL_MT_DISABLED;
    நிலையான வெறுமை emitcHeaders() VL_MT_DISABLED;
    நிலையான வெறுமை emitcImp();
    நிலையான வெறுமை emitcInlines() VL_MT_DISABLED;
    நிலையான வெறுமை emitcModel() VL_MT_DISABLED;
    நிலையான வெறுமை emitcPch() VL_MT_DISABLED;
    நிலையான வெறுமை emitcSyms(bool dpiHdrOnly = false) VL_MT_DISABLED;
};

#endif  // Guard
