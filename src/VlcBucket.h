// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: verilator_coverage: Bucket container
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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_VLCBUCKET_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_VLCBUCKET_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ V3ERROR_NO_GLOBAL_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ V3ERROR_NO_GLOBAL_
#ਜੇ_ਅੰਤ
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"

//********************************************************************
// VlcBuckets - Container of all coverage point hits for a given test
// This is a bitmap array - we store a single bit to indicate a test
// has hit that point with sufficient coverage.

class VlcBuckets final {
    // MEMBERS
    uint64_t* m_datap = nullptr;  ///< Pointer to first bucket (dynamically allocated)
    uint64_t m_ਅੰਕੜੇ_ਮਾਪ = 0;  ///< Current entries in m_datap
    uint64_t m_bucketsCovered = 0;  ///< Num buckets with sufficient coverage

    ਸਥਿਰ uint64_t cov_ਦੋਆਧਾਰੀ_ਅੰਕ(uint64_t ਬਿੰਦੂ) { ਵਾਪਸ 1ULL << (ਬਿੰਦੂ & 63); }
    uint64_t alloc_ਮਾਪ() ਅਚਲ { ਵਾਪਸ ਆਕਾਰ(uint64_t) * m_ਅੰਕੜੇ_ਮਾਪ / 64; }
    ਖਾਲੀ allocate(uint64_t ਬਿੰਦੂ) {
        ਅਚਲ uint64_t oldsize = m_ਅੰਕੜੇ_ਮਾਪ;
        ਜੇ (m_ਅੰਕੜੇ_ਮਾਪ < ਬਿੰਦੂ) m_ਅੰਕੜੇ_ਮਾਪ = (ਬਿੰਦੂ + 64) & ~63ULL;  // Keep power of two
        m_ਅੰਕੜੇ_ਮਾਪ *= 2;
        // UINFO(9, "Realloc " << allocSize() << " for " << point << "  " << cvtToHex(m_datap));
        uint64_t* ਅਚਲ newp = static_cast<uint64_t*>(std::realloc(m_datap, alloc_ਮਾਪ()));
        ਜੇ (VL_UNCOVERABLE(!newp)) {
            // cppcheck-suppress doubleFree  // cppcheck 1.90 bug - realloc doesn't free on fail
            ਮੁਕਤ(m_datap);  // LCOV_EXCL_LINE
            v3fatal("Out of memory increasing buckets");  // LCOV_EXCL_LINE
        }
        m_datap = newp;
        ਲਈ (uint64_t i = oldsize; i < m_ਅੰਕੜੇ_ਮਾਪ; i += 64) m_datap[i / 64] = 0;
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    VlcBuckets() { allocate(1024); }
    ~VlcBuckets() {
        m_ਅੰਕੜੇ_ਮਾਪ = 0;
        VL_DO_CLEAR(ਮੁਕਤ(m_datap), m_datap = nullptr);
    }

    // ACCESSORS
    ਸਥਿਰ uint64_t sufficient() { ਵਾਪਸ 1; }
    uint64_t bucketsCovered() ਅਚਲ { ਵਾਪਸ m_bucketsCovered; }

    // METHODS
    ਖਾਲੀ ਜੋੜੋ_ਅੰਕੜੇ(uint64_t ਬਿੰਦੂ, uint64_t ਪ੍ਰਾਪਤੀਆਂ) {
        ਜੇ (ਪ੍ਰਾਪਤੀਆਂ >= sufficient()) {
            // UINFO(9, "     addData " << point << " " << hits << " size=" << m_dataSize);
            ਜੇ (ਬਿੰਦੂ >= m_ਅੰਕੜੇ_ਮਾਪ) allocate(ਬਿੰਦੂ);
            m_datap[ਬਿੰਦੂ / 64] |= cov_ਦੋਆਧਾਰੀ_ਅੰਕ(ਬਿੰਦੂ);
            m_bucketsCovered++;
        }
    }
    ਖਾਲੀ clear_ਪ੍ਰਾਪਤੀਆਂ(uint64_t ਬਿੰਦੂ) ਅਚਲ {
        ਜੇ (ਬਿੰਦੂ >= m_ਅੰਕੜੇ_ਮਾਪ) ਵਾਪਸ;
        m_datap[ਬਿੰਦੂ / 64] &= ~cov_ਦੋਆਧਾਰੀ_ਅੰਕ(ਬਿੰਦੂ);
    }
    bool exists(uint64_t ਬਿੰਦੂ) ਅਚਲ {
        ਜੇ (ਬਿੰਦੂ >= m_ਅੰਕੜੇ_ਮਾਪ) ਵਾਪਸ false;
        ਵਾਪਸ (m_datap[ਬਿੰਦੂ / 64] & cov_ਦੋਆਧਾਰੀ_ਅੰਕ(ਬਿੰਦੂ)) ? true : false;
    }
    uint64_t ਪ੍ਰਾਪਤੀਆਂ(uint64_t ਬਿੰਦੂ) ਅਚਲ {
        ਜੇ (ਬਿੰਦੂ >= m_ਅੰਕੜੇ_ਮਾਪ) ਵਾਪਸ 0;
        ਵਾਪਸ (m_datap[ਬਿੰਦੂ / 64] & cov_ਦੋਆਧਾਰੀ_ਅੰਕ(ਬਿੰਦੂ)) ? 1 : 0;
    }
    uint64_t ਕੱਢੋ_ਗਿਣਤੀ() ਅਚਲ {
        uint64_t ਕੱਢੋ = 0;
        ਲਈ (uint64_t i = 0; i < m_ਅੰਕੜੇ_ਮਾਪ; ++i) {
            ਜੇ (ਪ੍ਰਾਪਤੀਆਂ(i)) ++ਕੱਢੋ;
        }
        ਵਾਪਸ ਕੱਢੋ;
    }
    uint64_t ਅੰਕੜੇ_ਕੱਢੋ_ਗਿਣਤੀ(ਅਚਲ VlcBuckets& ਬਚਿਆ) ਅਚਲ {
        uint64_t ਕੱਢੋ = 0;
        ਲਈ (uint64_t i = 0; i < m_ਅੰਕੜੇ_ਮਾਪ; ++i) {
            ਜੇ (ਪ੍ਰਾਪਤੀਆਂ(i) && ਬਚਿਆ.ਪ੍ਰਾਪਤੀਆਂ(i)) ++ਕੱਢੋ;
        }
        ਵਾਪਸ ਕੱਢੋ;
    }
    ਖਾਲੀ or_ਅੰਕੜੇ(ਅਚਲ VlcBuckets& ordata) {
        ਲਈ (uint64_t i = 0; i < m_ਅੰਕੜੇ_ਮਾਪ; ++i) {
            ਜੇ (ਪ੍ਰਾਪਤੀਆਂ(i) && ordata.ਪ੍ਰਾਪਤੀਆਂ(i)) clear_ਪ੍ਰਾਪਤੀਆਂ(i);
        }
    }

    ਖਾਲੀ dump() ਅਚਲ {
        std::cout << "#     ";
        ਲਈ (uint64_t i = 0; i < m_ਅੰਕੜੇ_ਮਾਪ; ++i) {
            ਜੇ (ਪ੍ਰਾਪਤੀਆਂ(i)) std::cout << "," << i;
        }
        std::cout << "\n";
    }
};

//######################################################################

#ਜੇ_ਅੰਤ  // guard
