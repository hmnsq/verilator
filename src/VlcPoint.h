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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ <iomanip>
#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <vector>

#ifndef V3ERROR_NO_GLOBAL_
#define V3ERROR_NO_GLOBAL_
#endif
#ಅನ್ತರ್ಭಾವ "verilated_cov_key.h"

#ಅನ್ತರ್ಭಾವ "V3Error.h"

//********************************************************************
// VlcPoint - A coverage point (across all tests)

class VlcPoint final {
    // MEMBERS
    string m_name;  //< Name of the point
    uint64_t m_point_ಅಂಕೆ;  //< Point number
    uint64_t m_testsCovering = 0;  //< Number tests with non-zero coverage of this point
    uint64_t m_ಎಣಿಕೆ = 0;  //< Count of hits across all tests

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    VlcPoint(ಧ್ರುವ string& name, uint64_t point_ಅಂಕೆ)
        : m_name{name}
        , m_point_ಅಂಕೆ{point_ಅಂಕೆ} {}
    ~VlcPoint() = ಸ್ವಭಾವ;
    // ACCESSORS
    ಧ್ರುವ string& name() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_name; }
    uint64_t point_ಅಂಕೆ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_point_ಅಂಕೆ; }
    uint64_t testsCovering() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_testsCovering; }
    ಶೂನ್ಯ ಎಣಿಕೆ_Inc(uint64_t inc) { m_ಎಣಿಕೆ += inc; }
    uint64_t ಎಣಿಕೆ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ಎಣಿಕೆ; }
    ಶೂನ್ಯ testsCoveringInc() { m_testsCovering++; }
    bool ok(unsigned annotateMin) ಧ್ರುವ {
        ಧ್ರುವ std::string threshStr = thresh();
        ಧ್ರುವ unsigned threshi = !threshStr.empty() ? std::atoi(threshStr.c_str()) : annotateMin;
        ಪ್ರತ್ಯಾಗಮ m_ಎಣಿಕೆ >= threshi;
    }
    // KEY ACCESSORS
    string filename() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ಕೀಲಿ_Extract(VL_CIK_FILENAME, m_name.c_str()); }
    string comment() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ಕೀಲಿ_Extract(VL_CIK_COMMENT, m_name.c_str()); }
    string hier() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ಕೀಲಿ_Extract(VL_CIK_HIER, m_name.c_str()); }
    string page() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ಕೀಲಿ_Extract("page", m_name.c_str()); }
    string type() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ typeExtract(m_name.c_str()); }
    // Covergroup-specific key accessors
    string bin() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ಕೀಲಿ_Extract(VL_CIK_BIN, m_name.c_str()); }
    string binType() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ಕೀಲಿ_Extract(VL_CIK_BIN_TYPE, m_name.c_str()); }
    bool isCross() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ !ಕೀಲಿ_Extract(VL_CIK_CROSS, m_name.c_str()).empty(); }
    string crossBins() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ಕೀಲಿ_Extract(VL_CIK_CROSS_BINS, m_name.c_str()); }
    // The covergroup's type_option.weight; "" for the default 1
    string groupWeight() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ಕೀಲಿ_Extract(VL_CIK_GROUP_WEIGHT, m_name.c_str()); }
    // The coverpoint's or cross's option.weight; "" for the default 1
    string weight() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ಕೀಲಿ_Extract(VL_CIK_WEIGHT, m_name.c_str()); }
    string thresh() ಧ್ರುವ {
        // string as maybe ""
        ಪ್ರತ್ಯಾಗಮ ಕೀಲಿ_Extract(VL_CIK_THRESH, m_name.c_str());
    }
    string linescov() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ಕೀಲಿ_Extract(VL_CIK_LINESCOV, m_name.c_str()); }
    bool isFsmState() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ type() == "fsm_state"; }
    bool isFsmArc() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ type() == "fsm_arc"; }
    // Arc-specific helpers are used after callers have already filtered to
    // FSM arc points, so they do not repeat the type check here.
    string fsmVarName() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ಕೀಲಿ_Extract(VL_CIK_FSM_VAR, m_name.c_str()); }
    string fsmFromState() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ಕೀಲಿ_Extract(VL_CIK_FSM_FROM, m_name.c_str()); }
    string fsmToState() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ಕೀಲಿ_Extract(VL_CIK_FSM_TO, m_name.c_str()); }
    string fsmTag() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ಕೀಲಿ_Extract(VL_CIK_FSM_TAG, m_name.c_str()); }
    bool isFsmResetInclude() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ fsmTag() == "reset_include"; }
    bool isFsmResetArc() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ fsmTag() == "reset"; }
    bool isFsmDefaultArc() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ fsmTag() == "default"; }
    bool fsmIsReset() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ isFsmResetArc() || isFsmResetInclude(); }
    ಪೂರ್ಣಾಙ್ಕ lineno() ಧ್ರುವ {
        ಧ್ರುವ string ಸಾಲು_Str = ಕೀಲಿ_Extract(VL_CIK_LINENO, m_name.c_str());
        ಪ್ರತ್ಯಾಗಮ std::atoi(ಸಾಲು_Str.c_str());
    }
    ಪೂರ್ಣಾಙ್ಕ column() ಧ್ರುವ {
        ಧ್ರುವ string columnStr = ಕೀಲಿ_Extract(VL_CIK_COLUMN, m_name.c_str());
        ಪ್ರತ್ಯಾಗಮ std::atoi(columnStr.c_str());
    }
    // METHODS
    ಸ್ಥಿರ string typeExtract(ಧ್ರುವ ವರ್ಣ* name) { ಪ್ರತ್ಯಾಗಮ ಕೀಲಿ_Extract(VL_CIK_TYPE, name); }
    // The value of a key of a record, as the design writes it, so without the escapes of the
    // coverage file, VerilatedCovKey::escape(), of characters that print, see unescape(); only
    // the record's name() keeps them all, so that --write writes it back as read
    ಸ್ಥಿರ string ಕೀಲಿ_Extract(ಧ್ರುವ ವರ್ಣ* short_ಕೀಲಿ, ಧ್ರುವ ವರ್ಣ* name) {
        // Hot function
        ಧ್ರುವ size_t shortLen = std::strlen(short_ಕೀಲಿ);
        ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ* cp = name; *cp; ++cp) {
            ಯದಿ (*cp == '\001') {
                ಯದಿ (0 == std::strncmp(cp + 1, short_ಕೀಲಿ, shortLen) && cp[shortLen + 1] == '\002') {
                    cp += shortLen + 2;  // Skip \001+short+\002
                    ಧ್ರುವ ವರ್ಣ* ep = cp;
                    ಯಾವತ್ (*ep && *ep != '\001') ++ep;
                    ಪ್ರತ್ಯಾಗಮ Verilated_Cov_ಕೀಲಿ::unescape(string(cp, ep - cp));
                }
            }
        }
        ಪ್ರತ್ಯಾಗಮ "";
    }
    ಸ್ಥಿರ ಶೂನ್ಯ dumpHeader(std::ostream& os) {
        os << "Points:\n";
        os << "  Num,    TestsCover,    Count,  Name\n";
    }
    ಶೂನ್ಯ dump(std::ostream& os) ಧ್ರುವ {
        os << "  " << std::setw(8) << std::setfill('0') << point_ಅಂಕೆ();
        os << ",  " << std::setw(7) << std::setfill(' ') << testsCovering();
        os << ",  " << std::setw(7) << std::setfill(' ') << ಎಣಿಕೆ();
        os << ",  \"" << name() << "\"\n";
    }
    ಶೂನ್ಯ dumpAnnotate(std::ostream& os, unsigned annotateMin) ಧ್ರುವ {
        os << (ok(annotateMin) ? "+" : "-");
        os << std::setw(6) << std::setfill('0') << ಎಣಿಕೆ();
        os << "  point: type=" << type() << " comment=" << comment() << " hier=" << hier();
        os << "\n";
        ಯದಿ (isCross()) {
            ಧ್ರುವ string bins = crossBins();
            ಯದಿ (!bins.empty()) {
                os << "        //  cross: [";
                bool first = true;
                size_t start = 0;
                ಯಾವತ್ (true) {
                    ಧ್ರುವ size_t comma = bins.find(',', start);
                    ಯದಿ (!first) os << ", ";
                    os << bins.substr(start, comma == string::npos ? string::npos : comma - start);
                    first = false;
                    ಯದಿ (comma == string::npos) ಭಙ್ಗ;
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
    using NameMap = std::map<ಧ್ರುವ std::string, uint64_t>;  // Sorted by name (ordered)
    NameMap m_nameMap;  //< Name to point-number
    std::vector<VlcPoint> m_points;  //< List of all points
    uint64_t m_ಅಂಕೆ_Points = 0;  //< Total unique points

    ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ debug() { ಪ್ರತ್ಯಾಗಮ V3Error::debugDefault(); }

ಸಾರ್ವಜನಿಕ:
    // ITERATORS
    using ByName = NameMap;
    using iterator = ByName::iterator;
    ByName::iterator begin() { ಪ್ರತ್ಯಾಗಮ m_nameMap.begin(); }
    ByName::iterator end() { ಪ್ರತ್ಯಾಗಮ m_nameMap.end(); }

    // CONSTRUCTORS
    VlcPoints() = ಸ್ವಭಾವ;
    ~VlcPoints() = ಸ್ವಭಾವ;

    // METHODS
    ಶೂನ್ಯ dump() {
        UINFO(2, "dumpPoints...");
        VlcPoint::dumpHeader(std::cout);
        ಪ್ರತಿ (ಧ್ರುವ auto& i : *this) {
            ಧ್ರುವ VlcPoint& point = point_ಸಂಖ್ಯೆ(i.second);
            point.dump(std::cout);
        }
    }
    VlcPoint& point_ಸಂಖ್ಯೆ(uint64_t ಅಂಕೆ) { ಪ್ರತ್ಯಾಗಮ m_points[ಅಂಕೆ]; }
    uint64_t findAddPoint(ಧ್ರುವ string& name, uint64_t ಎಣಿಕೆ) {
        ಧ್ರುವ auto pair = m_nameMap.emplace(name, m_ಅಂಕೆ_Points);
        ಯದಿ (pair.second) m_points.emplace_back(name, m_ಅಂಕೆ_Points++);
        ಧ್ರುವ uint64_t pointnum = pair.first->second;
        m_points[pointnum].ಎಣಿಕೆ_Inc(ಎಣಿಕೆ);
        ಪ್ರತ್ಯಾಗಮ pointnum;
    }
};

//######################################################################

#endif  // guard
