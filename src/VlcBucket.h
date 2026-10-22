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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_VLCBUCKET_H_
#وصف_ڪريو VERILATOR_VLCBUCKET_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#جيڪڏهن_وصف_نه_ٿيل V3ERROR_NO_GLOBAL_
#وصف_ڪريو V3ERROR_NO_GLOBAL_
#جيڪڏهن_پورو
#ضمن "V3Error.h"

//********************************************************************
// VlcBuckets - Container of all coverage point hits for a given test
// This is a bitmap array - we store a single bit to indicate a test
// has hit that point with sufficient coverage.

class VlcBuckets final {
    // MEMBERS
    uint64_t* m_datap = nullptr;  ///< Pointer to first bucket (dynamically allocated)
    uint64_t m_معلومات_سائيز = 0;  ///< Current entries in m_datap
    uint64_t m_bucketsCovered = 0;  ///< Num buckets with sufficient coverage

    ساكن uint64_t cov_ٻٽو_انگ(uint64_t نقطو) { أرجع 1ULL << (نقطو & 63); }
    uint64_t alloc_سائيز() ثابت { أرجع حجم(uint64_t) * m_معلومات_سائيز / 64; }
    فراغ allocate(uint64_t نقطو) {
        ثابت uint64_t oldsize = m_معلومات_سائيز;
        إذا (m_معلومات_سائيز < نقطو) m_معلومات_سائيز = (نقطو + 64) & ~63ULL;  // Keep power of two
        m_معلومات_سائيز *= 2;
        // UINFO(9, "Realloc " << allocSize() << " for " << point << "  " << cvtToHex(m_datap));
        uint64_t* ثابت newp = static_cast<uint64_t*>(std::realloc(m_datap, alloc_سائيز()));
        إذا (VL_UNCOVERABLE(!newp)) {
            // cppcheck-suppress doubleFree  // cppcheck 1.90 bug - realloc doesn't free on fail
            حرر(m_datap);  // LCOV_EXCL_LINE
            v3fatal("Out of memory increasing buckets");  // LCOV_EXCL_LINE
        }
        m_datap = newp;
        لكل (uint64_t i = oldsize; i < m_معلومات_سائيز; i += 64) m_datap[i / 64] = 0;
    }

عام:
    // CONSTRUCTORS
    VlcBuckets() { allocate(1024); }
    ~VlcBuckets() {
        m_معلومات_سائيز = 0;
        VL_DO_CLEAR(حرر(m_datap), m_datap = nullptr);
    }

    // ACCESSORS
    ساكن uint64_t sufficient() { أرجع 1; }
    uint64_t bucketsCovered() ثابت { أرجع m_bucketsCovered; }

    // METHODS
    فراغ جوڙيو_معلومات(uint64_t نقطو, uint64_t ڪاميابيون) {
        إذا (ڪاميابيون >= sufficient()) {
            // UINFO(9, "     addData " << point << " " << hits << " size=" << m_dataSize);
            إذا (نقطو >= m_معلومات_سائيز) allocate(نقطو);
            m_datap[نقطو / 64] |= cov_ٻٽو_انگ(نقطو);
            m_bucketsCovered++;
        }
    }
    فراغ clear_ڪاميابيون(uint64_t نقطو) ثابت {
        إذا (نقطو >= m_معلومات_سائيز) أرجع;
        m_datap[نقطو / 64] &= ~cov_ٻٽو_انگ(نقطو);
    }
    bool exists(uint64_t نقطو) ثابت {
        إذا (نقطو >= m_معلومات_سائيز) أرجع false;
        أرجع (m_datap[نقطو / 64] & cov_ٻٽو_انگ(نقطو)) ? true : false;
    }
    uint64_t ڪاميابيون(uint64_t نقطو) ثابت {
        إذا (نقطو >= m_معلومات_سائيز) أرجع 0;
        أرجع (m_datap[نقطو / 64] & cov_ٻٽو_انگ(نقطو)) ? 1 : 0;
    }
    uint64_t ڪڍو_ڳڻپ() ثابت {
        uint64_t ڪڍو = 0;
        لكل (uint64_t i = 0; i < m_معلومات_سائيز; ++i) {
            إذا (ڪاميابيون(i)) ++ڪڍو;
        }
        أرجع ڪڍو;
    }
    uint64_t معلومات_ڪڍو_ڳڻپ(ثابت VlcBuckets& بچيل) ثابت {
        uint64_t ڪڍو = 0;
        لكل (uint64_t i = 0; i < m_معلومات_سائيز; ++i) {
            إذا (ڪاميابيون(i) && بچيل.ڪاميابيون(i)) ++ڪڍو;
        }
        أرجع ڪڍو;
    }
    فراغ or_معلومات(ثابت VlcBuckets& ordata) {
        لكل (uint64_t i = 0; i < m_معلومات_سائيز; ++i) {
            إذا (ڪاميابيون(i) && ordata.ڪاميابيون(i)) clear_ڪاميابيون(i);
        }
    }

    فراغ dump() ثابت {
        std::cout << "#     ";
        لكل (uint64_t i = 0; i < m_معلومات_سائيز; ++i) {
            إذا (ڪاميابيون(i)) std::cout << "," << i;
        }
        std::cout << "\n";
    }
};

//######################################################################

#جيڪڏهن_پورو  // guard
