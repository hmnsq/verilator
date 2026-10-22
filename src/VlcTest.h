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

#اگر_غیر_تعریف_شدہ VERILATOR_VLCTEST_H_
#تعریف_کرو VERILATOR_VLCTEST_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "VlcBucket.h"
#شامل "VlcPoint.h"

#شامل <map>
#شامل <vector>

//********************************************************************
// VlcTest - a single testrun i.e. a file containing coverage data

class VlcTest final {
    // MEMBERS
    string m_name;  //< Name of the test
    دوہرا m_computrons;  //< Runtime for the test
    uint64_t m_testrun;  //< Test run number, for database use
    uint64_t m_rank = 0;  //< Execution rank suggestion
    uint64_t m_rankPoints = 0;  //< Ranked additional points
    uint64_t m_user = 0;  //< User data for algorithms (not persisted in .dat file)
    VlcBuckets m_buckets;  //< Coverage data for each coverage point

عوامی:
    // CONSTRUCTORS
    VlcTest(ثابت string& name, uint64_t testrun, دوہرا comp)
        : m_name{name}
        , m_computrons{comp}
        , m_testrun{testrun} {}
    ~VlcTest() = طےشدہ;

    // ACCESSORS
    ثابت string& name() ثابت { واپس m_name; }
    دوہرا computrons() ثابت { واپس m_computrons; }
    uint64_t testrun() ثابت { واپس m_testrun; }
    VlcBuckets& buckets() { واپس m_buckets; }
    uint64_t bucketsCovered() ثابت { واپس m_buckets.bucketsCovered(); }
    uint64_t rank() ثابت { واپس m_rank; }
    خالی rank(uint64_t flag) { m_rank = flag; }
    uint64_t rankPoints() ثابت { واپس m_rankPoints; }
    خالی rankPoints(uint64_t flag) { m_rankPoints = flag; }
    uint64_t user() ثابت { واپس m_user; }
    خالی user(uint64_t flag) { m_user = flag; }

    // METHODS
    ساکن خالی dump_سرخی() {
        std::cout << "Tests:\n";
        // std::cout<<"  Testrun, Computrons,";  // Currently not loaded
        std::cout << "  Covered,     Rank,  RankPts,  Filename\n";
    }
    خالی dump(bool bucketsToo) ثابت {
        اگر (testrun() || computrons() != 0.0) {  // currently unused // LCOV_EXCL_LINE
            std::cout << "  " << std::setw(8) << std::setfill('0') << testrun()  // LCOV_EXCL_LINE
                      << ",  " << std::setw(7) << std::setfill(' ')
                      << computrons()  // LCOV_EXCL_LINE
                      << ",";  // LCOV_EXCL_LINE
        }
        std::cout << "  " << std::setw(7) << std::setfill(' ') << bucketsCovered();
        std::cout << ",  " << std::setw(7) << std::setfill(' ') << rank();
        std::cout << ",  " << std::setw(7) << std::setfill(' ') << rankPoints();
        std::cout << ",  \"" << name() << "\"\n";
        اگر (bucketsToo) m_buckets.dump();
    }
};

//********************************************************************
// VlcTests - Container of all tests

class VlcTests final {
عوامی:
    // TYPES
    using ByName = std::سمتیہ<VlcTest*>;

نجی:
    // MEMBERS
    ByName m_tests;  //< List of all tests

    ساکن صحیح debug() { واپس V3Error::debugDefault(); }

عوامی:
    // ITERATORS
    using iterator = ByName::iterator;
    ByName::iterator begin() { واپس m_tests.begin(); }
    ByName::iterator انتہا() { واپس m_tests.انتہا(); }

    // CONSTRUCTORS
    VlcTests() = طےشدہ;
    ~VlcTests() {
        برائے (خودکار it = begin(); it != انتہا(); ++it) VL_DO_CLEAR(delete *it, *it = nullptr);
    }

    // METHODS
    خالی dump(bool bucketsToo) {
        UINFO(2, "dumpTests...");
        VlcTest::dump_سرخی();
        برائے (ثابت VlcTest* ثابت testp : m_tests) testp->dump(bucketsToo);
    }
    VlcTest* newTest(ثابت string& name, uint64_t testrun, دوہرا comp) {
        VlcTest* ثابت testp = new VlcTest{name, testrun, comp};
        m_tests.ڈالو_عقب(testp);
        واپس testp;
    }
    خالی clearUser() {
        برائے (ثابت خودکار& testp : m_tests) testp->user(0);
    }
};

//######################################################################

#اگر_ختم  // Guard
