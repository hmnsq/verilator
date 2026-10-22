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
    uint64_t m_point_अङ्कः;  //< Point number
    uint64_t m_testsCovering = 0;  //< Number tests with non-zero coverage of this point
    uint64_t m_गणनम् = 0;  //< Count of hits across all tests

सार्वजनिक:
    // CONSTRUCTORS
    VlcPoint(ध्रुव string& name, uint64_t point_अङ्कः)
        : m_name{name}
        , m_point_अङ्कः{point_अङ्कः} {}
    ~VlcPoint() = स्वभाव;
    // ACCESSORS
    ध्रुव string& name() ध्रुव { पटिनिवत्त m_name; }
    uint64_t point_अङ्कः() ध्रुव { पटिनिवत्त m_point_अङ्कः; }
    uint64_t testsCovering() ध्रुव { पटिनिवत्त m_testsCovering; }
    शून्य गणनम्_Inc(uint64_t inc) { m_गणनम् += inc; }
    uint64_t गणनम्() ध्रुव { पटिनिवत्त m_गणनम्; }
    शून्य testsCoveringInc() { m_testsCovering++; }
    bool ok(unsigned annotateMin) ध्रुव {
        ध्रुव std::string threshStr = thresh();
        ध्रुव unsigned threshi = !threshStr.empty() ? std::atoi(threshStr.c_str()) : annotateMin;
        पटिनिवत्त m_गणनम् >= threshi;
    }
    // KEY ACCESSORS
    string filename() ध्रुव { पटिनिवत्त कुञ्चिका_Extract(VL_CIK_FILENAME, m_name.c_str()); }
    string comment() ध्रुव { पटिनिवत्त कुञ्चिका_Extract(VL_CIK_COMMENT, m_name.c_str()); }
    string hier() ध्रुव { पटिनिवत्त कुञ्चिका_Extract(VL_CIK_HIER, m_name.c_str()); }
    string page() ध्रुव { पटिनिवत्त कुञ्चिका_Extract("page", m_name.c_str()); }
    string type() ध्रुव { पटिनिवत्त typeExtract(m_name.c_str()); }
    // Covergroup-specific key accessors
    string bin() ध्रुव { पटिनिवत्त कुञ्चिका_Extract(VL_CIK_BIN, m_name.c_str()); }
    string binType() ध्रुव { पटिनिवत्त कुञ्चिका_Extract(VL_CIK_BIN_TYPE, m_name.c_str()); }
    bool isCross() ध्रुव { पटिनिवत्त !कुञ्चिका_Extract(VL_CIK_CROSS, m_name.c_str()).empty(); }
    string crossBins() ध्रुव { पटिनिवत्त कुञ्चिका_Extract(VL_CIK_CROSS_BINS, m_name.c_str()); }
    // The covergroup's type_option.weight; "" for the default 1
    string groupWeight() ध्रुव { पटिनिवत्त कुञ्चिका_Extract(VL_CIK_GROUP_WEIGHT, m_name.c_str()); }
    // The coverpoint's or cross's option.weight; "" for the default 1
    string weight() ध्रुव { पटिनिवत्त कुञ्चिका_Extract(VL_CIK_WEIGHT, m_name.c_str()); }
    string thresh() ध्रुव {
        // string as maybe ""
        पटिनिवत्त कुञ्चिका_Extract(VL_CIK_THRESH, m_name.c_str());
    }
    string linescov() ध्रुव { पटिनिवत्त कुञ्चिका_Extract(VL_CIK_LINESCOV, m_name.c_str()); }
    bool isFsmState() ध्रुव { पटिनिवत्त type() == "fsm_state"; }
    bool isFsmArc() ध्रुव { पटिनिवत्त type() == "fsm_arc"; }
    // Arc-specific helpers are used after callers have already filtered to
    // FSM arc points, so they do not repeat the type check here.
    string fsmVarName() ध्रुव { पटिनिवत्त कुञ्चिका_Extract(VL_CIK_FSM_VAR, m_name.c_str()); }
    string fsmFromState() ध्रुव { पटिनिवत्त कुञ्चिका_Extract(VL_CIK_FSM_FROM, m_name.c_str()); }
    string fsmToState() ध्रुव { पटिनिवत्त कुञ्चिका_Extract(VL_CIK_FSM_TO, m_name.c_str()); }
    string fsmTag() ध्रुव { पटिनिवत्त कुञ्चिका_Extract(VL_CIK_FSM_TAG, m_name.c_str()); }
    bool isFsmResetInclude() ध्रुव { पटिनिवत्त fsmTag() == "reset_include"; }
    bool isFsmResetArc() ध्रुव { पटिनिवत्त fsmTag() == "reset"; }
    bool isFsmDefaultArc() ध्रुव { पटिनिवत्त fsmTag() == "default"; }
    bool fsmIsReset() ध्रुव { पटिनिवत्त isFsmResetArc() || isFsmResetInclude(); }
    पूर्णाङ्क lineno() ध्रुव {
        ध्रुव string पङ्क्तिः_Str = कुञ्चिका_Extract(VL_CIK_LINENO, m_name.c_str());
        पटिनिवत्त std::atoi(पङ्क्तिः_Str.c_str());
    }
    पूर्णाङ्क column() ध्रुव {
        ध्रुव string columnStr = कुञ्चिका_Extract(VL_CIK_COLUMN, m_name.c_str());
        पटिनिवत्त std::atoi(columnStr.c_str());
    }
    // METHODS
    स्थिर string typeExtract(ध्रुव वर्ण* name) { पटिनिवत्त कुञ्चिका_Extract(VL_CIK_TYPE, name); }
    // The value of a key of a record, as the design writes it, so without the escapes of the
    // coverage file, VerilatedCovKey::escape(), of characters that print, see unescape(); only
    // the record's name() keeps them all, so that --write writes it back as read
    स्थिर string कुञ्चिका_Extract(ध्रुव वर्ण* short_कुञ्चिका, ध्रुव वर्ण* name) {
        // Hot function
        ध्रुव size_t shortLen = std::strlen(short_कुञ्चिका);
        अत्थाय (ध्रुव वर्ण* cp = name; *cp; ++cp) {
            सचे (*cp == '\001') {
                सचे (0 == std::strncmp(cp + 1, short_कुञ्चिका, shortLen) && cp[shortLen + 1] == '\002') {
                    cp += shortLen + 2;  // Skip \001+short+\002
                    ध्रुव वर्ण* ep = cp;
                    याव (*ep && *ep != '\001') ++ep;
                    पटिनिवत्त Verilated_Cov_कुञ्चिका::unescape(string(cp, ep - cp));
                }
            }
        }
        पटिनिवत्त "";
    }
    स्थिर शून्य dumpHeader(std::ostream& os) {
        os << "Points:\n";
        os << "  Num,    TestsCover,    Count,  Name\n";
    }
    शून्य dump(std::ostream& os) ध्रुव {
        os << "  " << std::setw(8) << std::setfill('0') << point_अङ्कः();
        os << ",  " << std::setw(7) << std::setfill(' ') << testsCovering();
        os << ",  " << std::setw(7) << std::setfill(' ') << गणनम्();
        os << ",  \"" << name() << "\"\n";
    }
    शून्य dumpAnnotate(std::ostream& os, unsigned annotateMin) ध्रुव {
        os << (ok(annotateMin) ? "+" : "-");
        os << std::setw(6) << std::setfill('0') << गणनम्();
        os << "  point: type=" << type() << " comment=" << comment() << " hier=" << hier();
        os << "\n";
        सचे (isCross()) {
            ध्रुव string bins = crossBins();
            सचे (!bins.empty()) {
                os << "        //  cross: [";
                bool first = true;
                size_t start = 0;
                याव (true) {
                    ध्रुव size_t comma = bins.find(',', start);
                    सचे (!first) os << ", ";
                    os << bins.substr(start, comma == string::npos ? string::npos : comma - start);
                    first = false;
                    सचे (comma == string::npos) भङ्ग;
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
    uint64_t m_अङ्कः_Points = 0;  //< Total unique points

    स्थिर पूर्णाङ्क debug() { पटिनिवत्त V3Error::debugDefault(); }

सार्वजनिक:
    // ITERATORS
    using ByName = NameMap;
    using iterator = ByName::iterator;
    ByName::iterator begin() { पटिनिवत्त m_nameMap.begin(); }
    ByName::iterator end() { पटिनिवत्त m_nameMap.end(); }

    // CONSTRUCTORS
    VlcPoints() = स्वभाव;
    ~VlcPoints() = स्वभाव;

    // METHODS
    शून्य dump() {
        UINFO(2, "dumpPoints...");
        VlcPoint::dumpHeader(std::cout);
        अत्थाय (ध्रुव auto& i : *this) {
            ध्रुव VlcPoint& point = point_सङ्ख्या(i.second);
            point.dump(std::cout);
        }
    }
    VlcPoint& point_सङ्ख्या(uint64_t अङ्कः) { पटिनिवत्त m_points[अङ्कः]; }
    uint64_t findAddPoint(ध्रुव string& name, uint64_t गणनम्) {
        ध्रुव auto pair = m_nameMap.emplace(name, m_अङ्कः_Points);
        सचे (pair.second) m_points.emplace_back(name, m_अङ्कः_Points++);
        ध्रुव uint64_t pointnum = pair.first->second;
        m_points[pointnum].गणनम्_Inc(गणनम्);
        पटिनिवत्त pointnum;
    }
};

//######################################################################

#endif  // guard
