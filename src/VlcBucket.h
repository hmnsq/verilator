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

#यदि_अपरिभाषितम् VERILATOR_VLCBUCKET_H_
#परिभाषय VERILATOR_VLCBUCKET_H_

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#यदि_अपरिभाषितम् V3ERROR_NO_GLOBAL_
#परिभाषय V3ERROR_NO_GLOBAL_
#यदि_अन्तः
#अन्तर्भाव "V3Error.h"

//********************************************************************
// VlcBuckets - Container of all coverage point hits for a given test
// This is a bitmap array - we store a single bit to indicate a test
// has hit that point with sufficient coverage.

class VlcBuckets final {
    // MEMBERS
    uint64_t* m_datap = nullptr;  ///< Pointer to first bucket (dynamically allocated)
    uint64_t m_दत्त_आकार = 0;  ///< Current entries in m_datap
    uint64_t m_bucketsCovered = 0;  ///< Num buckets with sufficient coverage

    स्थिर uint64_t covBit(uint64_t point) { प्रत्यागम 1ULL << (point & 63); }
    uint64_t alloc_आकार() ध्रुव { प्रत्यागम परिमाण(uint64_t) * m_दत्त_आकार / 64; }
    शून्य allocate(uint64_t point) {
        ध्रुव uint64_t oldsize = m_दत्त_आकार;
        यदि (m_दत्त_आकार < point) m_दत्त_आकार = (point + 64) & ~63ULL;  // Keep power of two
        m_दत्त_आकार *= 2;
        // UINFO(9, "Realloc " << allocSize() << " for " << point << "  " << cvtToHex(m_datap));
        uint64_t* ध्रुव newp = static_cast<uint64_t*>(std::realloc(m_datap, alloc_आकार()));
        यदि (VL_UNCOVERABLE(!newp)) {
            // cppcheck-suppress doubleFree  // cppcheck 1.90 bug - realloc doesn't free on fail
            मुक्त(m_datap);  // LCOV_EXCL_LINE
            v3fatal("Out of memory increasing buckets");  // LCOV_EXCL_LINE
        }
        m_datap = newp;
        प्रति (uint64_t i = oldsize; i < m_दत्त_आकार; i += 64) m_datap[i / 64] = 0;
    }

सार्वजनिक:
    // CONSTRUCTORS
    VlcBuckets() { allocate(1024); }
    ~VlcBuckets() {
        m_दत्त_आकार = 0;
        VL_DO_CLEAR(मुक्त(m_datap), m_datap = nullptr);
    }

    // ACCESSORS
    स्थिर uint64_t sufficient() { प्रत्यागम 1; }
    uint64_t bucketsCovered() ध्रुव { प्रत्यागम m_bucketsCovered; }

    // METHODS
    शून्य सङ्कलय_दत्त(uint64_t point, uint64_t लब्धानि) {
        यदि (लब्धानि >= sufficient()) {
            // UINFO(9, "     addData " << point << " " << hits << " size=" << m_dataSize);
            यदि (point >= m_दत्त_आकार) allocate(point);
            m_datap[point / 64] |= covBit(point);
            m_bucketsCovered++;
        }
    }
    शून्य clear_लब्धानि(uint64_t point) ध्रुव {
        यदि (point >= m_दत्त_आकार) प्रत्यागम;
        m_datap[point / 64] &= ~covBit(point);
    }
    bool exists(uint64_t point) ध्रुव {
        यदि (point >= m_दत्त_आकार) प्रत्यागम false;
        प्रत्यागम (m_datap[point / 64] & covBit(point)) ? true : false;
    }
    uint64_t लब्धानि(uint64_t point) ध्रुव {
        यदि (point >= m_दत्त_आकार) प्रत्यागम 0;
        प्रत्यागम (m_datap[point / 64] & covBit(point)) ? 1 : 0;
    }
    uint64_t निष्कासय_गणनम्() ध्रुव {
        uint64_t निष्कासय = 0;
        प्रति (uint64_t i = 0; i < m_दत्त_आकार; ++i) {
            यदि (लब्धानि(i)) ++निष्कासय;
        }
        प्रत्यागम निष्कासय;
    }
    uint64_t दत्त_निष्कासय_गणनम्(ध्रुव VlcBuckets& remaining) ध्रुव {
        uint64_t निष्कासय = 0;
        प्रति (uint64_t i = 0; i < m_दत्त_आकार; ++i) {
            यदि (लब्धानि(i) && remaining.लब्धानि(i)) ++निष्कासय;
        }
        प्रत्यागम निष्कासय;
    }
    शून्य or_दत्त(ध्रुव VlcBuckets& ordata) {
        प्रति (uint64_t i = 0; i < m_दत्त_आकार; ++i) {
            यदि (लब्धानि(i) && ordata.लब्धानि(i)) clear_लब्धानि(i);
        }
    }

    शून्य dump() ध्रुव {
        std::cout << "#     ";
        प्रति (uint64_t i = 0; i < m_दत्त_आकार; ++i) {
            यदि (लब्धानि(i)) std::cout << "," << i;
        }
        std::cout << "\n";
    }
};

//######################################################################

#यदि_अन्तः  // guard
