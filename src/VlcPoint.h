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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ <iomanip>
#അന്തര്ഭാവ <map>
#അന്തര്ഭാവ <unordered_map>
#അന്തര്ഭാവ <vector>

#ifndef V3ERROR_NO_GLOBAL_
#define V3ERROR_NO_GLOBAL_
#endif
#അന്തര്ഭാവ "verilated_cov_key.h"

#അന്തര്ഭാവ "V3Error.h"

//********************************************************************
// VlcPoint - A coverage point (across all tests)

class VlcPoint final {
    // MEMBERS
    string m_name;  //< Name of the point
    uint64_t m_point_അക്കം;  //< Point number
    uint64_t m_testsCovering = 0;  //< Number tests with non-zero coverage of this point
    uint64_t m_എണ്ണം = 0;  //< Count of hits across all tests

പൊതു:
    // CONSTRUCTORS
    VlcPoint(സ്ഥിരം string& name, uint64_t point_അക്കം)
        : m_name{name}
        , m_point_അക്കം{point_അക്കം} {}
    ~VlcPoint() = സ്ഥിരസ്ഥിതി;
    // ACCESSORS
    സ്ഥിരം string& name() സ്ഥിരം { തിരികെ m_name; }
    uint64_t point_അക്കം() സ്ഥിരം { തിരികെ m_point_അക്കം; }
    uint64_t testsCovering() സ്ഥിരം { തിരികെ m_testsCovering; }
    ശൂന്യം എണ്ണം_Inc(uint64_t inc) { m_എണ്ണം += inc; }
    uint64_t എണ്ണം() സ്ഥിരം { തിരികെ m_എണ്ണം; }
    ശൂന്യം testsCoveringInc() { m_testsCovering++; }
    bool ok(unsigned annotateMin) സ്ഥിരം {
        സ്ഥിരം std::string threshStr = thresh();
        സ്ഥിരം unsigned threshi = !threshStr.empty() ? std::atoi(threshStr.c_str()) : annotateMin;
        തിരികെ m_എണ്ണം >= threshi;
    }
    // KEY ACCESSORS
    string filename() സ്ഥിരം { തിരികെ താക്കോൽ_Extract(VL_CIK_FILENAME, m_name.c_str()); }
    string comment() സ്ഥിരം { തിരികെ താക്കോൽ_Extract(VL_CIK_COMMENT, m_name.c_str()); }
    string hier() സ്ഥിരം { തിരികെ താക്കോൽ_Extract(VL_CIK_HIER, m_name.c_str()); }
    string page() സ്ഥിരം { തിരികെ താക്കോൽ_Extract("page", m_name.c_str()); }
    string type() സ്ഥിരം { തിരികെ typeExtract(m_name.c_str()); }
    // Covergroup-specific key accessors
    string bin() സ്ഥിരം { തിരികെ താക്കോൽ_Extract(VL_CIK_BIN, m_name.c_str()); }
    string binType() സ്ഥിരം { തിരികെ താക്കോൽ_Extract(VL_CIK_BIN_TYPE, m_name.c_str()); }
    bool isCross() സ്ഥിരം { തിരികെ !താക്കോൽ_Extract(VL_CIK_CROSS, m_name.c_str()).empty(); }
    string crossBins() സ്ഥിരം { തിരികെ താക്കോൽ_Extract(VL_CIK_CROSS_BINS, m_name.c_str()); }
    // The covergroup's type_option.weight; "" for the default 1
    string groupWeight() സ്ഥിരം { തിരികെ താക്കോൽ_Extract(VL_CIK_GROUP_WEIGHT, m_name.c_str()); }
    // The coverpoint's or cross's option.weight; "" for the default 1
    string weight() സ്ഥിരം { തിരികെ താക്കോൽ_Extract(VL_CIK_WEIGHT, m_name.c_str()); }
    string thresh() സ്ഥിരം {
        // string as maybe ""
        തിരികെ താക്കോൽ_Extract(VL_CIK_THRESH, m_name.c_str());
    }
    string linescov() സ്ഥിരം { തിരികെ താക്കോൽ_Extract(VL_CIK_LINESCOV, m_name.c_str()); }
    bool isFsmState() സ്ഥിരം { തിരികെ type() == "fsm_state"; }
    bool isFsmArc() സ്ഥിരം { തിരികെ type() == "fsm_arc"; }
    // Arc-specific helpers are used after callers have already filtered to
    // FSM arc points, so they do not repeat the type check here.
    string fsmVarName() സ്ഥിരം { തിരികെ താക്കോൽ_Extract(VL_CIK_FSM_VAR, m_name.c_str()); }
    string fsmFromState() സ്ഥിരം { തിരികെ താക്കോൽ_Extract(VL_CIK_FSM_FROM, m_name.c_str()); }
    string fsmToState() സ്ഥിരം { തിരികെ താക്കോൽ_Extract(VL_CIK_FSM_TO, m_name.c_str()); }
    string fsmTag() സ്ഥിരം { തിരികെ താക്കോൽ_Extract(VL_CIK_FSM_TAG, m_name.c_str()); }
    bool isFsmResetInclude() സ്ഥിരം { തിരികെ fsmTag() == "reset_include"; }
    bool isFsmResetArc() സ്ഥിരം { തിരികെ fsmTag() == "reset"; }
    bool isFsmDefaultArc() സ്ഥിരം { തിരികെ fsmTag() == "default"; }
    bool fsmIsReset() സ്ഥിരം { തിരികെ isFsmResetArc() || isFsmResetInclude(); }
    പൂർണ്ണസംഖ്യ lineno() സ്ഥിരം {
        സ്ഥിരം string വരി_Str = താക്കോൽ_Extract(VL_CIK_LINENO, m_name.c_str());
        തിരികെ std::atoi(വരി_Str.c_str());
    }
    പൂർണ്ണസംഖ്യ column() സ്ഥിരം {
        സ്ഥിരം string columnStr = താക്കോൽ_Extract(VL_CIK_COLUMN, m_name.c_str());
        തിരികെ std::atoi(columnStr.c_str());
    }
    // METHODS
    സ്ഥിതം string typeExtract(സ്ഥിരം അക്ഷരം* name) { തിരികെ താക്കോൽ_Extract(VL_CIK_TYPE, name); }
    // The value of a key of a record, as the design writes it, so without the escapes of the
    // coverage file, VerilatedCovKey::escape(), of characters that print, see unescape(); only
    // the record's name() keeps them all, so that --write writes it back as read
    സ്ഥിതം string താക്കോൽ_Extract(സ്ഥിരം അക്ഷരം* short_താക്കോൽ, സ്ഥിരം അക്ഷരം* name) {
        // Hot function
        സ്ഥിരം size_t shortLen = std::strlen(short_താക്കോൽ);
        വേണ്ടി (സ്ഥിരം അക്ഷരം* cp = name; *cp; ++cp) {
            എങ്കിൽ (*cp == '\001') {
                എങ്കിൽ (0 == std::strncmp(cp + 1, short_താക്കോൽ, shortLen) && cp[shortLen + 1] == '\002') {
                    cp += shortLen + 2;  // Skip \001+short+\002
                    സ്ഥിരം അക്ഷരം* ep = cp;
                    ആകുമ്പോൾ (*ep && *ep != '\001') ++ep;
                    തിരികെ Verilated_Cov_താക്കോൽ::unescape(string(cp, ep - cp));
                }
            }
        }
        തിരികെ "";
    }
    സ്ഥിതം ശൂന്യം dumpHeader(std::ostream& os) {
        os << "Points:\n";
        os << "  Num,    TestsCover,    Count,  Name\n";
    }
    ശൂന്യം dump(std::ostream& os) സ്ഥിരം {
        os << "  " << std::setw(8) << std::setfill('0') << point_അക്കം();
        os << ",  " << std::setw(7) << std::setfill(' ') << testsCovering();
        os << ",  " << std::setw(7) << std::setfill(' ') << എണ്ണം();
        os << ",  \"" << name() << "\"\n";
    }
    ശൂന്യം dumpAnnotate(std::ostream& os, unsigned annotateMin) സ്ഥിരം {
        os << (ok(annotateMin) ? "+" : "-");
        os << std::setw(6) << std::setfill('0') << എണ്ണം();
        os << "  point: type=" << type() << " comment=" << comment() << " hier=" << hier();
        os << "\n";
        എങ്കിൽ (isCross()) {
            സ്ഥിരം string bins = crossBins();
            എങ്കിൽ (!bins.empty()) {
                os << "        //  cross: [";
                bool first = true;
                size_t start = 0;
                ആകുമ്പോൾ (true) {
                    സ്ഥിരം size_t comma = bins.find(',', start);
                    എങ്കിൽ (!first) os << ", ";
                    os << bins.substr(start, comma == string::npos ? string::npos : comma - start);
                    first = false;
                    എങ്കിൽ (comma == string::npos) നിർത്തുക;
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
    using NameMap = std::map<സ്ഥിരം std::string, uint64_t>;  // Sorted by name (ordered)
    NameMap m_nameMap;  //< Name to point-number
    std::vector<VlcPoint> m_points;  //< List of all points
    uint64_t m_അക്കം_Points = 0;  //< Total unique points

    സ്ഥിതം പൂർണ്ണസംഖ്യ debug() { തിരികെ V3Error::debugDefault(); }

പൊതു:
    // ITERATORS
    using ByName = NameMap;
    using iterator = ByName::iterator;
    ByName::iterator begin() { തിരികെ m_nameMap.begin(); }
    ByName::iterator end() { തിരികെ m_nameMap.end(); }

    // CONSTRUCTORS
    VlcPoints() = സ്ഥിരസ്ഥിതി;
    ~VlcPoints() = സ്ഥിരസ്ഥിതി;

    // METHODS
    ശൂന്യം dump() {
        UINFO(2, "dumpPoints...");
        VlcPoint::dumpHeader(std::cout);
        വേണ്ടി (സ്ഥിരം auto& i : *this) {
            സ്ഥിരം VlcPoint& point = point_സംഖ്യ(i.second);
            point.dump(std::cout);
        }
    }
    VlcPoint& point_സംഖ്യ(uint64_t അക്കം) { തിരികെ m_points[അക്കം]; }
    uint64_t findAddPoint(സ്ഥിരം string& name, uint64_t എണ്ണം) {
        സ്ഥിരം auto pair = m_nameMap.emplace(name, m_അക്കം_Points);
        എങ്കിൽ (pair.second) m_points.emplace_back(name, m_അക്കം_Points++);
        സ്ഥിരം uint64_t pointnum = pair.first->second;
        m_points[pointnum].എണ്ണം_Inc(എണ്ണം);
        തിരികെ pointnum;
    }
};

//######################################################################

#endif  // guard
