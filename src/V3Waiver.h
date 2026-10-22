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

#यदि_अपरिभाषितम् VERILATOR_V3WAIVER_H_
#परिभाषय VERILATOR_V3WAIVER_H_

#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3Mutex.h"

#अन्तर्भाव <string>
#अन्तर्भाव <vector>

class V3Waiver final {
    // TYPES
    using WaiverList = std::सदिशम्<std::string>;
    स्थिर V3Mutex s_mutex;  // Protect members
    स्थिर WaiverList s_waiverList VL_GUARDED_BY(s_mutex);

सार्वजनिक:
    स्थिर शून्य सङ्कलय_प्रविष्टिः(V3_Error_संहिता error_संहिता, ध्रुव string& filename, ध्रुव std::string& सन्देशः_2)
        VL_MT_SAFE_EXCLUDES(s_mutex);
    स्थिर शून्य लिख(ध्रुव std::string& filename) VL_MT_SAFE_EXCLUDES(s_mutex);
};

#यदि_अन्तः  // Guard
