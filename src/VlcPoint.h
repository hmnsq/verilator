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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ <iomanip>
#અન્તર્ભાવ <map>
#અન્તર્ભાવ <unordered_map>
#અન્તર્ભાવ <vector>

#ifndef V3ERROR_NO_GLOBAL_
#define V3ERROR_NO_GLOBAL_
#endif
#અન્તર્ભાવ "verilated_cov_key.h"

#અન્તર્ભાવ "V3Error.h"

//********************************************************************
// VlcPoint - A coverage point (across all tests)

class VlcPoint final {
    // MEMBERS
    string m_name;  //< Name of the point
    uint64_t m_point_અંક;  //< Point number
    uint64_t m_testsCovering = 0;  //< Number tests with non-zero coverage of this point
    uint64_t m_ગણતરી = 0;  //< Count of hits across all tests

જાહેર:
    // CONSTRUCTORS
    VlcPoint(અચલ string& name, uint64_t point_અંક)
        : m_name{name}
        , m_point_અંક{point_અંક} {}
    ~VlcPoint() = મૂળભૂત;
    // ACCESSORS
    અચલ string& name() અચલ { પાછા m_name; }
    uint64_t point_અંક() અચલ { પાછા m_point_અંક; }
    uint64_t testsCovering() અચલ { પાછા m_testsCovering; }
    શૂન્ય ગણતરી_Inc(uint64_t inc) { m_ગણતરી += inc; }
    uint64_t ગણતરી() અચલ { પાછા m_ગણતરી; }
    શૂન્ય testsCoveringInc() { m_testsCovering++; }
    bool ok(unsigned annotateMin) અચલ {
        અચલ std::string threshStr = thresh();
        અચલ unsigned threshi = !threshStr.empty() ? std::atoi(threshStr.c_str()) : annotateMin;
        પાછા m_ગણતરી >= threshi;
    }
    // KEY ACCESSORS
    string filename() અચલ { પાછા ચાવી_Extract(VL_CIK_FILENAME, m_name.c_str()); }
    string comment() અચલ { પાછા ચાવી_Extract(VL_CIK_COMMENT, m_name.c_str()); }
    string hier() અચલ { પાછા ચાવી_Extract(VL_CIK_HIER, m_name.c_str()); }
    string page() અચલ { પાછા ચાવી_Extract("page", m_name.c_str()); }
    string type() અચલ { પાછા typeExtract(m_name.c_str()); }
    // Covergroup-specific key accessors
    string bin() અચલ { પાછા ચાવી_Extract(VL_CIK_BIN, m_name.c_str()); }
    string binType() અચલ { પાછા ચાવી_Extract(VL_CIK_BIN_TYPE, m_name.c_str()); }
    bool isCross() અચલ { પાછા !ચાવી_Extract(VL_CIK_CROSS, m_name.c_str()).empty(); }
    string crossBins() અચલ { પાછા ચાવી_Extract(VL_CIK_CROSS_BINS, m_name.c_str()); }
    // The covergroup's type_option.weight; "" for the default 1
    string groupWeight() અચલ { પાછા ચાવી_Extract(VL_CIK_GROUP_WEIGHT, m_name.c_str()); }
    // The coverpoint's or cross's option.weight; "" for the default 1
    string weight() અચલ { પાછા ચાવી_Extract(VL_CIK_WEIGHT, m_name.c_str()); }
    string thresh() અચલ {
        // string as maybe ""
        પાછા ચાવી_Extract(VL_CIK_THRESH, m_name.c_str());
    }
    string linescov() અચલ { પાછા ચાવી_Extract(VL_CIK_LINESCOV, m_name.c_str()); }
    bool isFsmState() અચલ { પાછા type() == "fsm_state"; }
    bool isFsmArc() અચલ { પાછા type() == "fsm_arc"; }
    // Arc-specific helpers are used after callers have already filtered to
    // FSM arc points, so they do not repeat the type check here.
    string fsmVarName() અચલ { પાછા ચાવી_Extract(VL_CIK_FSM_VAR, m_name.c_str()); }
    string fsmFromState() અચલ { પાછા ચાવી_Extract(VL_CIK_FSM_FROM, m_name.c_str()); }
    string fsmToState() અચલ { પાછા ચાવી_Extract(VL_CIK_FSM_TO, m_name.c_str()); }
    string fsmTag() અચલ { પાછા ચાવી_Extract(VL_CIK_FSM_TAG, m_name.c_str()); }
    bool isFsmResetInclude() અચલ { પાછા fsmTag() == "reset_include"; }
    bool isFsmResetArc() અચલ { પાછા fsmTag() == "reset"; }
    bool isFsmDefaultArc() અચલ { પાછા fsmTag() == "default"; }
    bool fsmIsReset() અચલ { પાછા isFsmResetArc() || isFsmResetInclude(); }
    પૂર્ણાંક lineno() અચલ {
        અચલ string લીટી_Str = ચાવી_Extract(VL_CIK_LINENO, m_name.c_str());
        પાછા std::atoi(લીટી_Str.c_str());
    }
    પૂર્ણાંક column() અચલ {
        અચલ string columnStr = ચાવી_Extract(VL_CIK_COLUMN, m_name.c_str());
        પાછા std::atoi(columnStr.c_str());
    }
    // METHODS
    સ્થિર string typeExtract(અચલ અક્ષર* name) { પાછા ચાવી_Extract(VL_CIK_TYPE, name); }
    // The value of a key of a record, as the design writes it, so without the escapes of the
    // coverage file, VerilatedCovKey::escape(), of characters that print, see unescape(); only
    // the record's name() keeps them all, so that --write writes it back as read
    સ્થિર string ચાવી_Extract(અચલ અક્ષર* short_ચાવી, અચલ અક્ષર* name) {
        // Hot function
        અચલ size_t shortLen = std::strlen(short_ચાવી);
        માટે (અચલ અક્ષર* cp = name; *cp; ++cp) {
            જો (*cp == '\001') {
                જો (0 == std::strncmp(cp + 1, short_ચાવી, shortLen) && cp[shortLen + 1] == '\002') {
                    cp += shortLen + 2;  // Skip \001+short+\002
                    અચલ અક્ષર* ep = cp;
                    જ્યારે (*ep && *ep != '\001') ++ep;
                    પાછા Verilated_Cov_ચાવી::unescape(string(cp, ep - cp));
                }
            }
        }
        પાછા "";
    }
    સ્થિર શૂન્ય dumpHeader(std::ostream& os) {
        os << "Points:\n";
        os << "  Num,    TestsCover,    Count,  Name\n";
    }
    શૂન્ય dump(std::ostream& os) અચલ {
        os << "  " << std::setw(8) << std::setfill('0') << point_અંક();
        os << ",  " << std::setw(7) << std::setfill(' ') << testsCovering();
        os << ",  " << std::setw(7) << std::setfill(' ') << ગણતરી();
        os << ",  \"" << name() << "\"\n";
    }
    શૂન્ય dumpAnnotate(std::ostream& os, unsigned annotateMin) અચલ {
        os << (ok(annotateMin) ? "+" : "-");
        os << std::setw(6) << std::setfill('0') << ગણતરી();
        os << "  point: type=" << type() << " comment=" << comment() << " hier=" << hier();
        os << "\n";
        જો (isCross()) {
            અચલ string bins = crossBins();
            જો (!bins.empty()) {
                os << "        //  cross: [";
                bool first = true;
                size_t start = 0;
                જ્યારે (true) {
                    અચલ size_t comma = bins.find(',', start);
                    જો (!first) os << ", ";
                    os << bins.substr(start, comma == string::npos ? string::npos : comma - start);
                    first = false;
                    જો (comma == string::npos) તોડો;
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
    using NameMap = std::map<અચલ std::string, uint64_t>;  // Sorted by name (ordered)
    NameMap m_nameMap;  //< Name to point-number
    std::vector<VlcPoint> m_points;  //< List of all points
    uint64_t m_અંક_Points = 0;  //< Total unique points

    સ્થિર પૂર્ણાંક debug() { પાછા V3Error::debugDefault(); }

જાહેર:
    // ITERATORS
    using ByName = NameMap;
    using iterator = ByName::iterator;
    ByName::iterator begin() { પાછા m_nameMap.begin(); }
    ByName::iterator end() { પાછા m_nameMap.end(); }

    // CONSTRUCTORS
    VlcPoints() = મૂળભૂત;
    ~VlcPoints() = મૂળભૂત;

    // METHODS
    શૂન્ય dump() {
        UINFO(2, "dumpPoints...");
        VlcPoint::dumpHeader(std::cout);
        માટે (અચલ auto& i : *this) {
            અચલ VlcPoint& point = point_સંખ્યા(i.second);
            point.dump(std::cout);
        }
    }
    VlcPoint& point_સંખ્યા(uint64_t અંક) { પાછા m_points[અંક]; }
    uint64_t findAddPoint(અચલ string& name, uint64_t ગણતરી) {
        અચલ auto pair = m_nameMap.emplace(name, m_અંક_Points);
        જો (pair.second) m_points.emplace_back(name, m_અંક_Points++);
        અચલ uint64_t pointnum = pair.first->second;
        m_points[pointnum].ગણતરી_Inc(ગણતરી);
        પાછા pointnum;
    }
};

//######################################################################

#endif  // guard
