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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3HASH_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3HASH_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <cstdint>
#ਅਨ੍ਤਰ੍ਭਾਵ <string>

//######################################################################
// V3Hash -- Generic hashing

class V3Hash final {
    uint32_t m_ਮੁੱਲ;  // The 32-bit hash value.

    ਸਥਿਰ uint32_t combine(uint32_t a, uint32_t b) {
        ਵਾਪਸ a ^ (b + 0x9e3779b9 + (a << 6) + (a >> 2));
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    V3Hash()
        : m_ਮੁੱਲ{0} {}
    explicit V3Hash(uint32_t val)
        : m_ਮੁੱਲ{val} {}
    explicit V3Hash(uint64_t val)
        : m_ਮੁੱਲ{combine(static_cast<uint32_t>(val), static_cast<uint32_t>(val >> 32))} {}
    explicit V3Hash(ਅਚਲ std::string& val);
    explicit V3Hash(ਦੁੱਗਣਾ val);

    explicit V3Hash(int32_t val)
        : V3Hash{static_cast<uint32_t>(val)} {}
    explicit V3Hash(int64_t val)
        : V3Hash{static_cast<uint64_t>(val)} {}
    explicit V3Hash(ਅਚਲ ਖਾਲੀ* val)
        : V3Hash{static_cast<uint64_t>(reinterpret_cast<uintptr_t>(val))} {}

    // METHODS
    uint32_t ਮੁੱਲ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_ਮੁੱਲ; }
    std::string ਮੰਜ਼ਿਲ_String() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;

    // OPERATORS
    // Comparisons
    bool ਸੰਚਾਲਕ==(ਅਚਲ V3Hash& rh) ਅਚਲ { ਵਾਪਸ m_ਮੁੱਲ == rh.m_ਮੁੱਲ; }
    bool ਸੰਚਾਲਕ!=(ਅਚਲ V3Hash& rh) ਅਚਲ { ਵਾਪਸ m_ਮੁੱਲ != rh.m_ਮੁੱਲ; }
    bool ਸੰਚਾਲਕ<(ਅਚਲ V3Hash& rh) ਅਚਲ { ਵਾਪਸ m_ਮੁੱਲ < rh.m_ਮੁੱਲ; }

    // '+' combines hashes
    ਸਾਂਚਾ <typename T>
    V3Hash ਸੰਚਾਲਕ+(T that) ਅਚਲ {
        ਵਾਪਸ V3Hash{combine(m_ਮੁੱਲ, V3Hash{that}.m_ਮੁੱਲ)};
    }
    V3Hash ਸੰਚਾਲਕ+(ਅਚਲ std::string& that) ਅਚਲ {
        ਵਾਪਸ V3Hash{combine(m_ਮੁੱਲ, V3Hash{that}.m_ਮੁੱਲ)};
    }

    // '+=' combines in place
    ਸਾਂਚਾ <typename T>
    V3Hash& ਸੰਚਾਲਕ+=(T that) {
        ਵਾਪਸ *this = *this + that;
    }
    V3Hash& ਸੰਚਾਲਕ+=(ਅਚਲ std::string& that) { ਵਾਪਸ *this = *this + that; }
};

std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, ਅਚਲ V3Hash& rhs) VL_MT_ਸੁਰੱਖਿਅਤ;

ਸਾਂਚਾ <>
ਬਣਤਰ std::hash<V3Hash> {
    std::ਮਾਪ_t ਸੰਚਾਲਕ()(ਅਚਲ V3Hash& h) ਅਚਲ noexcept { ਵਾਪਸ h.ਮੁੱਲ(); }
};

#ਜੇ_ਅੰਤ  // Guard
