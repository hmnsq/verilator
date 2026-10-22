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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3EMITC_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3EMITC_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

//============================================================================

class V3EmitC final {
ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ emitcConstPool() VL_MT_DISABLED;
    ਸਥਿਰ ਖਾਲੀ emitc_Exec_ਗ੍ਰਾਫ() VL_MT_DISABLED;
    ਸਥਿਰ ਖਾਲੀ emitcFiles() VL_MT_DISABLED;
    ਸਥਿਰ ਖਾਲੀ emitc_ਸਿਰਲੇਖਾਂ() VL_MT_DISABLED;
    ਸਥਿਰ ਖਾਲੀ emitcImp();
    ਸਥਿਰ ਖਾਲੀ emitcInlines() VL_MT_DISABLED;
    ਸਥਿਰ ਖਾਲੀ emitcModel() VL_MT_DISABLED;
    ਸਥਿਰ ਖਾਲੀ emitcPch() VL_MT_DISABLED;
    ਸਥਿਰ ਖਾਲੀ emitcSyms(bool dpiHdrOnly = false) VL_MT_DISABLED;
};

#ਜੇ_ਅੰਤ  // Guard
