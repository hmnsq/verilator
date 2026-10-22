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
    uint32_t m_मान;  // The 32-bit hash value.

    स्थिर uint32_t combine(uint32_t a, uint32_t b) {
        फर्काऊ a ^ (b + 0x9e3779b9 + (a << 6) + (a >> 2));
    }

सार्वजनिक:
    // CONSTRUCTORS
    V3Hash()
        : m_मान{0} {}
    explicit V3Hash(uint32_t val)
        : m_मान{val} {}
    explicit V3Hash(uint64_t val)
        : m_मान{combine(static_cast<uint32_t>(val), static_cast<uint32_t>(val >> 32))} {}
    explicit V3Hash(अचर std::string& val);
    explicit V3Hash(द्विगुण val);

    explicit V3Hash(int32_t val)
        : V3Hash{static_cast<uint32_t>(val)} {}
    explicit V3Hash(int64_t val)
        : V3Hash{static_cast<uint64_t>(val)} {}
    explicit V3Hash(अचर शून्य* val)
        : V3Hash{static_cast<uint64_t>(reinterpret_cast<uintptr_t>(val))} {}

    // METHODS
    uint32_t मान() अचर VL_MT_SAFE { फर्काऊ m_मान; }
    std::string toString() अचर VL_MT_SAFE;

    // OPERATORS
    // Comparisons
    bool चालक==(अचर V3Hash& rh) अचर { फर्काऊ m_मान == rh.m_मान; }
    bool चालक!=(अचर V3Hash& rh) अचर { फर्काऊ m_मान != rh.m_मान; }
    bool चालक<(अचर V3Hash& rh) अचर { फर्काऊ m_मान < rh.m_मान; }

    // '+' combines hashes
    ढाँचा <typename T>
    V3Hash चालक+(T that) अचर {
        फर्काऊ V3Hash{combine(m_मान, V3Hash{that}.m_मान)};
    }
    V3Hash चालक+(अचर std::string& that) अचर {
        फर्काऊ V3Hash{combine(m_मान, V3Hash{that}.m_मान)};
    }

    // '+=' combines in place
    ढाँचा <typename T>
    V3Hash& चालक+=(T that) {
        फर्काऊ *this = *this + that;
    }
    V3Hash& चालक+=(अचर std::string& that) { फर्काऊ *this = *this + that; }
};

std::ostream& चालक<<(std::ostream& os, अचर V3Hash& rhs) VL_MT_SAFE;

ढाँचा <>
संरचना std::hash<V3Hash> {
    std::माप_प्रकार चालक()(अचर V3Hash& h) अचर noexcept { फर्काऊ h.मान(); }
};

#endif  // Guard
