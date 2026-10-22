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

#که_نه_تعریف_شوی VERILATOR_VLCBUCKET_H_
#تعریف_کړه VERILATOR_VLCBUCKET_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#که_نه_تعریف_شوی V3ERROR_NO_GLOBAL_
#تعریف_کړه V3ERROR_NO_GLOBAL_
#د_که_پای
#ضمن "V3Error.h"

//********************************************************************
// VlcBuckets - Container of all coverage point hits for a given test
// This is a bitmap array - we store a single bit to indicate a test
// has hit that point with sufficient coverage.

class VlcBuckets final {
    // MEMBERS
    uint64_t* m_datap = nullptr;  ///< Pointer to first bucket (dynamically allocated)
    uint64_t m_معلومات_اندازه = 0;  ///< Current entries in m_datap
    uint64_t m_bucketsCovered = 0;  ///< Num buckets with sufficient coverage

    ساكن uint64_t cov_بټ(uint64_t ټکی) { أرجع 1ULL << (ټکی & 63); }
    uint64_t alloc_اندازه() ثابت { أرجع حجم(uint64_t) * m_معلومات_اندازه / 64; }
    فراغ allocate(uint64_t ټکی) {
        ثابت uint64_t oldsize = m_معلومات_اندازه;
        إذا (m_معلومات_اندازه < ټکی) m_معلومات_اندازه = (ټکی + 64) & ~63ULL;  // Keep power of two
        m_معلومات_اندازه *= 2;
        // UINFO(9, "Realloc " << allocSize() << " for " << point << "  " << cvtToHex(m_datap));
        uint64_t* ثابت newp = static_cast<uint64_t*>(std::realloc(m_datap, alloc_اندازه()));
        إذا (VL_UNCOVERABLE(!newp)) {
            // cppcheck-suppress doubleFree  // cppcheck 1.90 bug - realloc doesn't free on fail
            حرر(m_datap);  // LCOV_EXCL_LINE
            v3fatal("Out of memory increasing buckets");  // LCOV_EXCL_LINE
        }
        m_datap = newp;
        لكل (uint64_t i = oldsize; i < m_معلومات_اندازه; i += 64) m_datap[i / 64] = 0;
    }

عام:
    // CONSTRUCTORS
    VlcBuckets() { allocate(1024); }
    ~VlcBuckets() {
        m_معلومات_اندازه = 0;
        VL_DO_CLEAR(حرر(m_datap), m_datap = nullptr);
    }

    // ACCESSORS
    ساكن uint64_t sufficient() { أرجع 1; }
    uint64_t bucketsCovered() ثابت { أرجع m_bucketsCovered; }

    // METHODS
    فراغ جمع_کړه_معلومات(uint64_t ټکی, uint64_t موندنې) {
        إذا (موندنې >= sufficient()) {
            // UINFO(9, "     addData " << point << " " << hits << " size=" << m_dataSize);
            إذا (ټکی >= m_معلومات_اندازه) allocate(ټکی);
            m_datap[ټکی / 64] |= cov_بټ(ټکی);
            m_bucketsCovered++;
        }
    }
    فراغ clear_موندنې(uint64_t ټکی) ثابت {
        إذا (ټکی >= m_معلومات_اندازه) أرجع;
        m_datap[ټکی / 64] &= ~cov_بټ(ټکی);
    }
    bool exists(uint64_t ټکی) ثابت {
        إذا (ټکی >= m_معلومات_اندازه) أرجع false;
        أرجع (m_datap[ټکی / 64] & cov_بټ(ټکی)) ? true : false;
    }
    uint64_t موندنې(uint64_t ټکی) ثابت {
        إذا (ټکی >= m_معلومات_اندازه) أرجع 0;
        أرجع (m_datap[ټکی / 64] & cov_بټ(ټکی)) ? 1 : 0;
    }
    uint64_t راوباسه_شمېر() ثابت {
        uint64_t راوباسه = 0;
        لكل (uint64_t i = 0; i < m_معلومات_اندازه; ++i) {
            إذا (موندنې(i)) ++راوباسه;
        }
        أرجع راوباسه;
    }
    uint64_t معلومات_راوباسه_شمېر(ثابت VlcBuckets& پاتې_شوی) ثابت {
        uint64_t راوباسه = 0;
        لكل (uint64_t i = 0; i < m_معلومات_اندازه; ++i) {
            إذا (موندنې(i) && پاتې_شوی.موندنې(i)) ++راوباسه;
        }
        أرجع راوباسه;
    }
    فراغ or_معلومات(ثابت VlcBuckets& ordata) {
        لكل (uint64_t i = 0; i < m_معلومات_اندازه; ++i) {
            إذا (موندنې(i) && ordata.موندنې(i)) clear_موندنې(i);
        }
    }

    فراغ dump() ثابت {
        std::cout << "#     ";
        لكل (uint64_t i = 0; i < m_معلومات_اندازه; ++i) {
            إذا (موندنې(i)) std::cout << "," << i;
        }
        std::cout << "\n";
    }
};

//######################################################################

#د_که_پای  // guard
