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

#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ <cstdint>
#ଅନ୍ତର୍ଭାଵ <string>

//######################################################################
// V3Hash -- Generic hashing

class V3Hash final {
    uint32_t m_ମୂଲ୍ୟ;  // The 32-bit hash value.

    ସ୍ଥିର uint32_t combine(uint32_t a, uint32_t b) {
        ଫେରନ୍ତୁ a ^ (b + 0x9e3779b9 + (a << 6) + (a >> 2));
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    V3Hash()
        : m_ମୂଲ୍ୟ{0} {}
    explicit V3Hash(uint32_t val)
        : m_ମୂଲ୍ୟ{val} {}
    explicit V3Hash(uint64_t val)
        : m_ମୂଲ୍ୟ{combine(static_cast<uint32_t>(val), static_cast<uint32_t>(val >> 32))} {}
    explicit V3Hash(ଅଚଳ std::string& val);
    explicit V3Hash(ଦ୍ୱିଗୁଣ val);

    explicit V3Hash(int32_t val)
        : V3Hash{static_cast<uint32_t>(val)} {}
    explicit V3Hash(int64_t val)
        : V3Hash{static_cast<uint64_t>(val)} {}
    explicit V3Hash(ଅଚଳ ଶୂନ୍ୟ* val)
        : V3Hash{static_cast<uint64_t>(reinterpret_cast<uintptr_t>(val))} {}

    // METHODS
    uint32_t ମୂଲ୍ୟ() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_ମୂଲ୍ୟ; }
    std::string toString() ଅଚଳ VL_MT_SAFE;

    // OPERATORS
    // Comparisons
    bool operator==(ଅଚଳ V3Hash& rh) ଅଚଳ { ଫେରନ୍ତୁ m_ମୂଲ୍ୟ == rh.m_ମୂଲ୍ୟ; }
    bool operator!=(ଅଚଳ V3Hash& rh) ଅଚଳ { ଫେରନ୍ତୁ m_ମୂଲ୍ୟ != rh.m_ମୂଲ୍ୟ; }
    bool operator<(ଅଚଳ V3Hash& rh) ଅଚଳ { ଫେରନ୍ତୁ m_ମୂଲ୍ୟ < rh.m_ମୂଲ୍ୟ; }

    // '+' combines hashes
    template <typename T>
    V3Hash operator+(T that) ଅଚଳ {
        ଫେରନ୍ତୁ V3Hash{combine(m_ମୂଲ୍ୟ, V3Hash{that}.m_ମୂଲ୍ୟ)};
    }
    V3Hash operator+(ଅଚଳ std::string& that) ଅଚଳ {
        ଫେରନ୍ତୁ V3Hash{combine(m_ମୂଲ୍ୟ, V3Hash{that}.m_ମୂଲ୍ୟ)};
    }

    // '+=' combines in place
    template <typename T>
    V3Hash& operator+=(T that) {
        ଫେରନ୍ତୁ *this = *this + that;
    }
    V3Hash& operator+=(ଅଚଳ std::string& that) { ଫେରନ୍ତୁ *this = *this + that; }
};

std::ostream& operator<<(std::ostream& os, ଅଚଳ V3Hash& rhs) VL_MT_SAFE;

template <>
ସଂରଚନା std::hash<V3Hash> {
    std::size_t operator()(ଅଚଳ V3Hash& h) ଅଚଳ noexcept { ଫେରନ୍ତୁ h.ମୂଲ୍ୟ(); }
};

#endif  // Guard
