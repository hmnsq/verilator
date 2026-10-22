// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Link modules/signals together
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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3LINKLEVEL_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3LINKLEVEL_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"

#ಅನ್ತರ್ಭಾವ <vector>

//============================================================================

class V3LinkLevel final {
    using ModVec = std::ಸದಿಶ<Ast_ಗಂಟು_Module*>;

    ಸ್ಥಿರ ಶೂನ್ಯ timescaling(ಸ್ಥಿರಾಂಕ ModVec& mods) VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ wrap_ತುದಿ_Cell(AstNetlist* rootp) VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ wrap_ತುದಿ_Packages(AstNetlist* rootp) VL_MT_DISABLED;

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ mod_ವಿಂಗಡಿಸು_By_Level() VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ wrap_ತುದಿ(AstNetlist* rootp) VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ non_Wrap_ತುದಿ(AstNetlist* rootp) VL_MT_DISABLED;
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
