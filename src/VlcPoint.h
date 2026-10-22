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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ <iomanip>
#ଅନ୍ତର୍ଭାଵ <map>
#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <vector>

#ifndef V3ERROR_NO_GLOBAL_
#define V3ERROR_NO_GLOBAL_
#endif
#ଅନ୍ତର୍ଭାଵ "verilated_cov_key.h"

#ଅନ୍ତର୍ଭାଵ "V3Error.h"

//********************************************************************
// VlcPoint - A coverage point (across all tests)

class VlcPoint final {
    // MEMBERS
    string m_name;  //< Name of the point
    uint64_t m_point_ଅଙ୍କ;  //< Point number
    uint64_t m_testsCovering = 0;  //< Number tests with non-zero coverage of this point
    uint64_t m_ଗଣନା = 0;  //< Count of hits across all tests

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    VlcPoint(ଅଚଳ string& name, uint64_t point_ଅଙ୍କ)
        : m_name{name}
        , m_point_ଅଙ୍କ{point_ଅଙ୍କ} {}
    ~VlcPoint() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    // ACCESSORS
    ଅଚଳ string& name() ଅଚଳ { ଫେରନ୍ତୁ m_name; }
    uint64_t point_ଅଙ୍କ() ଅଚଳ { ଫେରନ୍ତୁ m_point_ଅଙ୍କ; }
    uint64_t testsCovering() ଅଚଳ { ଫେରନ୍ତୁ m_testsCovering; }
    ଶୂନ୍ୟ ଗଣନା_Inc(uint64_t inc) { m_ଗଣନା += inc; }
    uint64_t ଗଣନା_2() ଅଚଳ { ଫେରନ୍ତୁ m_ଗଣନା; }
    ଶୂନ୍ୟ testsCoveringInc() { m_testsCovering++; }
    bool ok(unsigned annotateMin) ଅଚଳ {
        ଅଚଳ std::string threshStr = thresh();
        ଅଚଳ unsigned threshi = !threshStr.empty() ? std::atoi(threshStr.c_str()) : annotateMin;
        ଫେରନ୍ତୁ m_ଗଣନା >= threshi;
    }
    // KEY ACCESSORS
    string filename() ଅଚଳ { ଫେରନ୍ତୁ ଚାବି_Extract(VL_CIK_FILENAME, m_name.c_str()); }
    string comment() ଅଚଳ { ଫେରନ୍ତୁ ଚାବି_Extract(VL_CIK_COMMENT, m_name.c_str()); }
    string hier() ଅଚଳ { ଫେରନ୍ତୁ ଚାବି_Extract(VL_CIK_HIER, m_name.c_str()); }
    string page() ଅଚଳ { ଫେରନ୍ତୁ ଚାବି_Extract("page", m_name.c_str()); }
    string type() ଅଚଳ { ଫେରନ୍ତୁ typeExtract(m_name.c_str()); }
    // Covergroup-specific key accessors
    string bin() ଅଚଳ { ଫେରନ୍ତୁ ଚାବି_Extract(VL_CIK_BIN, m_name.c_str()); }
    string binType() ଅଚଳ { ଫେରନ୍ତୁ ଚାବି_Extract(VL_CIK_BIN_TYPE, m_name.c_str()); }
    bool isCross() ଅଚଳ { ଫେରନ୍ତୁ !ଚାବି_Extract(VL_CIK_CROSS, m_name.c_str()).empty(); }
    string crossBins() ଅଚଳ { ଫେରନ୍ତୁ ଚାବି_Extract(VL_CIK_CROSS_BINS, m_name.c_str()); }
    // The covergroup's type_option.weight; "" for the default 1
    string groupWeight() ଅଚଳ { ଫେରନ୍ତୁ ଚାବି_Extract(VL_CIK_GROUP_WEIGHT, m_name.c_str()); }
    // The coverpoint's or cross's option.weight; "" for the default 1
    string weight() ଅଚଳ { ଫେରନ୍ତୁ ଚାବି_Extract(VL_CIK_WEIGHT, m_name.c_str()); }
    string thresh() ଅଚଳ {
        // string as maybe ""
        ଫେରନ୍ତୁ ଚାବି_Extract(VL_CIK_THRESH, m_name.c_str());
    }
    string linescov() ଅଚଳ { ଫେରନ୍ତୁ ଚାବି_Extract(VL_CIK_LINESCOV, m_name.c_str()); }
    bool isFsmState() ଅଚଳ { ଫେରନ୍ତୁ type() == "fsm_state"; }
    bool isFsmArc() ଅଚଳ { ଫେରନ୍ତୁ type() == "fsm_arc"; }
    // Arc-specific helpers are used after callers have already filtered to
    // FSM arc points, so they do not repeat the type check here.
    string fsmVarName() ଅଚଳ { ଫେରନ୍ତୁ ଚାବି_Extract(VL_CIK_FSM_VAR, m_name.c_str()); }
    string fsmFromState() ଅଚଳ { ଫେରନ୍ତୁ ଚାବି_Extract(VL_CIK_FSM_FROM, m_name.c_str()); }
    string fsmToState() ଅଚଳ { ଫେରନ୍ତୁ ଚାବି_Extract(VL_CIK_FSM_TO, m_name.c_str()); }
    string fsmTag() ଅଚଳ { ଫେରନ୍ତୁ ଚାବି_Extract(VL_CIK_FSM_TAG, m_name.c_str()); }
    bool isFsmResetInclude() ଅଚଳ { ଫେରନ୍ତୁ fsmTag() == "reset_include"; }
    bool isFsmResetArc() ଅଚଳ { ଫେରନ୍ତୁ fsmTag() == "reset"; }
    bool isFsmDefaultArc() ଅଚଳ { ଫେରନ୍ତୁ fsmTag() == "default"; }
    bool fsmIsReset() ଅଚଳ { ଫେରନ୍ତୁ isFsmResetArc() || isFsmResetInclude(); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା lineno() ଅଚଳ {
        ଅଚଳ string ଧାଡ଼ି_Str = ଚାବି_Extract(VL_CIK_LINENO, m_name.c_str());
        ଫେରନ୍ତୁ std::atoi(ଧାଡ଼ି_Str.c_str());
    }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା column() ଅଚଳ {
        ଅଚଳ string columnStr = ଚାବି_Extract(VL_CIK_COLUMN, m_name.c_str());
        ଫେରନ୍ତୁ std::atoi(columnStr.c_str());
    }
    // METHODS
    ସ୍ଥିର string typeExtract(ଅଚଳ ଅକ୍ଷର* name) { ଫେରନ୍ତୁ ଚାବି_Extract(VL_CIK_TYPE, name); }
    // The value of a key of a record, as the design writes it, so without the escapes of the
    // coverage file, VerilatedCovKey::escape(), of characters that print, see unescape(); only
    // the record's name() keeps them all, so that --write writes it back as read
    ସ୍ଥିର string ଚାବି_Extract(ଅଚଳ ଅକ୍ଷର* short_ଚାବି, ଅଚଳ ଅକ୍ଷର* name) {
        // Hot function
        ଅଚଳ size_t shortLen = std::strlen(short_ଚାବି);
        ପାଇଁ (ଅଚଳ ଅକ୍ଷର* cp = name; *cp; ++cp) {
            ଯଦି (*cp == '\001') {
                ଯଦି (0 == std::strncmp(cp + 1, short_ଚାବି, shortLen) && cp[shortLen + 1] == '\002') {
                    cp += shortLen + 2;  // Skip \001+short+\002
                    ଅଚଳ ଅକ୍ଷର* ep = cp;
                    ଯେତେବେଳେ (*ep && *ep != '\001') ++ep;
                    ଫେରନ୍ତୁ Verilated_Cov_ଚାବି::unescape(string(cp, ep - cp));
                }
            }
        }
        ଫେରନ୍ତୁ "";
    }
    ସ୍ଥିର ଶୂନ୍ୟ dumpHeader(std::ostream& os) {
        os << "Points:\n";
        os << "  Num,    TestsCover,    Count,  Name\n";
    }
    ଶୂନ୍ୟ dump(std::ostream& os) ଅଚଳ {
        os << "  " << std::setw(8) << std::setfill('0') << point_ଅଙ୍କ();
        os << ",  " << std::setw(7) << std::setfill(' ') << testsCovering();
        os << ",  " << std::setw(7) << std::setfill(' ') << ଗଣନା_2();
        os << ",  \"" << name() << "\"\n";
    }
    ଶୂନ୍ୟ dumpAnnotate(std::ostream& os, unsigned annotateMin) ଅଚଳ {
        os << (ok(annotateMin) ? "+" : "-");
        os << std::setw(6) << std::setfill('0') << ଗଣନା_2();
        os << "  point: type=" << type() << " comment=" << comment() << " hier=" << hier();
        os << "\n";
        ଯଦି (isCross()) {
            ଅଚଳ string bins = crossBins();
            ଯଦି (!bins.empty()) {
                os << "        //  cross: [";
                bool first = true;
                size_t start = 0;
                ଯେତେବେଳେ (true) {
                    ଅଚଳ size_t comma = bins.find(',', start);
                    ଯଦି (!first) os << ", ";
                    os << bins.substr(start, comma == string::npos ? string::npos : comma - start);
                    first = false;
                    ଯଦି (comma == string::npos) ଭାଙ୍ଗ;
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
    using NameMap = std::map<ଅଚଳ std::string, uint64_t>;  // Sorted by name (ordered)
    NameMap m_nameMap;  //< Name to point-number
    std::vector<VlcPoint> m_points;  //< List of all points
    uint64_t m_ଅଙ୍କ_Points = 0;  //< Total unique points

    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା debug() { ଫେରନ୍ତୁ V3Error::debugDefault(); }

ସର୍ବସାଧାରଣ:
    // ITERATORS
    using ByName = NameMap;
    using iterator = ByName::iterator;
    ByName::iterator begin() { ଫେରନ୍ତୁ m_nameMap.begin(); }
    ByName::iterator end() { ଫେରନ୍ତୁ m_nameMap.end(); }

    // CONSTRUCTORS
    VlcPoints() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~VlcPoints() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // METHODS
    ଶୂନ୍ୟ dump() {
        UINFO(2, "dumpPoints...");
        VlcPoint::dumpHeader(std::cout);
        ପାଇଁ (ଅଚଳ auto& i : *this) {
            ଅଚଳ VlcPoint& point = point_ସଂଖ୍ୟା(i.second);
            point.dump(std::cout);
        }
    }
    VlcPoint& point_ସଂଖ୍ୟା(uint64_t ଅଙ୍କ) { ଫେରନ୍ତୁ m_points[ଅଙ୍କ]; }
    uint64_t findAddPoint(ଅଚଳ string& name, uint64_t ଗଣନା_2) {
        ଅଚଳ auto pair = m_nameMap.emplace(name, m_ଅଙ୍କ_Points);
        ଯଦି (pair.second) m_points.emplace_back(name, m_ଅଙ୍କ_Points++);
        ଅଚଳ uint64_t pointnum = pair.first->second;
        m_points[pointnum].ଗଣନା_Inc(ଗଣନା_2);
        ଫେରନ୍ତୁ pointnum;
    }
};

//######################################################################

#endif  // guard
