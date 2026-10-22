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

#که_نه_تعریف_شوی VERILATOR_V3HASH_H_
#تعریف_کړه VERILATOR_V3HASH_H_

#ضمن "verilatedos.h"

#ضمن <cstdint>
#ضمن <string>

//######################################################################
// V3Hash -- Generic hashing

class V3Hash final {
    uint32_t m_ارزښت;  // The 32-bit hash value.

    ساكن uint32_t combine(uint32_t a, uint32_t b) {
        أرجع a ^ (b + 0x9e3779b9 + (a << 6) + (a >> 2));
    }

عام:
    // CONSTRUCTORS
    V3Hash()
        : m_ارزښت{0} {}
    explicit V3Hash(uint32_t val)
        : m_ارزښت{val} {}
    explicit V3Hash(uint64_t val)
        : m_ارزښت{combine(static_cast<uint32_t>(val), static_cast<uint32_t>(val >> 32))} {}
    explicit V3Hash(ثابت std::string& val);
    explicit V3Hash(مزدوج val);

    explicit V3Hash(int32_t val)
        : V3Hash{static_cast<uint32_t>(val)} {}
    explicit V3Hash(int64_t val)
        : V3Hash{static_cast<uint64_t>(val)} {}
    explicit V3Hash(ثابت فراغ* val)
        : V3Hash{static_cast<uint64_t>(reinterpret_cast<uintptr_t>(val))} {}

    // METHODS
    uint32_t ارزښت() ثابت VL_MT_خوندي { أرجع m_ارزښت; }
    std::string منزل_String() ثابت VL_MT_خوندي;

    // OPERATORS
    // Comparisons
    bool چلوونکی==(ثابت V3Hash& rh) ثابت { أرجع m_ارزښت == rh.m_ارزښت; }
    bool چلوونکی!=(ثابت V3Hash& rh) ثابت { أرجع m_ارزښت != rh.m_ارزښت; }
    bool چلوونکی<(ثابت V3Hash& rh) ثابت { أرجع m_ارزښت < rh.m_ارزښت; }

    // '+' combines hashes
    کينډۍ <typename T>
    V3Hash چلوونکی+(T that) ثابت {
        أرجع V3Hash{combine(m_ارزښت, V3Hash{that}.m_ارزښت)};
    }
    V3Hash چلوونکی+(ثابت std::string& that) ثابت {
        أرجع V3Hash{combine(m_ارزښت, V3Hash{that}.m_ارزښت)};
    }

    // '+=' combines in place
    کينډۍ <typename T>
    V3Hash& چلوونکی+=(T that) {
        أرجع *this = *this + that;
    }
    V3Hash& چلوونکی+=(ثابت std::string& that) { أرجع *this = *this + that; }
};

std::ostream& چلوونکی<<(std::ostream& os, ثابت V3Hash& rhs) VL_MT_خوندي;

کينډۍ <>
بنية std::hash<V3Hash> {
    std::اندازه_t چلوونکی()(ثابت V3Hash& h) ثابت noexcept { أرجع h.ارزښت(); }
};

#د_که_پای  // Guard
