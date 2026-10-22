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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ <V3Error.h>

//============================================================================

class V3DiagSarif final {
പൊതു:
    സ്ഥിതം ശൂന്യം തള്ളുക_Message(സ്ഥിരം VErrorMessage& സന്ദേശം) VL_MT_DISABLED;
    സ്ഥിതം ശൂന്യം output(bool success) VL_MT_DISABLED;
};

#endif  // Guard
