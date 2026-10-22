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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

//============================================================================

class V3CCtors final {
সর্বজনীন:
    স্থির শূন্য cctorsAll() VL_MT_DISABLED;

ব্যক্তিগত:
    স্থির শূন্য evalAsserts() VL_MT_DISABLED;
};

#endif  // Guard
