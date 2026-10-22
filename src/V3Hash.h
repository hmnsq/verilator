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

#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব <cstdint>
#অন্তর্ভাব <string>

//######################################################################
// V3Hash -- Generic hashing

class V3Hash final {
    uint32_t m_মান;  // The 32-bit hash value.

    স্থির uint32_t combine(uint32_t a, uint32_t b) {
        ফেরত a ^ (b + 0x9e3779b9 + (a << 6) + (a >> 2));
    }

সর্বজনীন:
    // CONSTRUCTORS
    V3Hash()
        : m_মান{0} {}
    explicit V3Hash(uint32_t val)
        : m_মান{val} {}
    explicit V3Hash(uint64_t val)
        : m_মান{combine(static_cast<uint32_t>(val), static_cast<uint32_t>(val >> 32))} {}
    explicit V3Hash(ধ্রুবক std::string& val);
    explicit V3Hash(দ্বিগুণ val);

    explicit V3Hash(int32_t val)
        : V3Hash{static_cast<uint32_t>(val)} {}
    explicit V3Hash(int64_t val)
        : V3Hash{static_cast<uint64_t>(val)} {}
    explicit V3Hash(ধ্রুবক শূন্য* val)
        : V3Hash{static_cast<uint64_t>(reinterpret_cast<uintptr_t>(val))} {}

    // METHODS
    uint32_t মান() ধ্রুবক VL_MT_SAFE { ফেরত m_মান; }
    std::string toString() ধ্রুবক VL_MT_SAFE;

    // OPERATORS
    // Comparisons
    bool operator==(ধ্রুবক V3Hash& rh) ধ্রুবক { ফেরত m_মান == rh.m_মান; }
    bool operator!=(ধ্রুবক V3Hash& rh) ধ্রুবক { ফেরত m_মান != rh.m_মান; }
    bool operator<(ধ্রুবক V3Hash& rh) ধ্রুবক { ফেরত m_মান < rh.m_মান; }

    // '+' combines hashes
    template <typename T>
    V3Hash operator+(T that) ধ্রুবক {
        ফেরত V3Hash{combine(m_মান, V3Hash{that}.m_মান)};
    }
    V3Hash operator+(ধ্রুবক std::string& that) ধ্রুবক {
        ফেরত V3Hash{combine(m_মান, V3Hash{that}.m_মান)};
    }

    // '+=' combines in place
    template <typename T>
    V3Hash& operator+=(T that) {
        ফেরত *this = *this + that;
    }
    V3Hash& operator+=(ধ্রুবক std::string& that) { ফেরত *this = *this + that; }
};

std::ostream& operator<<(std::ostream& os, ধ্রুবক V3Hash& rhs) VL_MT_SAFE;

template <>
কাঠামো std::hash<V3Hash> {
    std::আকার_t operator()(ধ্রুবক V3Hash& h) ধ্রুবক noexcept { ফেরত h.মান(); }
};

#endif  // Guard
