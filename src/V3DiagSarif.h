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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3DIAGSARIF_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3DIAGSARIF_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ <V3Error.h>

//============================================================================

class V3DiagSarif final {
ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ ತಳ್ಳು_ಸಂದೇಶ(ಸ್ಥಿರಾಂಕ VError_ಸಂದೇಶ& ಸಂದೇಶ_2) VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ output(bool success) VL_MT_DISABLED;
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
