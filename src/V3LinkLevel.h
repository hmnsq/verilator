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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3Error.h"

#அந்தர்பாவ <vector>

//============================================================================

class V3LinkLevel final {
    using ModVec = std::vector<Ast_முனை_Module*>;

    நிலையான வெறுமை timescaling(மாறிலி ModVec& mods) VL_MT_DISABLED;
    நிலையான வெறுமை wrap_உச்சி_Cell(AstNetlist* rootp) VL_MT_DISABLED;
    நிலையான வெறுமை wrap_உச்சி_Packages(AstNetlist* rootp) VL_MT_DISABLED;

பொது:
    நிலையான வெறுமை modSortByLevel() VL_MT_DISABLED;
    நிலையான வெறுமை wrap_உச்சி(AstNetlist* rootp) VL_MT_DISABLED;
    நிலையான வெறுமை non_Wrap_உச்சி(AstNetlist* rootp) VL_MT_DISABLED;
};

#endif  // Guard
