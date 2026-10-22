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

#अन्तर्भाव "V3Hash.h"

#अन्तर्भाव <cstring>
#अन्तर्भाव <functional>
#अन्तर्भाव <iomanip>
#अन्तर्भाव <iostream>
#अन्तर्भाव <sstream>

V3Hash::V3Hash(स्थिरांक std::string& val)
    : m_मूल्य{static_cast<uint32_t>(std::hash<std::string>{}(val))} {}

V3Hash::V3Hash(द्विगुण val) {
    // By value, so +0.0 and -0.0, which compare equal, hash the same
    जर (val == 0.0) val = 0.0;
    static_assert(आकार(द्विगुण) == आकार(uint64_t), "Hashing assumes 64-bit double");
    uint64_t bits;
    std::स_नकल(&bits, &val, आकार(bits));
    m_मूल्य = V3Hash{bits}.m_मूल्य;
}

std::ostream& चालक<<(std::ostream& os, स्थिरांक V3Hash& rhs) VL_MT_SAFE {
    परत os << 'h' << std::hex << std::setw(8) << std::setfill('0') << rhs.मूल्य();
}

std::string V3Hash::toString() स्थिरांक VL_MT_SAFE {
    std::ostringstream os;
    os << *हे;
    परत os.str();
}
