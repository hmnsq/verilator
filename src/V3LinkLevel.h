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

#ifndef VERILATOR_V3LINKLEVEL_H_
#define VERILATOR_V3LINKLEVEL_H_

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3Error.h"

#અન્તર્ભાવ <vector>

//============================================================================

class V3LinkLevel final {
    using ModVec = std::vector<Ast_ગાંઠ_Module*>;

    સ્થિર શૂન્ય timescaling(અચલ ModVec& mods) VL_MT_DISABLED;
    સ્થિર શૂન્ય wrap_ટોચ_Cell(AstNetlist* rootp) VL_MT_DISABLED;
    સ્થિર શૂન્ય wrap_ટોચ_Packages(AstNetlist* rootp) VL_MT_DISABLED;

જાહેર:
    સ્થિર શૂન્ય modSortByLevel() VL_MT_DISABLED;
    સ્થિર શૂન્ય wrap_ટોચ(AstNetlist* rootp) VL_MT_DISABLED;
    સ્થિર શૂન્ય non_Wrap_ટોચ(AstNetlist* rootp) VL_MT_DISABLED;
};

#endif  // Guard
