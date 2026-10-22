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

সর্বজনীন:
    // CONSTRUCTORS
    VlcPoint(ধ্রুবক string& name, uint64_t point_অঙ্ক)
        : m_name{name}
        , m_point_অঙ্ক{point_অঙ্ক} {}
    ~VlcPoint() = পূর্বনির্ধারিত;
    // ACCESSORS
    ধ্রুবক string& name() ধ্রুবক { ফেরত m_name; }
    uint64_t point_অঙ্ক() ধ্রুবক { ফেরত m_point_অঙ্ক; }
    uint64_t testsCovering() ধ্রুবক { ফেরত m_testsCovering; }
    শূন্য গুনতি_Inc(uint64_t inc) { m_গুনতি += inc; }
    uint64_t গুনতি() ধ্রুবক { ফেরত m_গুনতি; }
    শূন্য testsCoveringInc() { m_testsCovering++; }
    bool ok(unsigned annotateMin) ধ্রুবক {
        ধ্রুবক std::string threshStr = thresh();
        ধ্রুবক unsigned threshi = !threshStr.empty() ? std::atoi(threshStr.c_str()) : annotateMin;
        ফেরত m_গুনতি >= threshi;
    }
    // KEY ACCESSORS
    string filename() ধ্রুবক { ফেরত চাবি_Extract(VL_CIK_FILENAME, m_name.c_str()); }
    string comment() ধ্রুবক { ফেরত চাবি_Extract(VL_CIK_COMMENT, m_name.c_str()); }
    string hier() ধ্রুবক { ফেরত চাবি_Extract(VL_CIK_HIER, m_name.c_str()); }
    string page() ধ্রুবক { ফেরত চাবি_Extract("page", m_name.c_str()); }
    string type() ধ্রুবক { ফেরত typeExtract(m_name.c_str()); }
    // Covergroup-specific key accessors
    string bin() ধ্রুবক { ফেরত চাবি_Extract(VL_CIK_BIN, m_name.c_str()); }
    string binType() ধ্রুবক { ফেরত চাবি_Extract(VL_CIK_BIN_TYPE, m_name.c_str()); }
    bool isCross() ধ্রুবক { ফেরত !চাবি_Extract(VL_CIK_CROSS, m_name.c_str()).empty(); }
    string crossBins() ধ্রুবক { ফেরত চাবি_Extract(VL_CIK_CROSS_BINS, m_name.c_str()); }
    // The covergroup's type_option.weight; "" for the default 1
    string groupWeight() ধ্রুবক { ফেরত চাবি_Extract(VL_CIK_GROUP_WEIGHT, m_name.c_str()); }
    // The coverpoint's or cross's option.weight; "" for the default 1
    string weight() ধ্রুবক { ফেরত চাবি_Extract(VL_CIK_WEIGHT, m_name.c_str()); }
    string thresh() ধ্রুবক {
        // string as maybe ""
        ফেরত চাবি_Extract(VL_CIK_THRESH, m_name.c_str());
    }
    string linescov() ধ্রুবক { ফেরত চাবি_Extract(VL_CIK_LINESCOV, m_name.c_str()); }
    bool isFsmState() ধ্রুবক { ফেরত type() == "fsm_state"; }
    bool isFsmArc() ধ্রুবক { ফেরত type() == "fsm_arc"; }
    // Arc-specific helpers are used after callers have already filtered to
    // FSM arc points, so they do not repeat the type check here.
    string fsmVarName() ধ্রুবক { ফেরত চাবি_Extract(VL_CIK_FSM_VAR, m_name.c_str()); }
    string fsmFromState() ধ্রুবক { ফেরত চাবি_Extract(VL_CIK_FSM_FROM, m_name.c_str()); }
    string fsmToState() ধ্রুবক { ফেরত চাবি_Extract(VL_CIK_FSM_TO, m_name.c_str()); }
    string fsmTag() ধ্রুবক { ফেরত চাবি_Extract(VL_CIK_FSM_TAG, m_name.c_str()); }
    bool isFsmResetInclude() ধ্রুবক { ফেরত fsmTag() == "reset_include"; }
    bool isFsmResetArc() ধ্রুবক { ফেরত fsmTag() == "reset"; }
    bool isFsmDefaultArc() ধ্রুবক { ফেরত fsmTag() == "default"; }
    bool fsmIsReset() ধ্রুবক { ফেরত isFsmResetArc() || isFsmResetInclude(); }
    পূর্ণসংখ্যা lineno() ধ্রুবক {
        ধ্রুবক string সারি_Str = চাবি_Extract(VL_CIK_LINENO, m_name.c_str());
        ফেরত std::atoi(সারি_Str.c_str());
    }
    পূর্ণসংখ্যা column() ধ্রুবক {
        ধ্রুবক string columnStr = চাবি_Extract(VL_CIK_COLUMN, m_name.c_str());
        ফেরত std::atoi(columnStr.c_str());
    }
    // METHODS
    স্থির string typeExtract(ধ্রুবক অক্ষর* name) { ফেরত চাবি_Extract(VL_CIK_TYPE, name); }
    // The value of a key of a record, as the design writes it, so without the escapes of the
    // coverage file, VerilatedCovKey::escape(), of characters that print, see unescape(); only
    // the record's name() keeps them all, so that --write writes it back as read
    স্থির string চাবি_Extract(ধ্রুবক অক্ষর* short_চাবি, ধ্রুবক অক্ষর* name) {
        // Hot function
        ধ্রুবক আকার_t shortLen = std::strlen(short_চাবি);
        জন্য (ধ্রুবক অক্ষর* cp = name; *cp; ++cp) {
            যদি (*cp == '\001') {
                যদি (0 == std::strncmp(cp + 1, short_চাবি, shortLen) && cp[shortLen + 1] == '\002') {
                    cp += shortLen + 2;  // Skip \001+short+\002
                    ধ্রুবক অক্ষর* ep = cp;
                    যখন (*ep && *ep != '\001') ++ep;
                    ফেরত Verilated_Cov_চাবি::unescape(string(cp, ep - cp));
                }
            }
        }
        ফেরত "";
    }
    স্থির শূন্য dumpHeader(std::ostream& os) {
        os << "Points:\n";
        os << "  Num,    TestsCover,    Count,  Name\n";
    }
    শূন্য dump(std::ostream& os) ধ্রুবক {
        os << "  " << std::setw(8) << std::setfill('0') << point_অঙ্ক();
        os << ",  " << std::setw(7) << std::setfill(' ') << testsCovering();
        os << ",  " << std::setw(7) << std::setfill(' ') << গুনতি();
        os << ",  \"" << name() << "\"\n";
    }
    শূন্য dumpAnnotate(std::ostream& os, unsigned annotateMin) ধ্রুবক {
        os << (ok(annotateMin) ? "+" : "-");
        os << std::setw(6) << std::setfill('0') << গুনতি();
        os << "  point: type=" << type() << " comment=" << comment() << " hier=" << hier();
        os << "\n";
        যদি (isCross()) {
            ধ্রুবক string bins = crossBins();
            যদি (!bins.empty()) {
                os << "        //  cross: [";
                bool first = true;
                আকার_t start = 0;
                যখন (true) {
                    ধ্রুবক আকার_t comma = bins.find(',', start);
                    যদি (!first) os << ", ";
                    os << bins.substr(start, comma == string::npos ? string::npos : comma - start);
                    first = false;
                    যদি (comma == string::npos) বিরতি;
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
    using NameMap = std::map<ধ্রুবক std::string, uint64_t>;  // Sorted by name (ordered)
    NameMap m_nameMap;  //< Name to point-number
    std::vector<VlcPoint> m_points;  //< List of all points
    uint64_t m_অঙ্ক_Points = 0;  //< Total unique points

    স্থির পূর্ণসংখ্যা debug() { ফেরত V3Error::debugDefault(); }

সর্বজনীন:
    // ITERATORS
    using ByName = NameMap;
    using iterator = ByName::iterator;
    ByName::iterator begin() { ফেরত m_nameMap.begin(); }
    ByName::iterator end() { ফেরত m_nameMap.end(); }

    // CONSTRUCTORS
    VlcPoints() = পূর্বনির্ধারিত;
    ~VlcPoints() = পূর্বনির্ধারিত;

    // METHODS
    শূন্য dump() {
        UINFO(2, "dumpPoints...");
        VlcPoint::dumpHeader(std::cout);
        জন্য (ধ্রুবক auto& i : *this) {
            ধ্রুবক VlcPoint& point = point_সংখ্যা(i.second);
            point.dump(std::cout);
        }
    }
    VlcPoint& point_সংখ্যা(uint64_t অঙ্ক) { ফেরত m_points[অঙ্ক]; }
    uint64_t findAddPoint(ধ্রুবক string& name, uint64_t গুনতি) {
        ধ্রুবক auto pair = m_nameMap.emplace(name, m_অঙ্ক_Points);
        যদি (pair.second) m_points.emplace_back(name, m_অঙ্ক_Points++);
        ধ্রুবক uint64_t pointnum = pair.first->second;
        m_points[pointnum].গুনতি_Inc(গুনতি);
        ফেরত pointnum;
    }
};

//######################################################################

#endif  // guard
