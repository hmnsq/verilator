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

#నిర్వచితం_కాకపోతే VERILATOR_V3WAIVER_H_
#నిర్వచించు VERILATOR_V3WAIVER_H_

#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3Mutex.h"

#అన్తర్భావ <string>
#అన్తర్భావ <vector>

class V3Waiver final {
    // TYPES
    using WaiverList = std::సదిశ<std::string>;
    స్థిర V3Mutex s_mutex;  // Protect members
    స్థిర WaiverList s_waiverList VL_GUARDED_BY(s_mutex);

బహిరంగ:
    స్థిర శూన్య కూడు_నమోదు(V3_Error_సంకేతం error_సంకేతం, స్థిరరాశి string& filename, స్థిరరాశి std::string& సందేశం_2)
        VL_MT_సురక్షిత_EXCLUDES(s_mutex);
    స్థిర శూన్య లిఖ(స్థిరరాశి std::string& filename) VL_MT_సురక్షిత_EXCLUDES(s_mutex);
};

#అయితే_ముగింపు  // Guard
