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

#अगर_अघोषित VERILATOR_VLCTEST_H_
#घोषणा VERILATOR_VLCTEST_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "VlcBucket.h"
#समावेश "VlcPoint.h"

#समावेश <map>
#समावेश <vector>

//********************************************************************
// VlcTest - a single testrun i.e. a file containing coverage data

class VlcTest final {
    // MEMBERS
    string m_name;  //< Name of the test
    द्विगुन m_computrons;  //< Runtime for the test
    uint64_t m_testrun;  //< Test run number, for database use
    uint64_t m_rank = 0;  //< Execution rank suggestion
    uint64_t m_rankPoints = 0;  //< Ranked additional points
    uint64_t m_user = 0;  //< User data for algorithms (not persisted in .dat file)
    VlcBuckets m_buckets;  //< Coverage data for each coverage point

खुला:
    // CONSTRUCTORS
    VlcTest(स्थिर string& name, uint64_t testrun, द्विगुन comp)
        : m_name{name}
        , m_computrons{comp}
        , m_testrun{testrun} {}
    ~VlcTest() = शेष;

    // ACCESSORS
    स्थिर string& name() स्थिर { वापस m_name; }
    द्विगुन computrons() स्थिर { वापस m_computrons; }
    uint64_t testrun() स्थिर { वापस m_testrun; }
    VlcBuckets& buckets() { वापस m_buckets; }
    uint64_t bucketsCovered() स्थिर { वापस m_buckets.bucketsCovered(); }
    uint64_t rank() स्थिर { वापस m_rank; }
    व्योम rank(uint64_t flag) { m_rank = flag; }
    uint64_t rankPoints() स्थिर { वापस m_rankPoints; }
    व्योम rankPoints(uint64_t flag) { m_rankPoints = flag; }
    uint64_t user() स्थिर { वापस m_user; }
    व्योम user(uint64_t flag) { m_user = flag; }

    // METHODS
    अटल व्योम dumpHeader() {
        std::cout << "Tests:\n";
        // std::cout<<"  Testrun, Computrons,";  // Currently not loaded
        std::cout << "  Covered,     Rank,  RankPts,  Filename\n";
    }
    व्योम dump(bool bucketsToo) स्थिर {
        अगर (testrun() || computrons() != 0.0) {  // currently unused // LCOV_EXCL_LINE
            std::cout << "  " << std::setw(8) << std::setfill('0') << testrun()  // LCOV_EXCL_LINE
                      << ",  " << std::setw(7) << std::setfill(' ')
                      << computrons()  // LCOV_EXCL_LINE
                      << ",";  // LCOV_EXCL_LINE
        }
        std::cout << "  " << std::setw(7) << std::setfill(' ') << bucketsCovered();
        std::cout << ",  " << std::setw(7) << std::setfill(' ') << rank();
        std::cout << ",  " << std::setw(7) << std::setfill(' ') << rankPoints();
        std::cout << ",  \"" << name() << "\"\n";
        अगर (bucketsToo) m_buckets.dump();
    }
};

//********************************************************************
// VlcTests - Container of all tests

class VlcTests final {
खुला:
    // TYPES
    using ByName = std::vector<VlcTest*>;

निजी:
    // MEMBERS
    ByName m_tests;  //< List of all tests

    अटल पूर्णांक debug() { वापस V3Error::debugDefault(); }

खुला:
    // ITERATORS
    using iterator = ByName::iterator;
    ByName::iterator begin() { वापस m_tests.begin(); }
    ByName::iterator end() { वापस m_tests.end(); }

    // CONSTRUCTORS
    VlcTests() = शेष;
    ~VlcTests() {
        क्रम (स्वतः it = begin(); it != end(); ++it) VL_DO_CLEAR(delete *it, *it = nullptr);
    }

    // METHODS
    व्योम dump(bool bucketsToo) {
        UINFO(2, "dumpTests...");
        VlcTest::dumpHeader();
        क्रम (स्थिर VlcTest* स्थिर testp : m_tests) testp->dump(bucketsToo);
    }
    VlcTest* newTest(स्थिर string& name, uint64_t testrun, द्विगुन comp) {
        VlcTest* स्थिर testp = new VlcTest{name, testrun, comp};
        m_tests.धकेलो_back(testp);
        वापस testp;
    }
    व्योम clearUser() {
        क्रम (स्थिर स्वतः& testp : m_tests) testp->user(0);
    }
};

//######################################################################

#पूर्ण_अगर  // Guard
