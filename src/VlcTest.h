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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "VlcBucket.h"
#अन्तर्भाव "VlcPoint.h"

#अन्तर्भाव <map>
#अन्तर्भाव <vector>

//********************************************************************
// VlcTest - a single testrun i.e. a file containing coverage data

class VlcTest final {
    // MEMBERS
    string m_name;  //< Name of the test
    द्विगुण m_computrons;  //< Runtime for the test
    uint64_t m_testrun;  //< Test run number, for database use
    uint64_t m_rank = 0;  //< Execution rank suggestion
    uint64_t m_rankPoints = 0;  //< Ranked additional points
    uint64_t m_user = 0;  //< User data for algorithms (not persisted in .dat file)
    VlcBuckets m_buckets;  //< Coverage data for each coverage point

सार्वजनिक:
    // CONSTRUCTORS
    VlcTest(ध्रुव string& name, uint64_t testrun, द्विगुण comp)
        : m_name{name}
        , m_computrons{comp}
        , m_testrun{testrun} {}
    ~VlcTest() = स्वभाव;

    // ACCESSORS
    ध्रुव string& name() ध्रुव { प्रत्यागम m_name; }
    द्विगुण computrons() ध्रुव { प्रत्यागम m_computrons; }
    uint64_t testrun() ध्रुव { प्रत्यागम m_testrun; }
    VlcBuckets& buckets() { प्रत्यागम m_buckets; }
    uint64_t bucketsCovered() ध्रुव { प्रत्यागम m_buckets.bucketsCovered(); }
    uint64_t rank() ध्रुव { प्रत्यागम m_rank; }
    शून्य rank(uint64_t flag) { m_rank = flag; }
    uint64_t rankPoints() ध्रुव { प्रत्यागम m_rankPoints; }
    शून्य rankPoints(uint64_t flag) { m_rankPoints = flag; }
    uint64_t user() ध्रुव { प्रत्यागम m_user; }
    शून्य user(uint64_t flag) { m_user = flag; }

    // METHODS
    स्थिर शून्य dumpHeader() {
        std::cout << "Tests:\n";
        // std::cout<<"  Testrun, Computrons,";  // Currently not loaded
        std::cout << "  Covered,     Rank,  RankPts,  Filename\n";
    }
    शून्य dump(bool bucketsToo) ध्रुव {
        जइ (testrun() || computrons() != 0.0) {  // currently unused // LCOV_EXCL_LINE
            std::cout << "  " << std::setw(8) << std::setfill('0') << testrun()  // LCOV_EXCL_LINE
                      << ",  " << std::setw(7) << std::setfill(' ')
                      << computrons()  // LCOV_EXCL_LINE
                      << ",";  // LCOV_EXCL_LINE
        }
        std::cout << "  " << std::setw(7) << std::setfill(' ') << bucketsCovered();
        std::cout << ",  " << std::setw(7) << std::setfill(' ') << rank();
        std::cout << ",  " << std::setw(7) << std::setfill(' ') << rankPoints();
        std::cout << ",  \"" << name() << "\"\n";
        जइ (bucketsToo) m_buckets.dump();
    }
};

//********************************************************************
// VlcTests - Container of all tests

class VlcTests final {
सार्वजनिक:
    // TYPES
    using ByName = std::vector<VlcTest*>;

गुप्त:
    // MEMBERS
    ByName m_tests;  //< List of all tests

    स्थिर पूर्णाङ्क debug() { प्रत्यागम V3Error::debugDefault(); }

सार्वजनिक:
    // ITERATORS
    using iterator = ByName::iterator;
    ByName::iterator begin() { प्रत्यागम m_tests.begin(); }
    ByName::iterator end() { प्रत्यागम m_tests.end(); }

    // CONSTRUCTORS
    VlcTests() = स्वभाव;
    ~VlcTests() {
        प्रति (auto it = begin(); it != end(); ++it) VL_DO_CLEAR(delete *it, *it = nullptr);
    }

    // METHODS
    शून्य dump(bool bucketsToo) {
        UINFO(2, "dumpTests...");
        VlcTest::dumpHeader();
        प्रति (ध्रुव VlcTest* ध्रुव testp : m_tests) testp->dump(bucketsToo);
    }
    VlcTest* newTest(ध्रुव string& name, uint64_t testrun, द्विगुण comp) {
        VlcTest* ध्रुव testp = new VlcTest{name, testrun, comp};
        m_tests.प्रक्षिप_back(testp);
        प्रत्यागम testp;
    }
    शून्य clearUser() {
        प्रति (ध्रुव auto& testp : m_tests) testp->user(0);
    }
};

//######################################################################

#endif  // Guard
