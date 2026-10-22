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

#यदि_अपरिभाषितम् VERILATOR_V3EMITC_H_
#परिभाषय VERILATOR_V3EMITC_H_

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

//============================================================================

class V3EmitC final {
सार्वजनिक:
    स्थिर शून्य emitcConstPool() VL_MT_DISABLED;
    स्थिर शून्य emitcExecGraph() VL_MT_DISABLED;
    स्थिर शून्य emitcFiles() VL_MT_DISABLED;
    स्थिर शून्य emitc_शीर्षकाणि() VL_MT_DISABLED;
    स्थिर शून्य emitcImp();
    स्थिर शून्य emitcInlines() VL_MT_DISABLED;
    स्थिर शून्य emitcModel() VL_MT_DISABLED;
    स्थिर शून्य emitcPch() VL_MT_DISABLED;
    स्थिर शून्य emitcSyms(bool dpiHdrOnly = false) VL_MT_DISABLED;
};

#यदि_अन्तः  // Guard
