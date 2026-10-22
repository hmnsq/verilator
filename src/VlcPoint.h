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
    uint64_t m_point_अङ्क;  //< Point number
    uint64_t m_testsCovering = 0;  //< Number tests with non-zero coverage of this point
    uint64_t m_गन्ती = 0;  //< Count of hits across all tests

सार्वजनिक:
    // CONSTRUCTORS
    VlcPoint(अचर string& name, uint64_t point_अङ्क)
        : m_name{name}
        , m_point_अङ्क{point_अङ्क} {}
    ~VlcPoint() = स्वभाव;
    // ACCESSORS
    अचर string& name() अचर { फर्काऊ m_name; }
    uint64_t point_अङ्क() अचर { फर्काऊ m_point_अङ्क; }
    uint64_t testsCovering() अचर { फर्काऊ m_testsCovering; }
    शून्य गन्ती_Inc(uint64_t inc) { m_गन्ती += inc; }
    uint64_t गन्ती() अचर { फर्काऊ m_गन्ती; }
    शून्य testsCoveringInc() { m_testsCovering++; }
    bool ok(अचिन्हित annotateMin) अचर {
        अचर std::string threshStr = thresh();
        अचर अचिन्हित threshi = !threshStr.empty() ? std::म_से_प(threshStr.c_str()) : annotateMin;
        फर्काऊ m_गन्ती >= threshi;
    }
    // KEY ACCESSORS
    string filename() अचर { फर्काऊ साँचो_Extract(VL_CIK_FILENAME, m_name.c_str()); }
    string comment() अचर { फर्काऊ साँचो_Extract(VL_CIK_COMMENT, m_name.c_str()); }
    string hier() अचर { फर्काऊ साँचो_Extract(VL_CIK_HIER, m_name.c_str()); }
    string page() अचर { फर्काऊ साँचो_Extract("page", m_name.c_str()); }
    string type() अचर { फर्काऊ typeExtract(m_name.c_str()); }
    // Covergroup-specific key accessors
    string bin() अचर { फर्काऊ साँचो_Extract(VL_CIK_BIN, m_name.c_str()); }
    string binType() अचर { फर्काऊ साँचो_Extract(VL_CIK_BIN_TYPE, m_name.c_str()); }
    bool isCross() अचर { फर्काऊ !साँचो_Extract(VL_CIK_CROSS, m_name.c_str()).empty(); }
    string crossBins() अचर { फर्काऊ साँचो_Extract(VL_CIK_CROSS_BINS, m_name.c_str()); }
    // The covergroup's type_option.weight; "" for the default 1
    string groupWeight() अचर { फर्काऊ साँचो_Extract(VL_CIK_GROUP_WEIGHT, m_name.c_str()); }
    // The coverpoint's or cross's option.weight; "" for the default 1
    string weight() अचर { फर्काऊ साँचो_Extract(VL_CIK_WEIGHT, m_name.c_str()); }
    string thresh() अचर {
        // string as maybe ""
        फर्काऊ साँचो_Extract(VL_CIK_THRESH, m_name.c_str());
    }
    string linescov() अचर { फर्काऊ साँचो_Extract(VL_CIK_LINESCOV, m_name.c_str()); }
    bool isFsmState() अचर { फर्काऊ type() == "fsm_state"; }
    bool isFsmArc() अचर { फर्काऊ type() == "fsm_arc"; }
    // Arc-specific helpers are used after callers have already filtered to
    // FSM arc points, so they do not repeat the type check here.
    string fsmVarName() अचर { फर्काऊ साँचो_Extract(VL_CIK_FSM_VAR, m_name.c_str()); }
    string fsmFromState() अचर { फर्काऊ साँचो_Extract(VL_CIK_FSM_FROM, m_name.c_str()); }
    string fsmToState() अचर { फर्काऊ साँचो_Extract(VL_CIK_FSM_TO, m_name.c_str()); }
    string fsmTag() अचर { फर्काऊ साँचो_Extract(VL_CIK_FSM_TAG, m_name.c_str()); }
    bool isFsmResetInclude() अचर { फर्काऊ fsmTag() == "reset_include"; }
    bool isFsmResetArc() अचर { फर्काऊ fsmTag() == "reset"; }
    bool isFsmDefaultArc() अचर { फर्काऊ fsmTag() == "default"; }
    bool fsmIsReset() अचर { फर्काऊ isFsmResetArc() || isFsmResetInclude(); }
    पूर्णाङ्क lineno() अचर {
        अचर string पङ्क्ति_Str = साँचो_Extract(VL_CIK_LINENO, m_name.c_str());
        फर्काऊ std::म_से_प(पङ्क्ति_Str.c_str());
    }
    पूर्णाङ्क column() अचर {
        अचर string columnStr = साँचो_Extract(VL_CIK_COLUMN, m_name.c_str());
        फर्काऊ std::म_से_प(columnStr.c_str());
    }
    // METHODS
    स्थिर string typeExtract(अचर वर्ण* name) { फर्काऊ साँचो_Extract(VL_CIK_TYPE, name); }
    // The value of a key of a record, as the design writes it, so without the escapes of the
    // coverage file, VerilatedCovKey::escape(), of characters that print, see unescape(); only
    // the record's name() keeps them all, so that --write writes it back as read
    स्थिर string साँचो_Extract(अचर वर्ण* short_साँचो, अचर वर्ण* name) {
        // Hot function
        अचर माप_प्रकार shortLen = std::म_माप(short_साँचो);
        लागि (अचर वर्ण* cp = name; *cp; ++cp) {
            यदि (*cp == '\001') {
                यदि (0 == std::म_भेदन(cp + 1, short_साँचो, shortLen) && cp[shortLen + 1] == '\002') {
                    cp += shortLen + 2;  // Skip \001+short+\002
                    अचर वर्ण* ep = cp;
                    जबसम्म (*ep && *ep != '\001') ++ep;
                    फर्काऊ Verilated_Cov_साँचो::unescape(string(cp, ep - cp));
                }
            }
        }
        फर्काऊ "";
    }
    स्थिर शून्य dumpHeader(std::ostream& os) {
        os << "Points:\n";
        os << "  Num,    TestsCover,    Count,  Name\n";
    }
    शून्य dump(std::ostream& os) अचर {
        os << "  " << std::setw(8) << std::setfill('0') << point_अङ्क();
        os << ",  " << std::setw(7) << std::setfill(' ') << testsCovering();
        os << ",  " << std::setw(7) << std::setfill(' ') << गन्ती();
        os << ",  \"" << name() << "\"\n";
    }
    शून्य dumpAnnotate(std::ostream& os, अचिन्हित annotateMin) अचर {
        os << (ok(annotateMin) ? "+" : "-");
        os << std::setw(6) << std::setfill('0') << गन्ती();
        os << "  point: type=" << type() << " comment=" << comment() << " hier=" << hier();
        os << "\n";
        यदि (isCross()) {
            अचर string bins = crossBins();
            यदि (!bins.empty()) {
                os << "        //  cross: [";
                bool first = true;
                माप_प्रकार start = 0;
                जबसम्म (true) {
                    अचर माप_प्रकार comma = bins.find(',', start);
                    यदि (!first) os << ", ";
                    os << bins.substr(start, comma == string::npos ? string::npos : comma - start);
                    first = false;
                    यदि (comma == string::npos) रोक;
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
    using NameMap = std::map<अचर std::string, uint64_t>;  // Sorted by name (ordered)
    NameMap m_nameMap;  //< Name to point-number
    std::vector<VlcPoint> m_points;  //< List of all points
    uint64_t m_अङ्क_Points = 0;  //< Total unique points

    स्थिर पूर्णाङ्क debug() { फर्काऊ V3Error::debugDefault(); }

सार्वजनिक:
    // ITERATORS
    using ByName = NameMap;
    using iterator = ByName::iterator;
    ByName::iterator begin() { फर्काऊ m_nameMap.begin(); }
    ByName::iterator end() { फर्काऊ m_nameMap.end(); }

    // CONSTRUCTORS
    VlcPoints() = स्वभाव;
    ~VlcPoints() = स्वभाव;

    // METHODS
    शून्य dump() {
        UINFO(2, "dumpPoints...");
        VlcPoint::dumpHeader(std::cout);
        लागि (अचर स्वतः& i : *this) {
            अचर VlcPoint& point = point_सङ्ख्या(i.second);
            point.dump(std::cout);
        }
    }
    VlcPoint& point_सङ्ख्या(uint64_t अङ्क) { फर्काऊ m_points[अङ्क]; }
    uint64_t findAddPoint(अचर string& name, uint64_t गन्ती) {
        अचर स्वतः pair = m_nameMap.emplace(name, m_अङ्क_Points);
        यदि (pair.second) m_points.emplace_back(name, m_अङ्क_Points++);
        अचर uint64_t pointnum = pair.first->second;
        m_points[pointnum].गन्ती_Inc(गन्ती);
        फर्काऊ pointnum;
    }
};

//######################################################################

#endif  // guard
