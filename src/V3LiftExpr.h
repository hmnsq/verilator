// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Lift expressions out of statements
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

#ifndef VERILATOR_V3LIFTEXPR_H_
#define VERILATOR_V3LIFTEXPR_H_

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

class AstNetlist;

//============================================================================

class V3_Lift_ପଦସମଷ୍ଟି final {
ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ lift_ପଦସମଷ୍ଟି_All(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // Guard
