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

#অন্তর্ভাব "V3Hash.h"

#অন্তর্ভাব <cstring>
#অন্তর্ভাব <functional>
#অন্তর্ভাব <iomanip>
#অন্তর্ভাব <iostream>
#অন্তর্ভাব <sstream>

V3Hash::V3Hash(ধ্রুবক std::string& val)
    : m_মান{static_cast<uint32_t>(std::hash<std::string>{}(val))} {}

V3Hash::V3Hash(দ্বিগুণ val) {
    // By value, so +0.0 and -0.0, which compare equal, hash the same
    যদি (val == 0.0) val = 0.0;
    static_assert(আকার(দ্বিগুণ) == আকার(uint64_t), "Hashing assumes 64-bit double");
    uint64_t bits;
    std::memcpy(&bits, &val, আকার(bits));
    m_মান = V3Hash{bits}.m_মান;
}

std::ostream& operator<<(std::ostream& os, ধ্রুবক V3Hash& rhs) VL_MT_SAFE {
    ফেরত os << 'h' << std::hex << std::setw(8) << std::setfill('0') << rhs.মান();
}

std::string V3Hash::toString() ধ্রুবক VL_MT_SAFE {
    std::ostringstream os;
    os << *এই;
    ফেরত os.str();
}
