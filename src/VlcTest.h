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

#నిర్వచితం_కాకపోతే VERILATOR_VLCTEST_H_
#నిర్వచించు VERILATOR_VLCTEST_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "VlcBucket.h"
#అన్తర్భావ "VlcPoint.h"

#అన్తర్భావ <map>
#అన్తర్భావ <vector>

//********************************************************************
// VlcTest - a single testrun i.e. a file containing coverage data

class VlcTest final {
    // MEMBERS
    string m_name;  //< Name of the test
    ద్విగుణ m_computrons;  //< Runtime for the test
    uint64_t m_testrun;  //< Test run number, for database use
    uint64_t m_rank = 0;  //< Execution rank suggestion
    uint64_t m_rankPoints = 0;  //< Ranked additional points
    uint64_t m_user = 0;  //< User data for algorithms (not persisted in .dat file)
    VlcBuckets m_buckets;  //< Coverage data for each coverage point

బహిరంగ:
    // CONSTRUCTORS
    VlcTest(స్థిరరాశి string& name, uint64_t testrun, ద్విగుణ comp)
        : m_name{name}
        , m_computrons{comp}
        , m_testrun{testrun} {}
    ~VlcTest() = అప్రమేయ;

    // ACCESSORS
    స్థిరరాశి string& name() స్థిరరాశి { తిరిగి m_name; }
    ద్విగుణ computrons() స్థిరరాశి { తిరిగి m_computrons; }
    uint64_t testrun() స్థిరరాశి { తిరిగి m_testrun; }
    VlcBuckets& buckets() { తిరిగి m_buckets; }
    uint64_t bucketsCovered() స్థిరరాశి { తిరిగి m_buckets.bucketsCovered(); }
    uint64_t rank() స్థిరరాశి { తిరిగి m_rank; }
    శూన్య rank(uint64_t flag) { m_rank = flag; }
    uint64_t rankPoints() స్థిరరాశి { తిరిగి m_rankPoints; }
    శూన్య rankPoints(uint64_t flag) { m_rankPoints = flag; }
    uint64_t user() స్థిరరాశి { తిరిగి m_user; }
    శూన్య user(uint64_t flag) { m_user = flag; }

    // METHODS
    స్థిర శూన్య dump_శీర్షిక() {
        std::cout << "Tests:\n";
        // std::cout<<"  Testrun, Computrons,";  // Currently not loaded
        std::cout << "  Covered,     Rank,  RankPts,  Filename\n";
    }
    శూన్య dump(bool bucketsToo) స్థిరరాశి {
        ఒకవేళ (testrun() || computrons() != 0.0) {  // currently unused // LCOV_EXCL_LINE
            std::cout << "  " << std::setw(8) << std::setfill('0') << testrun()  // LCOV_EXCL_LINE
                      << ",  " << std::setw(7) << std::setfill(' ')
                      << computrons()  // LCOV_EXCL_LINE
                      << ",";  // LCOV_EXCL_LINE
        }
        std::cout << "  " << std::setw(7) << std::setfill(' ') << bucketsCovered();
        std::cout << ",  " << std::setw(7) << std::setfill(' ') << rank();
        std::cout << ",  " << std::setw(7) << std::setfill(' ') << rankPoints();
        std::cout << ",  \"" << name() << "\"\n";
        ఒకవేళ (bucketsToo) m_buckets.dump();
    }
};

//********************************************************************
// VlcTests - Container of all tests

class VlcTests final {
బహిరంగ:
    // TYPES
    using ByName = std::సదిశ<VlcTest*>;

అంతరంగ:
    // MEMBERS
    ByName m_tests;  //< List of all tests

    స్థిర పూర్ణాంకం debug() { తిరిగి V3Error::debugDefault(); }

బహిరంగ:
    // ITERATORS
    using iterator = ByName::iterator;
    ByName::iterator begin() { తిరిగి m_tests.begin(); }
    ByName::iterator చివర() { తిరిగి m_tests.చివర(); }

    // CONSTRUCTORS
    VlcTests() = అప్రమేయ;
    ~VlcTests() {
        కొరకు (స్వయంచాలక it = begin(); it != చివర(); ++it) VL_DO_CLEAR(delete *it, *it = nullptr);
    }

    // METHODS
    శూన్య dump(bool bucketsToo) {
        UINFO(2, "dumpTests...");
        VlcTest::dump_శీర్షిక();
        కొరకు (స్థిరరాశి VlcTest* స్థిరరాశి testp : m_tests) testp->dump(bucketsToo);
    }
    VlcTest* newTest(స్థిరరాశి string& name, uint64_t testrun, ద్విగుణ comp) {
        VlcTest* స్థిరరాశి testp = new VlcTest{name, testrun, comp};
        m_tests.నెట్టు_వెనుక(testp);
        తిరిగి testp;
    }
    శూన్య clearUser() {
        కొరకు (స్థిరరాశి స్వయంచాలక& testp : m_tests) testp->user(0);
    }
};

//######################################################################

#అయితే_ముగింపు  // Guard
