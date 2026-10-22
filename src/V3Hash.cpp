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

V3Hash::V3Hash(ಸ್ಥಿರಾಂಕ std::string& val)
    : m_ಮೌಲ್ಯ{static_cast<uint32_t>(std::hash<std::string>{}(val))} {}

V3Hash::V3Hash(ದ್ವಿಗುಣ val) {
    // By value, so +0.0 and -0.0, which compare equal, hash the same
    ಆದರೆ (val == 0.0) val = 0.0;
    static_assert(ಗಾತ್ರ(ದ್ವಿಗುಣ) == ಗಾತ್ರ(uint64_t), "Hashing assumes 64-bit double");
    uint64_t ಬಿಟ್ಟುಗಳು;
    std::ಸ್ಮರಣೆ_ನಕಲು(&ಬಿಟ್ಟುಗಳು, &val, ಗಾತ್ರ(ಬಿಟ್ಟುಗಳು));
    m_ಮೌಲ್ಯ = V3Hash{ಬಿಟ್ಟುಗಳು}.m_ಮೌಲ್ಯ;
}

std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, ಸ್ಥಿರಾಂಕ V3Hash& rhs) VL_MT_ಸುರಕ್ಷಿತ {
    ಹಿಂತಿರುಗಿ os << 'h' << std::hex << std::setw(8) << std::setfill('0') << rhs.ಮೌಲ್ಯ();
}

std::string V3Hash::ಗಮ್ಯ_String() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
    std::ostringstream os;
    os << *ಇದು;
    ಹಿಂತಿರುಗಿ os.str();
}
