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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

//============================================================================

class V3EmitC final {
പൊതു:
    സ്ഥിതം ശൂന്യം emitcConstPool() VL_MT_DISABLED;
    സ്ഥിതം ശൂന്യം emitcExecGraph() VL_MT_DISABLED;
    സ്ഥിതം ശൂന്യം emitcFiles() VL_MT_DISABLED;
    സ്ഥിതം ശൂന്യം emitcHeaders() VL_MT_DISABLED;
    സ്ഥിതം ശൂന്യം emitcImp();
    സ്ഥിതം ശൂന്യം emitcInlines() VL_MT_DISABLED;
    സ്ഥിതം ശൂന്യം emitcModel() VL_MT_DISABLED;
    സ്ഥിതം ശൂന്യം emitcPch() VL_MT_DISABLED;
    സ്ഥിതം ശൂന്യം emitcSyms(bool dpiHdrOnly = false) VL_MT_DISABLED;
};

#endif  // Guard
