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

#اگر_تعریف_نشده VERILATOR_V3WAIVER_H_
#تعریف_کن VERILATOR_V3WAIVER_H_

#ضمن "V3Error.h"
#ضمن "V3Mutex.h"

#ضمن <string>
#ضمن <vector>

class V3Waiver final {
    // TYPES
    using WaiverList = std::وکتور<std::string>;
    ساكن V3Mutex s_mutex;  // Protect members
    ساكن WaiverList s_waiverList VL_GUARDED_BY(s_mutex);

عام:
    ساكن فراغ جمع_کن_مدخل(V3_Error_کد error_کد, ثابت string& filename, ثابت std::string& پیام_2)
        VL_MT_امن_EXCLUDES(s_mutex);
    ساكن فراغ اكتب(ثابت std::string& filename) VL_MT_امن_EXCLUDES(s_mutex);
};

#پایان_اگر  // Guard
