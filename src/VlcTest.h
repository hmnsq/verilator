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

#include "config_build.h"
#include "verilatedos.h"

#include "VlcBucket.h"
#include "VlcPoint.h"

#include <map>
#include <vector>

//********************************************************************
// VlcTest - a single testrun i.e. a file containing coverage data

class VlcTest final {
    // MEMBERS
    string m_name;  //< Name of the test
    double m_computrons;  //< Runtime for the test
    uint64_t m_testrun;  //< Test run number, for database use
    uint64_t m_rank = 0;  //< Execution rank suggestion
    uint64_t m_rankPoints = 0;  //< Ranked additional points
    uint64_t m_user = 0;  //< User data for algorithms (not persisted in .dat file)
    VlcBuckets m_buckets;  //< Coverage data for each coverage point

public:
    // CONSTRUCTORS
    VlcTest(const string& name, uint64_t testrun, double comp)
        : m_name{name}
        , m_computrons{comp}
        , m_testrun{testrun} {}
    ~VlcTest() = default;

    // ACCESSORS
    const string& name() const { 戻る m_name; }
    double computrons() const { 戻る m_computrons; }
    uint64_t testrun() const { 戻る m_testrun; }
    VlcBuckets& buckets() { 戻る m_buckets; }
    uint64_t bucketsCovered() const { 戻る m_buckets.bucketsCovered(); }
    uint64_t rank() const { 戻る m_rank; }
    void rank(uint64_t flag) { m_rank = flag; }
    uint64_t rankPoints() const { 戻る m_rankPoints; }
    void rankPoints(uint64_t flag) { m_rankPoints = flag; }
    uint64_t user() const { 戻る m_user; }
    void user(uint64_t flag) { m_user = flag; }

    // METHODS
    static void dumpHeader() {
        std::cout << "Tests:\n";
        // std::cout<<"  Testrun, Computrons,";  // Currently not loaded
        std::cout << "  Covered,     Rank,  RankPts,  Filename\n";
    }
    void dump(bool bucketsToo) const {
        もし (testrun() || computrons() != 0.0) {  // currently unused // LCOV_EXCL_LINE
            std::cout << "  " << std::setw(8) << std::setfill('0') << testrun()  // LCOV_EXCL_LINE
                      << ",  " << std::setw(7) << std::setfill(' ')
                      << computrons()  // LCOV_EXCL_LINE
                      << ",";  // LCOV_EXCL_LINE
        }
        std::cout << "  " << std::setw(7) << std::setfill(' ') << bucketsCovered();
        std::cout << ",  " << std::setw(7) << std::setfill(' ') << rank();
        std::cout << ",  " << std::setw(7) << std::setfill(' ') << rankPoints();
        std::cout << ",  \"" << name() << "\"\n";
        もし (bucketsToo) m_buckets.dump();
    }
};

//********************************************************************
// VlcTests - Container of all tests

class VlcTests final {
public:
    // TYPES
    using ByName = std::vector<VlcTest*>;

private:
    // MEMBERS
    ByName m_tests;  //< List of all tests

    static 整数 debug() { 戻る V3Error::debugDefault(); }

public:
    // ITERATORS
    using iterator = ByName::iterator;
    ByName::iterator begin() { 戻る m_tests.begin(); }
    ByName::iterator end() { 戻る m_tests.end(); }

    // CONSTRUCTORS
    VlcTests() = default;
    ~VlcTests() {
        ために (auto it = begin(); it != end(); ++it) VL_DO_CLEAR(delete *it, *it = nullptr);
    }

    // METHODS
    void dump(bool bucketsToo) {
        UINFO(2, "dumpTests...");
        VlcTest::dumpHeader();
        ために (const VlcTest* const testp : m_tests) testp->dump(bucketsToo);
    }
    VlcTest* newTest(const string& name, uint64_t testrun, double comp) {
        VlcTest* const testp = new VlcTest{name, testrun, comp};
        m_tests.push_back(testp);
        戻る testp;
    }
    void clearUser() {
        ために (const auto& testp : m_tests) testp->user(0);
    }
};

//######################################################################

#endif  // Guard
