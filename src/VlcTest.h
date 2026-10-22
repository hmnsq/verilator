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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "VlcBucket.h"
#ଅନ୍ତର୍ଭାଵ "VlcPoint.h"

#ଅନ୍ତର୍ଭାଵ <map>
#ଅନ୍ତର୍ଭାଵ <vector>

//********************************************************************
// VlcTest - a single testrun i.e. a file containing coverage data

class VlcTest final {
    // MEMBERS
    string m_name;  //< Name of the test
    ଦ୍ୱିଗୁଣ m_computrons;  //< Runtime for the test
    uint64_t m_testrun;  //< Test run number, for database use
    uint64_t m_rank = 0;  //< Execution rank suggestion
    uint64_t m_rankPoints = 0;  //< Ranked additional points
    uint64_t m_user = 0;  //< User data for algorithms (not persisted in .dat file)
    VlcBuckets m_buckets;  //< Coverage data for each coverage point

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    VlcTest(ଅଚଳ string& name, uint64_t testrun, ଦ୍ୱିଗୁଣ comp)
        : m_name{name}
        , m_computrons{comp}
        , m_testrun{testrun} {}
    ~VlcTest() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // ACCESSORS
    ଅଚଳ string& name() ଅଚଳ { ଫେରନ୍ତୁ m_name; }
    ଦ୍ୱିଗୁଣ computrons() ଅଚଳ { ଫେରନ୍ତୁ m_computrons; }
    uint64_t testrun() ଅଚଳ { ଫେରନ୍ତୁ m_testrun; }
    VlcBuckets& buckets() { ଫେରନ୍ତୁ m_buckets; }
    uint64_t bucketsCovered() ଅଚଳ { ଫେରନ୍ତୁ m_buckets.bucketsCovered(); }
    uint64_t rank() ଅଚଳ { ଫେରନ୍ତୁ m_rank; }
    ଶୂନ୍ୟ rank(uint64_t flag) { m_rank = flag; }
    uint64_t rankPoints() ଅଚଳ { ଫେରନ୍ତୁ m_rankPoints; }
    ଶୂନ୍ୟ rankPoints(uint64_t flag) { m_rankPoints = flag; }
    uint64_t user() ଅଚଳ { ଫେରନ୍ତୁ m_user; }
    ଶୂନ୍ୟ user(uint64_t flag) { m_user = flag; }

    // METHODS
    ସ୍ଥିର ଶୂନ୍ୟ dumpHeader() {
        std::cout << "Tests:\n";
        // std::cout<<"  Testrun, Computrons,";  // Currently not loaded
        std::cout << "  Covered,     Rank,  RankPts,  Filename\n";
    }
    ଶୂନ୍ୟ dump(bool bucketsToo) ଅଚଳ {
        ଯଦି (testrun() || computrons() != 0.0) {  // currently unused // LCOV_EXCL_LINE
            std::cout << "  " << std::setw(8) << std::setfill('0') << testrun()  // LCOV_EXCL_LINE
                      << ",  " << std::setw(7) << std::setfill(' ')
                      << computrons()  // LCOV_EXCL_LINE
                      << ",";  // LCOV_EXCL_LINE
        }
        std::cout << "  " << std::setw(7) << std::setfill(' ') << bucketsCovered();
        std::cout << ",  " << std::setw(7) << std::setfill(' ') << rank();
        std::cout << ",  " << std::setw(7) << std::setfill(' ') << rankPoints();
        std::cout << ",  \"" << name() << "\"\n";
        ଯଦି (bucketsToo) m_buckets.dump();
    }
};

//********************************************************************
// VlcTests - Container of all tests

class VlcTests final {
ସର୍ବସାଧାରଣ:
    // TYPES
    using ByName = std::vector<VlcTest*>;

ବ୍ୟକ୍ତିଗତ:
    // MEMBERS
    ByName m_tests;  //< List of all tests

    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା debug() { ଫେରନ୍ତୁ V3Error::debugDefault(); }

ସର୍ବସାଧାରଣ:
    // ITERATORS
    using iterator = ByName::iterator;
    ByName::iterator begin() { ଫେରନ୍ତୁ m_tests.begin(); }
    ByName::iterator end() { ଫେରନ୍ତୁ m_tests.end(); }

    // CONSTRUCTORS
    VlcTests() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~VlcTests() {
        ପାଇଁ (auto it = begin(); it != end(); ++it) VL_DO_CLEAR(delete *it, *it = nullptr);
    }

    // METHODS
    ଶୂନ୍ୟ dump(bool bucketsToo) {
        UINFO(2, "dumpTests...");
        VlcTest::dumpHeader();
        ପାଇଁ (ଅଚଳ VlcTest* ଅଚଳ testp : m_tests) testp->dump(bucketsToo);
    }
    VlcTest* newTest(ଅଚଳ string& name, uint64_t testrun, ଦ୍ୱିଗୁଣ comp) {
        VlcTest* ଅଚଳ testp = new VlcTest{name, testrun, comp};
        m_tests.ଠେଲ_back(testp);
        ଫେରନ୍ତୁ testp;
    }
    ଶୂନ୍ୟ clearUser() {
        ପାଇଁ (ଅଚଳ auto& testp : m_tests) testp->user(0);
    }
};

//######################################################################

#endif  // Guard
