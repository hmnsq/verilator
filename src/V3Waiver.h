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

#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3Mutex.h"

#ଅନ୍ତର୍ଭାଵ <string>
#ଅନ୍ତର୍ଭାଵ <vector>

class V3Waiver final {
    // TYPES
    using WaiverList = std::vector<std::string>;
    ସ୍ଥିର V3Mutex s_mutex;  // Protect members
    ସ୍ଥିର WaiverList s_waiverList VL_GUARDED_BY(s_mutex);

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ addEntry(V3ErrorCode errorCode, ଅଚଳ string& filename, ଅଚଳ std::string& ସନ୍ଦେଶ)
        VL_MT_SAFE_EXCLUDES(s_mutex);
    ସ୍ଥିର ଶୂନ୍ୟ ଲିଖ(ଅଚଳ std::string& filename) VL_MT_SAFE_EXCLUDES(s_mutex);
};

#endif  // Guard
