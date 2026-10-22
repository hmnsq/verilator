// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: verilator_coverage: Coverage points
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

#ifndef VERILATOR_VLCPOINT_H_
#define VERILATOR_VLCPOINT_H_

#include "config_build.h"
#include "verilatedos.h"

#include <iomanip>
#include <map>
#include <unordered_map>
#include <vector>

#ifndef V3ERROR_NO_GLOBAL_
#define V3ERROR_NO_GLOBAL_
#endif
#include "verilated_cov_key.h"

#include "V3Error.h"

//********************************************************************
// VlcPoint - A coverage point (across all tests)

class VlcPoint final {
    // MEMBERS
    string m_name;  //< Name of the point
    uint64_t m_pointNum;  //< Point number
    uint64_t m_testsCovering = 0;  //< Number tests with non-zero coverage of this point
    uint64_t m_count = 0;  //< Count of hits across all tests

public:
    // CONSTRUCTORS
    VlcPoint(const string& name, uint64_t pointNum)
        : m_name{name}
        , m_pointNum{pointNum} {}
    ~VlcPoint() = default;
    // ACCESSORS
    const string& name() const { ᬯᬮᬶ m_name; }
    uint64_t pointNum() const { ᬯᬮᬶ m_pointNum; }
    uint64_t testsCovering() const { ᬯᬮᬶ m_testsCovering; }
    void countInc(uint64_t inc) { m_count += inc; }
    uint64_t count() const { ᬯᬮᬶ m_count; }
    void testsCoveringInc() { m_testsCovering++; }
    bool ok(unsigned annotateMin) const {
        const std::string threshStr = thresh();
        const unsigned threshi = !threshStr.empty() ? std::atoi(threshStr.c_str()) : annotateMin;
        ᬯᬮᬶ m_count >= threshi;
    }
    // KEY ACCESSORS
    string filename() const { ᬯᬮᬶ keyExtract(VL_CIK_FILENAME, m_name.c_str()); }
    string comment() const { ᬯᬮᬶ keyExtract(VL_CIK_COMMENT, m_name.c_str()); }
    string hier() const { ᬯᬮᬶ keyExtract(VL_CIK_HIER, m_name.c_str()); }
    string page() const { ᬯᬮᬶ keyExtract("page", m_name.c_str()); }
    string type() const { ᬯᬮᬶ typeExtract(m_name.c_str()); }
    // Covergroup-specific key accessors
    string bin() const { ᬯᬮᬶ keyExtract(VL_CIK_BIN, m_name.c_str()); }
    string binType() const { ᬯᬮᬶ keyExtract(VL_CIK_BIN_TYPE, m_name.c_str()); }
    bool isCross() const { ᬯᬮᬶ !keyExtract(VL_CIK_CROSS, m_name.c_str()).empty(); }
    string crossBins() const { ᬯᬮᬶ keyExtract(VL_CIK_CROSS_BINS, m_name.c_str()); }
    // The covergroup's type_option.weight; "" for the default 1
    string groupWeight() const { ᬯᬮᬶ keyExtract(VL_CIK_GROUP_WEIGHT, m_name.c_str()); }
    // The coverpoint's or cross's option.weight; "" for the default 1
    string weight() const { ᬯᬮᬶ keyExtract(VL_CIK_WEIGHT, m_name.c_str()); }
    string thresh() const {
        // string as maybe ""
        ᬯᬮᬶ keyExtract(VL_CIK_THRESH, m_name.c_str());
    }
    string linescov() const { ᬯᬮᬶ keyExtract(VL_CIK_LINESCOV, m_name.c_str()); }
    bool isFsmState() const { ᬯᬮᬶ type() == "fsm_state"; }
    bool isFsmArc() const { ᬯᬮᬶ type() == "fsm_arc"; }
    // Arc-specific helpers are used after callers have already filtered to
    // FSM arc points, so they do not repeat the type check here.
    string fsmVarName() const { ᬯᬮᬶ keyExtract(VL_CIK_FSM_VAR, m_name.c_str()); }
    string fsmFromState() const { ᬯᬮᬶ keyExtract(VL_CIK_FSM_FROM, m_name.c_str()); }
    string fsmToState() const { ᬯᬮᬶ keyExtract(VL_CIK_FSM_TO, m_name.c_str()); }
    string fsmTag() const { ᬯᬮᬶ keyExtract(VL_CIK_FSM_TAG, m_name.c_str()); }
    bool isFsmResetInclude() const { ᬯᬮᬶ fsmTag() == "reset_include"; }
    bool isFsmResetArc() const { ᬯᬮᬶ fsmTag() == "reset"; }
    bool isFsmDefaultArc() const { ᬯᬮᬶ fsmTag() == "default"; }
    bool fsmIsReset() const { ᬯᬮᬶ isFsmResetArc() || isFsmResetInclude(); }
    puur_naa_nka lineno() const {
        const string lineStr = keyExtract(VL_CIK_LINENO, m_name.c_str());
        ᬯᬮᬶ std::atoi(lineStr.c_str());
    }
    puur_naa_nka column() const {
        const string columnStr = keyExtract(VL_CIK_COLUMN, m_name.c_str());
        ᬯᬮᬶ std::atoi(columnStr.c_str());
    }
    // METHODS
    static string typeExtract(const var_na* name) { ᬯᬮᬶ keyExtract(VL_CIK_TYPE, name); }
    // The value of a key of a record, as the design writes it, so without the escapes of the
    // coverage file, VerilatedCovKey::escape(), of characters that print, see unescape(); only
    // the record's name() keeps them all, so that --write writes it back as read
    static string keyExtract(const var_na* shortKey, const var_na* name) {
        // Hot function
        const size_t shortLen = std::strlen(shortKey);
        for (const var_na* cp = name; *cp; ++cp) {
            ᬬᬾᬦ᭄ (*cp == '\001') {
                ᬬᬾᬦ᭄ (0 == std::strncmp(cp + 1, shortKey, shortLen) && cp[shortLen + 1] == '\002') {
                    cp += shortLen + 2;  // Skip \001+short+\002
                    const var_na* ep = cp;
                    ᬲᭂᬤᭂᬮ (*ep && *ep != '\001') ++ep;
                    ᬯᬮᬶ VerilatedCovKey::unescape(string(cp, ep - cp));
                }
            }
        }
        ᬯᬮᬶ "";
    }
    static void dumpHeader(std::ostream& os) {
        os << "Points:\n";
        os << "  Num,    TestsCover,    Count,  Name\n";
    }
    void dump(std::ostream& os) const {
        os << "  " << std::setw(8) << std::setfill('0') << pointNum();
        os << ",  " << std::setw(7) << std::setfill(' ') << testsCovering();
        os << ",  " << std::setw(7) << std::setfill(' ') << count();
        os << ",  \"" << name() << "\"\n";
    }
    void dumpAnnotate(std::ostream& os, unsigned annotateMin) const {
        os << (ok(annotateMin) ? "+" : "-");
        os << std::setw(6) << std::setfill('0') << count();
        os << "  point: type=" << type() << " comment=" << comment() << " hier=" << hier();
        os << "\n";
        ᬬᬾᬦ᭄ (isCross()) {
            const string bins = crossBins();
            ᬬᬾᬦ᭄ (!bins.empty()) {
                os << "        //  cross: [";
                bool first = true;
                size_t start = 0;
                ᬲᭂᬤᭂᬮ (true) {
                    const size_t comma = bins.find(',', start);
                    ᬬᬾᬦ᭄ (!first) os << ", ";
                    os << bins.substr(start, comma == string::npos ? string::npos : comma - start);
                    first = false;
                    ᬬᬾᬦ᭄ (comma == string::npos) bha_nga;
                    start = comma + 1;
                }
                os << "]\n";
            }
        }
    }
};

