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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ifndef V3ERROR_NO_GLOBAL_
#define V3ERROR_NO_GLOBAL_
#endif
#ଅନ୍ତର୍ଭାଵ "V3Error.h"

//********************************************************************
// VlcBuckets - Container of all coverage point hits for a given test
// This is a bitmap array - we store a single bit to indicate a test
// has hit that point with sufficient coverage.

class VlcBuckets final {
    // MEMBERS
    uint64_t* m_datap = nullptr;  ///< Pointer to first bucket (dynamically allocated)
    uint64_t m_dataSize = 0;  ///< Current entries in m_datap
    uint64_t m_bucketsCovered = 0;  ///< Num buckets with sufficient coverage

    ସ୍ଥିର uint64_t covBit(uint64_t point) { ଫେରନ୍ତୁ 1ULL << (point & 63); }
    uint64_t allocSize() ଅଚଳ { ଫେରନ୍ତୁ ଆକାର(uint64_t) * m_dataSize / 64; }
    ଶୂନ୍ୟ allocate(uint64_t point) {
        ଅଚଳ uint64_t oldsize = m_dataSize;
        ଯଦି (m_dataSize < point) m_dataSize = (point + 64) & ~63ULL;  // Keep power of two
        m_dataSize *= 2;
        // UINFO(9, "Realloc " << allocSize() << " for " << point << "  " << cvtToHex(m_datap));
        uint64_t* ଅଚଳ newp = static_cast<uint64_t*>(std::realloc(m_datap, allocSize()));
        ଯଦି (VL_UNCOVERABLE(!newp)) {
            // cppcheck-suppress doubleFree  // cppcheck 1.90 bug - realloc doesn't free on fail
            ମୁକ୍ତ(m_datap);  // LCOV_EXCL_LINE
            v3fatal("Out of memory increasing buckets");  // LCOV_EXCL_LINE
        }
        m_datap = newp;
        ପାଇଁ (uint64_t i = oldsize; i < m_dataSize; i += 64) m_datap[i / 64] = 0;
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    VlcBuckets() { allocate(1024); }
    ~VlcBuckets() {
        m_dataSize = 0;
        VL_DO_CLEAR(ମୁକ୍ତ(m_datap), m_datap = nullptr);
    }

    // ACCESSORS
    ସ୍ଥିର uint64_t sufficient() { ଫେରନ୍ତୁ 1; }
    uint64_t bucketsCovered() ଅଚଳ { ଫେରନ୍ତୁ m_bucketsCovered; }

    // METHODS
    ଶୂନ୍ୟ addData(uint64_t point, uint64_t hits) {
        ଯଦି (hits >= sufficient()) {
            // UINFO(9, "     addData " << point << " " << hits << " size=" << m_dataSize);
            ଯଦି (point >= m_dataSize) allocate(point);
            m_datap[point / 64] |= covBit(point);
            m_bucketsCovered++;
        }
    }
    ଶୂନ୍ୟ clearHits(uint64_t point) ଅଚଳ {
        ଯଦି (point >= m_dataSize) ଫେରନ୍ତୁ;
        m_datap[point / 64] &= ~covBit(point);
    }
    bool exists(uint64_t point) ଅଚଳ {
        ଯଦି (point >= m_dataSize) ଫେରନ୍ତୁ false;
        ଫେରନ୍ତୁ (m_datap[point / 64] & covBit(point)) ? true : false;
    }
    uint64_t hits(uint64_t point) ଅଚଳ {
        ଯଦି (point >= m_dataSize) ଫେରନ୍ତୁ 0;
        ଫେରନ୍ତୁ (m_datap[point / 64] & covBit(point)) ? 1 : 0;
    }
    uint64_t କାଢ଼_ଗଣନା() ଅଚଳ {
        uint64_t କାଢ଼ = 0;
        ପାଇଁ (uint64_t i = 0; i < m_dataSize; ++i) {
            ଯଦି (hits(i)) ++କାଢ଼;
        }
        ଫେରନ୍ତୁ କାଢ଼;
    }
    uint64_t data_କାଢ଼_ଗଣନା(ଅଚଳ VlcBuckets& remaining) ଅଚଳ {
        uint64_t କାଢ଼ = 0;
        ପାଇଁ (uint64_t i = 0; i < m_dataSize; ++i) {
            ଯଦି (hits(i) && remaining.hits(i)) ++କାଢ଼;
        }
        ଫେରନ୍ତୁ କାଢ଼;
    }
    ଶୂନ୍ୟ orData(ଅଚଳ VlcBuckets& ordata) {
        ପାଇଁ (uint64_t i = 0; i < m_dataSize; ++i) {
            ଯଦି (hits(i) && ordata.hits(i)) clearHits(i);
        }
    }

    ଶୂନ୍ୟ dump() ଅଚଳ {
        std::cout << "#     ";
        ପାଇଁ (uint64_t i = 0; i < m_dataSize; ++i) {
            ଯଦି (hits(i)) std::cout << "," << i;
        }
        std::cout << "\n";
    }
};

//######################################################################

#endif  // guard
