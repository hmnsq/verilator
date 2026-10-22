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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Ast.h"
#অন্তর্ভাব "V3Error.h"

#অন্তর্ভাব <vector>

//============================================================================

class V3LinkLevel final {
    using ModVec = std::vector<Ast_গ্রন্থি_Module*>;

    স্থির শূন্য timescaling(ধ্রুবক ModVec& mods) VL_MT_DISABLED;
    স্থির শূন্য wrap_শীর্ষ_Cell(AstNetlist* rootp) VL_MT_DISABLED;
    স্থির শূন্য wrap_শীর্ষ_Packages(AstNetlist* rootp) VL_MT_DISABLED;

সর্বজনীন:
    স্থির শূন্য modSortByLevel() VL_MT_DISABLED;
    স্থির শূন্য wrap_শীর্ষ(AstNetlist* rootp) VL_MT_DISABLED;
    স্থির শূন্য non_Wrap_শীর্ষ(AstNetlist* rootp) VL_MT_DISABLED;
};

#endif  // Guard
