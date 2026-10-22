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

#அந்தர்பாவ "V3Error.h"
#அந்தர்பாவ "V3Mutex.h"

#அந்தர்பாவ <string>
#அந்தர்பாவ <vector>

class V3Waiver final {
    // TYPES
    using WaiverList = std::vector<std::string>;
    நிலையான V3Mutex s_mutex;  // Protect members
    நிலையான WaiverList s_waiverList VL_GUARDED_BY(s_mutex);

பொது:
    நிலையான வெறுமை addEntry(V3ErrorCode errorCode, மாறிலி string& filename, மாறிலி std::string& செய்தி)
        VL_MT_SAFE_EXCLUDES(s_mutex);
    நிலையான வெறுமை லிக(மாறிலி std::string& filename) VL_MT_SAFE_EXCLUDES(s_mutex);
};

#endif  // Guard
