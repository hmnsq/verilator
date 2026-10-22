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

#அந்தர்பாவ "V3Hash.h"

#அந்தர்பாவ <cstring>
#அந்தர்பாவ <functional>
#அந்தர்பாவ <iomanip>
#அந்தர்பாவ <iostream>
#அந்தர்பாவ <sstream>

V3Hash::V3Hash(மாறிலி std::string& val)
    : m_மதிப்பு{static_cast<uint32_t>(std::hash<std::string>{}(val))} {}

V3Hash::V3Hash(இரட்டை val) {
    // By value, so +0.0 and -0.0, which compare equal, hash the same
    எனில் (val == 0.0) val = 0.0;
    static_assert(அளவு(இரட்டை) == அளவு(uint64_t), "Hashing assumes 64-bit double");
    uint64_t bits;
    std::memcpy(&bits, &val, அளவு(bits));
    m_மதிப்பு = V3Hash{bits}.m_மதிப்பு;
}

std::ostream& operator<<(std::ostream& os, மாறிலி V3Hash& rhs) VL_MT_SAFE {
    திரும்பு os << 'h' << std::hex << std::setw(8) << std::setfill('0') << rhs.மதிப்பு();
}

std::string V3Hash::toString() மாறிலி VL_MT_SAFE {
    std::ostringstream os;
    os << *இது;
    திரும்பு os.str();
}
