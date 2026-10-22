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

#অন্তর্ভাব "V3Error.h"
#অন্তর্ভাব "V3Mutex.h"

#অন্তর্ভাব <string>
#অন্তর্ভাব <vector>

class V3Waiver final {
    // TYPES
    using WaiverList = std::vector<std::string>;
    স্থির V3Mutex s_mutex;  // Protect members
    স্থির WaiverList s_waiverList VL_GUARDED_BY(s_mutex);

সার্বজনিক:
    স্থির শূন্য addEntry(V3ErrorCode errorCode, ধ্রুব string& filename, ধ্রুব std::string& বার্তা)
        VL_MT_SAFE_EXCLUDES(s_mutex);
    স্থির শূন্য লিখ(ধ্রুব std::string& filename) VL_MT_SAFE_EXCLUDES(s_mutex);
};

#endif  // Guard
