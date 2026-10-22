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

#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव <cstdint>
#अन्तर्भाव <string>

//######################################################################
// V3Hash -- Generic hashing

class V3Hash final {
    uint32_t m_मूल्य;  // The 32-bit hash value.

    स्थिर uint32_t combine(uint32_t a, uint32_t b) {
        परत a ^ (b + 0x9e3779b9 + (a << 6) + (a >> 2));
    }

सार्वजनिक:
    // CONSTRUCTORS
    V3Hash()
        : m_मूल्य{0} {}
    explicit V3Hash(uint32_t val)
        : m_मूल्य{val} {}
    explicit V3Hash(uint64_t val)
        : m_मूल्य{combine(static_cast<uint32_t>(val), static_cast<uint32_t>(val >> 32))} {}
    explicit V3Hash(स्थिरांक std::string& val);
    explicit V3Hash(द्विगुण val);

    explicit V3Hash(int32_t val)
        : V3Hash{static_cast<uint32_t>(val)} {}
    explicit V3Hash(int64_t val)
        : V3Hash{static_cast<uint64_t>(val)} {}
    explicit V3Hash(स्थिरांक शून्य* val)
        : V3Hash{static_cast<uint64_t>(reinterpret_cast<uintptr_t>(val))} {}

    // METHODS
    uint32_t मूल्य() स्थिरांक VL_MT_SAFE { परत m_मूल्य; }
    std::string toString() स्थिरांक VL_MT_SAFE;

    // OPERATORS
    // Comparisons
    bool चालक==(स्थिरांक V3Hash& rh) स्थिरांक { परत m_मूल्य == rh.m_मूल्य; }
    bool चालक!=(स्थिरांक V3Hash& rh) स्थिरांक { परत m_मूल्य != rh.m_मूल्य; }
    bool चालक<(स्थिरांक V3Hash& rh) स्थिरांक { परत m_मूल्य < rh.m_मूल्य; }

    // '+' combines hashes
    ढाँचा <typename T>
    V3Hash चालक+(T that) स्थिरांक {
        परत V3Hash{combine(m_मूल्य, V3Hash{that}.m_मूल्य)};
    }
    V3Hash चालक+(स्थिरांक std::string& that) स्थिरांक {
        परत V3Hash{combine(m_मूल्य, V3Hash{that}.m_मूल्य)};
    }

    // '+=' combines in place
    ढाँचा <typename T>
    V3Hash& चालक+=(T that) {
        परत *this = *this + that;
    }
    V3Hash& चालक+=(स्थिरांक std::string& that) { परत *this = *this + that; }
};

std::ostream& चालक<<(std::ostream& os, स्थिरांक V3Hash& rhs) VL_MT_SAFE;

ढाँचा <>
संरचना std::hash<V3Hash> {
    std::माप_प्रकार चालक()(स्थिरांक V3Hash& h) स्थिरांक noexcept { परत h.मूल्य(); }
};

#endif  // Guard
