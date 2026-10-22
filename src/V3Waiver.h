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

#अगर_अघोषित VERILATOR_V3WAIVER_H_
#घोषणा VERILATOR_V3WAIVER_H_

#समावेश "V3Error.h"
#समावेश "V3Mutex.h"

#समावेश <string>
#समावेश <vector>

class V3Waiver final {
    // TYPES
    using WaiverList = std::vector<std::string>;
    अटल V3Mutex s_mutex;  // Protect members
    अटल WaiverList s_waiverList VL_GUARDED_BY(s_mutex);

खुला:
    अटल व्योम addEntry(V3ErrorCode errorCode, स्थिर string& filename, स्थिर std::string& संदेश)
        VL_MT_SAFE_EXCLUDES(s_mutex);
    अटल व्योम ग_लिखो(स्थिर std::string& filename) VL_MT_SAFE_EXCLUDES(s_mutex);
};

#पूर्ण_अगर  // Guard
