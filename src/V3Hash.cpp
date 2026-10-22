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

#ಅನ್ತರ್ಭಾವ "V3Hash.h"

#ಅನ್ತರ್ಭಾವ <cstring>
#ಅನ್ತರ್ಭಾವ <functional>
#ಅನ್ತರ್ಭಾವ <iomanip>
#ಅನ್ತರ್ಭಾವ <iostream>
#ಅನ್ತರ್ಭಾವ <sstream>

V3Hash::V3Hash(ಧ್ರುವ std::string& val)
    : m_ಮೌಲ್ಯ{static_cast<uint32_t>(std::hash<std::string>{}(val))} {}

V3Hash::V3Hash(ದ್ವಿಗುಣ val) {
    // By value, so +0.0 and -0.0, which compare equal, hash the same
    ಯದಿ (val == 0.0) val = 0.0;
    static_assert(ಪರಿಮಾಣ(ದ್ವಿಗುಣ) == ಪರಿಮಾಣ(uint64_t), "Hashing assumes 64-bit double");
    uint64_t bits;
    std::memcpy(&bits, &val, ಪರಿಮಾಣ(bits));
    m_ಮೌಲ್ಯ = V3Hash{bits}.m_ಮೌಲ್ಯ;
}

std::ostream& operator<<(std::ostream& os, ಧ್ರುವ V3Hash& rhs) VL_MT_SAFE {
    ಪ್ರತ್ಯಾಗಮ os << 'h' << std::hex << std::setw(8) << std::setfill('0') << rhs.ಮೌಲ್ಯ();
}

std::string V3Hash::toString() ಧ್ರುವ VL_MT_SAFE {
    std::ostringstream os;
    os << *ಏತತ್;
    ಪ್ರತ್ಯಾಗಮ os.str();
}
