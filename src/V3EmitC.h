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

#अगर_अघोषित VERILATOR_V3EMITC_H_
#घोषणा VERILATOR_V3EMITC_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

//============================================================================

class V3EmitC final {
खुला:
    अटल व्योम emitcConstPool() VL_MT_DISABLED;
    अटल व्योम emitcExecGraph() VL_MT_DISABLED;
    अटल व्योम emitcFiles() VL_MT_DISABLED;
    अटल व्योम emitcHeaders() VL_MT_DISABLED;
    अटल व्योम emitcImp();
    अटल व्योम emitcInlines() VL_MT_DISABLED;
    अटल व्योम emitcModel() VL_MT_DISABLED;
    अटल व्योम emitcPch() VL_MT_DISABLED;
    अटल व्योम emitcSyms(bool dpiHdrOnly = false) VL_MT_DISABLED;
};

#पूर्ण_अगर  // Guard
