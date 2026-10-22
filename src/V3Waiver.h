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

#اگر_غیر_تعریف_شدہ VERILATOR_V3WAIVER_H_
#تعریف_کرو VERILATOR_V3WAIVER_H_

#شامل "V3Error.h"
#شامل "V3Mutex.h"

#شامل <string>
#شامل <vector>

class V3Waiver final {
    // TYPES
    using WaiverList = std::سمتیہ<std::string>;
    ساکن V3Mutex s_mutex;  // Protect members
    ساکن WaiverList s_waiverList VL_GUARDED_BY(s_mutex);

عوامی:
    ساکن خالی جوڑو_اندراج(V3_Error_رمز error_رمز, ثابت string& filename, ثابت std::string& پیغام_2)
        VL_MT_محفوظ_EXCLUDES(s_mutex);
    ساکن خالی اكتب(ثابت std::string& filename) VL_MT_محفوظ_EXCLUDES(s_mutex);
};

#اگر_ختم  // Guard
