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

#اگر_غیر_تعریف_شدہ VERILATOR_VLCBUCKET_H_
#تعریف_کرو VERILATOR_VLCBUCKET_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#اگر_غیر_تعریف_شدہ V3ERROR_NO_GLOBAL_
#تعریف_کرو V3ERROR_NO_GLOBAL_
#اگر_ختم
#شامل "V3Error.h"

//********************************************************************
// VlcBuckets - Container of all coverage point hits for a given test
// This is a bitmap array - we store a single bit to indicate a test
// has hit that point with sufficient coverage.

class VlcBuckets final {
    // MEMBERS
    uint64_t* m_datap = nullptr;  ///< Pointer to first bucket (dynamically allocated)
    uint64_t m_ڈیٹا_سائز = 0;  ///< Current entries in m_datap
    uint64_t m_bucketsCovered = 0;  ///< Num buckets with sufficient coverage

    ساکن uint64_t cov_ثنائی_ہندسہ(uint64_t نقطہ) { واپس 1ULL << (نقطہ & 63); }
    uint64_t alloc_سائز() ثابت { واپس حجم(uint64_t) * m_ڈیٹا_سائز / 64; }
    خالی allocate(uint64_t نقطہ) {
        ثابت uint64_t oldsize = m_ڈیٹا_سائز;
        اگر (m_ڈیٹا_سائز < نقطہ) m_ڈیٹا_سائز = (نقطہ + 64) & ~63ULL;  // Keep power of two
        m_ڈیٹا_سائز *= 2;
        // UINFO(9, "Realloc " << allocSize() << " for " << point << "  " << cvtToHex(m_datap));
        uint64_t* ثابت newp = static_cast<uint64_t*>(std::realloc(m_datap, alloc_سائز()));
        اگر (VL_UNCOVERABLE(!newp)) {
            // cppcheck-suppress doubleFree  // cppcheck 1.90 bug - realloc doesn't free on fail
            آزاد(m_datap);  // LCOV_EXCL_LINE
            v3fatal("Out of memory increasing buckets");  // LCOV_EXCL_LINE
        }
        m_datap = newp;
        برائے (uint64_t i = oldsize; i < m_ڈیٹا_سائز; i += 64) m_datap[i / 64] = 0;
    }

عوامی:
    // CONSTRUCTORS
    VlcBuckets() { allocate(1024); }
    ~VlcBuckets() {
        m_ڈیٹا_سائز = 0;
        VL_DO_CLEAR(آزاد(m_datap), m_datap = nullptr);
    }

    // ACCESSORS
    ساکن uint64_t sufficient() { واپس 1; }
    uint64_t bucketsCovered() ثابت { واپس m_bucketsCovered; }

    // METHODS
    خالی جوڑو_ڈیٹا(uint64_t نقطہ, uint64_t کامیابیاں) {
        اگر (کامیابیاں >= sufficient()) {
            // UINFO(9, "     addData " << point << " " << hits << " size=" << m_dataSize);
            اگر (نقطہ >= m_ڈیٹا_سائز) allocate(نقطہ);
            m_datap[نقطہ / 64] |= cov_ثنائی_ہندسہ(نقطہ);
            m_bucketsCovered++;
        }
    }
    خالی clear_کامیابیاں(uint64_t نقطہ) ثابت {
        اگر (نقطہ >= m_ڈیٹا_سائز) واپس;
        m_datap[نقطہ / 64] &= ~cov_ثنائی_ہندسہ(نقطہ);
    }
    bool exists(uint64_t نقطہ) ثابت {
        اگر (نقطہ >= m_ڈیٹا_سائز) واپس false;
        واپس (m_datap[نقطہ / 64] & cov_ثنائی_ہندسہ(نقطہ)) ? true : false;
    }
    uint64_t کامیابیاں(uint64_t نقطہ) ثابت {
        اگر (نقطہ >= m_ڈیٹا_سائز) واپس 0;
        واپس (m_datap[نقطہ / 64] & cov_ثنائی_ہندسہ(نقطہ)) ? 1 : 0;
    }
    uint64_t نکالو_گنتی() ثابت {
        uint64_t نکالو = 0;
        برائے (uint64_t i = 0; i < m_ڈیٹا_سائز; ++i) {
            اگر (کامیابیاں(i)) ++نکالو;
        }
        واپس نکالو;
    }
    uint64_t ڈیٹا_نکالو_گنتی(ثابت VlcBuckets& بقایا) ثابت {
        uint64_t نکالو = 0;
        برائے (uint64_t i = 0; i < m_ڈیٹا_سائز; ++i) {
            اگر (کامیابیاں(i) && بقایا.کامیابیاں(i)) ++نکالو;
        }
        واپس نکالو;
    }
    خالی or_ڈیٹا(ثابت VlcBuckets& ordata) {
        برائے (uint64_t i = 0; i < m_ڈیٹا_سائز; ++i) {
            اگر (کامیابیاں(i) && ordata.کامیابیاں(i)) clear_کامیابیاں(i);
        }
    }

    خالی dump() ثابت {
        std::cout << "#     ";
        برائے (uint64_t i = 0; i < m_ڈیٹا_سائز; ++i) {
            اگر (کامیابیاں(i)) std::cout << "," << i;
        }
        std::cout << "\n";
    }
};

//######################################################################

#اگر_ختم  // guard
