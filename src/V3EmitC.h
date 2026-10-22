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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3EMITC_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3EMITC_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

//============================================================================

class V3EmitC final {
ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ emitcConstPool() VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ emitc_Exec_ನಕ್ಷೆ() VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ emitcFiles() VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ emitc_ಶೀರ್ಷಿಕೆಗಳು() VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ emitcImp();
    ಸ್ಥಿರ ಶೂನ್ಯ emitcInlines() VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ emitcModel() VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ emitcPch() VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ emitcSyms(bool dpiHdrOnly = false) VL_MT_DISABLED;
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
