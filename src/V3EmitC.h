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

#که_نه_تعریف_شوی VERILATOR_V3EMITC_H_
#تعریف_کړه VERILATOR_V3EMITC_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

//============================================================================

class V3EmitC final {
عام:
    ساكن فراغ emitcConstPool() VL_MT_DISABLED;
    ساكن فراغ emitc_Exec_ګراف() VL_MT_DISABLED;
    ساكن فراغ emitcFiles() VL_MT_DISABLED;
    ساكن فراغ emitc_سرلیکونه() VL_MT_DISABLED;
    ساكن فراغ emitcImp();
    ساكن فراغ emitcInlines() VL_MT_DISABLED;
    ساكن فراغ emitcModel() VL_MT_DISABLED;
    ساكن فراغ emitcPch() VL_MT_DISABLED;
    ساكن فراغ emitcSyms(bool dpiHdrOnly = false) VL_MT_DISABLED;
};

#د_که_پای  // Guard
