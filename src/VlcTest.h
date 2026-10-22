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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_VLCTEST_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_VLCTEST_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "VlcBucket.h"
#ಅನ್ತರ್ಭಾವ "VlcPoint.h"

#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <vector>

//********************************************************************
// VlcTest - a single testrun i.e. a file containing coverage data

class VlcTest final {
    // MEMBERS
    string m_name;  //< Name of the test
    ದ್ವಿಗುಣ m_computrons;  //< Runtime for the test
    uint64_t m_testrun;  //< Test run number, for database use
    uint64_t m_rank = 0;  //< Execution rank suggestion
    uint64_t m_rankPoints = 0;  //< Ranked additional points
    uint64_t m_user = 0;  //< User data for algorithms (not persisted in .dat file)
    VlcBuckets m_buckets;  //< Coverage data for each coverage point

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    VlcTest(ಸ್ಥಿರಾಂಕ string& name, uint64_t testrun, ದ್ವಿಗುಣ comp)
        : m_name{name}
        , m_computrons{comp}
        , m_testrun{testrun} {}
    ~VlcTest() = ಪೂರ್ವನಿಯೋಜಿತ;

    // ACCESSORS
    ಸ್ಥಿರಾಂಕ string& name() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_name; }
    ದ್ವಿಗುಣ computrons() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_computrons; }
    uint64_t testrun() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_testrun; }
    VlcBuckets& buckets() { ಹಿಂತಿರುಗಿ m_buckets; }
    uint64_t bucketsCovered() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_buckets.bucketsCovered(); }
    uint64_t rank() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_rank; }
    ಶೂನ್ಯ rank(uint64_t flag) { m_rank = flag; }
    uint64_t rankPoints() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_rankPoints; }
    ಶೂನ್ಯ rankPoints(uint64_t flag) { m_rankPoints = flag; }
    uint64_t user() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_user; }
    ಶೂನ್ಯ user(uint64_t flag) { m_user = flag; }

    // METHODS
    ಸ್ಥಿರ ಶೂನ್ಯ dump_ಶೀರ್ಷಿಕೆ() {
        std::cout << "Tests:\n";
        // std::cout<<"  Testrun, Computrons,";  // Currently not loaded
        std::cout << "  Covered,     Rank,  RankPts,  Filename\n";
    }
    ಶೂನ್ಯ dump(bool bucketsToo) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (testrun() || computrons() != 0.0) {  // currently unused // LCOV_EXCL_LINE
            std::cout << "  " << std::setw(8) << std::setfill('0') << testrun()  // LCOV_EXCL_LINE
                      << ",  " << std::setw(7) << std::setfill(' ')
                      << computrons()  // LCOV_EXCL_LINE
                      << ",";  // LCOV_EXCL_LINE
        }
        std::cout << "  " << std::setw(7) << std::setfill(' ') << bucketsCovered();
        std::cout << ",  " << std::setw(7) << std::setfill(' ') << rank();
        std::cout << ",  " << std::setw(7) << std::setfill(' ') << rankPoints();
        std::cout << ",  \"" << name() << "\"\n";
        ಆದರೆ (bucketsToo) m_buckets.dump();
    }
};

//********************************************************************
// VlcTests - Container of all tests

class VlcTests final {
ಸಾರ್ವಜನಿಕ:
    // TYPES
    using ByName = std::ಸದಿಶ<VlcTest*>;

ಖಾಸಗಿ:
    // MEMBERS
    ByName m_tests;  //< List of all tests

    ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ debug() { ಹಿಂತಿರುಗಿ V3Error::debugDefault(); }

ಸಾರ್ವಜನಿಕ:
    // ITERATORS
    using iterator = ByName::iterator;
    ByName::iterator begin() { ಹಿಂತಿರುಗಿ m_tests.begin(); }
    ByName::iterator ಕೊನೆ() { ಹಿಂತಿರುಗಿ m_tests.ಕೊನೆ(); }

    // CONSTRUCTORS
    VlcTests() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~VlcTests() {
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ it = begin(); it != ಕೊನೆ(); ++it) VL_DO_CLEAR(delete *it, *it = nullptr);
    }

    // METHODS
    ಶೂನ್ಯ dump(bool bucketsToo) {
        UINFO(2, "dumpTests...");
        VlcTest::dump_ಶೀರ್ಷಿಕೆ();
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ VlcTest* ಸ್ಥಿರಾಂಕ testp : m_tests) testp->dump(bucketsToo);
    }
    VlcTest* newTest(ಸ್ಥಿರಾಂಕ string& name, uint64_t testrun, ದ್ವಿಗುಣ comp) {
        VlcTest* ಸ್ಥಿರಾಂಕ testp = new VlcTest{name, testrun, comp};
        m_tests.ತಳ್ಳು_ಹಿಂಭಾಗ(testp);
        ಹಿಂತಿರುಗಿ testp;
    }
    ಶೂನ್ಯ clearUser() {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& testp : m_tests) testp->user(0);
    }
};

//######################################################################

#ಆದರೆ_ಅಂತ್ಯ  // Guard
