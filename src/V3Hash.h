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

#شامل "verilatedos.h"

#شامل <cstdint>
#شامل <string>

//######################################################################
// V3Hash -- Generic hashing

class V3Hash final {
    uint32_t m_مقدار;  // The 32-bit hash value.

    ایستا uint32_t combine(uint32_t a, uint32_t b) {
        بازگشت a ^ (b + 0x9e3779b9 + (a << 6) + (a >> 2));
    }

عمومی:
    // CONSTRUCTORS
    V3Hash()
        : m_مقدار{0} {}
    explicit V3Hash(uint32_t val)
        : m_مقدار{val} {}
    explicit V3Hash(uint64_t val)
        : m_مقدار{combine(static_cast<uint32_t>(val), static_cast<uint32_t>(val >> 32))} {}
    explicit V3Hash(ثابت std::string& val);
    explicit V3Hash(دوگانه val);

    explicit V3Hash(int32_t val)
        : V3Hash{static_cast<uint32_t>(val)} {}
    explicit V3Hash(int64_t val)
        : V3Hash{static_cast<uint64_t>(val)} {}
    explicit V3Hash(ثابت تهی* val)
        : V3Hash{static_cast<uint64_t>(reinterpret_cast<uintptr_t>(val))} {}

    // METHODS
    uint32_t مقدار() ثابت VL_MT_SAFE { بازگشت m_مقدار; }
    std::string toString() ثابت VL_MT_SAFE;

    // OPERATORS
    // Comparisons
    bool operator==(ثابت V3Hash& rh) ثابت { بازگشت m_مقدار == rh.m_مقدار; }
    bool operator!=(ثابت V3Hash& rh) ثابت { بازگشت m_مقدار != rh.m_مقدار; }
    bool operator<(ثابت V3Hash& rh) ثابت { بازگشت m_مقدار < rh.m_مقدار; }

    // '+' combines hashes
    template <typename T>
    V3Hash operator+(T that) ثابت {
        بازگشت V3Hash{combine(m_مقدار, V3Hash{that}.m_مقدار)};
    }
    V3Hash operator+(ثابت std::string& that) ثابت {
        بازگشت V3Hash{combine(m_مقدار, V3Hash{that}.m_مقدار)};
    }

    // '+=' combines in place
    template <typename T>
    V3Hash& operator+=(T that) {
        بازگشت *this = *this + that;
    }
    V3Hash& operator+=(ثابت std::string& that) { بازگشت *this = *this + that; }
};

std::ostream& operator<<(std::ostream& os, ثابت V3Hash& rhs) VL_MT_SAFE;

template <>
ساختار std::hash<V3Hash> {
    std::size_t operator()(ثابت V3Hash& h) ثابت noexcept { بازگشت h.مقدار(); }
};

#endif  // Guard
