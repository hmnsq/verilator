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

#నిర్వచితం_కాకపోతే VERILATOR_V3HASH_H_
#నిర్వచించు VERILATOR_V3HASH_H_

#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ <cstdint>
#అన్తర్భావ <string>

//######################################################################
// V3Hash -- Generic hashing

class V3Hash final {
    uint32_t m_విలువ;  // The 32-bit hash value.

    స్థిర uint32_t combine(uint32_t a, uint32_t b) {
        తిరిగి a ^ (b + 0x9e3779b9 + (a << 6) + (a >> 2));
    }

బహిరంగ:
    // CONSTRUCTORS
    V3Hash()
        : m_విలువ{0} {}
    explicit V3Hash(uint32_t val)
        : m_విలువ{val} {}
    explicit V3Hash(uint64_t val)
        : m_విలువ{combine(static_cast<uint32_t>(val), static_cast<uint32_t>(val >> 32))} {}
    explicit V3Hash(స్థిరరాశి std::string& val);
    explicit V3Hash(ద్విగుణ val);

    explicit V3Hash(int32_t val)
        : V3Hash{static_cast<uint32_t>(val)} {}
    explicit V3Hash(int64_t val)
        : V3Hash{static_cast<uint64_t>(val)} {}
    explicit V3Hash(స్థిరరాశి శూన్య* val)
        : V3Hash{static_cast<uint64_t>(reinterpret_cast<uintptr_t>(val))} {}

    // METHODS
    uint32_t విలువ() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_విలువ; }
    std::string గమ్యం_String() స్థిరరాశి VL_MT_సురక్షిత;

    // OPERATORS
    // Comparisons
    bool క్రియాకారకం==(స్థిరరాశి V3Hash& rh) స్థిరరాశి { తిరిగి m_విలువ == rh.m_విలువ; }
    bool క్రియాకారకం!=(స్థిరరాశి V3Hash& rh) స్థిరరాశి { తిరిగి m_విలువ != rh.m_విలువ; }
    bool క్రియాకారకం<(స్థిరరాశి V3Hash& rh) స్థిరరాశి { తిరిగి m_విలువ < rh.m_విలువ; }

    // '+' combines hashes
    మూస <typename T>
    V3Hash క్రియాకారకం+(T that) స్థిరరాశి {
        తిరిగి V3Hash{combine(m_విలువ, V3Hash{that}.m_విలువ)};
    }
    V3Hash క్రియాకారకం+(స్థిరరాశి std::string& that) స్థిరరాశి {
        తిరిగి V3Hash{combine(m_విలువ, V3Hash{that}.m_విలువ)};
    }

    // '+=' combines in place
    మూస <typename T>
    V3Hash& క్రియాకారకం+=(T that) {
        తిరిగి *this = *this + that;
    }
    V3Hash& క్రియాకారకం+=(స్థిరరాశి std::string& that) { తిరిగి *this = *this + that; }
};

std::ostream& క్రియాకారకం<<(std::ostream& os, స్థిరరాశి V3Hash& rhs) VL_MT_సురక్షిత;

మూస <>
నిర్మాణం std::hash<V3Hash> {
    std::కొలత_t క్రియాకారకం()(స్థిరరాశి V3Hash& h) స్థిరరాశి noexcept { తిరిగి h.విలువ(); }
};

#అయితే_ముగింపు  // Guard
