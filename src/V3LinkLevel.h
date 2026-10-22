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

#अगर_अघोषित VERILATOR_V3LINKLEVEL_H_
#घोषणा VERILATOR_V3LINKLEVEL_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Ast.h"
#समावेश "V3Error.h"

#समावेश <vector>

//============================================================================

class V3LinkLevel final {
    using ModVec = std::vector<Ast_गाँठ_Module*>;

    अटल व्योम timescaling(स्थिर ModVec& mods) VL_MT_DISABLED;
    अटल व्योम wrap_शीर्ष_Cell(AstNetlist* rootp) VL_MT_DISABLED;
    अटल व्योम wrap_शीर्ष_Packages(AstNetlist* rootp) VL_MT_DISABLED;

खुला:
    अटल व्योम modSortByLevel() VL_MT_DISABLED;
    अटल व्योम wrap_शीर्ष(AstNetlist* rootp) VL_MT_DISABLED;
    अटल व्योम non_Wrap_शीर्ष(AstNetlist* rootp) VL_MT_DISABLED;
};

#पूर्ण_अगर  // Guard
