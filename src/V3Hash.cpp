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

#ଅନ୍ତର୍ଭାଵ "V3Hash.h"

#ଅନ୍ତର୍ଭାଵ <cstring>
#ଅନ୍ତର୍ଭାଵ <functional>
#ଅନ୍ତର୍ଭାଵ <iomanip>
#ଅନ୍ତର୍ଭାଵ <iostream>
#ଅନ୍ତର୍ଭାଵ <sstream>

V3Hash::V3Hash(ଅଚଳ std::string& val)
    : m_ମୂଲ୍ୟ{static_cast<uint32_t>(std::hash<std::string>{}(val))} {}

V3Hash::V3Hash(ଦ୍ୱିଗୁଣ val) {
    // By value, so +0.0 and -0.0, which compare equal, hash the same
    ଯଦି (val == 0.0) val = 0.0;
    static_assert(ଆକାର(ଦ୍ୱିଗୁଣ) == ଆକାର(uint64_t), "Hashing assumes 64-bit double");
    uint64_t bits;
    std::memcpy(&bits, &val, ଆକାର(bits));
    m_ମୂଲ୍ୟ = V3Hash{bits}.m_ମୂଲ୍ୟ;
}

std::ostream& operator<<(std::ostream& os, ଅଚଳ V3Hash& rhs) VL_MT_SAFE {
    ଫେରନ୍ତୁ os << 'h' << std::hex << std::setw(8) << std::setfill('0') << rhs.ମୂଲ୍ୟ();
}

std::string V3Hash::toString() ଅଚଳ VL_MT_SAFE {
    std::ostringstream os;
    os << *ଏହି;
    ଫେରନ୍ତୁ os.str();
}
