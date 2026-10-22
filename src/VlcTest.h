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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_VLCTEST_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_VLCTEST_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "VlcBucket.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "VlcPoint.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <map>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

//********************************************************************
// VlcTest - a single testrun i.e. a file containing coverage data

class VlcTest final {
    // MEMBERS
    string m_name;  //< Name of the test
    ਦੁੱਗਣਾ m_computrons;  //< Runtime for the test
    uint64_t m_testrun;  //< Test run number, for database use
    uint64_t m_rank = 0;  //< Execution rank suggestion
    uint64_t m_rankPoints = 0;  //< Ranked additional points
    uint64_t m_user = 0;  //< User data for algorithms (not persisted in .dat file)
    VlcBuckets m_buckets;  //< Coverage data for each coverage point

ਜਨਤਕ:
    // CONSTRUCTORS
    VlcTest(ਅਚਲ string& name, uint64_t testrun, ਦੁੱਗਣਾ comp)
        : m_name{name}
        , m_computrons{comp}
        , m_testrun{testrun} {}
    ~VlcTest() = ਮੂਲ;

    // ACCESSORS
    ਅਚਲ string& name() ਅਚਲ { ਵਾਪਸ m_name; }
    ਦੁੱਗਣਾ computrons() ਅਚਲ { ਵਾਪਸ m_computrons; }
    uint64_t testrun() ਅਚਲ { ਵਾਪਸ m_testrun; }
    VlcBuckets& buckets() { ਵਾਪਸ m_buckets; }
    uint64_t bucketsCovered() ਅਚਲ { ਵਾਪਸ m_buckets.bucketsCovered(); }
    uint64_t rank() ਅਚਲ { ਵਾਪਸ m_rank; }
    ਖਾਲੀ rank(uint64_t flag) { m_rank = flag; }
    uint64_t rankPoints() ਅਚਲ { ਵਾਪਸ m_rankPoints; }
    ਖਾਲੀ rankPoints(uint64_t flag) { m_rankPoints = flag; }
    uint64_t user() ਅਚਲ { ਵਾਪਸ m_user; }
    ਖਾਲੀ user(uint64_t flag) { m_user = flag; }

    // METHODS
    ਸਥਿਰ ਖਾਲੀ dump_ਸਿਰਲੇਖ() {
        std::cout << "Tests:\n";
        // std::cout<<"  Testrun, Computrons,";  // Currently not loaded
        std::cout << "  Covered,     Rank,  RankPts,  Filename\n";
    }
    ਖਾਲੀ dump(bool bucketsToo) ਅਚਲ {
        ਜੇ (testrun() || computrons() != 0.0) {  // currently unused // LCOV_EXCL_LINE
            std::cout << "  " << std::setw(8) << std::setfill('0') << testrun()  // LCOV_EXCL_LINE
                      << ",  " << std::setw(7) << std::setfill(' ')
                      << computrons()  // LCOV_EXCL_LINE
                      << ",";  // LCOV_EXCL_LINE
        }
        std::cout << "  " << std::setw(7) << std::setfill(' ') << bucketsCovered();
        std::cout << ",  " << std::setw(7) << std::setfill(' ') << rank();
        std::cout << ",  " << std::setw(7) << std::setfill(' ') << rankPoints();
        std::cout << ",  \"" << name() << "\"\n";
        ਜੇ (bucketsToo) m_buckets.dump();
    }
};

//********************************************************************
// VlcTests - Container of all tests

class VlcTests final {
ਜਨਤਕ:
    // TYPES
    using ByName = std::ਸਦਿਸ਼<VlcTest*>;

ਨਿਜੀ:
    // MEMBERS
    ByName m_tests;  //< List of all tests

    ਸਥਿਰ ਪੂਰਨ debug() { ਵਾਪਸ V3Error::debugDefault(); }

ਜਨਤਕ:
    // ITERATORS
    using iterator = ByName::iterator;
    ByName::iterator begin() { ਵਾਪਸ m_tests.begin(); }
    ByName::iterator ਸਿਰਾ() { ਵਾਪਸ m_tests.ਸਿਰਾ(); }

    // CONSTRUCTORS
    VlcTests() = ਮੂਲ;
    ~VlcTests() {
        ਲਈ (ਸਵੈਚਾਲਿਤ it = begin(); it != ਸਿਰਾ(); ++it) VL_DO_CLEAR(delete *it, *it = nullptr);
    }

    // METHODS
    ਖਾਲੀ dump(bool bucketsToo) {
        UINFO(2, "dumpTests...");
        VlcTest::dump_ਸਿਰਲੇਖ();
        ਲਈ (ਅਚਲ VlcTest* ਅਚਲ testp : m_tests) testp->dump(bucketsToo);
    }
    VlcTest* newTest(ਅਚਲ string& name, uint64_t testrun, ਦੁੱਗਣਾ comp) {
        VlcTest* ਅਚਲ testp = new VlcTest{name, testrun, comp};
        m_tests.ਧੱਕੋ_ਪਿੱਠ(testp);
        ਵਾਪਸ testp;
    }
    ਖਾਲੀ clearUser() {
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& testp : m_tests) testp->user(0);
    }
};

//######################################################################

#ਜੇ_ਅੰਤ  // Guard
