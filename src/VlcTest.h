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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "VlcBucket.h"
#അന്തര്ഭാവ "VlcPoint.h"

#അന്തര്ഭാവ <map>
#അന്തര്ഭാവ <vector>

//********************************************************************
// VlcTest - a single testrun i.e. a file containing coverage data

class VlcTest final {
    // MEMBERS
    string m_name;  //< Name of the test
    ഇരട്ട m_computrons;  //< Runtime for the test
    uint64_t m_testrun;  //< Test run number, for database use
    uint64_t m_rank = 0;  //< Execution rank suggestion
    uint64_t m_rankPoints = 0;  //< Ranked additional points
    uint64_t m_user = 0;  //< User data for algorithms (not persisted in .dat file)
    VlcBuckets m_buckets;  //< Coverage data for each coverage point

പൊതു:
    // CONSTRUCTORS
    VlcTest(സ്ഥിരം string& name, uint64_t testrun, ഇരട്ട comp)
        : m_name{name}
        , m_computrons{comp}
        , m_testrun{testrun} {}
    ~VlcTest() = സ്ഥിരസ്ഥിതി;

    // ACCESSORS
    സ്ഥിരം string& name() സ്ഥിരം { തിരികെ m_name; }
    ഇരട്ട computrons() സ്ഥിരം { തിരികെ m_computrons; }
    uint64_t testrun() സ്ഥിരം { തിരികെ m_testrun; }
    VlcBuckets& buckets() { തിരികെ m_buckets; }
    uint64_t bucketsCovered() സ്ഥിരം { തിരികെ m_buckets.bucketsCovered(); }
    uint64_t rank() സ്ഥിരം { തിരികെ m_rank; }
    ശൂന്യം rank(uint64_t flag) { m_rank = flag; }
    uint64_t rankPoints() സ്ഥിരം { തിരികെ m_rankPoints; }
    ശൂന്യം rankPoints(uint64_t flag) { m_rankPoints = flag; }
    uint64_t user() സ്ഥിരം { തിരികെ m_user; }
    ശൂന്യം user(uint64_t flag) { m_user = flag; }

    // METHODS
    സ്ഥിതം ശൂന്യം dumpHeader() {
        std::cout << "Tests:\n";
        // std::cout<<"  Testrun, Computrons,";  // Currently not loaded
        std::cout << "  Covered,     Rank,  RankPts,  Filename\n";
    }
    ശൂന്യം dump(bool bucketsToo) സ്ഥിരം {
        എങ്കിൽ (testrun() || computrons() != 0.0) {  // currently unused // LCOV_EXCL_LINE
            std::cout << "  " << std::setw(8) << std::setfill('0') << testrun()  // LCOV_EXCL_LINE
                      << ",  " << std::setw(7) << std::setfill(' ')
                      << computrons()  // LCOV_EXCL_LINE
                      << ",";  // LCOV_EXCL_LINE
        }
        std::cout << "  " << std::setw(7) << std::setfill(' ') << bucketsCovered();
        std::cout << ",  " << std::setw(7) << std::setfill(' ') << rank();
        std::cout << ",  " << std::setw(7) << std::setfill(' ') << rankPoints();
        std::cout << ",  \"" << name() << "\"\n";
        എങ്കിൽ (bucketsToo) m_buckets.dump();
    }
};

//********************************************************************
// VlcTests - Container of all tests

class VlcTests final {
പൊതു:
    // TYPES
    using ByName = std::vector<VlcTest*>;

സ്വകാര്യ:
    // MEMBERS
    ByName m_tests;  //< List of all tests

    സ്ഥിതം പൂർണ്ണസംഖ്യ debug() { തിരികെ V3Error::debugDefault(); }

പൊതു:
    // ITERATORS
    using iterator = ByName::iterator;
    ByName::iterator begin() { തിരികെ m_tests.begin(); }
    ByName::iterator end() { തിരികെ m_tests.end(); }

    // CONSTRUCTORS
    VlcTests() = സ്ഥിരസ്ഥിതി;
    ~VlcTests() {
        വേണ്ടി (auto it = begin(); it != end(); ++it) VL_DO_CLEAR(delete *it, *it = nullptr);
    }

    // METHODS
    ശൂന്യം dump(bool bucketsToo) {
        UINFO(2, "dumpTests...");
        VlcTest::dumpHeader();
        വേണ്ടി (സ്ഥിരം VlcTest* സ്ഥിരം testp : m_tests) testp->dump(bucketsToo);
    }
    VlcTest* newTest(സ്ഥിരം string& name, uint64_t testrun, ഇരട്ട comp) {
        VlcTest* സ്ഥിരം testp = new VlcTest{name, testrun, comp};
        m_tests.തള്ളുക_back(testp);
        തിരികെ testp;
    }
    ശൂന്യം clearUser() {
        വേണ്ടി (സ്ഥിരം auto& testp : m_tests) testp->user(0);
    }
};

//######################################################################

#endif  // Guard
