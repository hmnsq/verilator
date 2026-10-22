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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "VlcBucket.h"
#அந்தர்பாவ "VlcPoint.h"

#அந்தர்பாவ <map>
#அந்தர்பாவ <vector>

//********************************************************************
// VlcTest - a single testrun i.e. a file containing coverage data

class VlcTest final {
    // MEMBERS
    string m_name;  //< Name of the test
    இரட்டை m_computrons;  //< Runtime for the test
    uint64_t m_testrun;  //< Test run number, for database use
    uint64_t m_rank = 0;  //< Execution rank suggestion
    uint64_t m_rankPoints = 0;  //< Ranked additional points
    uint64_t m_user = 0;  //< User data for algorithms (not persisted in .dat file)
    VlcBuckets m_buckets;  //< Coverage data for each coverage point

பொது:
    // CONSTRUCTORS
    VlcTest(மாறிலி string& name, uint64_t testrun, இரட்டை comp)
        : m_name{name}
        , m_computrons{comp}
        , m_testrun{testrun} {}
    ~VlcTest() = இயல்பு;

    // ACCESSORS
    மாறிலி string& name() மாறிலி { திரும்பு m_name; }
    இரட்டை computrons() மாறிலி { திரும்பு m_computrons; }
    uint64_t testrun() மாறிலி { திரும்பு m_testrun; }
    VlcBuckets& buckets() { திரும்பு m_buckets; }
    uint64_t bucketsCovered() மாறிலி { திரும்பு m_buckets.bucketsCovered(); }
    uint64_t rank() மாறிலி { திரும்பு m_rank; }
    வெறுமை rank(uint64_t flag) { m_rank = flag; }
    uint64_t rankPoints() மாறிலி { திரும்பு m_rankPoints; }
    வெறுமை rankPoints(uint64_t flag) { m_rankPoints = flag; }
    uint64_t user() மாறிலி { திரும்பு m_user; }
    வெறுமை user(uint64_t flag) { m_user = flag; }

    // METHODS
    நிலையான வெறுமை dumpHeader() {
        std::cout << "Tests:\n";
        // std::cout<<"  Testrun, Computrons,";  // Currently not loaded
        std::cout << "  Covered,     Rank,  RankPts,  Filename\n";
    }
    வெறுமை dump(bool bucketsToo) மாறிலி {
        எனில் (testrun() || computrons() != 0.0) {  // currently unused // LCOV_EXCL_LINE
            std::cout << "  " << std::setw(8) << std::setfill('0') << testrun()  // LCOV_EXCL_LINE
                      << ",  " << std::setw(7) << std::setfill(' ')
                      << computrons()  // LCOV_EXCL_LINE
                      << ",";  // LCOV_EXCL_LINE
        }
        std::cout << "  " << std::setw(7) << std::setfill(' ') << bucketsCovered();
        std::cout << ",  " << std::setw(7) << std::setfill(' ') << rank();
        std::cout << ",  " << std::setw(7) << std::setfill(' ') << rankPoints();
        std::cout << ",  \"" << name() << "\"\n";
        எனில் (bucketsToo) m_buckets.dump();
    }
};

//********************************************************************
// VlcTests - Container of all tests

class VlcTests final {
பொது:
    // TYPES
    using ByName = std::vector<VlcTest*>;

தனிப்பட்ட:
    // MEMBERS
    ByName m_tests;  //< List of all tests

    நிலையான முழு debug() { திரும்பு V3Error::debugDefault(); }

பொது:
    // ITERATORS
    using iterator = ByName::iterator;
    ByName::iterator begin() { திரும்பு m_tests.begin(); }
    ByName::iterator end() { திரும்பு m_tests.end(); }

    // CONSTRUCTORS
    VlcTests() = இயல்பு;
    ~VlcTests() {
        க்கு (auto it = begin(); it != end(); ++it) VL_DO_CLEAR(delete *it, *it = nullptr);
    }

    // METHODS
    வெறுமை dump(bool bucketsToo) {
        UINFO(2, "dumpTests...");
        VlcTest::dumpHeader();
        க்கு (மாறிலி VlcTest* மாறிலி testp : m_tests) testp->dump(bucketsToo);
    }
    VlcTest* newTest(மாறிலி string& name, uint64_t testrun, இரட்டை comp) {
        VlcTest* மாறிலி testp = new VlcTest{name, testrun, comp};
        m_tests.தள்ளு_back(testp);
        திரும்பு testp;
    }
    வெறுமை clearUser() {
        க்கு (மாறிலி auto& testp : m_tests) testp->user(0);
    }
};

//######################################################################

#endif  // Guard
