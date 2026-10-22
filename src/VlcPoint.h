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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব <iomanip>
#অন্তর্ভাব <map>
#অন্তর্ভাব <unordered_map>
#অন্তর্ভাব <vector>

#ifndef V3ERROR_NO_GLOBAL_
#define V3ERROR_NO_GLOBAL_
#endif
#অন্তর্ভাব "verilated_cov_key.h"

#অন্তর্ভাব "V3Error.h"

//********************************************************************
// VlcPoint - A coverage point (across all tests)

class VlcPoint final {
    // MEMBERS
    string m_name;  //< Name of the point
    uint64_t m_point_অঙ্ক;  //< Point number
    uint64_t m_testsCovering = 0;  //< Number tests with non-zero coverage of this point
    uint64_t m_গুনতি = 0;  //< Count of hits across all tests

সার্বজনিক:
    // CONSTRUCTORS
    VlcPoint(ধ্রুব string& name, uint64_t point_অঙ্ক)
        : m_name{name}
        , m_point_অঙ্ক{point_অঙ্ক} {}
    ~VlcPoint() = স্বভাব;
    // ACCESSORS
    ধ্রুব string& name() ধ্রুব { প্রত্যাগম m_name; }
    uint64_t point_অঙ্ক() ধ্রুব { প্রত্যাগম m_point_অঙ্ক; }
    uint64_t testsCovering() ধ্রুব { প্রত্যাগম m_testsCovering; }
    শূন্য গুনতি_Inc(uint64_t inc) { m_গুনতি += inc; }
    uint64_t গুনতি() ধ্রুব { প্রত্যাগম m_গুনতি; }
    শূন্য testsCoveringInc() { m_testsCovering++; }
    bool ok(unsigned annotateMin) ধ্রুব {
        ধ্রুব std::string threshStr = thresh();
        ধ্রুব unsigned threshi = !threshStr.empty() ? std::atoi(threshStr.c_str()) : annotateMin;
        প্রত্যাগম m_গুনতি >= threshi;
    }
    // KEY ACCESSORS
    string filename() ধ্রুব { প্রত্যাগম চাবি_Extract(VL_CIK_FILENAME, m_name.c_str()); }
    string comment() ধ্রুব { প্রত্যাগম চাবি_Extract(VL_CIK_COMMENT, m_name.c_str()); }
    string hier() ধ্রুব { প্রত্যাগম চাবি_Extract(VL_CIK_HIER, m_name.c_str()); }
    string page() ধ্রুব { প্রত্যাগম চাবি_Extract("page", m_name.c_str()); }
    string type() ধ্রুব { প্রত্যাগম typeExtract(m_name.c_str()); }
    // Covergroup-specific key accessors
    string bin() ধ্রুব { প্রত্যাগম চাবি_Extract(VL_CIK_BIN, m_name.c_str()); }
    string binType() ধ্রুব { প্রত্যাগম চাবি_Extract(VL_CIK_BIN_TYPE, m_name.c_str()); }
    bool isCross() ধ্রুব { প্রত্যাগম !চাবি_Extract(VL_CIK_CROSS, m_name.c_str()).empty(); }
    string crossBins() ধ্রুব { প্রত্যাগম চাবি_Extract(VL_CIK_CROSS_BINS, m_name.c_str()); }
    // The covergroup's type_option.weight; "" for the default 1
    string groupWeight() ধ্রুব { প্রত্যাগম চাবি_Extract(VL_CIK_GROUP_WEIGHT, m_name.c_str()); }
    // The coverpoint's or cross's option.weight; "" for the default 1
    string weight() ধ্রুব { প্রত্যাগম চাবি_Extract(VL_CIK_WEIGHT, m_name.c_str()); }
    string thresh() ধ্রুব {
        // string as maybe ""
        প্রত্যাগম চাবি_Extract(VL_CIK_THRESH, m_name.c_str());
    }
    string linescov() ধ্রুব { প্রত্যাগম চাবি_Extract(VL_CIK_LINESCOV, m_name.c_str()); }
    bool isFsmState() ধ্রুব { প্রত্যাগম type() == "fsm_state"; }
    bool isFsmArc() ধ্রুব { প্রত্যাগম type() == "fsm_arc"; }
    // Arc-specific helpers are used after callers have already filtered to
    // FSM arc points, so they do not repeat the type check here.
    string fsmVarName() ধ্রুব { প্রত্যাগম চাবি_Extract(VL_CIK_FSM_VAR, m_name.c_str()); }
    string fsmFromState() ধ্রুব { প্রত্যাগম চাবি_Extract(VL_CIK_FSM_FROM, m_name.c_str()); }
    string fsmToState() ধ্রুব { প্রত্যাগম চাবি_Extract(VL_CIK_FSM_TO, m_name.c_str()); }
    string fsmTag() ধ্রুব { প্রত্যাগম চাবি_Extract(VL_CIK_FSM_TAG, m_name.c_str()); }
    bool isFsmResetInclude() ধ্রুব { প্রত্যাগম fsmTag() == "reset_include"; }
    bool isFsmResetArc() ধ্রুব { প্রত্যাগম fsmTag() == "reset"; }
    bool isFsmDefaultArc() ধ্রুব { প্রত্যাগম fsmTag() == "default"; }
    bool fsmIsReset() ধ্রুব { প্রত্যাগম isFsmResetArc() || isFsmResetInclude(); }
    পূর্ণাঙ্ক lineno() ধ্রুব {
        ধ্রুব string সারি_Str = চাবি_Extract(VL_CIK_LINENO, m_name.c_str());
        প্রত্যাগম std::atoi(সারি_Str.c_str());
    }
    পূর্ণাঙ্ক column() ধ্রুব {
        ধ্রুব string columnStr = চাবি_Extract(VL_CIK_COLUMN, m_name.c_str());
        প্রত্যাগম std::atoi(columnStr.c_str());
    }
    // METHODS
    স্থির string typeExtract(ধ্রুব বর্ণ* name) { প্রত্যাগম চাবি_Extract(VL_CIK_TYPE, name); }
    // The value of a key of a record, as the design writes it, so without the escapes of the
    // coverage file, VerilatedCovKey::escape(), of characters that print, see unescape(); only
    // the record's name() keeps them all, so that --write writes it back as read
    স্থির string চাবি_Extract(ধ্রুব বর্ণ* short_চাবি, ধ্রুব বর্ণ* name) {
        // Hot function
        ধ্রুব size_t shortLen = std::strlen(short_চাবি);
        প্রতি (ধ্রুব বর্ণ* cp = name; *cp; ++cp) {
            যদি (*cp == '\001') {
                যদি (0 == std::strncmp(cp + 1, short_চাবি, shortLen) && cp[shortLen + 1] == '\002') {
                    cp += shortLen + 2;  // Skip \001+short+\002
                    ধ্রুব বর্ণ* ep = cp;
                    যাবত্ (*ep && *ep != '\001') ++ep;
                    প্রত্যাগম Verilated_Cov_চাবি::unescape(string(cp, ep - cp));
                }
            }
        }
        প্রত্যাগম "";
    }
    স্থির শূন্য dumpHeader(std::ostream& os) {
        os << "Points:\n";
        os << "  Num,    TestsCover,    Count,  Name\n";
    }
    শূন্য dump(std::ostream& os) ধ্রুব {
        os << "  " << std::setw(8) << std::setfill('0') << point_অঙ্ক();
        os << ",  " << std::setw(7) << std::setfill(' ') << testsCovering();
        os << ",  " << std::setw(7) << std::setfill(' ') << গুনতি();
        os << ",  \"" << name() << "\"\n";
    }
    শূন্য dumpAnnotate(std::ostream& os, unsigned annotateMin) ধ্রুব {
        os << (ok(annotateMin) ? "+" : "-");
        os << std::setw(6) << std::setfill('0') << গুনতি();
        os << "  point: type=" << type() << " comment=" << comment() << " hier=" << hier();
        os << "\n";
        যদি (isCross()) {
            ধ্রুব string bins = crossBins();
            যদি (!bins.empty()) {
                os << "        //  cross: [";
                bool first = true;
                size_t start = 0;
                যাবত্ (true) {
                    ধ্রুব size_t comma = bins.find(',', start);
                    যদি (!first) os << ", ";
                    os << bins.substr(start, comma == string::npos ? string::npos : comma - start);
                    first = false;
                    যদি (comma == string::npos) ভঙ্গ;
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
    using NameMap = std::map<ধ্রুব std::string, uint64_t>;  // Sorted by name (ordered)
    NameMap m_nameMap;  //< Name to point-number
    std::vector<VlcPoint> m_points;  //< List of all points
    uint64_t m_অঙ্ক_Points = 0;  //< Total unique points

    স্থির পূর্ণাঙ্ক debug() { প্রত্যাগম V3Error::debugDefault(); }

সার্বজনিক:
    // ITERATORS
    using ByName = NameMap;
    using iterator = ByName::iterator;
    ByName::iterator begin() { প্রত্যাগম m_nameMap.begin(); }
    ByName::iterator end() { প্রত্যাগম m_nameMap.end(); }

    // CONSTRUCTORS
    VlcPoints() = স্বভাব;
    ~VlcPoints() = স্বভাব;

    // METHODS
    শূন্য dump() {
        UINFO(2, "dumpPoints...");
        VlcPoint::dumpHeader(std::cout);
        প্রতি (ধ্রুব auto& i : *this) {
            ধ্রুব VlcPoint& point = point_সংখ্যা(i.second);
            point.dump(std::cout);
        }
    }
    VlcPoint& point_সংখ্যা(uint64_t অঙ্ক) { প্রত্যাগম m_points[অঙ্ক]; }
    uint64_t findAddPoint(ধ্রুব string& name, uint64_t গুনতি) {
        ধ্রুব auto pair = m_nameMap.emplace(name, m_অঙ্ক_Points);
        যদি (pair.second) m_points.emplace_back(name, m_অঙ্ক_Points++);
        ধ্রুব uint64_t pointnum = pair.first->second;
        m_points[pointnum].গুনতি_Inc(গুনতি);
        প্রত্যাগম pointnum;
    }
};

//######################################################################

#endif  // guard
