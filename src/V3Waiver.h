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

#ضمن "V3Error.h"
#ضمن "V3Mutex.h"

#ضمن <string>
#ضمن <vector>

class V3Waiver final {
    // TYPES
    using WaiverList = std::vector<std::string>;
    ساكن V3Mutex s_mutex;  // Protect members
    ساكن WaiverList s_waiverList VL_GUARDED_BY(s_mutex);

عام:
    ساكن فراغ addEntry(V3ErrorCode errorCode, ثابت string& filename, ثابت std::string& رسالة)
        VL_MT_SAFE_EXCLUDES(s_mutex);
    ساكن فراغ اكتب(ثابت std::string& filename) VL_MT_SAFE_EXCLUDES(s_mutex);
};

#endif  // Guard
