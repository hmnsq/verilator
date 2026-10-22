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

#ifndef VERILATOR_V3WAIVER_H_
#define VERILATOR_V3WAIVER_H_

#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3Mutex.h"

#അന്തര്ഭാവ <string>
#അന്തര്ഭാവ <vector>

class V3Waiver final {
    // TYPES
    using WaiverList = std::vector<std::string>;
    സ്ഥിതം V3Mutex s_mutex;  // Protect members
    സ്ഥിതം WaiverList s_waiverList VL_GUARDED_BY(s_mutex);

പൊതു:
    സ്ഥിതം ശൂന്യം addEntry(V3ErrorCode errorCode, സ്ഥിരം string& filename, സ്ഥിരം std::string& സന്ദേശം)
        VL_MT_SAFE_EXCLUDES(s_mutex);
    സ്ഥിതം ശൂന്യം ലിഖ(സ്ഥിരം std::string& filename) VL_MT_SAFE_EXCLUDES(s_mutex);
};

#endif  // Guard
