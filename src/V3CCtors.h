// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Emit CFunc's for class construction and configuration
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

#ifndef VERILATOR_V3CCTORS_H_
#define VERILATOR_V3CCTORS_H_

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

//============================================================================

class V3CCtors final {
ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ cctorsAll() VL_MT_DISABLED;

ବ୍ୟକ୍ତିଗତ:
    ସ୍ଥିର ଶୂନ୍ୟ evalAsserts() VL_MT_DISABLED;
};

#endif  // Guard
