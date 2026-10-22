// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: verilator_coverage: Test/coverage file container
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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_VLCTEST_H_
#وصف_ڪريو VERILATOR_VLCTEST_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "VlcBucket.h"
#ضمن "VlcPoint.h"

#ضمن <map>
#ضمن <vector>

//********************************************************************
// VlcTest - a single testrun i.e. a file containing coverage data

class VlcTest final {
    // MEMBERS
    string m_name;  //< Name of the test
    مزدوج m_computrons;  //< Runtime for the test
    uint64_t m_testrun;  //< Test run number, for database use
    uint64_t m_rank = 0;  //< Execution rank suggestion
    uint64_t m_rankPoints = 0;  //< Ranked additional points
    uint64_t m_user = 0;  //< User data for algorithms (not persisted in .dat file)
    VlcBuckets m_buckets;  //< Coverage data for each coverage point

عام:
    // CONSTRUCTORS
    VlcTest(ثابت string& name, uint64_t testrun, مزدوج comp)
        : m_name{name}
        , m_computrons{comp}
        , m_testrun{testrun} {}
    ~VlcTest() = افتراضي;

    // ACCESSORS
    ثابت string& name() ثابت { أرجع m_name; }
    مزدوج computrons() ثابت { أرجع m_computrons; }
    uint64_t testrun() ثابت { أرجع m_testrun; }
    VlcBuckets& buckets() { أرجع m_buckets; }
    uint64_t bucketsCovered() ثابت { أرجع m_buckets.bucketsCovered(); }
    uint64_t rank() ثابت { أرجع m_rank; }
    فراغ rank(uint64_t flag) { m_rank = flag; }
    uint64_t rankPoints() ثابت { أرجع m_rankPoints; }
    فراغ rankPoints(uint64_t flag) { m_rankPoints = flag; }
    uint64_t user() ثابت { أرجع m_user; }
    فراغ user(uint64_t flag) { m_user = flag; }

    // METHODS
    ساكن فراغ dump_سرو() {
        std::cout << "Tests:\n";
        // std::cout<<"  Testrun, Computrons,";  // Currently not loaded
        std::cout << "  Covered,     Rank,  RankPts,  Filename\n";
    }
    فراغ dump(bool bucketsToo) ثابت {
        إذا (testrun() || computrons() != 0.0) {  // currently unused // LCOV_EXCL_LINE
            std::cout << "  " << std::setw(8) << std::setfill('0') << testrun()  // LCOV_EXCL_LINE
                      << ",  " << std::setw(7) << std::setfill(' ')
                      << computrons()  // LCOV_EXCL_LINE
                      << ",";  // LCOV_EXCL_LINE
        }
        std::cout << "  " << std::setw(7) << std::setfill(' ') << bucketsCovered();
        std::cout << ",  " << std::setw(7) << std::setfill(' ') << rank();
        std::cout << ",  " << std::setw(7) << std::setfill(' ') << rankPoints();
        std::cout << ",  \"" << name() << "\"\n";
        إذا (bucketsToo) m_buckets.dump();
    }
};

//********************************************************************
// VlcTests - Container of all tests

class VlcTests final {
عام:
    // TYPES
    using ByName = std::ويڪٽر<VlcTest*>;

خاص:
    // MEMBERS
    ByName m_tests;  //< List of all tests

    ساكن صحيح debug() { أرجع V3Error::debugDefault(); }

عام:
    // ITERATORS
    using iterator = ByName::iterator;
    ByName::iterator begin() { أرجع m_tests.begin(); }
    ByName::iterator پڇاڙي() { أرجع m_tests.پڇاڙي(); }

    // CONSTRUCTORS
    VlcTests() = افتراضي;
    ~VlcTests() {
        لكل (خودڪار it = begin(); it != پڇاڙي(); ++it) VL_DO_CLEAR(delete *it, *it = nullptr);
    }

    // METHODS
    فراغ dump(bool bucketsToo) {
        UINFO(2, "dumpTests...");
        VlcTest::dump_سرو();
        لكل (ثابت VlcTest* ثابت testp : m_tests) testp->dump(bucketsToo);
    }
    VlcTest* newTest(ثابت string& name, uint64_t testrun, مزدوج comp) {
        VlcTest* ثابت testp = new VlcTest{name, testrun, comp};
        m_tests.ڌڪو_پٺ(testp);
        أرجع testp;
    }
    فراغ clearUser() {
        لكل (ثابت خودڪار& testp : m_tests) testp->user(0);
    }
};

//######################################################################

#جيڪڏهن_پورو  // Guard
