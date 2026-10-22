// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Diag Sarif output file
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

#ifndef VERILATOR_V3DIAGSARIF_H_
#define VERILATOR_V3DIAGSARIF_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن <V3Error.h>

//============================================================================

class V3DiagSarif final {
عام:
    ساكن فراغ ئىتتىرىش_Message(ثابت VErrorMessage& ئۇچۇر) VL_MT_DISABLED;
    ساكن فراغ output(bool success) VL_MT_DISABLED;
};

#endif  // Guard
