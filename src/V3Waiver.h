// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Emit Waivers
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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3WAIVER_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3WAIVER_H_

#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3Mutex.h"

#ಅನ್ತರ್ಭಾವ <string>
#ಅನ್ತರ್ಭಾವ <vector>

class V3Waiver final {
    // TYPES
    using WaiverList = std::ಸದಿಶ<std::string>;
    ಸ್ಥಿರ V3Mutex s_mutex;  // Protect members
    ಸ್ಥಿರ WaiverList s_waiverList VL_GUARDED_BY(s_mutex);

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_ನಮೂದು(V3_Error_ಸಂಕೇತ error_ಸಂಕೇತ, ಸ್ಥಿರಾಂಕ string& filename, ಸ್ಥಿರಾಂಕ std::string& ಸಂದೇಶ_2)
        VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(s_mutex);
    ಸ್ಥಿರ ಶೂನ್ಯ ಲಿಖ(ಸ್ಥಿರಾಂಕ std::string& filename) VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(s_mutex);
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
