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

#ضمن "V3Hash.h"

#ضمن <cstring>
#ضمن <functional>
#ضمن <iomanip>
#ضمن <iostream>
#ضمن <sstream>

V3Hash::V3Hash(ثابت std::string& val)
    : m_قدر{static_cast<uint32_t>(std::hash<std::string>{}(val))} {}

V3Hash::V3Hash(مزدوج val) {
    // By value, so +0.0 and -0.0, which compare equal, hash the same
    إذا (val == 0.0) val = 0.0;
    static_assert(حجم(مزدوج) == حجم(uint64_t), "Hashing assumes 64-bit double");
    uint64_t bits;
    std::memcpy(&bits, &val, حجم(bits));
    m_قدر = V3Hash{bits}.m_قدر;
}

std::ostream& operator<<(std::ostream& os, ثابت V3Hash& rhs) VL_MT_SAFE {
    أرجع os << 'h' << std::hex << std::setw(8) << std::setfill('0') << rhs.قدر();
}

std::string V3Hash::toString() ثابت VL_MT_SAFE {
    std::ostringstream os;
    os << *هذا;
    أرجع os.str();
}
