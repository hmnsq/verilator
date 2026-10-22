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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव <iomanip>
#अन्तर्भाव <map>
#अन्तर्भाव <unordered_map>
#अन्तर्भाव <vector>

#ifndef V3ERROR_NO_GLOBAL_
#define V3ERROR_NO_GLOBAL_
#endif
#अन्तर्भाव "verilated_cov_key.h"

#अन्तर्भाव "V3Error.h"

//********************************************************************
// VlcPoint - A coverage point (across all tests)

class VlcPoint final {
    // MEMBERS
    string m_name;  //< Name of the point
    uint64_t m_point_अंक;  //< Point number
    uint64_t m_testsCovering = 0;  //< Number tests with non-zero coverage of this point
    uint64_t m_मोजणी = 0;  //< Count of hits across all tests

सार्वजनिक:
    // CONSTRUCTORS
    VlcPoint(ध्रुव string& name, uint64_t point_अंक)
        : m_name{name}
        , m_point_अंक{point_अंक} {}
    ~VlcPoint() = स्वभाव;
    // ACCESSORS
    ध्रुव string& name() ध्रुव { प्रत्यागम m_name; }
    uint64_t point_अंक() ध्रुव { प्रत्यागम m_point_अंक; }
    uint64_t testsCovering() ध्रुव { प्रत्यागम m_testsCovering; }
    शून्य मोजणी_Inc(uint64_t inc) { m_मोजणी += inc; }
    uint64_t मोजणी() ध्रुव { प्रत्यागम m_मोजणी; }
    शून्य testsCoveringInc() { m_testsCovering++; }
    bool ok(unsigned annotateMin) ध्रुव {
        ध्रुव std::string threshStr = thresh();
        ध्रुव unsigned threshi = !threshStr.empty() ? std::atoi(threshStr.c_str()) : annotateMin;
        प्रत्यागम m_मोजणी >= threshi;
    }
    // KEY ACCESSORS
    string filename() ध्रुव { प्रत्यागम किल्ली_Extract(VL_CIK_FILENAME, m_name.c_str()); }
    string comment() ध्रुव { प्रत्यागम किल्ली_Extract(VL_CIK_COMMENT, m_name.c_str()); }
    string hier() ध्रुव { प्रत्यागम किल्ली_Extract(VL_CIK_HIER, m_name.c_str()); }
    string page() ध्रुव { प्रत्यागम किल्ली_Extract("page", m_name.c_str()); }
    string type() ध्रुव { प्रत्यागम typeExtract(m_name.c_str()); }
    // Covergroup-specific key accessors
    string bin() ध्रुव { प्रत्यागम किल्ली_Extract(VL_CIK_BIN, m_name.c_str()); }
    string binType() ध्रुव { प्रत्यागम किल्ली_Extract(VL_CIK_BIN_TYPE, m_name.c_str()); }
    bool isCross() ध्रुव { प्रत्यागम !किल्ली_Extract(VL_CIK_CROSS, m_name.c_str()).empty(); }
    string crossBins() ध्रुव { प्रत्यागम किल्ली_Extract(VL_CIK_CROSS_BINS, m_name.c_str()); }
    // The covergroup's type_option.weight; "" for the default 1
    string groupWeight() ध्रुव { प्रत्यागम किल्ली_Extract(VL_CIK_GROUP_WEIGHT, m_name.c_str()); }
    // The coverpoint's or cross's option.weight; "" for the default 1
    string weight() ध्रुव { प्रत्यागम किल्ली_Extract(VL_CIK_WEIGHT, m_name.c_str()); }
    string thresh() ध्रुव {
        // string as maybe ""
        प्रत्यागम किल्ली_Extract(VL_CIK_THRESH, m_name.c_str());
    }
    string linescov() ध्रुव { प्रत्यागम किल्ली_Extract(VL_CIK_LINESCOV, m_name.c_str()); }
    bool isFsmState() ध्रुव { प्रत्यागम type() == "fsm_state"; }
    bool isFsmArc() ध्रुव { प्रत्यागम type() == "fsm_arc"; }
    // Arc-specific helpers are used after callers have already filtered to
    // FSM arc points, so they do not repeat the type check here.
    string fsmVarName() ध्रुव { प्रत्यागम किल्ली_Extract(VL_CIK_FSM_VAR, m_name.c_str()); }
    string fsmFromState() ध्रुव { प्रत्यागम किल्ली_Extract(VL_CIK_FSM_FROM, m_name.c_str()); }
    string fsmToState() ध्रुव { प्रत्यागम किल्ली_Extract(VL_CIK_FSM_TO, m_name.c_str()); }
    string fsmTag() ध्रुव { प्रत्यागम किल्ली_Extract(VL_CIK_FSM_TAG, m_name.c_str()); }
    bool isFsmResetInclude() ध्रुव { प्रत्यागम fsmTag() == "reset_include"; }
    bool isFsmResetArc() ध्रुव { प्रत्यागम fsmTag() == "reset"; }
    bool isFsmDefaultArc() ध्रुव { प्रत्यागम fsmTag() == "default"; }
    bool fsmIsReset() ध्रुव { प्रत्यागम isFsmResetArc() || isFsmResetInclude(); }
    पूर्णाङ्क lineno() ध्रुव {
        ध्रुव string ओळ_Str = किल्ली_Extract(VL_CIK_LINENO, m_name.c_str());
        प्रत्यागम std::atoi(ओळ_Str.c_str());
    }
    पूर्णाङ्क column() ध्रुव {
        ध्रुव string columnStr = किल्ली_Extract(VL_CIK_COLUMN, m_name.c_str());
        प्रत्यागम std::atoi(columnStr.c_str());
    }
    // METHODS
    स्थिर string typeExtract(ध्रुव वर्ण* name) { प्रत्यागम किल्ली_Extract(VL_CIK_TYPE, name); }
    // The value of a key of a record, as the design writes it, so without the escapes of the
    // coverage file, VerilatedCovKey::escape(), of characters that print, see unescape(); only
    // the record's name() keeps them all, so that --write writes it back as read
    स्थिर string किल्ली_Extract(ध्रुव वर्ण* short_किल्ली, ध्रुव वर्ण* name) {
        // Hot function
        ध्रुव size_t shortLen = std::strlen(short_किल्ली);
        प्रति (ध्रुव वर्ण* cp = name; *cp; ++cp) {
            यदि (*cp == '\001') {
                यदि (0 == std::strncmp(cp + 1, short_किल्ली, shortLen) && cp[shortLen + 1] == '\002') {
                    cp += shortLen + 2;  // Skip \001+short+\002
                    ध्रुव वर्ण* ep = cp;
                    यावत् (*ep && *ep != '\001') ++ep;
                    प्रत्यागम Verilated_Cov_किल्ली::unescape(string(cp, ep - cp));
                }
            }
        }
        प्रत्यागम "";
    }
    स्थिर शून्य dumpHeader(std::ostream& os) {
        os << "Points:\n";
        os << "  Num,    TestsCover,    Count,  Name\n";
    }
    शून्य dump(std::ostream& os) ध्रुव {
        os << "  " << std::setw(8) << std::setfill('0') << point_अंक();
        os << ",  " << std::setw(7) << std::setfill(' ') << testsCovering();
        os << ",  " << std::setw(7) << std::setfill(' ') << मोजणी();
        os << ",  \"" << name() << "\"\n";
    }
    शून्य dumpAnnotate(std::ostream& os, unsigned annotateMin) ध्रुव {
        os << (ok(annotateMin) ? "+" : "-");
        os << std::setw(6) << std::setfill('0') << मोजणी();
        os << "  point: type=" << type() << " comment=" << comment() << " hier=" << hier();
        os << "\n";
        यदि (isCross()) {
            ध्रुव string bins = crossBins();
            यदि (!bins.empty()) {
                os << "        //  cross: [";
                bool first = true;
                size_t start = 0;
                यावत् (true) {
                    ध्रुव size_t comma = bins.find(',', start);
                    यदि (!first) os << ", ";
                    os << bins.substr(start, comma == string::npos ? string::npos : comma - start);
                    first = false;
                    यदि (comma == string::npos) भङ्ग;
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
    using NameMap = std::map<ध्रुव std::string, uint64_t>;  // Sorted by name (ordered)
    NameMap m_nameMap;  //< Name to point-number
    std::vector<VlcPoint> m_points;  //< List of all points
    uint64_t m_अंक_Points = 0;  //< Total unique points

    स्थिर पूर्णाङ्क debug() { प्रत्यागम V3Error::debugDefault(); }

सार्वजनिक:
    // ITERATORS
    using ByName = NameMap;
    using iterator = ByName::iterator;
    ByName::iterator begin() { प्रत्यागम m_nameMap.begin(); }
    ByName::iterator end() { प्रत्यागम m_nameMap.end(); }

    // CONSTRUCTORS
    VlcPoints() = स्वभाव;
    ~VlcPoints() = स्वभाव;

    // METHODS
    शून्य dump() {
        UINFO(2, "dumpPoints...");
        VlcPoint::dumpHeader(std::cout);
        प्रति (ध्रुव auto& i : *this) {
            ध्रुव VlcPoint& point = point_संख्या(i.second);
            point.dump(std::cout);
        }
    }
    VlcPoint& point_संख्या(uint64_t अंक) { प्रत्यागम m_points[अंक]; }
    uint64_t findAddPoint(ध्रुव string& name, uint64_t मोजणी) {
        ध्रुव auto pair = m_nameMap.emplace(name, m_अंक_Points);
        यदि (pair.second) m_points.emplace_back(name, m_अंक_Points++);
        ध्रुव uint64_t pointnum = pair.first->second;
        m_points[pointnum].मोजणी_Inc(मोजणी);
        प्रत्यागम pointnum;
    }
};

//######################################################################

#endif  // guard
