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

#ifndef VERILATOR_VLCBUCKET_H_
#define VERILATOR_VLCBUCKET_H_

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#ifndef V3ERROR_NO_GLOBAL_
#define V3ERROR_NO_GLOBAL_
#endif
#અન્તર્ભાવ "V3Error.h"

//********************************************************************
// VlcBuckets - Container of all coverage point hits for a given test
// This is a bitmap array - we store a single bit to indicate a test
// has hit that point with sufficient coverage.

class VlcBuckets final {
    // MEMBERS
    uint64_t* m_datap = nullptr;  ///< Pointer to first bucket (dynamically allocated)
    uint64_t m_dataSize = 0;  ///< Current entries in m_datap
    uint64_t m_bucketsCovered = 0;  ///< Num buckets with sufficient coverage

    સ્થિર uint64_t covBit(uint64_t point) { પાછા 1ULL << (point & 63); }
    uint64_t allocSize() અચલ { પાછા કદ(uint64_t) * m_dataSize / 64; }
    શૂન્ય allocate(uint64_t point) {
        અચલ uint64_t oldsize = m_dataSize;
        જો (m_dataSize < point) m_dataSize = (point + 64) & ~63ULL;  // Keep power of two
        m_dataSize *= 2;
        // UINFO(9, "Realloc " << allocSize() << " for " << point << "  " << cvtToHex(m_datap));
        uint64_t* અચલ newp = static_cast<uint64_t*>(std::realloc(m_datap, allocSize()));
        જો (VL_UNCOVERABLE(!newp)) {
            // cppcheck-suppress doubleFree  // cppcheck 1.90 bug - realloc doesn't free on fail
            મુક્ત(m_datap);  // LCOV_EXCL_LINE
            v3fatal("Out of memory increasing buckets");  // LCOV_EXCL_LINE
        }
        m_datap = newp;
        માટે (uint64_t i = oldsize; i < m_dataSize; i += 64) m_datap[i / 64] = 0;
    }

જાહેર:
    // CONSTRUCTORS
    VlcBuckets() { allocate(1024); }
    ~VlcBuckets() {
        m_dataSize = 0;
        VL_DO_CLEAR(મુક્ત(m_datap), m_datap = nullptr);
    }

    // ACCESSORS
    સ્થિર uint64_t sufficient() { પાછા 1; }
    uint64_t bucketsCovered() અચલ { પાછા m_bucketsCovered; }

    // METHODS
    શૂન્ય addData(uint64_t point, uint64_t hits) {
        જો (hits >= sufficient()) {
            // UINFO(9, "     addData " << point << " " << hits << " size=" << m_dataSize);
            જો (point >= m_dataSize) allocate(point);
            m_datap[point / 64] |= covBit(point);
            m_bucketsCovered++;
        }
    }
    શૂન્ય clearHits(uint64_t point) અચલ {
        જો (point >= m_dataSize) પાછા;
        m_datap[point / 64] &= ~covBit(point);
    }
    bool exists(uint64_t point) અચલ {
        જો (point >= m_dataSize) પાછા false;
        પાછા (m_datap[point / 64] & covBit(point)) ? true : false;
    }
    uint64_t hits(uint64_t point) અચલ {
        જો (point >= m_dataSize) પાછા 0;
        પાછા (m_datap[point / 64] & covBit(point)) ? 1 : 0;
    }
    uint64_t કાઢો_ગણતરી() અચલ {
        uint64_t કાઢો = 0;
        માટે (uint64_t i = 0; i < m_dataSize; ++i) {
            જો (hits(i)) ++કાઢો;
        }
        પાછા કાઢો;
    }
    uint64_t data_કાઢો_ગણતરી(અચલ VlcBuckets& remaining) અચલ {
        uint64_t કાઢો = 0;
        માટે (uint64_t i = 0; i < m_dataSize; ++i) {
            જો (hits(i) && remaining.hits(i)) ++કાઢો;
        }
        પાછા કાઢો;
    }
    શૂન્ય orData(અચલ VlcBuckets& ordata) {
        માટે (uint64_t i = 0; i < m_dataSize; ++i) {
            જો (hits(i) && ordata.hits(i)) clearHits(i);
        }
    }

    શૂન્ય dump() અચલ {
        std::cout << "#     ";
        માટે (uint64_t i = 0; i < m_dataSize; ++i) {
            જો (hits(i)) std::cout << "," << i;
        }
        std::cout << "\n";
    }
};

//######################################################################

#endif  // guard
