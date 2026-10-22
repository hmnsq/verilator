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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_VLCBUCKET_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_VLCBUCKET_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ V3ERROR_NO_GLOBAL_
#ವ್ಯಾಖ್ಯಾನಿಸು V3ERROR_NO_GLOBAL_
#ಆದರೆ_ಅಂತ್ಯ
#ಅನ್ತರ್ಭಾವ "V3Error.h"

//********************************************************************
// VlcBuckets - Container of all coverage point hits for a given test
// This is a bitmap array - we store a single bit to indicate a test
// has hit that point with sufficient coverage.

class VlcBuckets final {
    // MEMBERS
    uint64_t* m_datap = nullptr;  ///< Pointer to first bucket (dynamically allocated)
    uint64_t m_ದತ್ತಾಂಶ_ಗಾತ್ರ = 0;  ///< Current entries in m_datap
    uint64_t m_bucketsCovered = 0;  ///< Num buckets with sufficient coverage

    ಸ್ಥಿರ uint64_t cov_ಬಿಟ್(uint64_t ಬಿಂದು) { ಹಿಂತಿರುಗಿ 1ULL << (ಬಿಂದು & 63); }
    uint64_t alloc_ಗಾತ್ರ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಗಾತ್ರ(uint64_t) * m_ದತ್ತಾಂಶ_ಗಾತ್ರ / 64; }
    ಶೂನ್ಯ allocate(uint64_t ಬಿಂದು) {
        ಸ್ಥಿರಾಂಕ uint64_t oldsize = m_ದತ್ತಾಂಶ_ಗಾತ್ರ;
        ಆದರೆ (m_ದತ್ತಾಂಶ_ಗಾತ್ರ < ಬಿಂದು) m_ದತ್ತಾಂಶ_ಗಾತ್ರ = (ಬಿಂದು + 64) & ~63ULL;  // Keep power of two
        m_ದತ್ತಾಂಶ_ಗಾತ್ರ *= 2;
        // UINFO(9, "Realloc " << allocSize() << " for " << point << "  " << cvtToHex(m_datap));
        uint64_t* ಸ್ಥಿರಾಂಕ newp = static_cast<uint64_t*>(std::realloc(m_datap, alloc_ಗಾತ್ರ()));
        ಆದರೆ (VL_UNCOVERABLE(!newp)) {
            // cppcheck-suppress doubleFree  // cppcheck 1.90 bug - realloc doesn't free on fail
            ಬಿಡುಗಡೆ(m_datap);  // LCOV_EXCL_LINE
            v3fatal("Out of memory increasing buckets");  // LCOV_EXCL_LINE
        }
        m_datap = newp;
        ಗಾಗಿ (uint64_t i = oldsize; i < m_ದತ್ತಾಂಶ_ಗಾತ್ರ; i += 64) m_datap[i / 64] = 0;
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    VlcBuckets() { allocate(1024); }
    ~VlcBuckets() {
        m_ದತ್ತಾಂಶ_ಗಾತ್ರ = 0;
        VL_DO_CLEAR(ಬಿಡುಗಡೆ(m_datap), m_datap = nullptr);
    }

    // ACCESSORS
    ಸ್ಥಿರ uint64_t sufficient() { ಹಿಂತಿರುಗಿ 1; }
    uint64_t bucketsCovered() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_bucketsCovered; }

    // METHODS
    ಶೂನ್ಯ ಕೂಡಿಸು_ದತ್ತಾಂಶ(uint64_t ಬಿಂದು, uint64_t ದೊರಕಿದವು) {
        ಆದರೆ (ದೊರಕಿದವು >= sufficient()) {
            // UINFO(9, "     addData " << point << " " << hits << " size=" << m_dataSize);
            ಆದರೆ (ಬಿಂದು >= m_ದತ್ತಾಂಶ_ಗಾತ್ರ) allocate(ಬಿಂದು);
            m_datap[ಬಿಂದು / 64] |= cov_ಬಿಟ್(ಬಿಂದು);
            m_bucketsCovered++;
        }
    }
    ಶೂನ್ಯ clear_ದೊರಕಿದವು(uint64_t ಬಿಂದು) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (ಬಿಂದು >= m_ದತ್ತಾಂಶ_ಗಾತ್ರ) ಹಿಂತಿರುಗಿ;
        m_datap[ಬಿಂದು / 64] &= ~cov_ಬಿಟ್(ಬಿಂದು);
    }
    bool exists(uint64_t ಬಿಂದು) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (ಬಿಂದು >= m_ದತ್ತಾಂಶ_ಗಾತ್ರ) ಹಿಂತಿರುಗಿ false;
        ಹಿಂತಿರುಗಿ (m_datap[ಬಿಂದು / 64] & cov_ಬಿಟ್(ಬಿಂದು)) ? true : false;
    }
    uint64_t ದೊರಕಿದವು(uint64_t ಬಿಂದು) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (ಬಿಂದು >= m_ದತ್ತಾಂಶ_ಗಾತ್ರ) ಹಿಂತಿರುಗಿ 0;
        ಹಿಂತಿರುಗಿ (m_datap[ಬಿಂದು / 64] & cov_ಬಿಟ್(ಬಿಂದು)) ? 1 : 0;
    }
    uint64_t ತೆಗೆ_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ {
        uint64_t ತೆಗೆ_2 = 0;
        ಗಾಗಿ (uint64_t i = 0; i < m_ದತ್ತಾಂಶ_ಗಾತ್ರ; ++i) {
            ಆದರೆ (ದೊರಕಿದವು(i)) ++ತೆಗೆ_2;
        }
        ಹಿಂತಿರುಗಿ ತೆಗೆ_2;
    }
    uint64_t ದತ್ತಾಂಶ_ತೆಗೆ_ಎಣಿಕೆ(ಸ್ಥಿರಾಂಕ VlcBuckets& ಉಳಿದ) ಸ್ಥಿರಾಂಕ {
        uint64_t ತೆಗೆ_2 = 0;
        ಗಾಗಿ (uint64_t i = 0; i < m_ದತ್ತಾಂಶ_ಗಾತ್ರ; ++i) {
            ಆದರೆ (ದೊರಕಿದವು(i) && ಉಳಿದ.ದೊರಕಿದವು(i)) ++ತೆಗೆ_2;
        }
        ಹಿಂತಿರುಗಿ ತೆಗೆ_2;
    }
    ಶೂನ್ಯ or_ದತ್ತಾಂಶ(ಸ್ಥಿರಾಂಕ VlcBuckets& ordata) {
        ಗಾಗಿ (uint64_t i = 0; i < m_ದತ್ತಾಂಶ_ಗಾತ್ರ; ++i) {
            ಆದರೆ (ದೊರಕಿದವು(i) && ordata.ದೊರಕಿದವು(i)) clear_ದೊರಕಿದವು(i);
        }
    }

    ಶೂನ್ಯ dump() ಸ್ಥಿರಾಂಕ {
        std::cout << "#     ";
        ಗಾಗಿ (uint64_t i = 0; i < m_ದತ್ತಾಂಶ_ಗಾತ್ರ; ++i) {
            ಆದರೆ (ದೊರಕಿದವು(i)) std::cout << "," << i;
        }
        std::cout << "\n";
    }
};

//######################################################################

#ಆದರೆ_ಅಂತ್ಯ  // guard
