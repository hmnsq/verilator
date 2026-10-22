// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Emit Makefile
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

#अगर_अघोषित VERILATOR_V3EMITMK_H_
#घोषणा VERILATOR_V3EMITMK_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

class V3HierGraph;

//============================================================================

class V3EmitMk final {
खुला:
    // Number of source files after which to use parallel compiles
    अटल स्थिर माप_प्रकार PARALLEL_FILE_CNT_THRESHOLD = 128;

    अटल व्योम emitmk() VL_MT_DISABLED;
    अटल व्योम emitHierVerilation(स्थिर V3HierGraph* planp) VL_MT_DISABLED;
};

#पूर्ण_अगर  // Guard
