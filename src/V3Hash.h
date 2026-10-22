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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3HASH_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3HASH_H_

#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ <cstdint>
#ಅನ್ತರ್ಭಾವ <string>

//######################################################################
// V3Hash -- Generic hashing

class V3Hash final {
    uint32_t m_ಮೌಲ್ಯ;  // The 32-bit hash value.

    ಸ್ಥಿರ uint32_t combine(uint32_t a, uint32_t b) {
        ಹಿಂತಿರುಗಿ a ^ (b + 0x9e3779b9 + (a << 6) + (a >> 2));
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    V3Hash()
        : m_ಮೌಲ್ಯ{0} {}
    explicit V3Hash(uint32_t val)
        : m_ಮೌಲ್ಯ{val} {}
    explicit V3Hash(uint64_t val)
        : m_ಮೌಲ್ಯ{combine(static_cast<uint32_t>(val), static_cast<uint32_t>(val >> 32))} {}
    explicit V3Hash(ಸ್ಥಿರಾಂಕ std::string& val);
    explicit V3Hash(ದ್ವಿಗುಣ val);

    explicit V3Hash(int32_t val)
        : V3Hash{static_cast<uint32_t>(val)} {}
    explicit V3Hash(int64_t val)
        : V3Hash{static_cast<uint64_t>(val)} {}
    explicit V3Hash(ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ* val)
        : V3Hash{static_cast<uint64_t>(reinterpret_cast<uintptr_t>(val))} {}

    // METHODS
    uint32_t ಮೌಲ್ಯ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_ಮೌಲ್ಯ; }
    std::string ಗಮ್ಯ_String() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;

    // OPERATORS
    // Comparisons
    bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ V3Hash& rh) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಮೌಲ್ಯ == rh.m_ಮೌಲ್ಯ; }
    bool ಕ್ರಿಯಾಕಾರಕ!=(ಸ್ಥಿರಾಂಕ V3Hash& rh) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಮೌಲ್ಯ != rh.m_ಮೌಲ್ಯ; }
    bool ಕ್ರಿಯಾಕಾರಕ<(ಸ್ಥಿರಾಂಕ V3Hash& rh) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಮೌಲ್ಯ < rh.m_ಮೌಲ್ಯ; }

    // '+' combines hashes
    ಮಾದರಿ <typename T>
    V3Hash ಕ್ರಿಯಾಕಾರಕ+(T that) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ V3Hash{combine(m_ಮೌಲ್ಯ, V3Hash{that}.m_ಮೌಲ್ಯ)};
    }
    V3Hash ಕ್ರಿಯಾಕಾರಕ+(ಸ್ಥಿರಾಂಕ std::string& that) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ V3Hash{combine(m_ಮೌಲ್ಯ, V3Hash{that}.m_ಮೌಲ್ಯ)};
    }

    // '+=' combines in place
    ಮಾದರಿ <typename T>
    V3Hash& ಕ್ರಿಯಾಕಾರಕ+=(T that) {
        ಹಿಂತಿರುಗಿ *this = *this + that;
    }
    V3Hash& ಕ್ರಿಯಾಕಾರಕ+=(ಸ್ಥಿರಾಂಕ std::string& that) { ಹಿಂತಿರುಗಿ *this = *this + that; }
};

std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, ಸ್ಥಿರಾಂಕ V3Hash& rhs) VL_MT_ಸುರಕ್ಷಿತ;

ಮಾದರಿ <>
ರಚನೆ std::hash<V3Hash> {
    std::ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ V3Hash& h) ಸ್ಥಿರಾಂಕ noexcept { ಹಿಂತಿರುಗಿ h.ಮೌಲ್ಯ(); }
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
