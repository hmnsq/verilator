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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Error.h"

#अन्तर्भाव <vector>

//============================================================================

class V3LinkLevel final {
    using ModVec = std::vector<Ast_गाठ_Module*>;

    स्थिर शून्य timescaling(स्थिरांक ModVec& mods) VL_MT_DISABLED;
    स्थिर शून्य wrap_शीर्ष_Cell(AstNetlist* rootp) VL_MT_DISABLED;
    स्थिर शून्य wrap_शीर्ष_Packages(AstNetlist* rootp) VL_MT_DISABLED;

सार्वजनिक:
    स्थिर शून्य modSortByLevel() VL_MT_DISABLED;
    स्थिर शून्य wrap_शीर्ष(AstNetlist* rootp) VL_MT_DISABLED;
    स्थिर शून्य non_Wrap_शीर्ष(AstNetlist* rootp) VL_MT_DISABLED;
};

#endif  // Guard
