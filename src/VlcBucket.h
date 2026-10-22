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

#अगर_अघोषित VERILATOR_VLCBUCKET_H_
#घोषणा VERILATOR_VLCBUCKET_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#अगर_अघोषित V3ERROR_NO_GLOBAL_
#घोषणा V3ERROR_NO_GLOBAL_
#पूर्ण_अगर
#समावेश "V3Error.h"

//********************************************************************
// VlcBuckets - Container of all coverage point hits for a given test
// This is a bitmap array - we store a single bit to indicate a test
// has hit that point with sufficient coverage.

class VlcBuckets final {
    // MEMBERS
    uint64_t* m_datap = nullptr;  ///< Pointer to first bucket (dynamically allocated)
    uint64_t m_तथ्य_आकार = 0;  ///< Current entries in m_datap
    uint64_t m_bucketsCovered = 0;  ///< Num buckets with sufficient coverage

    अटल uint64_t covBit(uint64_t point) { वापस 1ULL << (point & 63); }
    uint64_t alloc_आकार() स्थिर { वापस माप(uint64_t) * m_तथ्य_आकार / 64; }
    व्योम allocate(uint64_t point) {
        स्थिर uint64_t oldsize = m_तथ्य_आकार;
        अगर (m_तथ्य_आकार < point) m_तथ्य_आकार = (point + 64) & ~63ULL;  // Keep power of two
        m_तथ्य_आकार *= 2;
        // UINFO(9, "Realloc " << allocSize() << " for " << point << "  " << cvtToHex(m_datap));
        uint64_t* स्थिर newp = static_cast<uint64_t*>(std::पुनः_स्मृति(m_datap, alloc_आकार()));
        अगर (VL_UNCOVERABLE(!newp)) {
            // cppcheck-suppress doubleFree  // cppcheck 1.90 bug - realloc doesn't free on fail
            मुक्त(m_datap);  // LCOV_EXCL_LINE
            v3fatal("Out of memory increasing buckets");  // LCOV_EXCL_LINE
        }
        m_datap = newp;
        क्रम (uint64_t i = oldsize; i < m_तथ्य_आकार; i += 64) m_datap[i / 64] = 0;
    }

खुला:
    // CONSTRUCTORS
    VlcBuckets() { allocate(1024); }
    ~VlcBuckets() {
        m_तथ्य_आकार = 0;
        VL_DO_CLEAR(मुक्त(m_datap), m_datap = nullptr);
    }

    // ACCESSORS
    अटल uint64_t sufficient() { वापस 1; }
    uint64_t bucketsCovered() स्थिर { वापस m_bucketsCovered; }

    // METHODS
    व्योम add_तथ्य(uint64_t point, uint64_t hits) {
        अगर (hits >= sufficient()) {
            // UINFO(9, "     addData " << point << " " << hits << " size=" << m_dataSize);
            अगर (point >= m_तथ्य_आकार) allocate(point);
            m_datap[point / 64] |= covBit(point);
            m_bucketsCovered++;
        }
    }
    व्योम clearHits(uint64_t point) स्थिर {
        अगर (point >= m_तथ्य_आकार) वापस;
        m_datap[point / 64] &= ~covBit(point);
    }
    bool exists(uint64_t point) स्थिर {
        अगर (point >= m_तथ्य_आकार) वापस false;
        वापस (m_datap[point / 64] & covBit(point)) ? true : false;
    }
    uint64_t hits(uint64_t point) स्थिर {
        अगर (point >= m_तथ्य_आकार) वापस 0;
        वापस (m_datap[point / 64] & covBit(point)) ? 1 : 0;
    }
    uint64_t निकालो_गिनती() स्थिर {
        uint64_t निकालो = 0;
        क्रम (uint64_t i = 0; i < m_तथ्य_आकार; ++i) {
            अगर (hits(i)) ++निकालो;
        }
        वापस निकालो;
    }
    uint64_t तथ्य_निकालो_गिनती(स्थिर VlcBuckets& remaining) स्थिर {
        uint64_t निकालो = 0;
        क्रम (uint64_t i = 0; i < m_तथ्य_आकार; ++i) {
            अगर (hits(i) && remaining.hits(i)) ++निकालो;
        }
        वापस निकालो;
    }
    व्योम or_तथ्य(स्थिर VlcBuckets& ordata) {
        क्रम (uint64_t i = 0; i < m_तथ्य_आकार; ++i) {
            अगर (hits(i) && ordata.hits(i)) clearHits(i);
        }
    }

    व्योम dump() स्थिर {
        std::cout << "#     ";
        क्रम (uint64_t i = 0; i < m_तथ्य_आकार; ++i) {
            अगर (hits(i)) std::cout << "," << i;
        }
        std::cout << "\n";
    }
};

//######################################################################

#पूर्ण_अगर  // guard
