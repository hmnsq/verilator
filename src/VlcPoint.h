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
    VlcPoint(स्थिरांक string& name, uint64_t point_अंक)
        : m_name{name}
        , m_point_अंक{point_अंक} {}
    ~VlcPoint() = पूर्वनिर्धारित;
    // ACCESSORS
    स्थिरांक string& name() स्थिरांक { परत m_name; }
    uint64_t point_अंक() स्थिरांक { परत m_point_अंक; }
    uint64_t testsCovering() स्थिरांक { परत m_testsCovering; }
    शून्य मोजणी_Inc(uint64_t inc) { m_मोजणी += inc; }
    uint64_t मोजणी() स्थिरांक { परत m_मोजणी; }
    शून्य testsCoveringInc() { m_testsCovering++; }
    bool ok(अचिन्हित annotateMin) स्थिरांक {
        स्थिरांक std::string threshStr = thresh();
        स्थिरांक अचिन्हित threshi = !threshStr.empty() ? std::म_से_प(threshStr.c_str()) : annotateMin;
        परत m_मोजणी >= threshi;
    }
    // KEY ACCESSORS
    string filename() स्थिरांक { परत किल्ली_Extract(VL_CIK_FILENAME, m_name.c_str()); }
    string comment() स्थिरांक { परत किल्ली_Extract(VL_CIK_COMMENT, m_name.c_str()); }
    string hier() स्थिरांक { परत किल्ली_Extract(VL_CIK_HIER, m_name.c_str()); }
    string page() स्थिरांक { परत किल्ली_Extract("page", m_name.c_str()); }
    string type() स्थिरांक { परत typeExtract(m_name.c_str()); }
    // Covergroup-specific key accessors
    string bin() स्थिरांक { परत किल्ली_Extract(VL_CIK_BIN, m_name.c_str()); }
    string binType() स्थिरांक { परत किल्ली_Extract(VL_CIK_BIN_TYPE, m_name.c_str()); }
    bool isCross() स्थिरांक { परत !किल्ली_Extract(VL_CIK_CROSS, m_name.c_str()).empty(); }
    string crossBins() स्थिरांक { परत किल्ली_Extract(VL_CIK_CROSS_BINS, m_name.c_str()); }
    // The covergroup's type_option.weight; "" for the default 1
    string groupWeight() स्थिरांक { परत किल्ली_Extract(VL_CIK_GROUP_WEIGHT, m_name.c_str()); }
    // The coverpoint's or cross's option.weight; "" for the default 1
    string weight() स्थिरांक { परत किल्ली_Extract(VL_CIK_WEIGHT, m_name.c_str()); }
    string thresh() स्थिरांक {
        // string as maybe ""
        परत किल्ली_Extract(VL_CIK_THRESH, m_name.c_str());
    }
    string linescov() स्थिरांक { परत किल्ली_Extract(VL_CIK_LINESCOV, m_name.c_str()); }
    bool isFsmState() स्थिरांक { परत type() == "fsm_state"; }
    bool isFsmArc() स्थिरांक { परत type() == "fsm_arc"; }
    // Arc-specific helpers are used after callers have already filtered to
    // FSM arc points, so they do not repeat the type check here.
    string fsmVarName() स्थिरांक { परत किल्ली_Extract(VL_CIK_FSM_VAR, m_name.c_str()); }
    string fsmFromState() स्थिरांक { परत किल्ली_Extract(VL_CIK_FSM_FROM, m_name.c_str()); }
    string fsmToState() स्थिरांक { परत किल्ली_Extract(VL_CIK_FSM_TO, m_name.c_str()); }
    string fsmTag() स्थिरांक { परत किल्ली_Extract(VL_CIK_FSM_TAG, m_name.c_str()); }
    bool isFsmResetInclude() स्थिरांक { परत fsmTag() == "reset_include"; }
    bool isFsmResetArc() स्थिरांक { परत fsmTag() == "reset"; }
    bool isFsmDefaultArc() स्थिरांक { परत fsmTag() == "default"; }
    bool fsmIsReset() स्थिरांक { परत isFsmResetArc() || isFsmResetInclude(); }
    पूर्णांक lineno() स्थिरांक {
        स्थिरांक string ओळ_Str = किल्ली_Extract(VL_CIK_LINENO, m_name.c_str());
        परत std::म_से_प(ओळ_Str.c_str());
    }
    पूर्णांक column() स्थिरांक {
        स्थिरांक string columnStr = किल्ली_Extract(VL_CIK_COLUMN, m_name.c_str());
        परत std::म_से_प(columnStr.c_str());
    }
    // METHODS
    स्थिर string typeExtract(स्थिरांक अक्षर* name) { परत किल्ली_Extract(VL_CIK_TYPE, name); }
    // The value of a key of a record, as the design writes it, so without the escapes of the
    // coverage file, VerilatedCovKey::escape(), of characters that print, see unescape(); only
    // the record's name() keeps them all, so that --write writes it back as read
    स्थिर string किल्ली_Extract(स्थिरांक अक्षर* short_किल्ली, स्थिरांक अक्षर* name) {
        // Hot function
        स्थिरांक माप_प्रकार shortLen = std::म_माप(short_किल्ली);
        साठी (स्थिरांक अक्षर* cp = name; *cp; ++cp) {
            जर (*cp == '\001') {
                जर (0 == std::म_भेदन(cp + 1, short_किल्ली, shortLen) && cp[shortLen + 1] == '\002') {
                    cp += shortLen + 2;  // Skip \001+short+\002
                    स्थिरांक अक्षर* ep = cp;
                    पर्यंत (*ep && *ep != '\001') ++ep;
                    परत Verilated_Cov_किल्ली::unescape(string(cp, ep - cp));
                }
            }
        }
        परत "";
    }
    स्थिर शून्य dumpHeader(std::ostream& os) {
        os << "Points:\n";
        os << "  Num,    TestsCover,    Count,  Name\n";
    }
    शून्य dump(std::ostream& os) स्थिरांक {
        os << "  " << std::setw(8) << std::setfill('0') << point_अंक();
        os << ",  " << std::setw(7) << std::setfill(' ') << testsCovering();
        os << ",  " << std::setw(7) << std::setfill(' ') << मोजणी();
        os << ",  \"" << name() << "\"\n";
    }
    शून्य dumpAnnotate(std::ostream& os, अचिन्हित annotateMin) स्थिरांक {
        os << (ok(annotateMin) ? "+" : "-");
        os << std::setw(6) << std::setfill('0') << मोजणी();
        os << "  point: type=" << type() << " comment=" << comment() << " hier=" << hier();
        os << "\n";
        जर (isCross()) {
            स्थिरांक string bins = crossBins();
            जर (!bins.empty()) {
                os << "        //  cross: [";
                bool first = true;
                माप_प्रकार start = 0;
                पर्यंत (true) {
                    स्थिरांक माप_प्रकार comma = bins.find(',', start);
                    जर (!first) os << ", ";
                    os << bins.substr(start, comma == string::npos ? string::npos : comma - start);
                    first = false;
                    जर (comma == string::npos) थांबा;
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
    using NameMap = std::map<स्थिरांक std::string, uint64_t>;  // Sorted by name (ordered)
    NameMap m_nameMap;  //< Name to point-number
    std::vector<VlcPoint> m_points;  //< List of all points
    uint64_t m_अंक_Points = 0;  //< Total unique points

    स्थिर पूर्णांक debug() { परत V3Error::debugDefault(); }

सार्वजनिक:
    // ITERATORS
    using ByName = NameMap;
    using iterator = ByName::iterator;
    ByName::iterator begin() { परत m_nameMap.begin(); }
    ByName::iterator end() { परत m_nameMap.end(); }

    // CONSTRUCTORS
    VlcPoints() = पूर्वनिर्धारित;
    ~VlcPoints() = पूर्वनिर्धारित;

    // METHODS
    शून्य dump() {
        UINFO(2, "dumpPoints...");
        VlcPoint::dumpHeader(std::cout);
        साठी (स्थिरांक स्वतः& i : *this) {
            स्थिरांक VlcPoint& point = point_संख्या(i.second);
            point.dump(std::cout);
        }
    }
    VlcPoint& point_संख्या(uint64_t अंक) { परत m_points[अंक]; }
    uint64_t findAddPoint(स्थिरांक string& name, uint64_t मोजणी) {
        स्थिरांक स्वतः pair = m_nameMap.emplace(name, m_अंक_Points);
        जर (pair.second) m_points.emplace_back(name, m_अंक_Points++);
        स्थिरांक uint64_t pointnum = pair.first->second;
        m_points[pointnum].मोजणी_Inc(मोजणी);
        परत pointnum;
    }
};

//######################################################################

#endif  // guard
