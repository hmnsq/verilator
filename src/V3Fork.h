// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Create separate tasks for forked processes that
//              can outlive their parents
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

#यदि_अपरिभाषितम् VERILATOR_V3FORK_H_
#परिभाषय VERILATOR_V3FORK_H_

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

class AstNetlist;

//============================================================================

class V3Fork final {
सार्वजनिक:
    // Move/copy variables to "anonymous" objects if their lifetime might exceed the scope of a
    // procedure that declared them. Update the references appropriately.
    स्थिर शून्य makeDynamicScopes(AstNetlist* nodep) VL_MT_DISABLED;
    // Create tasks out of blocks/statments that can outlive processes in which they were forked.
    // Return value: number of tasks created
    स्थिर शून्य makeTasks(AstNetlist* nodep) VL_MT_DISABLED;
};

#यदि_अन्तः  // Guard
