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

#جے_بنا_تعریف VERILATOR_VLCBUCKET_H_
#تعریف_کرو VERILATOR_VLCBUCKET_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#جے_بنا_تعریف V3ERROR_NO_GLOBAL_
#تعریف_کرو V3ERROR_NO_GLOBAL_
#جے_انت
#ضمن "V3Error.h"

//********************************************************************
// VlcBuckets - Container of all coverage point hits for a given test
// This is a bitmap array - we store a single bit to indicate a test
// has hit that point with sufficient coverage.

class VlcBuckets final {
    // MEMBERS
    uint64_t* m_datap = nullptr;  ///< Pointer to first bucket (dynamically allocated)
    uint64_t m_انکڑے_ناپ = 0;  ///< Current entries in m_datap
    uint64_t m_bucketsCovered = 0;  ///< Num buckets with sufficient coverage

    ساكن uint64_t cov_ثنائی_ہندسہ(uint64_t نقطہ) { أرجع 1ULL << (نقطہ & 63); }
    uint64_t alloc_ناپ() ثابت { أرجع حجم(uint64_t) * m_انکڑے_ناپ / 64; }
    فراغ allocate(uint64_t نقطہ) {
        ثابت uint64_t oldsize = m_انکڑے_ناپ;
        إذا (m_انکڑے_ناپ < نقطہ) m_انکڑے_ناپ = (نقطہ + 64) & ~63ULL;  // Keep power of two
        m_انکڑے_ناپ *= 2;
        // UINFO(9, "Realloc " << allocSize() << " for " << point << "  " << cvtToHex(m_datap));
        uint64_t* ثابت newp = static_cast<uint64_t*>(std::realloc(m_datap, alloc_ناپ()));
        إذا (VL_UNCOVERABLE(!newp)) {
            // cppcheck-suppress doubleFree  // cppcheck 1.90 bug - realloc doesn't free on fail
            حرر(m_datap);  // LCOV_EXCL_LINE
            v3fatal("Out of memory increasing buckets");  // LCOV_EXCL_LINE
        }
        m_datap = newp;
        لكل (uint64_t i = oldsize; i < m_انکڑے_ناپ; i += 64) m_datap[i / 64] = 0;
    }

عام:
    // CONSTRUCTORS
    VlcBuckets() { allocate(1024); }
    ~VlcBuckets() {
        m_انکڑے_ناپ = 0;
        VL_DO_CLEAR(حرر(m_datap), m_datap = nullptr);
    }

    // ACCESSORS
    ساكن uint64_t sufficient() { أرجع 1; }
    uint64_t bucketsCovered() ثابت { أرجع m_bucketsCovered; }

    // METHODS
    فراغ جوڑو_انکڑے(uint64_t نقطہ, uint64_t کامیابیاں) {
        إذا (کامیابیاں >= sufficient()) {
            // UINFO(9, "     addData " << point << " " << hits << " size=" << m_dataSize);
            إذا (نقطہ >= m_انکڑے_ناپ) allocate(نقطہ);
            m_datap[نقطہ / 64] |= cov_ثنائی_ہندسہ(نقطہ);
            m_bucketsCovered++;
        }
    }
    فراغ clear_کامیابیاں(uint64_t نقطہ) ثابت {
        إذا (نقطہ >= m_انکڑے_ناپ) أرجع;
        m_datap[نقطہ / 64] &= ~cov_ثنائی_ہندسہ(نقطہ);
    }
    bool exists(uint64_t نقطہ) ثابت {
        إذا (نقطہ >= m_انکڑے_ناپ) أرجع false;
        أرجع (m_datap[نقطہ / 64] & cov_ثنائی_ہندسہ(نقطہ)) ? true : false;
    }
    uint64_t کامیابیاں(uint64_t نقطہ) ثابت {
        إذا (نقطہ >= m_انکڑے_ناپ) أرجع 0;
        أرجع (m_datap[نقطہ / 64] & cov_ثنائی_ہندسہ(نقطہ)) ? 1 : 0;
    }
    uint64_t نکالو_گنتی() ثابت {
        uint64_t نکالو = 0;
        لكل (uint64_t i = 0; i < m_انکڑے_ناپ; ++i) {
            إذا (کامیابیاں(i)) ++نکالو;
        }
        أرجع نکالو;
    }
    uint64_t انکڑے_نکالو_گنتی(ثابت VlcBuckets& بچیا) ثابت {
        uint64_t نکالو = 0;
        لكل (uint64_t i = 0; i < m_انکڑے_ناپ; ++i) {
            إذا (کامیابیاں(i) && بچیا.کامیابیاں(i)) ++نکالو;
        }
        أرجع نکالو;
    }
    فراغ or_انکڑے(ثابت VlcBuckets& ordata) {
        لكل (uint64_t i = 0; i < m_انکڑے_ناپ; ++i) {
            إذا (کامیابیاں(i) && ordata.کامیابیاں(i)) clear_کامیابیاں(i);
        }
    }

    فراغ dump() ثابت {
        std::cout << "#     ";
        لكل (uint64_t i = 0; i < m_انکڑے_ناپ; ++i) {
            إذا (کامیابیاں(i)) std::cout << "," << i;
        }
        std::cout << "\n";
    }
};

//######################################################################

#جے_انت  // guard
