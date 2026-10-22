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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "VlcBucket.h"
#অন্তর্ভাব "VlcPoint.h"

#অন্তর্ভাব <map>
#অন্তর্ভাব <vector>

//********************************************************************
// VlcTest - a single testrun i.e. a file containing coverage data

class VlcTest final {
    // MEMBERS
    string m_name;  //< Name of the test
    দ্বিগুণ m_computrons;  //< Runtime for the test
    uint64_t m_testrun;  //< Test run number, for database use
    uint64_t m_rank = 0;  //< Execution rank suggestion
    uint64_t m_rankPoints = 0;  //< Ranked additional points
    uint64_t m_user = 0;  //< User data for algorithms (not persisted in .dat file)
    VlcBuckets m_buckets;  //< Coverage data for each coverage point

সর্বজনীন:
    // CONSTRUCTORS
    VlcTest(ধ্রুবক string& name, uint64_t testrun, দ্বিগুণ comp)
        : m_name{name}
        , m_computrons{comp}
        , m_testrun{testrun} {}
    ~VlcTest() = পূর্বনির্ধারিত;

    // ACCESSORS
    ধ্রুবক string& name() ধ্রুবক { ফেরত m_name; }
    দ্বিগুণ computrons() ধ্রুবক { ফেরত m_computrons; }
    uint64_t testrun() ধ্রুবক { ফেরত m_testrun; }
    VlcBuckets& buckets() { ফেরত m_buckets; }
    uint64_t bucketsCovered() ধ্রুবক { ফেরত m_buckets.bucketsCovered(); }
    uint64_t rank() ধ্রুবক { ফেরত m_rank; }
    শূন্য rank(uint64_t flag) { m_rank = flag; }
    uint64_t rankPoints() ধ্রুবক { ফেরত m_rankPoints; }
    শূন্য rankPoints(uint64_t flag) { m_rankPoints = flag; }
    uint64_t user() ধ্রুবক { ফেরত m_user; }
    শূন্য user(uint64_t flag) { m_user = flag; }

    // METHODS
    স্থির শূন্য dumpHeader() {
        std::cout << "Tests:\n";
        // std::cout<<"  Testrun, Computrons,";  // Currently not loaded
        std::cout << "  Covered,     Rank,  RankPts,  Filename\n";
    }
    শূন্য dump(bool bucketsToo) ধ্রুবক {
        যদি (testrun() || computrons() != 0.0) {  // currently unused // LCOV_EXCL_LINE
            std::cout << "  " << std::setw(8) << std::setfill('0') << testrun()  // LCOV_EXCL_LINE
                      << ",  " << std::setw(7) << std::setfill(' ')
                      << computrons()  // LCOV_EXCL_LINE
                      << ",";  // LCOV_EXCL_LINE
        }
        std::cout << "  " << std::setw(7) << std::setfill(' ') << bucketsCovered();
        std::cout << ",  " << std::setw(7) << std::setfill(' ') << rank();
        std::cout << ",  " << std::setw(7) << std::setfill(' ') << rankPoints();
        std::cout << ",  \"" << name() << "\"\n";
        যদি (bucketsToo) m_buckets.dump();
    }
};

//********************************************************************
// VlcTests - Container of all tests

class VlcTests final {
সর্বজনীন:
    // TYPES
    using ByName = std::vector<VlcTest*>;

ব্যক্তিগত:
    // MEMBERS
    ByName m_tests;  //< List of all tests

    স্থির পূর্ণসংখ্যা debug() { ফেরত V3Error::debugDefault(); }

সর্বজনীন:
    // ITERATORS
    using iterator = ByName::iterator;
    ByName::iterator begin() { ফেরত m_tests.begin(); }
    ByName::iterator end() { ফেরত m_tests.end(); }

    // CONSTRUCTORS
    VlcTests() = পূর্বনির্ধারিত;
    ~VlcTests() {
        জন্য (auto it = begin(); it != end(); ++it) VL_DO_CLEAR(delete *it, *it = nullptr);
    }

    // METHODS
    শূন্য dump(bool bucketsToo) {
        UINFO(2, "dumpTests...");
        VlcTest::dumpHeader();
        জন্য (ধ্রুবক VlcTest* ধ্রুবক testp : m_tests) testp->dump(bucketsToo);
    }
    VlcTest* newTest(ধ্রুবক string& name, uint64_t testrun, দ্বিগুণ comp) {
        VlcTest* ধ্রুবক testp = new VlcTest{name, testrun, comp};
        m_tests.ঠেলো_back(testp);
        ফেরত testp;
    }
    শূন্য clearUser() {
        জন্য (ধ্রুবক auto& testp : m_tests) testp->user(0);
    }
};

//######################################################################

#endif  // Guard
