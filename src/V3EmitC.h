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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

//============================================================================

class V3EmitC final {
সার্বজনিক:
    স্থির শূন্য emitcConstPool() VL_MT_DISABLED;
    স্থির শূন্য emitcExecGraph() VL_MT_DISABLED;
    স্থির শূন্য emitcFiles() VL_MT_DISABLED;
    স্থির শূন্য emitcHeaders() VL_MT_DISABLED;
    স্থির শূন্য emitcImp();
    স্থির শূন্য emitcInlines() VL_MT_DISABLED;
    স্থির শূন্য emitcModel() VL_MT_DISABLED;
    স্থির শূন্য emitcPch() VL_MT_DISABLED;
    স্থির শূন্য emitcSyms(bool dpiHdrOnly = false) VL_MT_DISABLED;
};

#endif  // Guard
