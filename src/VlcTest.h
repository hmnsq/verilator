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

সার্বজনিক:
    // CONSTRUCTORS
    VlcTest(ধ্রুব string& name, uint64_t testrun, দ্বিগুণ comp)
        : m_name{name}
        , m_computrons{comp}
        , m_testrun{testrun} {}
    ~VlcTest() = স্বভাব;

    // ACCESSORS
    ধ্রুব string& name() ধ্রুব { প্রত্যাগম m_name; }
    দ্বিগুণ computrons() ধ্রুব { প্রত্যাগম m_computrons; }
    uint64_t testrun() ধ্রুব { প্রত্যাগম m_testrun; }
    VlcBuckets& buckets() { প্রত্যাগম m_buckets; }
    uint64_t bucketsCovered() ধ্রুব { প্রত্যাগম m_buckets.bucketsCovered(); }
    uint64_t rank() ধ্রুব { প্রত্যাগম m_rank; }
    শূন্য rank(uint64_t flag) { m_rank = flag; }
    uint64_t rankPoints() ধ্রুব { প্রত্যাগম m_rankPoints; }
    শূন্য rankPoints(uint64_t flag) { m_rankPoints = flag; }
    uint64_t user() ধ্রুব { প্রত্যাগম m_user; }
    শূন্য user(uint64_t flag) { m_user = flag; }

    // METHODS
    স্থির শূন্য dumpHeader() {
        std::cout << "Tests:\n";
        // std::cout<<"  Testrun, Computrons,";  // Currently not loaded
        std::cout << "  Covered,     Rank,  RankPts,  Filename\n";
    }
    শূন্য dump(bool bucketsToo) ধ্রুব {
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
সার্বজনিক:
    // TYPES
    using ByName = std::vector<VlcTest*>;

গুপ্ত:
    // MEMBERS
    ByName m_tests;  //< List of all tests

    স্থির পূর্ণাঙ্ক debug() { প্রত্যাগম V3Error::debugDefault(); }

সার্বজনিক:
    // ITERATORS
    using iterator = ByName::iterator;
    ByName::iterator begin() { প্রত্যাগম m_tests.begin(); }
    ByName::iterator end() { প্রত্যাগম m_tests.end(); }

    // CONSTRUCTORS
    VlcTests() = স্বভাব;
    ~VlcTests() {
        প্রতি (auto it = begin(); it != end(); ++it) VL_DO_CLEAR(delete *it, *it = nullptr);
    }

    // METHODS
    শূন্য dump(bool bucketsToo) {
        UINFO(2, "dumpTests...");
        VlcTest::dumpHeader();
        প্রতি (ধ্রুব VlcTest* ধ্রুব testp : m_tests) testp->dump(bucketsToo);
    }
    VlcTest* newTest(ধ্রুব string& name, uint64_t testrun, দ্বিগুণ comp) {
        VlcTest* ধ্রুব testp = new VlcTest{name, testrun, comp};
        m_tests.ঠেলো_back(testp);
        প্রত্যাগম testp;
    }
    শূন্য clearUser() {
        প্রতি (ধ্রুব auto& testp : m_tests) testp->user(0);
    }
};

//######################################################################

#endif  // Guard
