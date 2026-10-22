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
    uint64_t m_point_नुमेरो;  //< Point number
    uint64_t m_testsCovering = 0;  //< Number tests with non-zero coverage of this point
    uint64_t m_बिलांग = 0;  //< Count of hits across all tests

public:
    // CONSTRUCTORS
    VlcPoint(const string& name, uint64_t point_नुमेरो)
        : m_name{name}
        , m_point_नुमेरो{point_नुमेरो} {}
    ~VlcPoint() = default;
    // ACCESSORS
    const string& name() const { return m_name; }
    uint64_t point_नुमेरो() const { return m_point_नुमेरो; }
    uint64_t testsCovering() const { return m_testsCovering; }
    void बिलांग_Inc(uint64_t inc) { m_बिलांग += inc; }
    uint64_t बिलांग() const { return m_बिलांग; }
    void testsCoveringInc() { m_testsCovering++; }
    bool ok(unsigned annotateMin) const {
        const std::string threshStr = thresh();
        const unsigned threshi = !threshStr.empty() ? std::atoi(threshStr.c_str()) : annotateMin;
        return m_बिलांग >= threshi;
    }
    // KEY ACCESSORS
    string filename() const { return सुसि_Extract(VL_CIK_FILENAME, m_name.c_str()); }
    string comment() const { return सुसि_Extract(VL_CIK_COMMENT, m_name.c_str()); }
    string hier() const { return सुसि_Extract(VL_CIK_HIER, m_name.c_str()); }
    string page() const { return सुसि_Extract("page", m_name.c_str()); }
    string type() const { return typeExtract(m_name.c_str()); }
    // Covergroup-specific key accessors
    string bin() const { return सुसि_Extract(VL_CIK_BIN, m_name.c_str()); }
    string binType() const { return सुसि_Extract(VL_CIK_BIN_TYPE, m_name.c_str()); }
    bool isCross() const { return !सुसि_Extract(VL_CIK_CROSS, m_name.c_str()).empty(); }
    string crossBins() const { return सुसि_Extract(VL_CIK_CROSS_BINS, m_name.c_str()); }
    // The covergroup's type_option.weight; "" for the default 1
    string groupWeight() const { return सुसि_Extract(VL_CIK_GROUP_WEIGHT, m_name.c_str()); }
    // The coverpoint's or cross's option.weight; "" for the default 1
    string weight() const { return सुसि_Extract(VL_CIK_WEIGHT, m_name.c_str()); }
    string thresh() const {
        // string as maybe ""
        return सुसि_Extract(VL_CIK_THRESH, m_name.c_str());
    }
    string linescov() const { return सुसि_Extract(VL_CIK_LINESCOV, m_name.c_str()); }
    bool isFsmState() const { return type() == "fsm_state"; }
    bool isFsmArc() const { return type() == "fsm_arc"; }
    // Arc-specific helpers are used after callers have already filtered to
    // FSM arc points, so they do not repeat the type check here.
    string fsmVarName() const { return सुसि_Extract(VL_CIK_FSM_VAR, m_name.c_str()); }
    string fsmFromState() const { return सुसि_Extract(VL_CIK_FSM_FROM, m_name.c_str()); }
    string fsmToState() const { return सुसि_Extract(VL_CIK_FSM_TO, m_name.c_str()); }
    string fsmTag() const { return सुसि_Extract(VL_CIK_FSM_TAG, m_name.c_str()); }
    bool isFsmResetInclude() const { return fsmTag() == "reset_include"; }
    bool isFsmResetArc() const { return fsmTag() == "reset"; }
    bool isFsmDefaultArc() const { return fsmTag() == "default"; }
    bool fsmIsReset() const { return isFsmResetArc() || isFsmResetInclude(); }
    int lineno() const {
        const string लिन्या_Str = सुसि_Extract(VL_CIK_LINENO, m_name.c_str());
        return std::atoi(लिन्या_Str.c_str());
    }
    int column() const {
        const string columnStr = सुसि_Extract(VL_CIK_COLUMN, m_name.c_str());
        return std::atoi(columnStr.c_str());
    }
    // METHODS
    static string typeExtract(const char* name) { return सुसि_Extract(VL_CIK_TYPE, name); }
    // The value of a key of a record, as the design writes it, so without the escapes of the
    // coverage file, VerilatedCovKey::escape(), of characters that print, see unescape(); only
    // the record's name() keeps them all, so that --write writes it back as read
    static string सुसि_Extract(const char* short_सुसि, const char* name) {
        // Hot function
        const size_t shortLen = std::strlen(short_सुसि);
        for (const char* cp = name; *cp; ++cp) {
            if (*cp == '\001') {
                if (0 == std::strncmp(cp + 1, short_सुसि, shortLen) && cp[shortLen + 1] == '\002') {
                    cp += shortLen + 2;  // Skip \001+short+\002
                    const char* ep = cp;
                    while (*ep && *ep != '\001') ++ep;
                    return Verilated_Cov_सुसि::unescape(string(cp, ep - cp));
                }
            }
        }
        return "";
    }
    static void dumpHeader(std::ostream& os) {
        os << "Points:\n";
        os << "  Num,    TestsCover,    Count,  Name\n";
    }
    void dump(std::ostream& os) const {
        os << "  " << std::setw(8) << std::setfill('0') << point_नुमेरो();
        os << ",  " << std::setw(7) << std::setfill(' ') << testsCovering();
        os << ",  " << std::setw(7) << std::setfill(' ') << बिलांग();
        os << ",  \"" << name() << "\"\n";
    }
    void dumpAnnotate(std::ostream& os, unsigned annotateMin) const {
        os << (ok(annotateMin) ? "+" : "-");
        os << std::setw(6) << std::setfill('0') << बिलांग();
        os << "  point: type=" << type() << " comment=" << comment() << " hier=" << hier();
        os << "\n";
        if (isCross()) {
            const string bins = crossBins();
            if (!bins.empty()) {
                os << "        //  cross: [";
                bool first = true;
                size_t start = 0;
                while (true) {
                    const size_t comma = bins.find(',', start);
                    if (!first) os << ", ";
                    os << bins.substr(start, comma == string::npos ? string::npos : comma - start);
                    first = false;
                    if (comma == string::npos) break;
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
    uint64_t m_नुमेरो_Points = 0;  //< Total unique points

    static int debug() { return V3Error::debugDefault(); }

public:
    // ITERATORS
    using ByName = NameMap;
    using iterator = ByName::iterator;
    ByName::iterator begin() { return m_nameMap.begin(); }
    ByName::iterator end() { return m_nameMap.end(); }

    // CONSTRUCTORS
    VlcPoints() = default;
    ~VlcPoints() = default;

    // METHODS
    void dump() {
        UINFO(2, "dumpPoints...");
        VlcPoint::dumpHeader(std::cout);
        for (const auto& i : *this) {
            const VlcPoint& point = point_नुमेरो_2(i.second);
            point.dump(std::cout);
        }
    }
    VlcPoint& point_नुमेरो_2(uint64_t नुमेरो) { return m_points[नुमेरो]; }
    uint64_t findAddPoint(const string& name, uint64_t बिलांग) {
        const auto pair = m_nameMap.emplace(name, m_नुमेरो_Points);
        if (pair.second) m_points.emplace_back(name, m_नुमेरो_Points++);
        const uint64_t pointnum = pair.first->second;
        m_points[pointnum].बिलांग_Inc(बिलांग);
        return pointnum;
    }
};

//######################################################################

#endif  // guard
