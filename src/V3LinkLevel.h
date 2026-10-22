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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3Error.h"

#شامل <vector>

//============================================================================

class V3LinkLevel final {
    using ModVec = std::vector<Ast_گره_Module*>;

    ایستا تهی timescaling(ثابت ModVec& mods) VL_MT_DISABLED;
    ایستا تهی wrap_بالاترین_Cell(AstNetlist* rootp) VL_MT_DISABLED;
    ایستا تهی wrap_بالاترین_Packages(AstNetlist* rootp) VL_MT_DISABLED;

عمومی:
    ایستا تهی modSortByLevel() VL_MT_DISABLED;
    ایستا تهی wrap_بالاترین(AstNetlist* rootp) VL_MT_DISABLED;
    ایستا تهی non_Wrap_بالاترین(AstNetlist* rootp) VL_MT_DISABLED;
};

#endif  // Guard
