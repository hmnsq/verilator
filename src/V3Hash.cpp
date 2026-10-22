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

#അന്തര്ഭാവ "V3Hash.h"

#അന്തര്ഭാവ <cstring>
#അന്തര്ഭാവ <functional>
#അന്തര്ഭാവ <iomanip>
#അന്തര്ഭാവ <iostream>
#അന്തര്ഭാവ <sstream>

V3Hash::V3Hash(സ്ഥിരം std::string& val)
    : m_മൂല്യം{static_cast<uint32_t>(std::hash<std::string>{}(val))} {}

V3Hash::V3Hash(ഇരട്ട val) {
    // By value, so +0.0 and -0.0, which compare equal, hash the same
    എങ്കിൽ (val == 0.0) val = 0.0;
    static_assert(വലുപ്പം(ഇരട്ട) == വലുപ്പം(uint64_t), "Hashing assumes 64-bit double");
    uint64_t bits;
    std::memcpy(&bits, &val, വലുപ്പം(bits));
    m_മൂല്യം = V3Hash{bits}.m_മൂല്യം;
}

std::ostream& operator<<(std::ostream& os, സ്ഥിരം V3Hash& rhs) VL_MT_SAFE {
    തിരികെ os << 'h' << std::hex << std::setw(8) << std::setfill('0') << rhs.മൂല്യം();
}

std::string V3Hash::toString() സ്ഥിരം VL_MT_SAFE {
    std::ostringstream os;
    os << *ഇത്;
    തിരികെ os.str();
}
