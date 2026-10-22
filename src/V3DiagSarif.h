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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3DIAGSARIF_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3DIAGSARIF_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <V3Error.h>

//============================================================================

class V3DiagSarif final {
ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ ਧੱਕੋ_ਸੁਨੇਹਾ(ਅਚਲ VError_ਸੁਨੇਹਾ& ਸੁਨੇਹਾ_2) VL_MT_DISABLED;
    ਸਥਿਰ ਖਾਲੀ output(bool success) VL_MT_DISABLED;
};

#ਜੇ_ਅੰਤ  // Guard
