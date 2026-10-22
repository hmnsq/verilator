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

#अगर_अघोषित VERILATOR_VLCPOINT_H_
#घोषणा VERILATOR_VLCPOINT_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश <iomanip>
#समावेश <map>
#समावेश <unordered_map>
#समावेश <vector>

#अगर_अघोषित V3ERROR_NO_GLOBAL_
#घोषणा V3ERROR_NO_GLOBAL_
#पूर्ण_अगर
#समावेश "verilated_cov_key.h"

#समावेश "V3Error.h"

//********************************************************************
// VlcPoint - A coverage point (across all tests)

class VlcPoint final {
    // MEMBERS
    string m_name;  //< Name of the point
    uint64_t m_point_अंक;  //< Point number
    uint64_t m_testsCovering = 0;  //< Number tests with non-zero coverage of this point
    uint64_t m_गिनती = 0;  //< Count of hits across all tests

खुला:
    // CONSTRUCTORS
    VlcPoint(स्थिर string& name, uint64_t point_अंक)
        : m_name{name}
        , m_point_अंक{point_अंक} {}
    ~VlcPoint() = शेष;
    // ACCESSORS
    स्थिर string& name() स्थिर { वापस m_name; }
    uint64_t point_अंक() स्थिर { वापस m_point_अंक; }
    uint64_t testsCovering() स्थिर { वापस m_testsCovering; }
    व्योम गिनती_Inc(uint64_t inc) { m_गिनती += inc; }
    uint64_t गिनती() स्थिर { वापस m_गिनती; }
    व्योम testsCoveringInc() { m_testsCovering++; }
    bool ok(अचिन्हित annotateMin) स्थिर {
        स्थिर std::string threshStr = thresh();
        स्थिर अचिन्हित threshi = !threshStr.empty() ? std::म_से_प(threshStr.c_str()) : annotateMin;
        वापस m_गिनती >= threshi;
    }
    // KEY ACCESSORS
    string filename() स्थिर { वापस कुंजी_Extract(VL_CIK_FILENAME, m_name.c_str()); }
    string comment() स्थिर { वापस कुंजी_Extract(VL_CIK_COMMENT, m_name.c_str()); }
    string hier() स्थिर { वापस कुंजी_Extract(VL_CIK_HIER, m_name.c_str()); }
    string page() स्थिर { वापस कुंजी_Extract("page", m_name.c_str()); }
    string type() स्थिर { वापस typeExtract(m_name.c_str()); }
    // Covergroup-specific key accessors
    string bin() स्थिर { वापस कुंजी_Extract(VL_CIK_BIN, m_name.c_str()); }
    string binType() स्थिर { वापस कुंजी_Extract(VL_CIK_BIN_TYPE, m_name.c_str()); }
    bool isCross() स्थिर { वापस !कुंजी_Extract(VL_CIK_CROSS, m_name.c_str()).empty(); }
    string crossBins() स्थिर { वापस कुंजी_Extract(VL_CIK_CROSS_BINS, m_name.c_str()); }
    // The covergroup's type_option.weight; "" for the default 1
    string groupWeight() स्थिर { वापस कुंजी_Extract(VL_CIK_GROUP_WEIGHT, m_name.c_str()); }
    // The coverpoint's or cross's option.weight; "" for the default 1
    string weight() स्थिर { वापस कुंजी_Extract(VL_CIK_WEIGHT, m_name.c_str()); }
    string thresh() स्थिर {
        // string as maybe ""
        वापस कुंजी_Extract(VL_CIK_THRESH, m_name.c_str());
    }
    string linescov() स्थिर { वापस कुंजी_Extract(VL_CIK_LINESCOV, m_name.c_str()); }
    bool isFsmState() स्थिर { वापस type() == "fsm_state"; }
    bool isFsmArc() स्थिर { वापस type() == "fsm_arc"; }
    // Arc-specific helpers are used after callers have already filtered to
    // FSM arc points, so they do not repeat the type check here.
    string fsmVarName() स्थिर { वापस कुंजी_Extract(VL_CIK_FSM_VAR, m_name.c_str()); }
    string fsmFromState() स्थिर { वापस कुंजी_Extract(VL_CIK_FSM_FROM, m_name.c_str()); }
    string fsmToState() स्थिर { वापस कुंजी_Extract(VL_CIK_FSM_TO, m_name.c_str()); }
    string fsmTag() स्थिर { वापस कुंजी_Extract(VL_CIK_FSM_TAG, m_name.c_str()); }
    bool isFsmResetInclude() स्थिर { वापस fsmTag() == "reset_include"; }
    bool isFsmResetArc() स्थिर { वापस fsmTag() == "reset"; }
    bool isFsmDefaultArc() स्थिर { वापस fsmTag() == "default"; }
    bool fsmIsReset() स्थिर { वापस isFsmResetArc() || isFsmResetInclude(); }
    पूर्णांक lineno() स्थिर {
        स्थिर string पंक्ति_Str = कुंजी_Extract(VL_CIK_LINENO, m_name.c_str());
        वापस std::म_से_प(पंक्ति_Str.c_str());
    }
    पूर्णांक column() स्थिर {
        स्थिर string columnStr = कुंजी_Extract(VL_CIK_COLUMN, m_name.c_str());
        वापस std::म_से_प(columnStr.c_str());
    }
    // METHODS
    अटल string typeExtract(स्थिर अक्षर* name) { वापस कुंजी_Extract(VL_CIK_TYPE, name); }
    // The value of a key of a record, as the design writes it, so without the escapes of the
    // coverage file, VerilatedCovKey::escape(), of characters that print, see unescape(); only
    // the record's name() keeps them all, so that --write writes it back as read
    अटल string कुंजी_Extract(स्थिर अक्षर* short_कुंजी, स्थिर अक्षर* name) {
        // Hot function
        स्थिर माप_प्रकार shortLen = std::म_माप(short_कुंजी);
        क्रम (स्थिर अक्षर* cp = name; *cp; ++cp) {
            अगर (*cp == '\001') {
                अगर (0 == std::म_भेदन(cp + 1, short_कुंजी, shortLen) && cp[shortLen + 1] == '\002') {
                    cp += shortLen + 2;  // Skip \001+short+\002
                    स्थिर अक्षर* ep = cp;
                    जबतक (*ep && *ep != '\001') ++ep;
                    वापस Verilated_Cov_कुंजी::unescape(string(cp, ep - cp));
                }
            }
        }
        वापस "";
    }
    अटल व्योम dumpHeader(std::ostream& os) {
        os << "Points:\n";
        os << "  Num,    TestsCover,    Count,  Name\n";
    }
    व्योम dump(std::ostream& os) स्थिर {
        os << "  " << std::setw(8) << std::setfill('0') << point_अंक();
        os << ",  " << std::setw(7) << std::setfill(' ') << testsCovering();
        os << ",  " << std::setw(7) << std::setfill(' ') << गिनती();
        os << ",  \"" << name() << "\"\n";
    }
    व्योम dumpAnnotate(std::ostream& os, अचिन्हित annotateMin) स्थिर {
        os << (ok(annotateMin) ? "+" : "-");
        os << std::setw(6) << std::setfill('0') << गिनती();
        os << "  point: type=" << type() << " comment=" << comment() << " hier=" << hier();
        os << "\n";
        अगर (isCross()) {
            स्थिर string bins = crossBins();
            अगर (!bins.empty()) {
                os << "        //  cross: [";
                bool first = true;
                माप_प्रकार start = 0;
                जबतक (true) {
                    स्थिर माप_प्रकार comma = bins.find(',', start);
                    अगर (!first) os << ", ";
                    os << bins.substr(start, comma == string::npos ? string::npos : comma - start);
                    first = false;
                    अगर (comma == string::npos) अवरोध;
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
    using NameMap = std::map<स्थिर std::string, uint64_t>;  // Sorted by name (ordered)
    NameMap m_nameMap;  //< Name to point-number
    std::vector<VlcPoint> m_points;  //< List of all points
    uint64_t m_अंक_Points = 0;  //< Total unique points

    अटल पूर्णांक debug() { वापस V3Error::debugDefault(); }

खुला:
    // ITERATORS
    using ByName = NameMap;
    using iterator = ByName::iterator;
    ByName::iterator begin() { वापस m_nameMap.begin(); }
    ByName::iterator end() { वापस m_nameMap.end(); }

    // CONSTRUCTORS
    VlcPoints() = शेष;
    ~VlcPoints() = शेष;

    // METHODS
    व्योम dump() {
        UINFO(2, "dumpPoints...");
        VlcPoint::dumpHeader(std::cout);
        क्रम (स्थिर स्वतः& i : *this) {
            स्थिर VlcPoint& point = point_संख्या(i.second);
            point.dump(std::cout);
        }
    }
    VlcPoint& point_संख्या(uint64_t अंक) { वापस m_points[अंक]; }
    uint64_t findAddPoint(स्थिर string& name, uint64_t गिनती) {
        स्थिर स्वतः pair = m_nameMap.emplace(name, m_अंक_Points);
        अगर (pair.second) m_points.emplace_back(name, m_अंक_Points++);
        स्थिर uint64_t pointnum = pair.first->second;
        m_points[pointnum].गिनती_Inc(गिनती);
        वापस pointnum;
    }
};

//######################################################################

#पूर्ण_अगर  // guard
