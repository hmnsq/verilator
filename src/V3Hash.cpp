// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Hash calculation
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

#అన్తర్భావ "V3Hash.h"

#అన్తర్భావ <cstring>
#అన్తర్భావ <functional>
#అన్తర్భావ <iomanip>
#అన్తర్భావ <iostream>
#అన్తర్భావ <sstream>

V3Hash::V3Hash(స్థిరరాశి std::string& val)
    : m_విలువ{static_cast<uint32_t>(std::hash<std::string>{}(val))} {}

V3Hash::V3Hash(ద్విగుణ val) {
    // By value, so +0.0 and -0.0, which compare equal, hash the same
    ఒకవేళ (val == 0.0) val = 0.0;
    static_assert(పరిమాణం(ద్విగుణ) == పరిమాణం(uint64_t), "Hashing assumes 64-bit double");
    uint64_t బిట్లు;
    std::జ్ఞాపకం_నకలు(&బిట్లు, &val, పరిమాణం(బిట్లు));
    m_విలువ = V3Hash{బిట్లు}.m_విలువ;
}

std::ostream& క్రియాకారకం<<(std::ostream& os, స్థిరరాశి V3Hash& rhs) VL_MT_సురక్షిత {
    తిరిగి os << 'h' << std::hex << std::setw(8) << std::setfill('0') << rhs.విలువ();
}

std::string V3Hash::గమ్యం_String() స్థిరరాశి VL_MT_సురక్షిత {
    std::ostringstream os;
    os << *ఇది;
    తిరిగి os.str();
}
