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

#నిర్వచితం_కాకపోతే VERILATOR_VLCBUCKET_H_
#నిర్వచించు VERILATOR_VLCBUCKET_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#నిర్వచితం_కాకపోతే V3ERROR_NO_GLOBAL_
#నిర్వచించు V3ERROR_NO_GLOBAL_
#అయితే_ముగింపు
#అన్తర్భావ "V3Error.h"

//********************************************************************
// VlcBuckets - Container of all coverage point hits for a given test
// This is a bitmap array - we store a single bit to indicate a test
// has hit that point with sufficient coverage.

class VlcBuckets final {
    // MEMBERS
    uint64_t* m_datap = nullptr;  ///< Pointer to first bucket (dynamically allocated)
    uint64_t m_దత్తాంశం_కొలత = 0;  ///< Current entries in m_datap
    uint64_t m_bucketsCovered = 0;  ///< Num buckets with sufficient coverage

    స్థిర uint64_t cov_బిట్టు(uint64_t బిందువు) { తిరిగి 1ULL << (బిందువు & 63); }
    uint64_t alloc_కొలత() స్థిరరాశి { తిరిగి పరిమాణం(uint64_t) * m_దత్తాంశం_కొలత / 64; }
    శూన్య allocate(uint64_t బిందువు) {
        స్థిరరాశి uint64_t oldsize = m_దత్తాంశం_కొలత;
        ఒకవేళ (m_దత్తాంశం_కొలత < బిందువు) m_దత్తాంశం_కొలత = (బిందువు + 64) & ~63ULL;  // Keep power of two
        m_దత్తాంశం_కొలత *= 2;
        // UINFO(9, "Realloc " << allocSize() << " for " << point << "  " << cvtToHex(m_datap));
        uint64_t* స్థిరరాశి newp = static_cast<uint64_t*>(std::realloc(m_datap, alloc_కొలత()));
        ఒకవేళ (VL_UNCOVERABLE(!newp)) {
            // cppcheck-suppress doubleFree  // cppcheck 1.90 bug - realloc doesn't free on fail
            విడుదల(m_datap);  // LCOV_EXCL_LINE
            v3fatal("Out of memory increasing buckets");  // LCOV_EXCL_LINE
        }
        m_datap = newp;
        కొరకు (uint64_t i = oldsize; i < m_దత్తాంశం_కొలత; i += 64) m_datap[i / 64] = 0;
    }

బహిరంగ:
    // CONSTRUCTORS
    VlcBuckets() { allocate(1024); }
    ~VlcBuckets() {
        m_దత్తాంశం_కొలత = 0;
        VL_DO_CLEAR(విడుదల(m_datap), m_datap = nullptr);
    }

    // ACCESSORS
    స్థిర uint64_t sufficient() { తిరిగి 1; }
    uint64_t bucketsCovered() స్థిరరాశి { తిరిగి m_bucketsCovered; }

    // METHODS
    శూన్య కూడు_దత్తాంశం(uint64_t బిందువు, uint64_t తగిలినవి) {
        ఒకవేళ (తగిలినవి >= sufficient()) {
            // UINFO(9, "     addData " << point << " " << hits << " size=" << m_dataSize);
            ఒకవేళ (బిందువు >= m_దత్తాంశం_కొలత) allocate(బిందువు);
            m_datap[బిందువు / 64] |= cov_బిట్టు(బిందువు);
            m_bucketsCovered++;
        }
    }
    శూన్య clear_తగిలినవి(uint64_t బిందువు) స్థిరరాశి {
        ఒకవేళ (బిందువు >= m_దత్తాంశం_కొలత) తిరిగి;
        m_datap[బిందువు / 64] &= ~cov_బిట్టు(బిందువు);
    }
    bool exists(uint64_t బిందువు) స్థిరరాశి {
        ఒకవేళ (బిందువు >= m_దత్తాంశం_కొలత) తిరిగి false;
        తిరిగి (m_datap[బిందువు / 64] & cov_బిట్టు(బిందువు)) ? true : false;
    }
    uint64_t తగిలినవి(uint64_t బిందువు) స్థిరరాశి {
        ఒకవేళ (బిందువు >= m_దత్తాంశం_కొలత) తిరిగి 0;
        తిరిగి (m_datap[బిందువు / 64] & cov_బిట్టు(బిందువు)) ? 1 : 0;
    }
    uint64_t తీయి_లెక్క() స్థిరరాశి {
        uint64_t తీయి = 0;
        కొరకు (uint64_t i = 0; i < m_దత్తాంశం_కొలత; ++i) {
            ఒకవేళ (తగిలినవి(i)) ++తీయి;
        }
        తిరిగి తీయి;
    }
    uint64_t దత్తాంశం_తీయి_లెక్క(స్థిరరాశి VlcBuckets& మిగిలిన) స్థిరరాశి {
        uint64_t తీయి = 0;
        కొరకు (uint64_t i = 0; i < m_దత్తాంశం_కొలత; ++i) {
            ఒకవేళ (తగిలినవి(i) && మిగిలిన.తగిలినవి(i)) ++తీయి;
        }
        తిరిగి తీయి;
    }
    శూన్య or_దత్తాంశం(స్థిరరాశి VlcBuckets& ordata) {
        కొరకు (uint64_t i = 0; i < m_దత్తాంశం_కొలత; ++i) {
            ఒకవేళ (తగిలినవి(i) && ordata.తగిలినవి(i)) clear_తగిలినవి(i);
        }
    }

    శూన్య dump() స్థిరరాశి {
        std::cout << "#     ";
        కొరకు (uint64_t i = 0; i < m_దత్తాంశం_కొలత; ++i) {
            ఒకవేళ (తగిలినవి(i)) std::cout << "," << i;
        }
        std::cout << "\n";
    }
};

//######################################################################

#అయితే_ముగింపు  // guard
