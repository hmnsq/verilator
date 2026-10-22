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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

#ifndef V3ERROR_NO_GLOBAL_
#define V3ERROR_NO_GLOBAL_
#endif
#অন্তর্ভাৱ "V3Error.h"

//********************************************************************
// VlcBuckets - Container of all coverage point hits for a given test
// This is a bitmap array - we store a single bit to indicate a test
// has hit that point with sufficient coverage.

class VlcBuckets final {
    // MEMBERS
    uint64_t* m_datap = nullptr;  ///< Pointer to first bucket (dynamically allocated)
    uint64_t m_dataSize = 0;  ///< Current entries in m_datap
    uint64_t m_bucketsCovered = 0;  ///< Num buckets with sufficient coverage

    স্থির uint64_t covBit(uint64_t point) { ঘূৰাই 1ULL << (point & 63); }
    uint64_t allocSize() ধ্রুৱ { ঘূৰাই পরিমাণ(uint64_t) * m_dataSize / 64; }
    শূন্য allocate(uint64_t point) {
        ধ্রুৱ uint64_t oldsize = m_dataSize;
        যদি (m_dataSize < point) m_dataSize = (point + 64) & ~63ULL;  // Keep power of two
        m_dataSize *= 2;
        // UINFO(9, "Realloc " << allocSize() << " for " << point << "  " << cvtToHex(m_datap));
        uint64_t* ধ্রুৱ newp = static_cast<uint64_t*>(std::realloc(m_datap, allocSize()));
        যদি (VL_UNCOVERABLE(!newp)) {
            // cppcheck-suppress doubleFree  // cppcheck 1.90 bug - realloc doesn't free on fail
            মুক্ত(m_datap);  // LCOV_EXCL_LINE
            v3fatal("Out of memory increasing buckets");  // LCOV_EXCL_LINE
        }
        m_datap = newp;
        বাবে (uint64_t i = oldsize; i < m_dataSize; i += 64) m_datap[i / 64] = 0;
    }

সার্ৱজনিক:
    // CONSTRUCTORS
    VlcBuckets() { allocate(1024); }
    ~VlcBuckets() {
        m_dataSize = 0;
        VL_DO_CLEAR(মুক্ত(m_datap), m_datap = nullptr);
    }

    // ACCESSORS
    স্থির uint64_t sufficient() { ঘূৰাই 1; }
    uint64_t bucketsCovered() ধ্রুৱ { ঘূৰাই m_bucketsCovered; }

    // METHODS
    শূন্য addData(uint64_t point, uint64_t hits) {
        যদি (hits >= sufficient()) {
            // UINFO(9, "     addData " << point << " " << hits << " size=" << m_dataSize);
            যদি (point >= m_dataSize) allocate(point);
            m_datap[point / 64] |= covBit(point);
            m_bucketsCovered++;
        }
    }
    শূন্য clearHits(uint64_t point) ধ্রুৱ {
        যদি (point >= m_dataSize) ঘূৰাই;
        m_datap[point / 64] &= ~covBit(point);
    }
    bool exists(uint64_t point) ধ্রুৱ {
        যদি (point >= m_dataSize) ঘূৰাই false;
        ঘূৰাই (m_datap[point / 64] & covBit(point)) ? true : false;
    }
    uint64_t hits(uint64_t point) ধ্রুৱ {
        যদি (point >= m_dataSize) ঘূৰাই 0;
        ঘূৰাই (m_datap[point / 64] & covBit(point)) ? 1 : 0;
    }
    uint64_t বেৰ_কৰো_গুনতি() ধ্রুৱ {
        uint64_t বেৰ_কৰো = 0;
        বাবে (uint64_t i = 0; i < m_dataSize; ++i) {
            যদি (hits(i)) ++বেৰ_কৰো;
        }
        ঘূৰাই বেৰ_কৰো;
    }
    uint64_t data_বেৰ_কৰো_গুনতি(ধ্রুৱ VlcBuckets& remaining) ধ্রুৱ {
        uint64_t বেৰ_কৰো = 0;
        বাবে (uint64_t i = 0; i < m_dataSize; ++i) {
            যদি (hits(i) && remaining.hits(i)) ++বেৰ_কৰো;
        }
        ঘূৰাই বেৰ_কৰো;
    }
    শূন্য orData(ধ্রুৱ VlcBuckets& ordata) {
        বাবে (uint64_t i = 0; i < m_dataSize; ++i) {
            যদি (hits(i) && ordata.hits(i)) clearHits(i);
        }
    }

    শূন্য dump() ধ্রুৱ {
        std::cout << "#     ";
        বাবে (uint64_t i = 0; i < m_dataSize; ++i) {
            যদি (hits(i)) std::cout << "," << i;
        }
        std::cout << "\n";
    }
};

//######################################################################

#endif  // guard
