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

#اگر_تعریف_نشده VERILATOR_VLCBUCKET_H_
#تعریف_کن VERILATOR_VLCBUCKET_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#اگر_تعریف_نشده V3ERROR_NO_GLOBAL_
#تعریف_کن V3ERROR_NO_GLOBAL_
#پایان_اگر
#ضمن "V3Error.h"

//********************************************************************
// VlcBuckets - Container of all coverage point hits for a given test
// This is a bitmap array - we store a single bit to indicate a test
// has hit that point with sufficient coverage.

class VlcBuckets final {
    // MEMBERS
    uint64_t* m_datap = nullptr;  ///< Pointer to first bucket (dynamically allocated)
    uint64_t m_داده_اندازه = 0;  ///< Current entries in m_datap
    uint64_t m_bucketsCovered = 0;  ///< Num buckets with sufficient coverage

    ساكن uint64_t cov_بیت(uint64_t نقطه) { أرجع 1ULL << (نقطه & 63); }
    uint64_t alloc_اندازه() ثابت { أرجع حجم(uint64_t) * m_داده_اندازه / 64; }
    فراغ allocate(uint64_t نقطه) {
        ثابت uint64_t oldsize = m_داده_اندازه;
        إذا (m_داده_اندازه < نقطه) m_داده_اندازه = (نقطه + 64) & ~63ULL;  // Keep power of two
        m_داده_اندازه *= 2;
        // UINFO(9, "Realloc " << allocSize() << " for " << point << "  " << cvtToHex(m_datap));
        uint64_t* ثابت newp = static_cast<uint64_t*>(std::realloc(m_datap, alloc_اندازه()));
        إذا (VL_UNCOVERABLE(!newp)) {
            // cppcheck-suppress doubleFree  // cppcheck 1.90 bug - realloc doesn't free on fail
            حرر(m_datap);  // LCOV_EXCL_LINE
            v3fatal("Out of memory increasing buckets");  // LCOV_EXCL_LINE
        }
        m_datap = newp;
        لكل (uint64_t i = oldsize; i < m_داده_اندازه; i += 64) m_datap[i / 64] = 0;
    }

عام:
    // CONSTRUCTORS
    VlcBuckets() { allocate(1024); }
    ~VlcBuckets() {
        m_داده_اندازه = 0;
        VL_DO_CLEAR(حرر(m_datap), m_datap = nullptr);
    }

    // ACCESSORS
    ساكن uint64_t sufficient() { أرجع 1; }
    uint64_t bucketsCovered() ثابت { أرجع m_bucketsCovered; }

    // METHODS
    فراغ جمع_کن_داده(uint64_t نقطه, uint64_t اصابتها) {
        إذا (اصابتها >= sufficient()) {
            // UINFO(9, "     addData " << point << " " << hits << " size=" << m_dataSize);
            إذا (نقطه >= m_داده_اندازه) allocate(نقطه);
            m_datap[نقطه / 64] |= cov_بیت(نقطه);
            m_bucketsCovered++;
        }
    }
    فراغ clear_اصابتها(uint64_t نقطه) ثابت {
        إذا (نقطه >= m_داده_اندازه) أرجع;
        m_datap[نقطه / 64] &= ~cov_بیت(نقطه);
    }
    bool exists(uint64_t نقطه) ثابت {
        إذا (نقطه >= m_داده_اندازه) أرجع false;
        أرجع (m_datap[نقطه / 64] & cov_بیت(نقطه)) ? true : false;
    }
    uint64_t اصابتها(uint64_t نقطه) ثابت {
        إذا (نقطه >= m_داده_اندازه) أرجع 0;
        أرجع (m_datap[نقطه / 64] & cov_بیت(نقطه)) ? 1 : 0;
    }
    uint64_t برداشت_شمار() ثابت {
        uint64_t برداشت = 0;
        لكل (uint64_t i = 0; i < m_داده_اندازه; ++i) {
            إذا (اصابتها(i)) ++برداشت;
        }
        أرجع برداشت;
    }
    uint64_t داده_برداشت_شمار(ثابت VlcBuckets& مانده) ثابت {
        uint64_t برداشت = 0;
        لكل (uint64_t i = 0; i < m_داده_اندازه; ++i) {
            إذا (اصابتها(i) && مانده.اصابتها(i)) ++برداشت;
        }
        أرجع برداشت;
    }
    فراغ or_داده(ثابت VlcBuckets& ordata) {
        لكل (uint64_t i = 0; i < m_داده_اندازه; ++i) {
            إذا (اصابتها(i) && ordata.اصابتها(i)) clear_اصابتها(i);
        }
    }

    فراغ dump() ثابت {
        std::cout << "#     ";
        لكل (uint64_t i = 0; i < m_داده_اندازه; ++i) {
            إذا (اصابتها(i)) std::cout << "," << i;
        }
        std::cout << "\n";
    }
};

//######################################################################

#پایان_اگر  // guard
