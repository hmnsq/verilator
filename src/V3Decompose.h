// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Split arrays and structs into separate variables
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

#अगर_अघोषित VERILATOR_V3DECOMPOSE_H_
#घोषणा VERILATOR_V3DECOMPOSE_H_

#समावेश "verilatedos.h"

//============================================================================

class AstNetlist;

class V3Decompose final {
खुला:
    अटल व्योम decomposeAll(AstNetlist* nodep) VL_MT_DISABLED;
};

#पूर्ण_अगर  // Guard
