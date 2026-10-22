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

#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ <cstdint>
#অন্তর্ভাৱ <string>

//######################################################################
// V3Hash -- Generic hashing

class V3Hash final {
    uint32_t m_মান;  // The 32-bit hash value.

    স্থির uint32_t combine(uint32_t a, uint32_t b) {
        ঘূৰাই a ^ (b + 0x9e3779b9 + (a << 6) + (a >> 2));
    }

সার্ৱজনিক:
    // CONSTRUCTORS
    V3Hash()
        : m_মান{0} {}
    explicit V3Hash(uint32_t val)
        : m_মান{val} {}
    explicit V3Hash(uint64_t val)
        : m_মান{combine(static_cast<uint32_t>(val), static_cast<uint32_t>(val >> 32))} {}
    explicit V3Hash(ধ্রুৱ std::string& val);
    explicit V3Hash(দ্ৱিগুণ val);

    explicit V3Hash(int32_t val)
        : V3Hash{static_cast<uint32_t>(val)} {}
    explicit V3Hash(int64_t val)
        : V3Hash{static_cast<uint64_t>(val)} {}
    explicit V3Hash(ধ্রুৱ শূন্য* val)
        : V3Hash{static_cast<uint64_t>(reinterpret_cast<uintptr_t>(val))} {}

    // METHODS
    uint32_t মান() ধ্রুৱ VL_MT_SAFE { ঘূৰাই m_মান; }
    std::string toString() ধ্রুৱ VL_MT_SAFE;

    // OPERATORS
    // Comparisons
    bool operator==(ধ্রুৱ V3Hash& rh) ধ্রুৱ { ঘূৰাই m_মান == rh.m_মান; }
    bool operator!=(ধ্রুৱ V3Hash& rh) ধ্রুৱ { ঘূৰাই m_মান != rh.m_মান; }
    bool operator<(ধ্রুৱ V3Hash& rh) ধ্রুৱ { ঘূৰাই m_মান < rh.m_মান; }

    // '+' combines hashes
    template <typename T>
    V3Hash operator+(T that) ধ্রুৱ {
        ঘূৰাই V3Hash{combine(m_মান, V3Hash{that}.m_মান)};
    }
    V3Hash operator+(ধ্রুৱ std::string& that) ধ্রুৱ {
        ঘূৰাই V3Hash{combine(m_মান, V3Hash{that}.m_মান)};
    }

    // '+=' combines in place
    template <typename T>
    V3Hash& operator+=(T that) {
        ঘূৰাই *this = *this + that;
    }
    V3Hash& operator+=(ধ্রুৱ std::string& that) { ঘূৰাই *this = *this + that; }
};

std::ostream& operator<<(std::ostream& os, ধ্রুৱ V3Hash& rhs) VL_MT_SAFE;

template <>
সংরচনা std::hash<V3Hash> {
    std::size_t operator()(ধ্রুৱ V3Hash& h) ধ্রুৱ noexcept { ঘূৰাই h.মান(); }
};

#endif  // Guard
