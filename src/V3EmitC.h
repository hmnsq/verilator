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

#నిర్వచితం_కాకపోతే VERILATOR_V3EMITC_H_
#నిర్వచించు VERILATOR_V3EMITC_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

//============================================================================

class V3EmitC final {
బహిరంగ:
    స్థిర శూన్య emitcConstPool() VL_MT_DISABLED;
    స్థిర శూన్య emitc_Exec_లేఖాచిత్రం() VL_MT_DISABLED;
    స్థిర శూన్య emitcFiles() VL_MT_DISABLED;
    స్థిర శూన్య emitc_శీర్షికలు() VL_MT_DISABLED;
    స్థిర శూన్య emitcImp();
    స్థిర శూన్య emitcInlines() VL_MT_DISABLED;
    స్థిర శూన్య emitcModel() VL_MT_DISABLED;
    స్థిర శూన్య emitcPch() VL_MT_DISABLED;
    స్థిర శూన్య emitcSyms(bool dpiHdrOnly = false) VL_MT_DISABLED;
};

#అయితే_ముగింపు  // Guard
