// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Process force/release
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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3FORCE_H_
#وصف_ڪريو VERILATOR_V3FORCE_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

class AstNetlist;

//============================================================================

class V3Force final {
عام:
    ساكن فراغ force_And_سونپيو_All(AstNetlist* nodep) VL_MT_DISABLED;
};

#جيڪڏهن_پورو  // Guard
