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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن <iomanip>
#ضمن <map>
#ضمن <unordered_map>
#ضمن <vector>

#ifndef V3ERROR_NO_GLOBAL_
#define V3ERROR_NO_GLOBAL_
#endif
#ضمن "verilated_cov_key.h"

#ضمن "V3Error.h"

//********************************************************************
// VlcPoint - A coverage point (across all tests)

class VlcPoint final {
    // MEMBERS
    string m_name;  //< Name of the point
    uint64_t m_point_ژمارە;  //< Point number
    uint64_t m_testsCovering = 0;  //< Number tests with non-zero coverage of this point
    uint64_t m_ژماردن = 0;  //< Count of hits across all tests

عام:
    // CONSTRUCTORS
    VlcPoint(ثابت string& name, uint64_t point_ژمارە)
        : m_name{name}
        , m_point_ژمارە{point_ژمارە} {}
    ~VlcPoint() = افتراضي;
    // ACCESSORS
    ثابت string& name() ثابت { أرجع m_name; }
    uint64_t point_ژمارە() ثابت { أرجع m_point_ژمارە; }
    uint64_t testsCovering() ثابت { أرجع m_testsCovering; }
    فراغ ژماردن_Inc(uint64_t inc) { m_ژماردن += inc; }
    uint64_t ژماردن() ثابت { أرجع m_ژماردن; }
    فراغ testsCoveringInc() { m_testsCovering++; }
    bool ok(unsigned annotateMin) ثابت {
        ثابت std::string threshStr = thresh();
        ثابت unsigned threshi = !threshStr.empty() ? std::atoi(threshStr.c_str()) : annotateMin;
        أرجع m_ژماردن >= threshi;
    }
    // KEY ACCESSORS
    string filename() ثابت { أرجع کلیل_Extract(VL_CIK_FILENAME, m_name.c_str()); }
    string comment() ثابت { أرجع کلیل_Extract(VL_CIK_COMMENT, m_name.c_str()); }
    string hier() ثابت { أرجع کلیل_Extract(VL_CIK_HIER, m_name.c_str()); }
    string page() ثابت { أرجع کلیل_Extract("page", m_name.c_str()); }
    string type() ثابت { أرجع typeExtract(m_name.c_str()); }
    // Covergroup-specific key accessors
    string bin() ثابت { أرجع کلیل_Extract(VL_CIK_BIN, m_name.c_str()); }
    string binType() ثابت { أرجع کلیل_Extract(VL_CIK_BIN_TYPE, m_name.c_str()); }
    bool isCross() ثابت { أرجع !کلیل_Extract(VL_CIK_CROSS, m_name.c_str()).empty(); }
    string crossBins() ثابت { أرجع کلیل_Extract(VL_CIK_CROSS_BINS, m_name.c_str()); }
    // The covergroup's type_option.weight; "" for the default 1
    string groupWeight() ثابت { أرجع کلیل_Extract(VL_CIK_GROUP_WEIGHT, m_name.c_str()); }
    // The coverpoint's or cross's option.weight; "" for the default 1
    string weight() ثابت { أرجع کلیل_Extract(VL_CIK_WEIGHT, m_name.c_str()); }
    string thresh() ثابت {
        // string as maybe ""
        أرجع کلیل_Extract(VL_CIK_THRESH, m_name.c_str());
    }
    string linescov() ثابت { أرجع کلیل_Extract(VL_CIK_LINESCOV, m_name.c_str()); }
    bool isFsmState() ثابت { أرجع type() == "fsm_state"; }
    bool isFsmArc() ثابت { أرجع type() == "fsm_arc"; }
    // Arc-specific helpers are used after callers have already filtered to
    // FSM arc points, so they do not repeat the type check here.
    string fsmVarName() ثابت { أرجع کلیل_Extract(VL_CIK_FSM_VAR, m_name.c_str()); }
    string fsmFromState() ثابت { أرجع کلیل_Extract(VL_CIK_FSM_FROM, m_name.c_str()); }
    string fsmToState() ثابت { أرجع کلیل_Extract(VL_CIK_FSM_TO, m_name.c_str()); }
    string fsmTag() ثابت { أرجع کلیل_Extract(VL_CIK_FSM_TAG, m_name.c_str()); }
    bool isFsmResetInclude() ثابت { أرجع fsmTag() == "reset_include"; }
    bool isFsmResetArc() ثابت { أرجع fsmTag() == "reset"; }
    bool isFsmDefaultArc() ثابت { أرجع fsmTag() == "default"; }
    bool fsmIsReset() ثابت { أرجع isFsmResetArc() || isFsmResetInclude(); }
    صحيح lineno() ثابت {
        ثابت string هێڵ_Str = کلیل_Extract(VL_CIK_LINENO, m_name.c_str());
        أرجع std::atoi(هێڵ_Str.c_str());
    }
    صحيح column() ثابت {
        ثابت string columnStr = کلیل_Extract(VL_CIK_COLUMN, m_name.c_str());
        أرجع std::atoi(columnStr.c_str());
    }
    // METHODS
    ساكن string typeExtract(ثابت حرف* name) { أرجع کلیل_Extract(VL_CIK_TYPE, name); }
    // The value of a key of a record, as the design writes it, so without the escapes of the
    // coverage file, VerilatedCovKey::escape(), of characters that print, see unescape(); only
    // the record's name() keeps them all, so that --write writes it back as read
    ساكن string کلیل_Extract(ثابت حرف* short_کلیل, ثابت حرف* name) {
        // Hot function
        ثابت size_t shortLen = std::strlen(short_کلیل);
        لكل (ثابت حرف* cp = name; *cp; ++cp) {
            إذا (*cp == '\001') {
                إذا (0 == std::strncmp(cp + 1, short_کلیل, shortLen) && cp[shortLen + 1] == '\002') {
                    cp += shortLen + 2;  // Skip \001+short+\002
                    ثابت حرف* ep = cp;
                    طالما (*ep && *ep != '\001') ++ep;
                    أرجع Verilated_Cov_کلیل::unescape(string(cp, ep - cp));
                }
            }
        }
        أرجع "";
    }
    ساكن فراغ dumpHeader(std::ostream& os) {
        os << "Points:\n";
        os << "  Num,    TestsCover,    Count,  Name\n";
    }
    فراغ dump(std::ostream& os) ثابت {
        os << "  " << std::setw(8) << std::setfill('0') << point_ژمارە();
        os << ",  " << std::setw(7) << std::setfill(' ') << testsCovering();
        os << ",  " << std::setw(7) << std::setfill(' ') << ژماردن();
        os << ",  \"" << name() << "\"\n";
    }
    فراغ dumpAnnotate(std::ostream& os, unsigned annotateMin) ثابت {
        os << (ok(annotateMin) ? "+" : "-");
        os << std::setw(6) << std::setfill('0') << ژماردن();
        os << "  point: type=" << type() << " comment=" << comment() << " hier=" << hier();
        os << "\n";
        إذا (isCross()) {
            ثابت string bins = crossBins();
            إذا (!bins.empty()) {
                os << "        //  cross: [";
                bool first = true;
                size_t start = 0;
                طالما (true) {
                    ثابت size_t comma = bins.find(',', start);
                    إذا (!first) os << ", ";
                    os << bins.substr(start, comma == string::npos ? string::npos : comma - start);
                    first = false;
                    إذا (comma == string::npos) اكسر;
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
    using NameMap = std::map<ثابت std::string, uint64_t>;  // Sorted by name (ordered)
    NameMap m_nameMap;  //< Name to point-number
    std::vector<VlcPoint> m_points;  //< List of all points
    uint64_t m_ژمارە_Points = 0;  //< Total unique points

    ساكن صحيح debug() { أرجع V3Error::debugDefault(); }

عام:
    // ITERATORS
    using ByName = NameMap;
    using iterator = ByName::iterator;
    ByName::iterator begin() { أرجع m_nameMap.begin(); }
    ByName::iterator end() { أرجع m_nameMap.end(); }

    // CONSTRUCTORS
    VlcPoints() = افتراضي;
    ~VlcPoints() = افتراضي;

    // METHODS
    فراغ dump() {
        UINFO(2, "dumpPoints...");
        VlcPoint::dumpHeader(std::cout);
        لكل (ثابت auto& i : *this) {
            ثابت VlcPoint& point = point_ژمار(i.second);
            point.dump(std::cout);
        }
    }
    VlcPoint& point_ژمار(uint64_t ژمارە) { أرجع m_points[ژمارە]; }
    uint64_t findAddPoint(ثابت string& name, uint64_t ژماردن) {
        ثابت auto pair = m_nameMap.emplace(name, m_ژمارە_Points);
        إذا (pair.second) m_points.emplace_back(name, m_ژمارە_Points++);
        ثابت uint64_t pointnum = pair.first->second;
        m_points[pointnum].ژماردن_Inc(ژماردن);
        أرجع pointnum;
    }
};

//######################################################################

#endif  // guard
