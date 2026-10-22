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

#ifndef VERILATOR_V3HASH_H_
#define VERILATOR_V3HASH_H_

#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ <cstdint>
#ಅನ್ತರ್ಭಾವ <string>

//######################################################################
// V3Hash -- Generic hashing

class V3Hash final {
    uint32_t m_ಮೌಲ್ಯ;  // The 32-bit hash value.

    ಸ್ಥಿರ uint32_t combine(uint32_t a, uint32_t b) {
        ಪ್ರತ್ಯಾಗಮ a ^ (b + 0x9e3779b9 + (a << 6) + (a >> 2));
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    V3Hash()
        : m_ಮೌಲ್ಯ{0} {}
    explicit V3Hash(uint32_t val)
        : m_ಮೌಲ್ಯ{val} {}
    explicit V3Hash(uint64_t val)
        : m_ಮೌಲ್ಯ{combine(static_cast<uint32_t>(val), static_cast<uint32_t>(val >> 32))} {}
    explicit V3Hash(ಧ್ರುವ std::string& val);
    explicit V3Hash(ದ್ವಿಗುಣ val);

    explicit V3Hash(int32_t val)
        : V3Hash{static_cast<uint32_t>(val)} {}
    explicit V3Hash(int64_t val)
        : V3Hash{static_cast<uint64_t>(val)} {}
    explicit V3Hash(ಧ್ರುವ ಶೂನ್ಯ* val)
        : V3Hash{static_cast<uint64_t>(reinterpret_cast<uintptr_t>(val))} {}

    // METHODS
    uint32_t ಮೌಲ್ಯ() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_ಮೌಲ್ಯ; }
    std::string toString() ಧ್ರುವ VL_MT_SAFE;

    // OPERATORS
    // Comparisons
    bool operator==(ಧ್ರುವ V3Hash& rh) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ಮೌಲ್ಯ == rh.m_ಮೌಲ್ಯ; }
    bool operator!=(ಧ್ರುವ V3Hash& rh) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ಮೌಲ್ಯ != rh.m_ಮೌಲ್ಯ; }
    bool operator<(ಧ್ರುವ V3Hash& rh) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ಮೌಲ್ಯ < rh.m_ಮೌಲ್ಯ; }

    // '+' combines hashes
    template <typename T>
    V3Hash operator+(T that) ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ V3Hash{combine(m_ಮೌಲ್ಯ, V3Hash{that}.m_ಮೌಲ್ಯ)};
    }
    V3Hash operator+(ಧ್ರುವ std::string& that) ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ V3Hash{combine(m_ಮೌಲ್ಯ, V3Hash{that}.m_ಮೌಲ್ಯ)};
    }

    // '+=' combines in place
    template <typename T>
    V3Hash& operator+=(T that) {
        ಪ್ರತ್ಯಾಗಮ *this = *this + that;
    }
    V3Hash& operator+=(ಧ್ರುವ std::string& that) { ಪ್ರತ್ಯಾಗಮ *this = *this + that; }
};

std::ostream& operator<<(std::ostream& os, ಧ್ರುವ V3Hash& rhs) VL_MT_SAFE;

template <>
ಸಂರಚನಾ std::hash<V3Hash> {
    std::size_t operator()(ಧ್ರುವ V3Hash& h) ಧ್ರುವ noexcept { ಪ್ರತ್ಯಾಗಮ h.ಮೌಲ್ಯ(); }
};

#endif  // Guard
