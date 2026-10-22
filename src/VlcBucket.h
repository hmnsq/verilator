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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ifndef V3ERROR_NO_GLOBAL_
#define V3ERROR_NO_GLOBAL_
#endif
#ಅನ್ತರ್ಭಾವ "V3Error.h"

//********************************************************************
// VlcBuckets - Container of all coverage point hits for a given test
// This is a bitmap array - we store a single bit to indicate a test
// has hit that point with sufficient coverage.

class VlcBuckets final {
    // MEMBERS
    uint64_t* m_datap = nullptr;  ///< Pointer to first bucket (dynamically allocated)
    uint64_t m_dataSize = 0;  ///< Current entries in m_datap
    uint64_t m_bucketsCovered = 0;  ///< Num buckets with sufficient coverage

    ಸ್ಥಿರ uint64_t covBit(uint64_t point) { ಪ್ರತ್ಯಾಗಮ 1ULL << (point & 63); }
    uint64_t allocSize() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ಪರಿಮಾಣ(uint64_t) * m_dataSize / 64; }
    ಶೂನ್ಯ allocate(uint64_t point) {
        ಧ್ರುವ uint64_t oldsize = m_dataSize;
        ಯದಿ (m_dataSize < point) m_dataSize = (point + 64) & ~63ULL;  // Keep power of two
        m_dataSize *= 2;
        // UINFO(9, "Realloc " << allocSize() << " for " << point << "  " << cvtToHex(m_datap));
        uint64_t* ಧ್ರುವ newp = static_cast<uint64_t*>(std::realloc(m_datap, allocSize()));
        ಯದಿ (VL_UNCOVERABLE(!newp)) {
            // cppcheck-suppress doubleFree  // cppcheck 1.90 bug - realloc doesn't free on fail
            ಮುಕ್ತ(m_datap);  // LCOV_EXCL_LINE
            v3fatal("Out of memory increasing buckets");  // LCOV_EXCL_LINE
        }
        m_datap = newp;
        ಪ್ರತಿ (uint64_t i = oldsize; i < m_dataSize; i += 64) m_datap[i / 64] = 0;
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    VlcBuckets() { allocate(1024); }
    ~VlcBuckets() {
        m_dataSize = 0;
        VL_DO_CLEAR(ಮುಕ್ತ(m_datap), m_datap = nullptr);
    }

    // ACCESSORS
    ಸ್ಥಿರ uint64_t sufficient() { ಪ್ರತ್ಯಾಗಮ 1; }
    uint64_t bucketsCovered() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_bucketsCovered; }

    // METHODS
    ಶೂನ್ಯ addData(uint64_t point, uint64_t hits) {
        ಯದಿ (hits >= sufficient()) {
            // UINFO(9, "     addData " << point << " " << hits << " size=" << m_dataSize);
            ಯದಿ (point >= m_dataSize) allocate(point);
            m_datap[point / 64] |= covBit(point);
            m_bucketsCovered++;
        }
    }
    ಶೂನ್ಯ clearHits(uint64_t point) ಧ್ರುವ {
        ಯದಿ (point >= m_dataSize) ಪ್ರತ್ಯಾಗಮ;
        m_datap[point / 64] &= ~covBit(point);
    }
    bool exists(uint64_t point) ಧ್ರುವ {
        ಯದಿ (point >= m_dataSize) ಪ್ರತ್ಯಾಗಮ false;
        ಪ್ರತ್ಯಾಗಮ (m_datap[point / 64] & covBit(point)) ? true : false;
    }
    uint64_t hits(uint64_t point) ಧ್ರುವ {
        ಯದಿ (point >= m_dataSize) ಪ್ರತ್ಯಾಗಮ 0;
        ಪ್ರತ್ಯಾಗಮ (m_datap[point / 64] & covBit(point)) ? 1 : 0;
    }
    uint64_t ತೆಗೆ_ಎಣಿಕೆ() ಧ್ರುವ {
        uint64_t ತೆಗೆ = 0;
        ಪ್ರತಿ (uint64_t i = 0; i < m_dataSize; ++i) {
            ಯದಿ (hits(i)) ++ತೆಗೆ;
        }
        ಪ್ರತ್ಯಾಗಮ ತೆಗೆ;
    }
    uint64_t data_ತೆಗೆ_ಎಣಿಕೆ(ಧ್ರುವ VlcBuckets& remaining) ಧ್ರುವ {
        uint64_t ತೆಗೆ = 0;
        ಪ್ರತಿ (uint64_t i = 0; i < m_dataSize; ++i) {
            ಯದಿ (hits(i) && remaining.hits(i)) ++ತೆಗೆ;
        }
        ಪ್ರತ್ಯಾಗಮ ತೆಗೆ;
    }
    ಶೂನ್ಯ orData(ಧ್ರುವ VlcBuckets& ordata) {
        ಪ್ರತಿ (uint64_t i = 0; i < m_dataSize; ++i) {
            ಯದಿ (hits(i) && ordata.hits(i)) clearHits(i);
        }
    }

    ಶೂನ್ಯ dump() ಧ್ರುವ {
        std::cout << "#     ";
        ಪ್ರತಿ (uint64_t i = 0; i < m_dataSize; ++i) {
            ಯದಿ (hits(i)) std::cout << "," << i;
        }
        std::cout << "\n";
    }
};

//######################################################################

#endif  // guard
