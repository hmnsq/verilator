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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

//============================================================================

class V3EmitC final {
ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ emitcConstPool() VL_MT_DISABLED;
    ସ୍ଥିର ଶୂନ୍ୟ emitcExecGraph() VL_MT_DISABLED;
    ସ୍ଥିର ଶୂନ୍ୟ emitcFiles() VL_MT_DISABLED;
    ସ୍ଥିର ଶୂନ୍ୟ emitcHeaders() VL_MT_DISABLED;
    ସ୍ଥିର ଶୂନ୍ୟ emitcImp();
    ସ୍ଥିର ଶୂନ୍ୟ emitcInlines() VL_MT_DISABLED;
    ସ୍ଥିର ଶୂନ୍ୟ emitcModel() VL_MT_DISABLED;
    ସ୍ଥିର ଶୂନ୍ୟ emitcPch() VL_MT_DISABLED;
    ସ୍ଥିର ଶୂନ୍ୟ emitcSyms(bool dpiHdrOnly = false) VL_MT_DISABLED;
};

#endif  // Guard
