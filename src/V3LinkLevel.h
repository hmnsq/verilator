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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3Error.h"

#ଅନ୍ତର୍ଭାଵ <vector>

//============================================================================

class V3LinkLevel final {
    using ModVec = std::vector<Ast_ଗଣ୍ଠି_Module*>;

    ସ୍ଥିର ଶୂନ୍ୟ timescaling(ଅଚଳ ModVec& mods) VL_MT_DISABLED;
    ସ୍ଥିର ଶୂନ୍ୟ wrap_ଶୀର୍ଷ_Cell(AstNetlist* rootp) VL_MT_DISABLED;
    ସ୍ଥିର ଶୂନ୍ୟ wrap_ଶୀର୍ଷ_Packages(AstNetlist* rootp) VL_MT_DISABLED;

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ modSortByLevel() VL_MT_DISABLED;
    ସ୍ଥିର ଶୂନ୍ୟ wrap_ଶୀର୍ଷ(AstNetlist* rootp) VL_MT_DISABLED;
    ସ୍ଥିର ଶୂନ୍ୟ non_Wrap_ଶୀର୍ଷ(AstNetlist* rootp) VL_MT_DISABLED;
};

#endif  // Guard
