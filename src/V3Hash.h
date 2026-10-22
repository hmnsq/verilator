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

#اگر_غیر_تعریف_شدہ VERILATOR_V3HASH_H_
#تعریف_کرو VERILATOR_V3HASH_H_

#شامل "verilatedos.h"

#شامل <cstdint>
#شامل <string>

//######################################################################
// V3Hash -- Generic hashing

class V3Hash final {
    uint32_t m_قدر;  // The 32-bit hash value.

    ساکن uint32_t combine(uint32_t a, uint32_t b) {
        واپس a ^ (b + 0x9e3779b9 + (a << 6) + (a >> 2));
    }

عوامی:
    // CONSTRUCTORS
    V3Hash()
        : m_قدر{0} {}
    explicit V3Hash(uint32_t val)
        : m_قدر{val} {}
    explicit V3Hash(uint64_t val)
        : m_قدر{combine(static_cast<uint32_t>(val), static_cast<uint32_t>(val >> 32))} {}
    explicit V3Hash(ثابت std::string& val);
    explicit V3Hash(دوہرا val);

    explicit V3Hash(int32_t val)
        : V3Hash{static_cast<uint32_t>(val)} {}
    explicit V3Hash(int64_t val)
        : V3Hash{static_cast<uint64_t>(val)} {}
    explicit V3Hash(ثابت خالی* val)
        : V3Hash{static_cast<uint64_t>(reinterpret_cast<uintptr_t>(val))} {}

    // METHODS
    uint32_t قدر() ثابت VL_MT_محفوظ { واپس m_قدر; }
    std::string مقصد_String() ثابت VL_MT_محفوظ;

    // OPERATORS
    // Comparisons
    bool عامل==(ثابت V3Hash& rh) ثابت { واپس m_قدر == rh.m_قدر; }
    bool عامل!=(ثابت V3Hash& rh) ثابت { واپس m_قدر != rh.m_قدر; }
    bool عامل<(ثابت V3Hash& rh) ثابت { واپس m_قدر < rh.m_قدر; }

    // '+' combines hashes
    سانچہ <typename T>
    V3Hash عامل+(T that) ثابت {
        واپس V3Hash{combine(m_قدر, V3Hash{that}.m_قدر)};
    }
    V3Hash عامل+(ثابت std::string& that) ثابت {
        واپس V3Hash{combine(m_قدر, V3Hash{that}.m_قدر)};
    }

    // '+=' combines in place
    سانچہ <typename T>
    V3Hash& عامل+=(T that) {
        واپس *this = *this + that;
    }
    V3Hash& عامل+=(ثابت std::string& that) { واپس *this = *this + that; }
};

std::ostream& عامل<<(std::ostream& os, ثابت V3Hash& rhs) VL_MT_محفوظ;

سانچہ <>
ساخت std::hash<V3Hash> {
    std::سائز_t عامل()(ثابت V3Hash& h) ثابت noexcept { واپس h.قدر(); }
};

#اگر_ختم  // Guard
