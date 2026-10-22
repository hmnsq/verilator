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

#अगर_अघोषित VERILATOR_V3HASH_H_
#घोषणा VERILATOR_V3HASH_H_

#समावेश "verilatedos.h"

#समावेश <cstdint>
#समावेश <string>

//######################################################################
// V3Hash -- Generic hashing

class V3Hash final {
    uint32_t m_मूल्य;  // The 32-bit hash value.

    अटल uint32_t combine(uint32_t a, uint32_t b) {
        वापस a ^ (b + 0x9e3779b9 + (a << 6) + (a >> 2));
    }

खुला:
    // CONSTRUCTORS
    V3Hash()
        : m_मूल्य{0} {}
    explicit V3Hash(uint32_t val)
        : m_मूल्य{val} {}
    explicit V3Hash(uint64_t val)
        : m_मूल्य{combine(static_cast<uint32_t>(val), static_cast<uint32_t>(val >> 32))} {}
    explicit V3Hash(स्थिर std::string& val);
    explicit V3Hash(द्विगुन val);

    explicit V3Hash(int32_t val)
        : V3Hash{static_cast<uint32_t>(val)} {}
    explicit V3Hash(int64_t val)
        : V3Hash{static_cast<uint64_t>(val)} {}
    explicit V3Hash(स्थिर व्योम* val)
        : V3Hash{static_cast<uint64_t>(reinterpret_cast<uintptr_t>(val))} {}

    // METHODS
    uint32_t मूल्य() स्थिर VL_MT_SAFE { वापस m_मूल्य; }
    std::string toString() स्थिर VL_MT_SAFE;

    // OPERATORS
    // Comparisons
    bool चालक==(स्थिर V3Hash& rh) स्थिर { वापस m_मूल्य == rh.m_मूल्य; }
    bool चालक!=(स्थिर V3Hash& rh) स्थिर { वापस m_मूल्य != rh.m_मूल्य; }
    bool चालक<(स्थिर V3Hash& rh) स्थिर { वापस m_मूल्य < rh.m_मूल्य; }

    // '+' combines hashes
    ढाँचा <typename T>
    V3Hash चालक+(T that) स्थिर {
        वापस V3Hash{combine(m_मूल्य, V3Hash{that}.m_मूल्य)};
    }
    V3Hash चालक+(स्थिर std::string& that) स्थिर {
        वापस V3Hash{combine(m_मूल्य, V3Hash{that}.m_मूल्य)};
    }

    // '+=' combines in place
    ढाँचा <typename T>
    V3Hash& चालक+=(T that) {
        वापस *this = *this + that;
    }
    V3Hash& चालक+=(स्थिर std::string& that) { वापस *this = *this + that; }
};

std::ostream& चालक<<(std::ostream& os, स्थिर V3Hash& rhs) VL_MT_SAFE;

ढाँचा <>
काष्ठा std::hash<V3Hash> {
    std::माप_प्रकार चालक()(स्थिर V3Hash& h) स्थिर noexcept { वापस h.मूल्य(); }
};

#पूर्ण_अगर  // Guard
