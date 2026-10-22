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

#ifndef VERILATOR_VLCTEST_H_
#define VERILATOR_VLCTEST_H_

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
    دوگانه m_computrons;  //< Runtime for the test
    uint64_t m_testrun;  //< Test run number, for database use
    uint64_t m_rank = 0;  //< Execution rank suggestion
    uint64_t m_rankPoints = 0;  //< Ranked additional points
    uint64_t m_user = 0;  //< User data for algorithms (not persisted in .dat file)
    VlcBuckets m_buckets;  //< Coverage data for each coverage point

عمومی:
    // CONSTRUCTORS
    VlcTest(ثابت string& name, uint64_t testrun, دوگانه comp)
        : m_name{name}
        , m_computrons{comp}
        , m_testrun{testrun} {}
    ~VlcTest() = پیشفرض;

    // ACCESSORS
    ثابت string& name() ثابت { بازگشت m_name; }
    دوگانه computrons() ثابت { بازگشت m_computrons; }
    uint64_t testrun() ثابت { بازگشت m_testrun; }
    VlcBuckets& buckets() { بازگشت m_buckets; }
    uint64_t bucketsCovered() ثابت { بازگشت m_buckets.bucketsCovered(); }
    uint64_t rank() ثابت { بازگشت m_rank; }
    تهی rank(uint64_t flag) { m_rank = flag; }
    uint64_t rankPoints() ثابت { بازگشت m_rankPoints; }
    تهی rankPoints(uint64_t flag) { m_rankPoints = flag; }
    uint64_t user() ثابت { بازگشت m_user; }
    تهی user(uint64_t flag) { m_user = flag; }

    // METHODS
    ایستا تهی dumpHeader() {
        std::cout << "Tests:\n";
        // std::cout<<"  Testrun, Computrons,";  // Currently not loaded
        std::cout << "  Covered,     Rank,  RankPts,  Filename\n";
    }
    تهی dump(bool bucketsToo) ثابت {
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
عمومی:
    // TYPES
    using ByName = std::vector<VlcTest*>;

خصوصی:
    // MEMBERS
    ByName m_tests;  //< List of all tests

    ایستا صحیح debug() { بازگشت V3Error::debugDefault(); }

عمومی:
    // ITERATORS
    using iterator = ByName::iterator;
    ByName::iterator begin() { بازگشت m_tests.begin(); }
    ByName::iterator end() { بازگشت m_tests.end(); }

    // CONSTRUCTORS
    VlcTests() = پیشفرض;
    ~VlcTests() {
        برای (auto it = begin(); it != end(); ++it) VL_DO_CLEAR(delete *it, *it = nullptr);
    }

    // METHODS
    تهی dump(bool bucketsToo) {
        UINFO(2, "dumpTests...");
        VlcTest::dumpHeader();
        برای (ثابت VlcTest* ثابت testp : m_tests) testp->dump(bucketsToo);
    }
    VlcTest* newTest(ثابت string& name, uint64_t testrun, دوگانه comp) {
        VlcTest* ثابت testp = new VlcTest{name, testrun, comp};
        m_tests.افزودن_back(testp);
        بازگشت testp;
    }
    تهی clearUser() {
        برای (ثابت auto& testp : m_tests) testp->user(0);
    }
};

//######################################################################

#endif  // Guard
