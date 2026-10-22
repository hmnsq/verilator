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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3LINKLEVEL_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3LINKLEVEL_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

//============================================================================

class V3LinkLevel final {
    using ModVec = std::ਸਦਿਸ਼<Ast_ਗੰਢ_Module*>;

    ਸਥਿਰ ਖਾਲੀ timescaling(ਅਚਲ ModVec& mods) VL_MT_DISABLED;
    ਸਥਿਰ ਖਾਲੀ wrap_ਸਿਖਰ_Cell(AstNetlist* rootp) VL_MT_DISABLED;
    ਸਥਿਰ ਖਾਲੀ wrap_ਸਿਖਰ_Packages(AstNetlist* rootp) VL_MT_DISABLED;

ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ mod_ਛਾਂਟੋ_By_Level() VL_MT_DISABLED;
    ਸਥਿਰ ਖਾਲੀ wrap_ਸਿਖਰ(AstNetlist* rootp) VL_MT_DISABLED;
    ਸਥਿਰ ਖਾਲੀ non_Wrap_ਸਿਖਰ(AstNetlist* rootp) VL_MT_DISABLED;
};

#ਜੇ_ਅੰਤ  // Guard
