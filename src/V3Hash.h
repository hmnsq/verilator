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

#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ <cstdint>
#અન્તર્ભાવ <string>

//######################################################################
// V3Hash -- Generic hashing

class V3Hash final {
    uint32_t m_કિંમત;  // The 32-bit hash value.

    સ્થિર uint32_t combine(uint32_t a, uint32_t b) {
        પાછા a ^ (b + 0x9e3779b9 + (a << 6) + (a >> 2));
    }

જાહેર:
    // CONSTRUCTORS
    V3Hash()
        : m_કિંમત{0} {}
    explicit V3Hash(uint32_t val)
        : m_કિંમત{val} {}
    explicit V3Hash(uint64_t val)
        : m_કિંમત{combine(static_cast<uint32_t>(val), static_cast<uint32_t>(val >> 32))} {}
    explicit V3Hash(અચલ std::string& val);
    explicit V3Hash(દ્વિગુણ val);

    explicit V3Hash(int32_t val)
        : V3Hash{static_cast<uint32_t>(val)} {}
    explicit V3Hash(int64_t val)
        : V3Hash{static_cast<uint64_t>(val)} {}
    explicit V3Hash(અચલ શૂન્ય* val)
        : V3Hash{static_cast<uint64_t>(reinterpret_cast<uintptr_t>(val))} {}

    // METHODS
    uint32_t કિંમત() અચલ VL_MT_SAFE { પાછા m_કિંમત; }
    std::string toString() અચલ VL_MT_SAFE;

    // OPERATORS
    // Comparisons
    bool operator==(અચલ V3Hash& rh) અચલ { પાછા m_કિંમત == rh.m_કિંમત; }
    bool operator!=(અચલ V3Hash& rh) અચલ { પાછા m_કિંમત != rh.m_કિંમત; }
    bool operator<(અચલ V3Hash& rh) અચલ { પાછા m_કિંમત < rh.m_કિંમત; }

    // '+' combines hashes
    template <typename T>
    V3Hash operator+(T that) અચલ {
        પાછા V3Hash{combine(m_કિંમત, V3Hash{that}.m_કિંમત)};
    }
    V3Hash operator+(અચલ std::string& that) અચલ {
        પાછા V3Hash{combine(m_કિંમત, V3Hash{that}.m_કિંમત)};
    }

    // '+=' combines in place
    template <typename T>
    V3Hash& operator+=(T that) {
        પાછા *this = *this + that;
    }
    V3Hash& operator+=(અચલ std::string& that) { પાછા *this = *this + that; }
};

std::ostream& operator<<(std::ostream& os, અચલ V3Hash& rhs) VL_MT_SAFE;

template <>
સંરચના std::hash<V3Hash> {
    std::size_t operator()(અચલ V3Hash& h) અચલ noexcept { પાછા h.કિંમત(); }
};

#endif  // Guard
