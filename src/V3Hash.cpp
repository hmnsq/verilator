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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Hash.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <cstring>
#ਅਨ੍ਤਰ੍ਭਾਵ <functional>
#ਅਨ੍ਤਰ੍ਭਾਵ <iomanip>
#ਅਨ੍ਤਰ੍ਭਾਵ <iostream>
#ਅਨ੍ਤਰ੍ਭਾਵ <sstream>

V3Hash::V3Hash(ਅਚਲ std::string& val)
    : m_ਮੁੱਲ{static_cast<uint32_t>(std::hash<std::string>{}(val))} {}

V3Hash::V3Hash(ਦੁੱਗਣਾ val) {
    // By value, so +0.0 and -0.0, which compare equal, hash the same
    ਜੇ (val == 0.0) val = 0.0;
    static_assert(ਆਕਾਰ(ਦੁੱਗਣਾ) == ਆਕਾਰ(uint64_t), "Hashing assumes 64-bit double");
    uint64_t ਬਿੱਟ;
    std::ਸਮ੍ਰਿਤੀ_ਨਕਲ(&ਬਿੱਟ, &val, ਆਕਾਰ(ਬਿੱਟ));
    m_ਮੁੱਲ = V3Hash{ਬਿੱਟ}.m_ਮੁੱਲ;
}

std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, ਅਚਲ V3Hash& rhs) VL_MT_ਸੁਰੱਖਿਅਤ {
    ਵਾਪਸ os << 'h' << std::hex << std::setw(8) << std::setfill('0') << rhs.ਮੁੱਲ();
}

std::string V3Hash::ਮੰਜ਼ਿਲ_String() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
    std::ostringstream os;
    os << *ਇਹ;
    ਵਾਪਸ os.str();
}
