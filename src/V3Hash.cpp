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

#समावेश "V3Hash.h"

#समावेश <cstring>
#समावेश <functional>
#समावेश <iomanip>
#समावेश <iostream>
#समावेश <sstream>

V3Hash::V3Hash(स्थिर std::string& val)
    : m_मूल्य{static_cast<uint32_t>(std::hash<std::string>{}(val))} {}

V3Hash::V3Hash(द्विगुन val) {
    // By value, so +0.0 and -0.0, which compare equal, hash the same
    अगर (val == 0.0) val = 0.0;
    static_assert(माप(द्विगुन) == माप(uint64_t), "Hashing assumes 64-bit double");
    uint64_t bits;
    std::स_नकल(&bits, &val, माप(bits));
    m_मूल्य = V3Hash{bits}.m_मूल्य;
}

std::ब_प्रवाह& चालक<<(std::ब_प्रवाह& os, स्थिर V3Hash& rhs) VL_MT_SAFE {
    वापस os << 'h' << std::षष्ठ << std::रखो_चौड़ाई(8) << std::रखो_भरो('0') << rhs.मूल्य();
}

std::string V3Hash::toString() स्थिर VL_MT_SAFE {
    std::ostringstream os;
    os << *यह;
    वापस os.str();
}
