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

#यदि_अपरिभाषितम् VERILATOR_V3EMITMK_H_
#परिभाषय VERILATOR_V3EMITMK_H_

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

class V3HierGraph;

//============================================================================

class V3EmitMk final {
सार्वजनिक:
    // Number of source files after which to use parallel compiles
    स्थिर ध्रुव आकार_t PARALLEL_FILE_CNT_देहली = 128;

    स्थिर शून्य emitmk() VL_MT_DISABLED;
    स्थिर शून्य emitHierVerilation(ध्रुव V3HierGraph* planp) VL_MT_DISABLED;
};

#यदि_अन्तः  // Guard