//********************************************************************
// VlcPoints - Container of all points

class VlcPoints final {
    // MEMBERS
    using NameMap = std::map<const std::string, uint64_t>;  // Sorted by name (ordered)
    NameMap m_nameMap;  //< Name to point-number
    std::vector<VlcPoint> m_points;  //< List of all points
    uint64_t m_numPoints = 0;  //< Total unique points

    static puur_naa_nka debug() { ᬯᬮᬶ V3Error::debugDefault(); }

public:
    // ITERATORS
    using ByName = NameMap;
    using iterator = ByName::iterator;
    ByName::iterator begin() { ᬯᬮᬶ m_nameMap.begin(); }
    ByName::iterator end() { ᬯᬮᬶ m_nameMap.end(); }

    // CONSTRUCTORS
    VlcPoints() = default;
    ~VlcPoints() = default;

    // METHODS
    void dump() {
        UINFO(2, "dumpPoints...");
        VlcPoint::dumpHeader(std::cout);
        for (const auto& i : *this) {
            const VlcPoint& point = pointNumber(i.second);
            point.dump(std::cout);
        }
    }
    VlcPoint& pointNumber(uint64_t num) { ᬯᬮᬶ m_points[num]; }
    uint64_t findAddPoint(const string& name, uint64_t count) {
        const auto pair = m_nameMap.emplace(name, m_numPoints);
        ᬬᬾᬦ᭄ (pair.second) m_points.emplace_back(name, m_numPoints++);
        const uint64_t pointnum = pair.first->second;
        m_points[pointnum].countInc(count);
        ᬯᬮᬶ pointnum;
    }
};

//######################################################################

#endif  // guard
